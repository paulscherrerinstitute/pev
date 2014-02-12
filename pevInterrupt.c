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

struct intrEngine {
    struct intrEngine *next;
    int card;
    unsigned int priority;
    struct pev_ioctl_evt *intrQueue;
    struct isrEntry *isrList;
    char threadName[16];
    epicsThreadId tid;
    epicsMutexId isrListLock;
};

/* Calculate thread priorities for interrupt handlers. 
   At the moment: Max for all but VME interrupts
   VME: level 7 => max - 1, level 6 => max - 2, ...
   Interrupts from the same card with the same priority
   will be handled by the same thread (fifo based).
*/
unsigned int pevIntrCalcPriority(unsigned int src_id)
{
    unsigned int priority = epicsThreadPriorityBaseMax;
    if ((src_id & 0xf0) == EVT_SRC_VME)
    {
        int level = src_id & 0x0f;
        while (level++ <= 7)
            epicsThreadHighestPriorityLevelBelow(priority, &priority);
    }
    return priority;
}

LOCAL epicsMutexId pevIntrListLock;
LOCAL struct intrEngine* pevIntrEngines;

static const char* src_name[] = {"LOC","VME","DMA","USR","USR1","USR2","???","???"};

LOCAL void pevIntrThread(void* arg)
{
    struct intrEngine* engine = arg;
    unsigned int intr, src_id, vec_id, handler_called;
    const struct isrEntry* isr;
    unsigned int card = engine->card;
    struct pev_ioctl_evt *intrQueue = engine->intrQueue;
    epicsMutexId lock = engine->isrListLock;
    const char* threadName = engine->threadName;
    
    if (pevDebug)
        printf("%s starting\n", threadName);

    if (pevx_evt_queue_enable(card, intrQueue) != 0)
    {
        errlogPrintf("%s: pevx_evt_queue_enable failed\n",
            threadName);        
        return;
    }

    epicsMutexLock(lock);
    while (1)
    {
        epicsMutexUnlock(lock);
        intr = pevx_evt_read(card, intrQueue, -1); /* wait until event occurs */
        epicsMutexLock(lock); /* make sure the list does not change, see also pevInterruptExit */

        src_id = intr>>8;
        vec_id = intr&0xff;

        if (pevDebug >= 2)
            printf("%s: New interrupt src_id=0x%02x (%s %i) vec_id=0x%02x\n",
                threadName, src_id, src_name[src_id >> 4], src_id & 0xf, vec_id);

        handler_called = FALSE;
        for (isr = engine->isrList; isr; isr = isr->next)
        {
            if (pevDebug >= 2)
                printf("%s: check isr->src_id=0x%02x isr->vec_id=0x%02x\n",
                    threadName, isr->src_id, isr->vec_id);

            if ((isr->src_id == EVT_SRC_VME ? (src_id & 0xf0) : src_id) != isr->src_id)
                continue;

            if (isr->vec_id && isr->vec_id != vec_id)
                continue;

            if (pevDebug >= 2)
                printf("%s: match func=%p (%s) usr=%p\n",
                    threadName, isr->func, isr->funcname, isr->usr);
            
            isr->func(isr->usr, src_id, vec_id);
            handler_called = TRUE;
        }
        pevx_evt_unmask(card, intrQueue, src_id);
        if (!handler_called)
        {
            errlogPrintf("%s: unhandled interrupt src_id=0x%02x vec_id=0x%02x\n",
                threadName, src_id, vec_id);
        }
    }
}

static inline struct intrEngine* pevGetIntrEngine(unsigned int card, unsigned int src_id)
{
    struct intrEngine** pengine;
    unsigned int priority;
    
    if (pevDebug)
        printf("pevGetIntrEngine(card=%d, src_id=0x%02x)\n",
            card, src_id);

    priority = pevIntrCalcPriority(src_id);

    if (!pevIntrListLock)
    {
        errlogPrintf("pevGetIntrEngine(card=%d, src_id=0x%02x): pevIntrListLock is not initialized\n",
            card, src_id);
        return NULL;
    }
    epicsMutexLock(pevIntrListLock);
    for (pengine = &pevIntrEngines; *pengine; pengine=&(*pengine)->next)
    {
        if ((*pengine)->card == card && (*pengine)->priority == priority)
        {
            epicsMutexUnlock(pevIntrListLock);
            if (pevDebug)
                printf("pevGetIntrEngine(card=%d, src_id=0x%02x) priority=%u: re-use existing engine %s\n",
                    card, src_id, priority, (*pengine)->threadName);
            return *pengine;
        }
    }
    if (pevDebug)
        printf("pevGetIntrEngine(card=%d, src_id=0x%02x) priority=%u: creating new engine\n",
            card, src_id, priority);

    if (!pevx_init(card))
    {
        errlogPrintf("pevGetIntrEngine(card=%d, src_id=0x%02x): pevx_init() failed\n",
            card, src_id);
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }
    
    *pengine = (struct intrEngine*) calloc(1, sizeof(struct intrEngine));
    if (!*pengine)
    {
        errlogPrintf("pevGetIntrEngine(card=%d, src_id=0x%02x): out of memory for new interrupt engine\n",
            card, src_id);
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }

    /* add new card to interrupt handling */
    (*pengine)->card = card;
    (*pengine)->priority = priority;

    (*pengine)->isrListLock = epicsMutexCreate();
    if (!(*pengine)->isrListLock)
    {
        errlogPrintf("pevGetIntrEngine(card=%d, src_id=0x%02x): epicsMutexCreate() failed\n",
            card, src_id);
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }
    
    (*pengine)->intrQueue = pevx_evt_queue_alloc(card, 0);
    if (!(*pengine)->intrQueue)
    {
        epicsMutexDestroy((*pengine)->isrListLock);
        errlogPrintf("pevGetIntrEngine(card=%d, src_id=0x%02x): pevx_evt_queue_alloc() failed\n",
            card, src_id);        
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }
    sprintf((*pengine)->threadName, "pevIntr%d.%u", card, epicsThreadPriorityBaseMax-priority);
    (*pengine)->tid = epicsThreadCreate((*pengine)->threadName, priority,
        epicsThreadGetStackSize(epicsThreadStackSmall),
        pevIntrThread, *pengine);
    if (!(*pengine)->tid)
    {
        pevx_evt_queue_free(card, (*pengine)->intrQueue);
        epicsMutexDestroy((*pengine)->isrListLock);
        errlogPrintf("pevGetIntrEngine(card=%d, src_id=0x%02x): epicsThreadCreate(%s, %d, %u, %p, %p) failed\n",
            card, src_id,
            (*pengine)->threadName, priority,
            epicsThreadGetStackSize(epicsThreadStackSmall),
            pevIntrThread, *pengine);
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }
    epicsMutexUnlock(pevIntrListLock);
    return *pengine;
}

int pevConnectInterrupt(unsigned int card, unsigned int src_id, unsigned int vec_id, void (*func)(void* usr, unsigned int src_id, unsigned int vec_id), void* usr)
{
    struct intrEngine* engine;
    struct isrEntry** pisr;
    Dl_info sym;
    
    memset(&sym, 0, sizeof(sym));
    dladdr(func, &sym);
    
    if (pevDebug)
        printf("pevConnectInterrupt(card=%d, src_id=0x%02x, vec_id = 0x%02x, func=%p (%s), usr=%p)\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);

    engine = pevGetIntrEngine(card, src_id);
    if (!engine)
    {
        errlogPrintf("pevConnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): pevGetIntrEngine() failed\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);
        return S_dev_noMemory;
    }
            
    epicsMutexLock(engine->isrListLock);
    for (pisr = &engine->isrList; *pisr; pisr=&(*pisr)->next);
    *pisr = calloc(1, sizeof(struct isrEntry));
    if (!*pisr)
    {
        errlogPrintf("pevConnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): out of memory\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);        
        epicsMutexUnlock(engine->isrListLock);
        return S_dev_noMemory;
    }
    (*pisr)->src_id = src_id;
    (*pisr)->vec_id = vec_id;
    (*pisr)->func = func;
    (*pisr)->usr = usr;
    (*pisr)->funcfile = sym.dli_fname;
    (*pisr)->funcname = sym.dli_sname;
    (*pisr)->funcoffset = (void*)func - sym.dli_saddr;
    epicsMutexUnlock(engine->isrListLock);

    if (src_id == EVT_SRC_VME)
    {
        int i;
        int vme_src_id;

        for (i = 1; i <= 7; i++)
        {
            vme_src_id = EVT_SRC_VME + i;
            if (pevDebug)
                printf("pevConnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): pevx_evt_register(vme_src_id=%#04x)\n",
                    card, src_id, vec_id, func, sym.dli_sname, usr, vme_src_id);
            if (pevx_evt_register(card, engine->intrQueue, vme_src_id) != 0)
            {
                errlogPrintf("pevConnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): pevx_evt_register(vme_src_id=%#04x) failed\n",
                    card, src_id, vec_id, func, sym.dli_sname, usr, vme_src_id);
                return S_dev_badVector;
            }
        }
        return S_dev_success;
    }
    if (pevDebug)
        printf("pevConnectInterrupt(): pevx_evt_register(card=%d, src_id=0x%02x)\n", card, src_id);
    if (pevx_evt_register(card, engine->intrQueue, src_id) != 0)
    {
        errlogPrintf("pevConnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): pevx_evt_register failed\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);
        return S_dev_badVector;
    }
    return S_dev_success;
}

int pevDisconnectInterrupt(unsigned int card, unsigned int src_id, unsigned int vec_id, void (*func)(void* usr, unsigned int src_id, unsigned int vec_id), void* usr)
{
    struct intrEngine* engine;
    struct isrEntry** pisr;
    struct isrEntry* isr;
    Dl_info sym;
    
    memset(&sym, 0, sizeof(sym));
    dladdr(func, &sym);
    
    if (pevDebug)
        printf("pevDisconnectInterrupt(card=%d, src_id=0x%02x, vec_id = 0x%02x, func=%p (%s), usr=%p)\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);

    engine = pevGetIntrEngine(card, src_id);
    if (!engine)
    {
        errlogPrintf("pevDisconnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): pevGetIntrEngine() failed\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);
        return S_dev_noMemory;
    }
    
    epicsMutexLock(engine->isrListLock);
    for (pisr = &engine->isrList; *pisr; pisr = &(*pisr)->next)
    {
        if ((*pisr)->src_id == src_id &&
            (*pisr)->vec_id == vec_id &&
            (*pisr)->func == func &&
            (usr == (void*)-1 || (*pisr)->usr == usr)) break;
    }
    if (!*pisr)
    {
        errlogPrintf("pevDisconnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): not connected\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);
        epicsMutexUnlock(engine->isrListLock);
        return S_dev_noMemory;
    }
    if (pevDebug)
        printf("pevDisconnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p (%s), usr=%p): removing handler\n",
            card, src_id, vec_id, func, sym.dli_sname, usr);
    isr = *pisr;
    *pisr = (*pisr)->next;
    memset(isr, 0, sizeof(struct isrEntry));
    free (isr);
    epicsMutexUnlock(engine->isrListLock);
    return S_dev_success;
}

int pevDisableInterrupt(unsigned int card, unsigned int src_id)
{
    struct intrEngine* engine;

    if (pevDebug)
        printf("pevDisableInterrupt(card=%d, src_id=0x%02x)\n", card, src_id);

    engine = pevGetIntrEngine(card, src_id);
    if (!engine)
    {
        errlogPrintf("pevDisableInterrupt(card=%d, src_id=%#04x): pevGetIntrEngine() failed\n",
            card, src_id);
        return S_dev_noMemory;
    }

    return pevx_evt_mask(card, engine->intrQueue, src_id) == 0 ? S_dev_success : S_dev_intEnFail;
}

int pevEnableInterrupt(unsigned int card, unsigned int src_id)
{
    struct intrEngine* engine;

    if (pevDebug)
        printf("pevEnableInterrupt(card=%d, src_id=0x%02x)\n", card, src_id);

    engine = pevGetIntrEngine(card, src_id);
    if (!engine)
    {
        errlogPrintf("pevEnableInterrupt(card=%d, src_id=%#04x): pevGetIntrEngine() failed\n",
            card, src_id);
        return S_dev_noMemory;
    }

    return pevx_evt_unmask(card, engine->intrQueue, src_id) == 0 ? S_dev_success : S_dev_intEnFail;
}

void pevInterruptShow(const iocshArgBuf *args)
{
    struct isrEntry* isr;
    struct intrEngine* engine;

    printf("pev interrupts maps:\n");
    for (engine = pevIntrEngines; engine; engine = engine->next)
    {
        for (isr = engine->isrList; isr; isr = isr->next)
        {
            printf("  card=%d, src=0x%02x (%s",
                engine->card, isr->src_id, src_name[isr->src_id >> 4]);
            if (isr->src_id != EVT_SRC_VME || isr->src_id & 0xf)
                printf(" %d", isr->src_id & 0xf);
            printf (") vec=");
            if (isr->vec_id)
                printf("0x%02x (%d)", isr->vec_id, isr->vec_id);
            else
                printf("any");
            printf(" func=%p", isr->func);
            if (isr->funcname)
            {
                printf (" (%s:%s", isr->funcfile, isr->funcname);
                if (isr->funcoffset)  printf ("+0x%x", isr->funcoffset);
                printf (") ");
            }
            if (isr->usr)
                printf ("usr=%p", isr->usr);
            else
                printf ("usr=NULL");
            printf ("\n");
        }
    }
}

LOCAL void pevInterruptExit(void* dummy)
{
    struct intrEngine* engine;
    
    for (engine = pevIntrEngines; engine; engine = engine->next)
    {
        if (pevDebug)
            printf("pevInterruptExit(): stopping interrupt handler %s\n",
            engine->threadName);

        /* make sure that no handler is active */
        epicsMutexLock(engine->isrListLock);
        
        /* we cannot kill a thread nor can we wake up pevx_evt_read() */
        /* but since we don't release the lock, the thread is effectively stopped */

        pevx_evt_queue_disable(engine->card, engine->intrQueue);
        pevx_evt_queue_free(engine->card, engine->intrQueue);
    }
    if (pevDebug)
        printf("pevInterruptExit(): done\n");
}

static const iocshFuncDef pevInterruptShowDef =
    { "pevInterruptShow", 0, NULL };

int pevInterruptInit()
{
    if (pevDebug)
        printf("pevInterruptInit()\n");

    iocshRegister(&pevInterruptShowDef, pevInterruptShow);

    pevIntrListLock = epicsMutexCreate();
    if (!pevIntrListLock)
    {
        errlogPrintf("pevInterruptInit(): epicsMutexCreate() failed\n");
        return S_dev_noMemory;
    }

    epicsAtExit(pevInterruptExit, NULL);

    return S_dev_success;
}
