#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include <pevxulib.h>
struct pev_ioctl_evt *pevx_evt_queue_alloc(uint, int); /* missing in header */

#include <errlog.h>
#include <devLib.h>
#include <iocsh.h>
#include <epicsThread.h>
#include <epicsExit.h>
#include <epicsMutex.h>
#include <epicsExport.h>

#include "pev.h"
#include "pevPrivate.h"

int pevIntrDebug = 0;
epicsExportAddress(int, pevIntrDebug);

struct isrEntry {
    struct isrEntry *next;
    unsigned int src_id;
    unsigned int vec_id;
    void (*func)(void*, unsigned int src_id, unsigned int vec_id);
    void* usr;
    const char* funcfile;
    const char* funcname;
    int funcoffset;
};

struct intrThread {
    struct intrThread* next;
    struct intrEngine* engine;
    unsigned int priority;
    struct pev_ioctl_evt *intrQueue;
    epicsThreadId tid;
};

LOCAL struct intrEngine {
    unsigned int card;
    struct isrEntry *isrList;
    epicsMutexId lock;
    struct intrThread *threadList;
} pevIntrList[MAX_PEV_CARDS];

LOCAL epicsMutexId pevIntrListLock;

static const char* src_name[] = {"LOC","VME","DMA","USR","USR1","USR2","???","???"};

/* Calculate thread priorities for interrupt handlers. 
   At the moment: max - 1 for all but VME interrupts
   VME: level 7 => max - 2, level 6 => max - 3, ...
   Interrupts from the same card with the same priority
   will be handled by the same thread (fifo based).
   NB: DMA interrupts are max - 10
*/
LOCAL unsigned int pevIntrGetPriorityFromSrcId(unsigned int src_id)
{
    unsigned int priority = epicsThreadPriorityBaseMax-1;

    if ((src_id & 0xf0) == EVT_SRC_VME)
    {
/*
        while (src_id++ <= EVT_SRC_VME+7)
            epicsThreadHighestPriorityLevelBelow(priority, &priority);
*/
        priority += (src_id & 0x0f)-9;
    }
    return priority;
}

LOCAL void pevIntrThread(void* arg)
{
    struct intrThread* self = arg;
    struct intrEngine* engine = self->engine;
    struct pev_ioctl_evt *intrQueue = self->intrQueue;
    epicsMutexId lock = engine->lock;
    unsigned int card = engine->card;
    unsigned int intr, src_id, vec_id, handler_called;
    const struct isrEntry* isr;
    
    if (pevx_evt_queue_enable(card, intrQueue) != 0)
    {
        errlogPrintf("pevIntrThread(card=%u): pevx_evt_queue_enable failed\n",
            card);        
        return;
    }

    if (pevIntrDebug)
        printf("pevIntrThread(card=%u) starting\n", card);

    epicsMutexLock(lock);
    while (1)
    {
        epicsMutexUnlock(lock);
        intr = pevx_evt_read(card, intrQueue, -1); /* wait until event occurs */
        epicsMutexLock(lock); /* make sure the list does not change, see also pevIntrExit */

        src_id = intr>>8;
        vec_id = intr&0xff;

        if (pevIntrDebug >= 2)
            printf("pevIntrThread(card=%u): New interrupt src_id=0x%02x (%s %i) vec_id=0x%02x\n",
                card, src_id, src_name[src_id >> 4], src_id & 0xf, vec_id);

        handler_called = FALSE;
        for (isr = pevIntrList[card].isrList; isr; isr = isr->next)
        {
            if (pevIntrDebug >= 2)
                printf("pevIntrThread(card=%u): check isr->src_id=0x%02x isr->vec_id=0x%02x\n",
                    card, isr->src_id, isr->vec_id);

            if ((isr->src_id == EVT_SRC_VME ? (src_id & 0xf0) : src_id) != isr->src_id)
                continue;

            if (isr->vec_id && isr->vec_id != vec_id)
                continue;

            if (pevIntrDebug >= 2)
                printf("pevIntrThread(card=%u): match func=%p (%s) usr=%p\n",
                    card, isr->func, isr->funcname, isr->usr);
            
            isr->func(isr->usr, src_id, vec_id);
            handler_called = TRUE;
        }
        pevx_evt_unmask(card, intrQueue, src_id);
        if (!handler_called)
        {
            errlogPrintf("pevIntrThread(card=%u): unhandled interrupt src_id=0x%02x vec_id=0x%02x\n",
                card, src_id, vec_id);
        }
    }
}

LOCAL struct intrEngine* pevIntrStartEngine(unsigned int card)
{
    if (pevIntrDebug)
        printf("pevIntrStartEngine(card=%u)\n", card);

    if (!pevIntrListLock)
    {
        errlogPrintf("pevIntrStartEngine(card=%u): pevIntrListLock is not initialized\n",
            card);
        return NULL;
    }
    epicsMutexLock(pevIntrListLock);

    /* maybe the card has been initialized by another thread meanwhile? */
    if (pevIntrList[card].lock)
    {
        epicsMutexUnlock(pevIntrListLock);
        return &pevIntrList[card];
    }

    pevIntrList[card].card = card;
    
    if (!pevx_init(card))
    {
        errlogPrintf("pevIntrStartEngine(card=%u): pevx_init() failed\n",
            card);
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }

    /* add new card to interrupt handling */
    pevIntrList[card].lock = epicsMutexCreate();
    if (!pevIntrList[card].lock)
    {
        errlogPrintf("pevIntrStartEngine(card=%u): epicsMutexCreate() failed\n",
            card);
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }
    epicsMutexUnlock(pevIntrListLock);
    return &pevIntrList[card];
}

static inline struct intrEngine* pevIntrGetEngine(unsigned int card)
{
    if (card > MAX_PEV_CARDS) return NULL;
    if (pevIntrList[card].lock) return &pevIntrList[card];
    return pevIntrStartEngine(card);
}

int pevIntrConnect(unsigned int card, unsigned int src_id, unsigned int vec_id, void (*func)(void* usr, unsigned int src_id, unsigned int vec_id), void* usr)
{
    struct intrEngine* engine;
    struct isrEntry** pisr;
    Dl_info sym;
    
    memset(&sym, 0, sizeof(sym));
    dladdr(func, &sym);
    
    if (pevIntrDebug)
        printf("pevIntrConnect(card=%u, src_id=0x%02x, vec_id = 0x%02x, func=%p (%s), usr=%p)\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);

    engine = pevIntrGetEngine(card);
    if (!engine)
    {
        errlogPrintf("pevIntrConnect(card=%u, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): pevIntrGetEngine() failed\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);
        return S_dev_noMemory;
    }
            
    epicsMutexLock(engine->lock);
    for (pisr = &engine->isrList; *pisr; pisr=&(*pisr)->next);
    *pisr = calloc(1, sizeof(struct isrEntry));
    if (!*pisr)
    {
        errlogPrintf("pevIntrConnect(card=%u, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): out of memory\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);        
        epicsMutexUnlock(engine->lock);
        return S_dev_noMemory;
    }
    (*pisr)->src_id = src_id;
    (*pisr)->vec_id = vec_id;
    (*pisr)->func = func;
    (*pisr)->usr = usr;
    (*pisr)->funcfile = sym.dli_fname;
    (*pisr)->funcname = sym.dli_sname;
    (*pisr)->funcoffset = (void*)func - sym.dli_saddr;
    epicsMutexUnlock(engine->lock);

    return S_dev_success;
}

int pevIntrDisconnect(unsigned int card, unsigned int src_id, unsigned int vec_id, void (*func)(void* usr, unsigned int src_id, unsigned int vec_id), void* usr)
{
    struct intrEngine* engine;
    struct isrEntry** pisr;
    struct isrEntry* isr;
    Dl_info sym;
    
    memset(&sym, 0, sizeof(sym));
    dladdr(func, &sym);
    
    if (pevIntrDebug)
        printf("pevIntrDisconnect(card=%u, src_id=0x%02x, vec_id = 0x%02x, func=%p (%s), usr=%p)\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);

    engine = pevIntrGetEngine(card);
    if (!engine)
    {
        errlogPrintf("pevIntrDisconnect(card=%u, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): pevIntrGetEngine() failed\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);
        return S_dev_noMemory;
    }
    
    epicsMutexLock(engine->lock);
    for (pisr = &engine->isrList; *pisr; pisr = &(*pisr)->next)
    {
        if ((*pisr)->src_id == src_id &&
            (*pisr)->vec_id == vec_id &&
            (*pisr)->func == func &&
            (usr == NULL || (*pisr)->usr == usr)) break;
    }
    if (!*pisr)
    {
        errlogPrintf("pevIntrDisconnect(card=%u, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): not connected\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);
        epicsMutexUnlock(engine->lock);
        return S_dev_noMemory;
    }
    if (pevIntrDebug)
        printf("pevIntrDisconnect(card=%u, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): removing handler\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);
    isr = *pisr;
    *pisr = (*pisr)->next;
    memset(isr, 0, sizeof(struct isrEntry));
    free (isr);
    epicsMutexUnlock(engine->lock);
    return S_dev_success;
}

struct intrThread* pevIntrGetHandler(unsigned int card, unsigned int src_id)
{
    unsigned int priority;
    struct intrEngine* engine;
    struct intrThread** pt;
    char threadName[16];
    
    priority = pevIntrGetPriorityFromSrcId(src_id);
    engine = pevIntrGetEngine(card);
    epicsMutexLock(engine->lock);
    for (pt = &engine->threadList; *pt; pt = &(*pt)->next)
    {
        if ((*pt)->priority == priority)
        {
            if (pevIntrDebug)
            {
                epicsThreadGetName((*pt)->tid, threadName, sizeof(threadName));
                printf("pevIntrGetHandler(card=%u, src_id=0x%02x): re-using existing handler %s\n",
                    card, src_id, threadName);
            }
            epicsMutexUnlock(engine->lock);
            return *pt;
        }
    }
    sprintf(threadName, "pevIntr%u.%02x", card, src_id);
    if (pevIntrDebug)
        printf("pevIntrGetHandler(card=%u, src_id=0x%02x): creating new handler %s\n",
            card, src_id, threadName);
    *pt = (struct intrThread*) calloc(1, sizeof(struct intrThread));
    if (!*pt)
    {
        errlogPrintf("pevIntrGetHandler(card=%u, src_id=%#04x): out of memory\n",
            card, src_id);
        epicsMutexUnlock(engine->lock);
        return NULL;
    }
    (*pt)->engine = engine;
    (*pt)->priority = priority;
    (*pt)->intrQueue = pevx_evt_queue_alloc(card, 0);
    if (!(*pt)->intrQueue)
    {
        errlogPrintf("pevIntrGetHandler(card=%u, src_id=%#04x): pevx_evt_queue_alloc() failed\n",
            card, src_id);
        free(*pt);
        *pt = NULL;
        epicsMutexUnlock(engine->lock);
        return NULL;
    }
    (*pt)->tid = epicsThreadCreate(threadName, priority,
        epicsThreadGetStackSize(epicsThreadStackSmall),
        pevIntrThread, *pt);
    if (!(*pt)->tid)
    {
        errlogPrintf("pevIntrGetHandler(card=%u, src_id=%#04x): epicsThreadCreate(%s, %d, %d, %p, %p) failed\n",
            card, src_id,
            threadName, priority,
            epicsThreadGetStackSize(epicsThreadStackSmall),
            pevIntrThread, *pt);
        pevx_evt_queue_free(card, (*pt)->intrQueue);
        free(*pt);
        *pt = NULL;
        epicsMutexUnlock(engine->lock);
        return NULL;
    }
    epicsMutexUnlock(engine->lock);
    return *pt;
}

int pevIntrEnable(unsigned int card, unsigned int src_id)
{
    struct intrThread* handler;

    if (pevIntrDebug)
        printf("pevIntrEnable(card=%u, src_id=0x%02x)\n", card, src_id);

    handler = pevIntrGetHandler(card, src_id);
    if (!handler)
    {
        errlogPrintf("pevIntrEnable(card=%u, src_id=%#04x): pevIntrGetHandler() failed\n",
            card, src_id);
        return S_dev_noMemory;
    }

    pevx_evt_register(card, handler->intrQueue, src_id);
    /* returns -1 when src_id is already registered. Don't care */    
    
    return pevx_evt_unmask(card, handler->intrQueue, src_id) == 0 ? S_dev_success : S_dev_intEnFail;
}

int pevIntrDisable(unsigned int card, unsigned int src_id)
{
    struct intrThread* handler;

    if (pevIntrDebug)
        printf("pevIntrDisable(card=%u, src_id=0x%02x)\n", card, src_id);

    handler = pevIntrGetHandler(card, src_id);
    if (!handler)
    {
        errlogPrintf("pevIntrDisable(card=%u, src_id=%#04x): pevIntrGetHandler() failed\n",
            card, src_id);
        return S_dev_noMemory;
    }

    return pevx_evt_mask(card, handler->intrQueue, src_id) == 0 ? S_dev_success : S_dev_intEnFail;
}

void pevIntrShow(const iocshArgBuf *args)
{
    struct isrEntry* isr;
    struct intrThread* t;
    unsigned int card;

    printf("pev interrupts:\n");
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        if (pevIntrList[card].threadList || pevIntrList[card].isrList)
            printf (" card %u\n", card);
        for (t = pevIntrList[card].threadList; t; t = t->next)
        {
            char threadName[16];
            epicsThreadGetName(t->tid, threadName, sizeof(threadName));
            printf("  thread %s priority %u\n", threadName, t->priority);
        }
        for (isr = pevIntrList[card].isrList; isr; isr = isr->next)
        {
            printf("  src=0x%02x (%s",
                isr->src_id, src_name[isr->src_id >> 4]);
            if (isr->src_id != EVT_SRC_VME || isr->src_id & 0xf)
                printf(" %d", isr->src_id & 0xf);
            printf(") vec=");
            if (isr->vec_id)
                printf("0x%02x (%d)", isr->vec_id, isr->vec_id);
            else
                printf("any");
            printf(" func=");
            if (isr->funcname)
            {
                printf ("%s", isr->funcname);
                if (isr->funcoffset) printf ("+0x%x", isr->funcoffset);
            }
            else
            {
                printf ("%p", isr->func);
            }
            if (isr->funcfile)
                printf (" (%s)", isr->funcfile);
            if (isr->usr)
                printf (" usr=%p", isr->usr);
            else
                printf (" usr=NULL");
            printf ("\n");
        }
    }
}

LOCAL void pevIntrExit(void* dummy)
{
    unsigned int card;
    struct intrThread* t;
    
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        if (!pevIntrList[card].lock) continue;
        if (pevIntrDebug)
            printf("pevIntrExit(): stopping interrupt handling on card %d\n",
                card);

        /* make sure that no handler is active */
        epicsMutexLock(pevIntrList[card].lock);
        
        /* we cannot kill a thread nor can we wake up pevx_evt_read() */
        /* but since we don't release the lock, the thread is effectively stopped */
        
        for (t = pevIntrList[card].threadList; t; t = t->next)
        {
            pevx_evt_queue_disable(card, t->intrQueue);
            pevx_evt_queue_free(card, t->intrQueue);
        }
    }
    if (pevIntrDebug)
        printf("pevIntrExit(): done\n");
}

static const iocshFuncDef pevIntrShowDef =
    { "pevIntrShow", 0, NULL };

int pevIntrInit()
{
    if (pevIntrDebug)
        printf("pevIntrInit()\n");

    iocshRegister(&pevIntrShowDef, pevIntrShow);

    pevIntrListLock = epicsMutexCreate();
    if (!pevIntrListLock)
    {
        errlogPrintf("pevIntrInit(): epicsMutexCreate() failed\n");
        return S_dev_noMemory;
    }

    epicsAtExit(pevIntrExit, NULL);

    return S_dev_success;
}
