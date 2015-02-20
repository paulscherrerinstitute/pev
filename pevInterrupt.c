#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pevxulib.h>
struct pev_ioctl_evt *pevx_evt_queue_alloc(uint, int); /* missing in header */

#include <errlog.h>
#include <devLib.h>
#include <iocsh.h>
#include <epicsThread.h>
#include <epicsExit.h>
#include <epicsMutex.h>
#include <symbolname.h>
#include <epicsExport.h>

#include "pev.h"
#include "pevPrivate.h"

int pevIntrDebug = 0;
epicsExportAddress(int, pevIntrDebug);

struct isrEntry {
    struct isrEntry *next;
    unsigned int src_id;
    unsigned int vec_id;
    void (*func)();
    void* usr;
    unsigned long long intrCount;
};

struct intrHandler {
    struct intrHandler* next;
    struct intrEngine* engine;
    unsigned int priority;
    struct pev_ioctl_evt *intrQueue;
    epicsThreadId tid;
};

static struct intrEngine {
    unsigned int card;
    struct isrEntry *isrList;
    epicsMutexId handlerListLock;
    struct intrHandler *handlerList;
    unsigned long long intrCount;
    unsigned long long unhandledIntrCount;
} pevIntrList[MAX_PEV_CARDS];

static epicsMutexId pevIntrListLock;

static const char* src_name[] = {"LOC","VME","DMA","USR","USR1","USR2","???","???"};

/* Calculate thread priorities for interrupt handlers. 
   At the moment: max - 1 for all but VME interrupts
   VME: level 7 => max - 2, level 6 => max - 3, ...
   Interrupts from the same card with the same priority
   will be handled by the same thread (fifo based).
   NB: DMA interrupts are max - 10
*/
unsigned int pevIntrGetPriorityFromSrcId(unsigned int src_id)
{
    unsigned int priority = epicsThreadPriorityBaseMax-1;

    if ((src_id & 0xf0) == EVT_SRC_VME)
    {
        priority += (src_id & 0x0f)-9;
    }
    return priority;
}

void pevIntrHandlerThread(void* arg)
{
    struct intrHandler* self = arg;
    struct intrEngine* engine = self->engine;
    struct pev_ioctl_evt *intrQueue = self->intrQueue;
    epicsMutexId handlerListLock = engine->handlerListLock;
    unsigned int card = engine->card;
    unsigned int intr, src_id, vec_id, handler_called;
    struct isrEntry* isr;
    
    if (pevx_evt_queue_enable(card, intrQueue) != 0)
    {
        errlogPrintf("pevIntrHandlerThread(card=%u): pevx_evt_queue_enable failed\n",
            card);        
        return;
    }

    if (pevIntrDebug)
        printf("pevIntrHandlerThread(card=%u) starting\n", card);

    epicsMutexLock(handlerListLock);
    while (1)
    {
        epicsMutexUnlock(handlerListLock);
        intr = pevx_evt_read(card, intrQueue, -1); /* wait until event occurs */
        epicsMutexLock(handlerListLock); /* make sure the list does not change, see also pevIntrExit */

        engine->intrCount++;

        src_id = intr>>8;
        vec_id = intr&0xff;

        if (pevIntrDebug >= 2)
            printf("pevIntrHandlerThread(card=%u): New interrupt src_id=0x%02x (%s %i) vec_id=0x%02x\n",
                card, src_id, src_name[src_id >> 4], src_id & 0xf, vec_id);

        handler_called = FALSE;
        for (isr = pevIntrList[card].isrList; isr != NULL; isr = isr->next)
        {
            if (pevIntrDebug >= 2)
                printf("pevIntrHandlerThread(card=%u): check isr->src_id=0x%02x isr->vec_id=0x%02x\n",
                    card, isr->src_id, isr->vec_id);

            /* if user installed handler for EVT_SRC_VME (without level), accept all VME levels */
            if ((isr->src_id == EVT_SRC_VME ? (src_id & 0xf0) : src_id) != isr->src_id)
                continue;

            /* if user installed handler for vector 0, accept all vectors */
            if (isr->vec_id && isr->vec_id != vec_id)
                continue;

            isr->intrCount++;

            if (pevIntrDebug >= 2)
            {
                char* fname = symbolName(isr->func, 0);
                char* uname = symbolName(isr->usr, 0);
                printf("pevIntrHandlerThread(card=%u): match func=%s usr=%s count=%llu\n",
                    card, fname, uname, isr->intrCount);
                free (uname);
                free (fname);
            }
                        
            /* pass src_id and vec_id to user function for closer inspection */
            isr->func(isr->usr, src_id, vec_id);
            handler_called = TRUE;
        }
        pevx_evt_unmask(card, intrQueue, src_id);
        if (!handler_called)
        {
            engine->unhandledIntrCount++;
            
            if (pevIntrDebug >= 2)
                printf("pevIntrHandlerThread(card=%u): unhandled interrupt src_id=0x%02x (%s %i) vec_id=0x%02x\n",
                    card, src_id, src_name[src_id >> 4], src_id & 0xf, vec_id);
        }
    }
}

struct intrEngine* pevIntrStartEngine(unsigned int card)
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
    if (pevIntrList[card].handlerListLock)
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
    pevIntrList[card].handlerListLock = epicsMutexCreate();
    if (!pevIntrList[card].handlerListLock)
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
    if (pevIntrList[card].handlerListLock) return &pevIntrList[card];
    return pevIntrStartEngine(card);
}

static void debugMsg(const char* caller, unsigned int card, unsigned int src_id, unsigned int vec_id, void (*func)(), void* usr, const char* msg)
{
    char* fname = symbolName(func, 0);
    char* uname = symbolName(usr, 0);
    errlogPrintf("%s(card=%u, src_id=0x%02x, vec_id = 0x%02x, func=%s, usr=%s)%s%s\n",
        caller, card, src_id, vec_id, fname, uname, msg ? ": " : "", msg ? msg : "");
    free (uname);
    free (fname);
}

int pevIntrConnect(unsigned int card, unsigned int src_id, unsigned int vec_id, void (*func)(), void* usr)
{
    struct intrEngine* engine;
    struct isrEntry** pisr;
    
    if (pevIntrDebug)
    {
        debugMsg("pevIntrConnect", card, src_id, vec_id, func, usr, NULL);
    }

    engine = pevIntrGetEngine(card);
    if (!engine)
    {
        debugMsg("pevIntrConnect", card, src_id, vec_id, func, usr, "pevIntrGetEngine() failed");
        return S_dev_noMemory;
    }
            
    epicsMutexLock(engine->handlerListLock);
    for (pisr = &engine->isrList; *pisr != NULL; pisr=&(*pisr)->next);
    *pisr = calloc(1, sizeof(struct isrEntry));
    if (!*pisr)
    {
        debugMsg("pevIntrConnect", card, src_id, vec_id, func, usr, "out of memory");
        epicsMutexUnlock(engine->handlerListLock);
        return S_dev_noMemory;
    }
    (*pisr)->src_id = src_id;
    (*pisr)->vec_id = vec_id;
    (*pisr)->func = func;
    (*pisr)->usr = usr;
    epicsMutexUnlock(engine->handlerListLock);

    return S_dev_success;
}

int pevIntrDisconnect(unsigned int card, unsigned int src_id, unsigned int vec_id, void (*func)(), void* usr)
{
    struct intrEngine* engine;
    struct isrEntry** pisr;
    struct isrEntry* isr;

    if (pevIntrDebug)
    {
        debugMsg("pevIntrDisconnect", card, src_id, vec_id, func, usr, NULL);
    }

    engine = pevIntrGetEngine(card);
    if (!engine)
    {
        debugMsg("pevIntrDisconnect", card, src_id, vec_id, func, usr, "pevIntrGetEngine() failed");
        return S_dev_noMemory;
    }
    
    epicsMutexLock(engine->handlerListLock);
    for (pisr = &engine->isrList; *pisr != NULL; pisr = &(*pisr)->next)
    {
        if ((*pisr)->src_id == src_id &&
            (*pisr)->vec_id == vec_id &&
            (*pisr)->func == func &&
            (usr == NULL || (*pisr)->usr == usr)) break;
    }
    if (!*pisr)
    {
        debugMsg("pevIntrDisconnect", card, src_id, vec_id, func, usr, "not connected");
        epicsMutexUnlock(engine->handlerListLock);
        return S_dev_vectorNotInUse;;
    }
    isr = *pisr;
    *pisr = (*pisr)->next;
    memset(isr, 0, sizeof(struct isrEntry));
    free (isr);
    epicsMutexUnlock(engine->handlerListLock);
    return S_dev_success;
}

struct intrHandler* pevIntrGetHandler(unsigned int card, unsigned int src_id)
{
    unsigned int priority;
    struct intrEngine* engine;
    struct intrHandler** phandler;
    char threadName[16];
    
    priority = pevIntrGetPriorityFromSrcId(src_id);
    engine = pevIntrGetEngine(card);
    epicsMutexLock(engine->handlerListLock);
    for (phandler = &engine->handlerList; *phandler != NULL; phandler = &(*phandler)->next)
    {
        if ((*phandler)->priority == priority)
        {
            if (pevIntrDebug)
            {
                epicsThreadGetName((*phandler)->tid, threadName, sizeof(threadName));
                printf("pevIntrGetHandler(card=%u, src_id=0x%02x): re-using existing handler %s\n",
                    card, src_id, threadName);
            }
            epicsMutexUnlock(engine->handlerListLock);
            return *phandler;
        }
    }
    sprintf(threadName, "pevIntr%u.%d", card, priority);
    if (pevIntrDebug)
        printf("pevIntrGetHandler(card=%u, src_id=0x%02x): creating new handler %s\n",
            card, src_id, threadName);
    *phandler = (struct intrHandler*) calloc(1, sizeof(struct intrHandler));
    if (!*phandler)
    {
        errlogPrintf("pevIntrGetHandler(card=%u, src_id=%#04x): out of memory\n",
            card, src_id);
        epicsMutexUnlock(engine->handlerListLock);
        return NULL;
    }
    (*phandler)->engine = engine;
    (*phandler)->priority = priority;
    (*phandler)->intrQueue = pevx_evt_queue_alloc(card, 0);
    if (!(*phandler)->intrQueue)
    {
        errlogPrintf("pevIntrGetHandler(card=%u, src_id=%#04x): pevx_evt_queue_alloc() failed\n",
            card, src_id);
        free(*phandler);
        *phandler = NULL;
        epicsMutexUnlock(engine->handlerListLock);
        return NULL;
    }
    (*phandler)->tid = epicsThreadCreate(threadName, priority,
        epicsThreadGetStackSize(epicsThreadStackSmall),
        pevIntrHandlerThread, *phandler);
    if (!(*phandler)->tid)
    {
        errlogPrintf("pevIntrGetHandler(card=%u, src_id=%#04x): epicsThreadCreate(%s, %d, %d, %p, %p) failed\n",
            card, src_id,
            threadName, priority,
            epicsThreadGetStackSize(epicsThreadStackSmall),
            pevIntrHandlerThread, *phandler);
        pevx_evt_queue_free(card, (*phandler)->intrQueue);
        free(*phandler);
        *phandler = NULL;
        epicsMutexUnlock(engine->handlerListLock);
        return NULL;
    }
    epicsMutexUnlock(engine->handlerListLock);
    return *phandler;
}

int pevIntrEnable(unsigned int card, unsigned int src_id)
{
    struct intrHandler* handler;

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
    struct intrHandler* handler;

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
    struct intrHandler* handler;
    unsigned int card;
    int level = args[0].ival;

    printf("pev interrupts:\n");
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        if (pevIntrList[card].handlerList || pevIntrList[card].isrList)
            printf (" card %u: %lld interrupts (%lld unhandled)\n",
                card, pevIntrList[card].intrCount, pevIntrList[card].unhandledIntrCount);
        for (handler = pevIntrList[card].handlerList; handler != NULL; handler = handler->next)
        {
            char threadName[16];
            epicsThreadGetName(handler->tid, threadName, sizeof(threadName));
            printf("  thread %s\n", threadName);
        }
        for (isr = pevIntrList[card].isrList; isr != NULL; isr = isr->next)
        {
            char *name;

            printf("   src=0x%02x (%s",
                isr->src_id, src_name[isr->src_id >> 4]);
            if (isr->src_id != EVT_SRC_VME)
                printf(" %d", (isr->src_id & 0xf) + 1);
            else if (isr->src_id & 0xf)
                printf(" %d", isr->src_id & 0xf);
            printf(")");

            if (isr->vec_id)
                printf (" vec=0x%02x (%d)", isr->vec_id, isr->vec_id);

            printf(" count=%llu", isr->intrCount);
            
            name = symbolName(isr->func, level);
            printf(" func=%s", name);
            free(name);

            name = symbolName(isr->usr, 0);
            printf (" usr=%s", name);
            free(name);
            
            printf ("\n");
        }
    }
}
static const iocshArg pevIntrShowArg0 = { "level", iocshArgInt };
static const iocshArg * const pevIntrShowArgs[] = {
    &pevIntrShowArg0,
};
static const iocshFuncDef pevIntrShowDef =
    { "pevIntrShow", 1, pevIntrShowArgs };

void pevIntrExit(void* dummy)
{
    unsigned int card;
    struct intrHandler* handler;
    
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        if (!pevIntrList[card].handlerListLock) continue;
        if (pevIntrDebug)
            printf("pevIntrExit(): stopping interrupt handling on card %d\n",
                card);

        /* make sure that no handler is active */
        epicsMutexLock(pevIntrList[card].handlerListLock);
        
        /* we cannot kill a thread nor can we wake up pevx_evt_read() */
        /* but since we don't release the lock, the thread is effectively stopped */
        
        for (handler = pevIntrList[card].handlerList; handler != NULL; handler = handler->next)
        {
            pevx_evt_queue_disable(card, handler->intrQueue);
            pevx_evt_queue_free(card, handler->intrQueue);
        }
    }
    if (pevIntrDebug)
        printf("pevIntrExit(): done\n");
}

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
