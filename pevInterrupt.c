#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h> 

#include <pevxulib.h>
struct pev_ioctl_evt *pevx_evt_queue_alloc(uint, int); /* missing in header */

#include <errlog.h>
#include <devLib.h>
#include <iocsh.h>
#include <epicsThread.h>
#include <epicsExit.h>
#include <epicsMutex.h>
#include <symbolname.h>
#include <keypress.h> 
#include <epicsExport.h>

#include "pev.h"
#include "pevPrivate.h"

int pevIntrDebug = 0;
epicsExportAddress(int, pevIntrDebug);

int pevIntrPrio = 73;                   /* default (lowest) priority */
epicsExportAddress(int, pevIntrPrio);

struct isrEntry {
    struct isrEntry *next;
    unsigned int src_id;
    unsigned int vec_id;
    void (*func)();
    void* usr;
    unsigned long long intrCount;
    unsigned long long lastCount;
};

struct intrHandler {
    struct intrHandler* next;
    struct intrEngine* engine;
    unsigned int priority;
    struct pev_ioctl_evt *intrQueue;
    unsigned long long intrCount;
    unsigned long long lastCount;
    unsigned long long unhandledIntrCount;
    unsigned long long queuedIntrCount;
    epicsThreadId tid;
};

static struct intrEngine {
    struct isrEntry *isrList;
    epicsMutexId handlerListLock;
    struct intrHandler *handlerList;
    unsigned long long intrCount;
    unsigned long long lastCount;
} pevIntrList[MAX_PEV_CARDS];

static epicsMutexId pevIntrListLock;

const char* pevIntrSrcName(unsigned int src_id)
{
    switch (src_id & 0xf0)
    {
        case EVT_SRC_LOC:  return "LOC";
        case EVT_SRC_VME:  return "VME";
        case EVT_SRC_DMA:  return "DMA";
        case EVT_SRC_USR:  return "USR";
        case EVT_SRC_USR1: return "USR1";
        case EVT_SRC_USR2: return "USR2";
        case 0x60:         return "SRC6"; 
        case 0x70:         return "SRC7"; 
        default:           return "????";
    }
}

void pevIntrHandlerThread(void* arg)
{
    struct intrHandler* self = arg;
    struct intrEngine* engine = self->engine;
    struct pev_ioctl_evt *intrQueue = self->intrQueue;
    epicsMutexId handlerListLock = engine->handlerListLock;
    unsigned int card = pevIntrList - engine;
    unsigned int intr, src_id, vec_id, handler_called;
    struct isrEntry* isr;
    const char* threadName = epicsThreadGetNameSelf();
    
    if (pevIntrDebug)
        printf("pevIntrHandlerThread %s starting. card = %d\n", threadName, card);

    if (pevx_evt_queue_enable(card, intrQueue) != 0)
    {
        errlogPrintf("pevIntrHandlerThread %s: pevx_evt_queue_enable failed\n", threadName);        
        return;
    }

    epicsMutexLock(handlerListLock);
    while (1)
    {
        epicsMutexUnlock(handlerListLock);
        intr = pevx_evt_read(card, intrQueue, -1); /* wait until event occurs */
        epicsMutexLock(handlerListLock); /* make sure the list does not change, see also pevIntrExit */

        engine->intrCount++;
        self->intrCount++;

        src_id = intr>>8;
        vec_id = intr&0xff;

        if (pevIntrDebug >= 3)
            printf("pevIntrHandlerThread %s: New interrupt src_id=0x%02x (%s %i) vec_id=0x%02x queue_size=%d\n",
                threadName, src_id, pevIntrSrcName(src_id), src_id & 0xf, vec_id, intrQueue->evt_cnt);

        if (intrQueue->evt_cnt > 0)
        {
            self->queuedIntrCount += intrQueue->evt_cnt;
            if (pevIntrDebug >= 2)
                printf("pevIntrHandlerThread %s: %d interrupts left in queue\n",
                    threadName, intrQueue->evt_cnt);
        }

        handler_called = FALSE;
        for (isr = pevIntrList[card].isrList; isr != NULL; isr = isr->next)
        {
            if (pevIntrDebug >= 3)
                printf("pevIntrHandlerThread %s: check isr->src_id=0x%02x isr->vec_id=0x%02x\n",
                    threadName, isr->src_id, isr->vec_id);

            if (isr->src_id & 0xff00)
            {
                /* range of src_ids  */
                if (((isr->src_id >> 8) & 0xff) < src_id || (isr->src_id & 0xff) > src_id)
                    continue;
            }
            else
                if (isr->src_id != src_id)
                    continue;

            /* if user installed handler for vector 0, accept all vectors */
            if (isr->vec_id && isr->vec_id != vec_id)
                continue;

            if (pevIntrDebug >= 3)
            {
                char* fname = symbolName(isr->func, 0);
                char* uname = symbolName(isr->usr, 0);
                printf("pevIntrHandlerThread %s: match func=%s usr=%s count=%llu\n",
                    threadName, fname, uname, isr->intrCount);
                free (uname);
                free (fname);
            }

            isr->intrCount++;

            /* pass src_id and vec_id to user function for closer inspection */
            isr->func(isr->usr, src_id, vec_id);
            handler_called = TRUE;
        }
        if (!handler_called)
        {
            self->unhandledIntrCount++;
            
            if (pevIntrDebug >= 2)
                printf("pevIntrHandlerThread %s: unhandled interrupt src_id=0x%02x (%s %i) vec_id=0x%02x\n",
                    threadName, src_id, pevIntrSrcName(src_id), src_id & 0xf, vec_id);
        }
        pevx_evt_unmask(card, intrQueue, src_id);
    }
}

struct intrEngine* pevIntrGetEngine(unsigned int card)
{
    if (pevIntrDebug)
        printf("pevIntrGetEngine(card=%u)\n", card);

    if (card > MAX_PEV_CARDS)
    {
        errlogPrintf("pevIntrGetEngine(card=%d): pev supports only %d cards\n", card, MAX_PEV_CARDS);
        return NULL;
    }

    if (pevIntrList[card].handlerListLock)
        return &pevIntrList[card];

    if (!pevIntrListLock)
    {
        errlogPrintf("pevIntrGetEngine(card=%u): pevIntrListLock is not initialized\n",
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

    if (!pevx_init(card))
    {
        errlogPrintf("pevIntrGetEngine(card=%u): pevx_init() failed\n",
            card);
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }

    /* add new card to interrupt handling */
    pevIntrList[card].handlerListLock = epicsMutexCreate();
    if (!pevIntrList[card].handlerListLock)
    {
        errlogPrintf("pevIntrGetEngine(card=%u): epicsMutexCreate() failed\n",
            card);
        epicsMutexUnlock(pevIntrListLock);
        return NULL;
    }
    epicsMutexUnlock(pevIntrListLock);
    return &pevIntrList[card];
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
    if (!engine) return S_dev_noMemory;
            
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
    if (!engine) return S_dev_noMemory;
    
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
    char* p;

/* Calculate thread priorities for interrupt handlers. 
   At the moment: max - 1 for all but VME interrupts
   VME: level 7 => max - 2, level 6 => max - 3, ...
   Interrupts from the same card with the same priority
   will be handled by the same thread (fifo based).
   NB: DMA interrupts are max - 10
*/

/* make sure that THREAD_NAME_LEN matches thead name format below */
#define THREAD_NAME_LEN 16
    char threadName[THREAD_NAME_LEN];
    
    engine = pevIntrGetEngine(card);
    if (!engine) return NULL;
    
    if (src_id >= 0x60)
    {
        errlogPrintf("pevIntrGetHandler(card=%u, src_id=%#02x): TOSCA II has only 6 interrupt controllers\n",
            card, src_id);        
        return NULL;
    }
    
/* PEV limitation and bug: We can have only 16 event queues.
   After that the driver crashes and we need to reboot the IFC.
   Thus re-use event queues:
    - one for each IRQ controller (8)
    - one for each VME IRQ levels (7)
   The VME IRQs have higher priority, all others share one priority.
*/
    priority = pevIntrPrio;
    p = threadName + sprintf(threadName, "pev%u.%s-IRQ", card, pevIntrSrcName(src_id));
    if ((src_id & 0xf0) == EVT_SRC_VME)
    {
        int lvl = src_id & 0x0f;
        if (lvl >= 1 && lvl <= 7)
        {
            sprintf(p, "%d", lvl);
            priority += lvl;
        }
    }

    epicsMutexLock(engine->handlerListLock);
    for (phandler = &engine->handlerList; *phandler != NULL; phandler = &(*phandler)->next)
    {
        char otherthreadName[THREAD_NAME_LEN];
        epicsThreadGetName((*phandler)->tid, otherthreadName, sizeof(otherthreadName));
        if (strcmp(threadName, otherthreadName) == 0)
        {
            if (pevIntrDebug)
            {
                printf("pevIntrGetHandler(card=%u, src_id=0x%02x): re-using existing handler %s\n",
                    card, src_id, otherthreadName);
            }
            epicsMutexUnlock(engine->handlerListLock);
            return *phandler;
        }
    }
    if (pevIntrDebug)
        printf("pevIntrGetHandler(card=%u, src_id=0x%02x): creating new handler %s priority %d\n",
            card, src_id, threadName, priority);
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
        errlogPrintf("pevIntrGetHandler(card=%u, src_id=%#04x): pevx_evt_queue_alloc() failed: %s\n",
            card, src_id, strerror(errno));
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

void pevIntrShow(int level)
{
    struct isrEntry* isr;
    struct intrHandler* handler;
    unsigned int card;
    long long count;
    int period = 0;
    int anything_reported = 0;

    if (level < 0) printf("redrawing every %d seconds (press any key to stop)\n", -level);
    while (1)
    {
        for (card = 0; card < MAX_PEV_CARDS; card++)
        {
            if (pevIntrList[card].handlerList || pevIntrList[card].isrList)
            {
                count = pevIntrList[card].intrCount;
                printf ("  card %u: %lld interrupts received", card, count);
                if (period)
                {
                    printf(", %.2f Hz", ((double)(count - pevIntrList[card].lastCount))/period);
                }
                pevIntrList[card].lastCount = count;
                printf ("\n");
                anything_reported = 1;
            }
            
            if (pevIntrList[card].handlerList)
                printf ("   Interrupt threads:\n");
            for (handler = pevIntrList[card].handlerList; handler != NULL; handler = handler->next)
            {
                char threadName[THREAD_NAME_LEN];
                count = handler->intrCount;
                epicsThreadGetName(handler->tid, threadName, sizeof(threadName));
                printf("    %s: %lld interrupts, %lld unhandled, %lld queued",
                    threadName, count, handler->unhandledIntrCount, handler->queuedIntrCount);
                if (period)
                {
                    printf(", %.2f Hz", ((double)(count - handler->lastCount))/period);
                    handler->unhandledIntrCount = 0;
                    handler->queuedIntrCount = 0;
                }
                handler->lastCount = count;
                printf ("\n");
            }
            if (pevIntrList[card].isrList)
                printf ("   Interrupt handlers:\n");
            for (isr = pevIntrList[card].isrList; isr != NULL; isr = isr->next)
            {
                char *name;
                count = isr->intrCount;
                printf("    src=0x%02x=%s-%d",
                    isr->src_id, pevIntrSrcName(isr->src_id), isr->src_id & 0xf);
                if (isr->src_id & 0xff00)
                {
                    /* range */
                    int s = isr->src_id >> 8;
                    if ((s & 0xf0) != (isr->src_id & 0xf0))
                        printf("-%s", pevIntrSrcName(s));
                    printf ("-%d", s & 0xf);
                }
                    
                if (isr->vec_id)
                    printf (" vec=0x%02x=%-3d", isr->vec_id, isr->vec_id);

                name = symbolName(isr->func, level);
                printf(" func=%s", name);
                free(name);

                if (level >= 0)
                {
                    name = symbolName(isr->usr, 0);
                    printf (" usr=%s", name);
                    free(name);
                }
                printf(" count=%llu", count);

                if (period)
                {
                    printf(", %.2f Hz", ((double)(count - isr->lastCount))/period);
                }
                isr->lastCount = count;

                printf ("\n");
            }
        }
        if (level >= 0) break;
        period = -level;
        if (waitForKeypress(period)) break;
        printf ("\n");
    }
    if (!anything_reported)
    {
        printf ("  No interrupt installed\n");
    }
}

void pevIntrShowFunc(const iocshArgBuf *args)
{
    int level = args[0].ival;
    
    pevIntrShow(level);
}
static const iocshArg pevIntrShowArg0 = { "level (negative for periodic update)", iocshArgInt };
static const iocshArg * const pevIntrShowArgs[] = {
    &pevIntrShowArg0,
};
static const iocshFuncDef pevIntrShowDef =
    { "pevIntrShow", 1, pevIntrShowArgs };


void pevIntrStartHandler(const iocshArgBuf *args)
{
    int card = args[0].ival;
    int src_id = args[1].ival;
    
    pevIntrGetHandler(card, src_id);
}
static const iocshArg pevIntrStartHandlerArg0 = { "card", iocshArgInt };
static const iocshArg pevIntrStartHandlerArg1 = { "src_id", iocshArgInt };
static const iocshArg * const pevIntrStartHandlerArgs[] = {
    &pevIntrStartHandlerArg0,
    &pevIntrStartHandlerArg1,
};
static const iocshFuncDef pevIntrStartHandlerDef =
    { "pevIntrStartHandler", 2, pevIntrStartHandlerArgs };

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
            if (pevIntrDebug)
            {
                char threadName[THREAD_NAME_LEN];
                epicsThreadGetName(handler->tid, threadName, sizeof(threadName));
                printf("pevIntrExit(): stopping %s\n", threadName);
                printf("pevIntrExit(): calling pevx_evt_queue_disable()\n");
            }
            pevx_evt_queue_disable(card, handler->intrQueue);
            if (pevIntrDebug) printf("pevIntrExit(): calling pevx_evt_queue_free()\n"); 
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

    pevIntrListLock = epicsMutexCreate();
    if (!pevIntrListLock)
    {
        errlogPrintf("pevIntrInit(): epicsMutexCreate() failed\n");
        return S_dev_noMemory;
    }

    epicsAtExit(pevIntrExit, NULL);

    iocshRegister(&pevIntrShowDef, pevIntrShowFunc);
    iocshRegister(&pevIntrStartHandlerDef, pevIntrStartHandler);

    return S_dev_success;
}
