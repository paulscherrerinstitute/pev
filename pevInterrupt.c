#include <stdlib.h>
#include <stdio.h>

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

struct pevIsrEntry {
    struct pevIsrEntry *next;
    unsigned int src_id;
    unsigned int vec_id;
    void (*func)(void*, unsigned int src_id, unsigned int vec_id);
    void* usr;
};

LOCAL struct pevIntrEngine {
    struct pev_ioctl_evt *intrEvent;
    struct pevIsrEntry *isrList;
    epicsThreadId tid;
    epicsMutexId isrListLock;
} pevIntrList[MAX_PEV_CARDS];

LOCAL epicsMutexId pevIntrListLock;

LOCAL void pevIntrThread(void* arg)
{
    unsigned int card = (int)arg;
    unsigned int intr, src_id, vec_id, handler_called;
    const struct pevIsrEntry* isr;
    struct pev_ioctl_evt *intrEvent = pevIntrList[card].intrEvent;
    struct pevIsrEntry *isrList = pevIntrList[card].isrList;
    epicsMutexId lock = pevIntrList[card].isrListLock;
    
    const char* src_name[] = {"LOC","VME","DMA","USR","USR1","USR2","???","???"};

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
            printf("pevIntrThread(card=%d): New interrupt src_id=0x%02x (%s-%i) vec_id=0x%02x\n",
                card, src_id, src_name[src_id >> 8], src_id & 0xf0, vec_id);

        handler_called = FALSE;
        for (isr = isrList; isr; isr = isr->next)
        {
            if (pevDebug >= 2)
                printf("pevIntrThread(card=%d): check isr->src_id=0x%02x isr->vec_id=0x%02x\n",
                    card, isr->src_id, isr->vec_id);

            if ((isr->src_id == EVT_SRC_VME ? (src_id & 0xf0) : src_id) != isr->src_id)
                continue;

            if (isr->vec_id && isr->vec_id != vec_id)
                continue;

            if (pevDebug >= 2)
                printf("pevIntrThread(card=%d): match func=%p usr=%p\n",
                    card, isr->func, isr->usr);
            
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

LOCAL struct pevIntrEngine* pevStartIntrEngine(unsigned int card)
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

static inline struct pevIntrEngine* pevGetIntrEngine(unsigned int card)
{
    if (card > MAX_PEV_CARDS) return NULL;
    if (pevIntrList[card].intrEvent) return &pevIntrList[card];
    return pevStartIntrEngine(card);
}

int pevConnectInterrupt(unsigned int card, unsigned int src_id, unsigned int vec_id, void (*func)(void* usr, unsigned int src_id, unsigned int vec_id), void* usr)
{
    struct pevIsrEntry* isr;
    
    if (pevDebug)
        printf("pevConnectInterrupt(card=%d, src_id=0x%02x, vec_id = 0x%02x, func=%p, usr=%p)\n",
            card, src_id, vec_id, func, usr);

    if (!pevGetIntrEngine(card))
    {
        errlogPrintf("pevConnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p, usr=%p): pevGetIntrEngine() failed\n",
            card, src_id, vec_id, func, usr);
        return S_dev_noMemory;
    }
            
    isr = malloc(sizeof(*isr));
    if (isr == NULL)
    {
        errlogPrintf("pevConnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p, usr=%p): out of memory\n",
            card, src_id, vec_id, func, usr);        
        return S_dev_noMemory;
    }
    isr->src_id = src_id;
    isr->vec_id = vec_id;
    isr->func = func;
    isr->usr = usr;
    
    epicsMutexLock(pevIntrList[card].isrListLock);
    isr->next = pevIntrList[card].isrList;
    pevIntrList[card].isrList = isr;
    epicsMutexUnlock(pevIntrList[card].isrListLock);

    if (src_id == EVT_SRC_VME)
    {
        int i;
        int vme_src_id;

        for (i = 1; i <= 7; i++)
        {
            vme_src_id = EVT_SRC_VME + i;
            if (pevDebug)
                printf("pevConnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p, usr=%p): pevx_evt_register(vme_src_id=%#04x)\n",
                    card, src_id, vec_id, func, usr, vme_src_id);
            if (pevx_evt_register(card, pevIntrList[card].intrEvent, vme_src_id) != 0)
            {
                errlogPrintf("pevConnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p, usr=%p): pevx_evt_register(vme_src_id=%#04x) failed\n",
                    card, src_id, vec_id, func, usr, vme_src_id);
                return S_dev_badVector;
            }
        }
        return S_dev_success;
    }
    if (pevDebug)
        printf("pevConnectInterrupt(): pevx_evt_register(card=%d, src_id=0x%02x)\n", card, src_id);
    if (pevx_evt_register(card, pevIntrList[card].intrEvent, src_id) != 0)
    {
        errlogPrintf("pevConnectInterrupt(card=%d, src_id=%#04x, vec_id=%#04x, func=%p, usr=%p): pevx_evt_register failed\n",
            card, src_id, vec_id, func, usr);
        return S_dev_badVector;
    }
    return S_dev_success;
}

int pevDisableInterrupt(unsigned int card, unsigned int src_id)
{
    if (pevDebug)
        printf("pevDisableInterrupt(card=%d, src_id=0x%02x)\n", card, src_id);

    if (!pevGetIntrEngine(card))
    {
        errlogPrintf("pevDisableInterrupt(card=%d, src_id=%#04x): pevGetIntrEngine() failed\n",
            card, src_id);
        return S_dev_noMemory;
    }

    return pevx_evt_mask(card, pevIntrList[card].intrEvent, src_id) == 0 ? S_dev_success : S_dev_intEnFail;
}

int pevEnableInterrupt(unsigned int card, unsigned int src_id)
{
    if (pevDebug)
        printf("pevEnableInterrupt(card=%d, src_id=0x%02x)\n", card, src_id);

    if (!pevGetIntrEngine(card))
    {
        errlogPrintf("pevEnableInterrupt(card=%d, src_id=%#04x): pevGetIntrEngine() failed\n",
            card, src_id);
        return S_dev_noMemory;
    }

    return pevx_evt_unmask(card, pevIntrList[card].intrEvent, src_id) == 0 ? S_dev_success : S_dev_intEnFail;
}

void pevInterruptShow(const iocshArgBuf *args)
{
    struct pevIsrEntry* isr;
    unsigned int card;

    printf("pev interrupts maps:\n");
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        for (isr = pevIntrList[card].isrList; isr; isr = isr->next)
        {
            printf("  card=%d, src=0x%02x vec=0x%02x func=%p usr=%p\n", card, isr->src_id, isr->vec_id, isr->func, isr->usr);
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
