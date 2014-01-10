#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include <pevulib.h>
#include <pevxulib.h>

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

LOCAL struct intrEngine {
    struct pev_ioctl_evt *intrEvent;
    struct isrEntry *isrList;
    epicsThreadId tid;
    epicsMutexId isrListLock;
} pevIntrList[MAX_PEV_CARDS];

LOCAL epicsMutexId pevIntrListLock;

static const char* src_name[] = {"LOC","VME","DMA","USR","USR1","USR2","???","???"};

LOCAL void pevIntrThread(void* arg)
{
    unsigned int card = (int)arg;
    unsigned int intr, src_id, vec_id, handler_called;
    const struct isrEntry* isr;
    struct pev_ioctl_evt *intrEvent = pevIntrList[card].intrEvent;
    epicsMutexId lock = pevIntrList[card].isrListLock;
    
    if (pevx_evt_queue_enable(card, intrEvent) != 0)
    {
        errlogPrintf("pevIntrThread(card=%d): pevx_evt_queue_enable failed\n",
            card);        
        return;
    }

    if (pevDebug)
        printf("pevIntrThread(card=%d) starting\n", card);

    epicsMutexLock(lock);
    while (1)
    {
        epicsMutexUnlock(lock);
        intr = pevx_evt_read(card, intrEvent, -1); /* wait until event occurs */
        epicsMutexLock(lock); /* make sure the list does not change, see also pevInterruptExit */

        src_id = intr>>8;
        vec_id = intr&0xff;

        if (pevDebug >= 2)
            printf("pevIntrThread(card=%d): New interrupt src_id=0x%02x (%s %i) vec_id=0x%02x\n",
                card, src_id, src_name[src_id >> 4], src_id & 0xf, vec_id);

        handler_called = FALSE;
        for (isr = pevIntrList[card].isrList; isr; isr = isr->next)
        {
            if (pevDebug >= 2)
                printf("pevIntrThread(card=%d): check isr->src_id=0x%02x isr->vec_id=0x%02x\n",
                    card, isr->src_id, isr->vec_id);

            if ((isr->src_id == EVT_SRC_VME ? (src_id & 0xf0) : src_id) != isr->src_id)
                continue;

            if (isr->vec_id && isr->vec_id != vec_id)
                continue;

            if (pevDebug >= 2)
                printf("pevIntrThread(card=%d): match func=%p (%s) usr=%p\n",
                    card, isr->func, isr->funcname, isr->usr);
            
            isr->func(isr->usr, src_id, vec_id);
            handler_called = TRUE;
        }
        pevx_evt_unmask(card, intrEvent, src_id);
        if (!handler_called)
        {
            errlogPrintf("pevIntrThread(card=%d): unhandled interrupt src_id=0x%02x vec_id=0x%02x\n",
                card, src_id, vec_id);
        }
    }
}

LOCAL struct intrEngine* pevStartIntrEngine(unsigned int card)
{
    char threadName[16];

    if (pevDebug)
        printf("pevStartIntrEngine(card=%d)\n", card);

    if (!pevIntrListLock)
    {
        errlogPrintf("pevStartIntrEngine(card=%d): pevIntrListLock is not initialized\n",
            card);
        return NULL;
    }
    epicsMutexLock(pevIntrListLock);

    /* maybe the card has been initialized by another thread meanwhile? */
    if (pevIntrList[card].tid)
    {
        epicsMutexUnlock(pevIntrListLock);
        return &pevIntrList[card];
    }

    if (!pevx_init(card))
    {
        errlogPrintf("pevStartIntrEngine(card=%d): pevx_init() failed\n",
            card);
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }

    /* add new card to interrupt handling */
    pevIntrList[card].isrList = NULL;
    pevIntrList[card].isrListLock = epicsMutexCreate();
    if (!pevIntrList[card].isrListLock)
    {
        errlogPrintf("pevStartIntrEngine(card=%d): epicsMutexCreate() failed\n",
            card);
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }
    
    pevIntrList[card].intrEvent = pevx_evt_queue_alloc(card, 0); /* not in header */
    if (!pevIntrList[card].intrEvent)
    {
        epicsMutexDestroy(pevIntrList[card].isrListLock);
        errlogPrintf("pevStartIntrEngine(card=%d): pevx_evt_queue_alloc() failed\n",
            card);        
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }
    sprintf(threadName, "pevIntr%d", card);
    pevIntrList[card].tid = epicsThreadCreate(threadName, epicsThreadPriorityMax,
        epicsThreadGetStackSize(epicsThreadStackSmall),
        pevIntrThread, (void*)card);
    if (!pevIntrList[card].tid)
    {
        pevx_evt_queue_free(card, pevIntrList[card].intrEvent);
        epicsMutexDestroy(pevIntrList[card].isrListLock);
        errlogPrintf("pevStartIntrEngine(card=%d): epicsThreadCreate(%s, %d, %d, %p, %p) failed\n",
            card,
            threadName, epicsThreadPriorityMax,
            epicsThreadGetStackSize(epicsThreadStackSmall),
            pevIntrThread, NULL);
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }
    epicsMutexUnlock(pevIntrListLock);
    return &pevIntrList[card];
}

static inline struct intrEngine* pevGetIntrEngine(unsigned int card)
{
    if (card > MAX_PEV_CARDS) return NULL;
    if (pevIntrList[card].intrEvent) return &pevIntrList[card];
    return pevStartIntrEngine(card);
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

    engine = pevGetIntrEngine(card);
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
            if (pevx_evt_register(card, engine->intrEvent, vme_src_id) != 0)
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
    if (pevx_evt_register(card, engine->intrEvent, src_id) != 0)
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

    engine = pevGetIntrEngine(card);
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

    engine = pevGetIntrEngine(card);
    if (!engine)
    {
        errlogPrintf("pevDisableInterrupt(card=%d, src_id=%#04x): pevGetIntrEngine() failed\n",
            card, src_id);
        return S_dev_noMemory;
    }

    return pevx_evt_mask(card, engine->intrEvent, src_id) == 0 ? S_dev_success : S_dev_intEnFail;
}

int pevEnableInterrupt(unsigned int card, unsigned int src_id)
{
    struct intrEngine* engine;

    if (pevDebug)
        printf("pevEnableInterrupt(card=%d, src_id=0x%02x)\n", card, src_id);

    engine = pevGetIntrEngine(card);
    if (!engine)
    {
        errlogPrintf("pevEnableInterrupt(card=%d, src_id=%#04x): pevGetIntrEngine() failed\n",
            card, src_id);
        return S_dev_noMemory;
    }

    return pevx_evt_unmask(card, engine->intrEvent, src_id) == 0 ? S_dev_success : S_dev_intEnFail;
}

void pevInterruptShow(const iocshArgBuf *args)
{
    struct isrEntry* isr;
    unsigned int card;

    printf("pev interrupts maps:\n");
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        for (isr = pevIntrList[card].isrList; isr; isr = isr->next)
        {
            printf("  card=%d, src=0x%02x (%s",
                card, isr->src_id, src_name[isr->src_id >> 4]);
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
    unsigned int card;
    
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        if (!pevIntrList[card].tid) continue;
        if (pevDebug)
            printf("pevInterruptExit(): stopping interrupt handling on card %d\n",
                card);

        /* make sure that no handler is active */
        epicsMutexLock(pevIntrList[card].isrListLock);
        
        /* we cannot kill a thread nor can we wake up pevx_evt_read() */
        /* but since we don't release the lock, the thread is effectively stopped */

        pevx_evt_queue_disable(card, pevIntrList[card].intrEvent);
        pevx_evt_queue_free(card, pevIntrList[card].intrEvent);
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
