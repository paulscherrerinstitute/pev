#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include <pevxulib.h>

#include <errlog.h>
#include <devLib.h>
#include <iocsh.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include <epicsExit.h>
#include <epicsMessageQueue.h>
#include <epicsExport.h>

#include "pev.h"
#include "pevPrivate.h"

volatile int pevDmaControllerMask = 3; /* use both DMA engines */
epicsExportAddress(int, pevDmaControllerMask);

/* mental note: a hash table or a binary tree would be more efficient than a linked list */
struct pevDmaBufEntry {
    struct pevDmaBufEntry* next;
    struct pev_ioctl_buf buf;
};

LOCAL struct pevDmaEngine {
    epicsMessageQueueId dmaMsgQ;
    struct pevDmaBufEntry* dmaBufList;
    epicsMutexId bufListLock;
    unsigned int pevDmaLastTransferStatus;
} pevDmaList[MAX_PEV_CARDS];

LOCAL epicsMutexId pevDmaListLock;

struct dmaReq {
    struct pev_ioctl_dma_req pev_dma;
    void (*callback)(void* usr, int status);
    void *usr;
};
    
const char* pevDmaSpaceName(unsigned int dma_space)
{
    switch (dma_space & DMA_SPACE_MASK)
    {
        case DMA_SPACE_PCIE: return "PCIe";
        case DMA_SPACE_VME:  return "VME";
        case DMA_SPACE_SHM:  return "SHM";
        case DMA_SPACE_USR:  return "USR";
        case DMA_SPACE_USR1: return "USR1";
        case DMA_SPACE_USR2: return "USR2";
        case DMA_SPACE_BUF:  return "Buffer";
        
        case DMA_SPACE_PCIE | DMA_SPACE_WS: return "PCIe word swap";
        case DMA_SPACE_VME  | DMA_SPACE_WS: return "VME word swap";
        case DMA_SPACE_SHM  | DMA_SPACE_WS: return "SHM word swap";
        case DMA_SPACE_USR  | DMA_SPACE_WS: return "USR word swap";
        case DMA_SPACE_USR1 | DMA_SPACE_WS: return "USR1 word swap";
        case DMA_SPACE_USR2 | DMA_SPACE_WS: return "USR2 word swap";
        case DMA_SPACE_BUF  | DMA_SPACE_WS: return "Buffer word swap";
        
        case DMA_SPACE_PCIE | DMA_SPACE_DS: return "PCIe dword swap";
        case DMA_SPACE_VME  | DMA_SPACE_DS: return "VME dword swap";
        case DMA_SPACE_SHM  | DMA_SPACE_DS: return "SHM dword swap";
        case DMA_SPACE_USR  | DMA_SPACE_DS: return "USR dword swap";
        case DMA_SPACE_USR1 | DMA_SPACE_DS: return "USR1 dword swap";
        case DMA_SPACE_USR2 | DMA_SPACE_DS: return "USR2 dword swap";
        case DMA_SPACE_BUF  | DMA_SPACE_DS: return "Buffer dword swap";
        
        case DMA_SPACE_PCIE | DMA_SPACE_QS: return "PCIe qword swap";
        case DMA_SPACE_VME  | DMA_SPACE_QS: return "VME qword swap";
        case DMA_SPACE_SHM  | DMA_SPACE_QS: return "SHM qword swap";
        case DMA_SPACE_USR  | DMA_SPACE_QS: return "USR qword swap";
        case DMA_SPACE_USR1 | DMA_SPACE_QS: return "USR1 qword swap";
        case DMA_SPACE_USR2 | DMA_SPACE_QS: return "USR2 qword swap";
        case DMA_SPACE_BUF  | DMA_SPACE_QS: return "Buffer qword swap";
        
        case DMA_SPACE_PCIE | DMA_SWAP: return "PCIe swap";
        case DMA_SPACE_VME  | DMA_SWAP: return "VME swap";
        case DMA_SPACE_SHM  | DMA_SWAP: return "SHM swap";
        case DMA_SPACE_USR  | DMA_SWAP: return "USR swap";
        case DMA_SPACE_USR1 | DMA_SWAP: return "USR1 swap";
        case DMA_SPACE_USR2 | DMA_SWAP: return "USR2 swap";
        case DMA_SPACE_BUF  | DMA_SWAP: return "Buffer swap";
        
        default:             return "unknown";
    }
}

LOCAL void pevDmaThread(void* usr)
{
    struct dmaReq dmaRequest;

    unsigned int card = ((int)usr) >> 8;
    unsigned int dmaChannel = ((int)usr) & 0xff;
    unsigned int dmaChannelMask = 1 << dmaChannel;
    unsigned int dmaChannelFlag = dmaChannel * DMA_START_CTLR_1;
    
    if (pevDebug)
        printf("pevDmaThread(card=%d, dmaChannel=%d) starting\n", card, dmaChannel);

    while (pevDmaControllerMask & dmaChannelMask)
    {
        epicsMessageQueueReceive(
            pevDmaList[card].dmaMsgQ, &dmaRequest, sizeof(dmaRequest)); /* blocks */
        if (!(pevDmaControllerMask & dmaChannelMask)) break; /* see pevDmaExit */  
            
        if (pevDebug)
            printf("pevDmaThread(card=%d, dmaChannel=%d) request 0x%x bytes %s:0x%lx -> %s:0x%lx\n",
                card, dmaChannel, dmaRequest.pev_dma.size,
                pevDmaSpaceName(dmaRequest.pev_dma.src_space), dmaRequest.pev_dma.src_addr,
                pevDmaSpaceName(dmaRequest.pev_dma.des_space), dmaRequest.pev_dma.des_addr);

        dmaRequest.pev_dma.start_mode |= dmaChannelFlag;
        if (pevx_dma_move(card, &dmaRequest.pev_dma) != 0) /* blocks */
        {
            errlogPrintf("pevDmaThread(card=%d, dmaChannel=%d): pevx_dma_move() 0x%x bytes %s:0x%lx -> %s:0x%lx failed: status = 0x%x\n",
                card, dmaChannel, dmaRequest.pev_dma.size,
                pevDmaSpaceName(dmaRequest.pev_dma.src_space), dmaRequest.pev_dma.src_addr,
                pevDmaSpaceName(dmaRequest.pev_dma.des_space), dmaRequest.pev_dma.des_addr,
                dmaRequest.pev_dma.dma_status);
        }
        pevDmaList[card].pevDmaLastTransferStatus = dmaRequest.pev_dma.dma_status;
        dmaRequest.callback(dmaRequest.usr,
            dmaRequest.pev_dma.dma_status & DMA_STATUS_ENDED ? S_dev_success : dmaRequest.pev_dma.dma_status);
    }

    if (pevDebug)
        printf("pevDmaThread(card=%d, dmaChannel=%d) stopped\n", card, dmaChannel);
}

LOCAL struct pevDmaEngine* pevStartDmaEngine(unsigned int card)
{
    unsigned int dmaChannel = 0;
    unsigned int dmaChannelMask = pevDmaControllerMask & 3; /* only 2 dma channels at the moment */
    char threadName [16];
    
    if (!pevDmaListLock)
    {
        errlogPrintf("pevStartDmaEngine(card=%d): pevDmaListLock is not initialized\n",
            card);
        return NULL;
    }
    epicsMutexLock(pevDmaListLock);

    /* maybe the card has been initialized by another thread meanwhile? */
    if (pevDmaList[card].dmaMsgQ)
    {
        epicsMutexUnlock(pevDmaListLock);
        return &pevDmaList[card];
    }
    
    if (!pevx_init(card))
    {
        errlogPrintf("pevStartDmaEngine(card=%d): pevx_init() failed\n",
            card);
        epicsMutexUnlock(pevDmaListLock);
        return NULL;
    }

    /* mental note: implement priority queue! */
    pevDmaList[card].dmaMsgQ = epicsMessageQueueCreate(1000, sizeof(struct dmaReq));
    if (!pevDmaList[card].dmaMsgQ)
    {
        errlogPrintf("pevStartDmaEngine(card=%d): Cannot create message queue\n",
            card);
        epicsMutexUnlock(pevDmaListLock);
        return NULL;
    }
    
    pevDmaList[card].bufListLock = epicsMutexCreate();
    if (!pevDmaList[card].bufListLock)
    {
        errlogPrintf("pevStartDmaEngine(card=%d): Cannot create mutex\n",
            card);
        epicsMutexUnlock(pevDmaListLock);
        return NULL;
    }
    
    /* distribute DMA request between DMA channels */
    while (dmaChannelMask)
    {
        sprintf(threadName, "pevDma%d.%d", card, dmaChannel);
        if (pevDebug)
            printf("pevStartDmaEngine(card=%d): starting thread %s\n",
                card, threadName);
        if (!epicsThreadCreate(threadName, epicsThreadPriorityMax-1,
            epicsThreadGetStackSize(epicsThreadStackSmall),
            pevDmaThread, (void*)(card<<8|dmaChannel)))
        {
            errlogPrintf("pevDmaInit(card=%d): epicsThreadCreate(%s) failed\n",
                card, threadName);
            epicsMutexUnlock(pevDmaListLock);
            return NULL;
        }
        dmaChannel++;
        dmaChannelMask >>= 1;
    }
    epicsMutexUnlock(pevDmaListLock);
    return &pevDmaList[card];
}

static inline struct pevDmaEngine* pevGetDmaEngine(unsigned int card)
{
    if (card > MAX_PEV_CARDS) return NULL;
    if (pevDmaList[card].dmaMsgQ) return &pevDmaList[card];
    return pevStartDmaEngine(card);
}

LOCAL size_t pevDmaUsrToBusAddr(unsigned int card, void* useraddr)
{
    struct pevDmaBufEntry *bufEntry;
    size_t dmaaddr;

    for (bufEntry = pevDmaList[card].dmaBufList; bufEntry; bufEntry = bufEntry->next)
    {
        if (pevDebug)
            printf("pevDmaUsrToBusAddr(card=%d, useraddr=%p): compare against %p\n",
                card, useraddr, bufEntry->buf.u_addr);
        if (useraddr >= bufEntry->buf.u_addr && useraddr < bufEntry->buf.u_addr + bufEntry->buf.size)
        {
            dmaaddr = useraddr - bufEntry->buf.u_addr + (size_t) bufEntry->buf.b_addr;
            if (pevDebug)
                printf("pevDmaUsrToBusAddr(card=%d, useraddr=%p) dmaaddr=0x%zx\n",
                    card, useraddr, dmaaddr);            
            return dmaaddr;
        }
    }
    if (pevDebug)
        printf("pevDmaUsrToBusAddr(card=%d, useraddr=%p): not a DMA buffer\n",
            card, useraddr);
    return 0;
}

void* pevDmaRealloc(unsigned int card, void* old, size_t size)
{
    struct pevDmaBufEntry** pbufEntry = NULL;
    
    if (pevDebug)
        printf("pevDmaRealloc(card=%d, old=%p, size=0x%zx)\n", card, old, size);

    if (!pevGetDmaEngine(card))
    {
        errlogPrintf("pevDmaRealloc(card=%d, old=%p, size=0x%zx): pevGetDmaEngine() failed\n",
            card, old, size);
        return NULL;
    }

    epicsMutexLock(pevDmaList[card].bufListLock);
    for (pbufEntry = &pevDmaList[card].dmaBufList; *pbufEntry; pbufEntry = &(*pbufEntry)->next)
    {
        if (old == (*pbufEntry)->buf.u_addr)
        {
            if (size && size <= (*pbufEntry)->buf.size)
            {
                if (pevDebug)
                    printf("pevDmaRealloc(card=%d, old=%p, size=0x%zx) new size fits in old block\n",
                        card, old, size);            
                epicsMutexUnlock(pevDmaList[card].bufListLock);
                return old;
            }
            pevx_buf_free(card, &(*pbufEntry)->buf);
            if (!size)
            {
                if (pevDebug)
                    printf("pevDmaRealloc(card=%d, old=%p, size=0x%zx) releasing old block\n",
                        card, old, size);
                goto fail;     
            }
            old = NULL;
        }
    }
    if (old)
    {
        if (pevDebug)
            printf("pevDmaRealloc(card=%d, old=%p, size=0x%zx) old block not found\n",
                card, old, size);            
    }
    if (!size)
    {
        if (pevDebug)
            printf("pevDmaRealloc(card=%d, old=%p, size=0x%zx) no size, return NULL\n",
                card, old, size);            
        epicsMutexUnlock(pevDmaList[card].bufListLock);
        return NULL;
    }
    if (!*pbufEntry)
    {
        *pbufEntry = calloc(1,sizeof (struct pevDmaBufEntry));
        if (!*pbufEntry)
        {
            errlogPrintf("pevDmaBufEntry(): out of memory\n");        
            epicsMutexUnlock(pevDmaList[card].bufListLock);
            return NULL;
        }
    }
    (*pbufEntry)->buf.size = size;
    if (pevx_buf_alloc(card, &(*pbufEntry)->buf) == MAP_FAILED)
    {
        errlogPrintf("pevDmaBufEntry(): pevx_buf_alloc() failed\n");
        goto fail;
    }
    if ((*pbufEntry)->buf.size < size)
    {
        errlogPrintf("pevDmaBufEntry(): pevx_buf_alloc() could not allocate enough memory\n");
        goto fail;
    }
    if (pevDebug)
        printf("pevDmaRealloc(card=%d, size=0x%x) u_addr=%p, b_addr=%p, k_addr=%p\n",
            card, size, (*pbufEntry)->buf.u_addr, (*pbufEntry)->buf.b_addr, (*pbufEntry)->buf.k_addr);
    epicsMutexUnlock(pevDmaList[card].bufListLock);
    return (*pbufEntry)->buf.u_addr;
fail:
    pbufEntry = &(*pbufEntry)->next;
    free(*pbufEntry);
    epicsMutexUnlock(pevDmaList[card].bufListLock);
    return NULL;
}

int pevDmaTransfer(unsigned int card, unsigned int src_space, size_t src_addr,
    unsigned int des_space, size_t des_addr, size_t size, unsigned int swap_mode,
    unsigned int priority, void (*callback)(void* usr, int status), void *usr)
{
    struct dmaReq dmaRequest;
    
    if (pevDebug)
        printf("pevDmaTransfer(card=%d, src_space=0x%x=%s, src_addr=0x%zx, "
            "des_space=0x%x=%s, des_addr=0x%zx, size=0x%zx, swap_mode=0x%x, "
            "priority=%d, callback=%p, usr=%p)\n",
            card, src_space, pevDmaSpaceName(src_space), src_addr,
            des_space, pevDmaSpaceName(des_space), des_addr,
            size, swap_mode, priority, callback, usr);

    if (src_space == DMA_SPACE_BUF)
    {
        src_addr = pevDmaUsrToBusAddr(card, (void*)src_addr);
        if (!src_addr) return S_dev_badArgument;
        src_space = 0;
    }
    if (des_space == DMA_SPACE_BUF)
    {
        des_addr = pevDmaUsrToBusAddr(card, (void*)des_addr);
        if (!des_addr) return S_dev_badArgument;
        des_space = 0;
    }
    
    if (!pevGetDmaEngine(card))
    {
        errlogPrintf("pevDmaTransfer(card=%d, ...): pevGetDmaEngine() failed\n",
            card);
        return S_dev_noMemory;
    }

    dmaRequest.pev_dma.src_space = src_space;
    dmaRequest.pev_dma.src_addr  = src_addr;
    dmaRequest.pev_dma.des_space = des_space;
    dmaRequest.pev_dma.des_addr  = des_addr;
    dmaRequest.pev_dma.size = size;
    
    dmaRequest.pev_dma.start_mode = DMA_MODE_PIPE; /* DMA_MODE_PIPE for MAP_SPACE_SHM ? */
    dmaRequest.pev_dma.end_mode = 0;
    dmaRequest.pev_dma.intr_mode = DMA_INTR_ENA;
    dmaRequest.pev_dma.wait_mode = DMA_WAIT_INTR | DMA_WAIT_1S | (1<<4);
    dmaRequest.pev_dma.src_mode = DMA_PCIE_RR2;
    dmaRequest.pev_dma.des_mode = DMA_PCIE_RR2 | swap_mode;
    
    if (callback)
    {
        dmaRequest.callback = callback;
        dmaRequest.usr = usr;
        return epicsMessageQueueTrySend(pevDmaList[card].dmaMsgQ, &dmaRequest, sizeof(dmaRequest));
    }    
    
    if (pevx_dma_move(card, &dmaRequest.pev_dma) == 0 &&  /* blocks */
            dmaRequest.pev_dma.dma_status & DMA_STATUS_ENDED)
    {
        return S_dev_success;
    }
    
    return dmaRequest.pev_dma.dma_status;
}

int pevDmaGetMapInfo(const void* address, struct pevMapInfo* info)
{
    struct pevDmaBufEntry *bufEntry;
    unsigned int card;

    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        for (bufEntry = pevDmaList[card].dmaBufList; bufEntry; bufEntry = bufEntry->next)
        {
            if (address >= bufEntry->buf.u_addr &&
                address < bufEntry->buf.u_addr + bufEntry->buf.size)
            {
                info->name  = "DMA buffer";
                info->card  = card;
                info->start = (size_t) bufEntry->buf.b_addr;
                info->size  = bufEntry->buf.size;
                return 1;
            }
        }
    }
    return 0;
}

void pevDmaReport(int level)
{
/*
    struct pev_ioctl_dma_sts dmaStatus;
*/
    unsigned int card;
    unsigned int dmaStatus;
  
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        if (!pevDmaList[card].dmaMsgQ) continue;
        printf("\t dmaMessageQueue card %d: ", card);
        epicsMessageQueueShow(pevDmaList[card].dmaMsgQ, level);
/*  No description what pevx_dma_status does
        printf("\n\t dmaStatus = %d", pevx_dma_status(card, &dmaStatus));
*/
        dmaStatus = pevDmaList[card].pevDmaLastTransferStatus;
        printf("\n\t Last DMA transfer Status : 0x%x =", dmaStatus);
        if (dmaStatus & DMA_STATUS_WAITING) printf(" START_WAITING");
        if (dmaStatus & DMA_STATUS_TMO) printf(" TIMEOUT");
        if (dmaStatus & DMA_STATUS_ENDED) printf(" ENDED_WAITING");
        if (dmaStatus & DMA_STATUS_DONE) printf(" DONE");
        printf("\n");
    }
}

void pevDmaExit()
{
    unsigned int card;
    unsigned int dmaChannel;
    unsigned int dmaChannelMask;
    struct pevDmaBufEntry* bufEntry;

    /* terminate the DMA thread loops */
    pevDmaControllerMask = 0;

    /* destroying the queue cancels all pending requests and terminates the dma threads */
    for (card = 0; card <  MAX_PEV_CARDS; card++)
    {
        if (!pevDmaList[card].dmaMsgQ) continue;
        if (pevDebug)
            printf("pevInterruptExit(): stopping DMA handling on card %d\n",
                card);
        epicsMessageQueueDestroy(pevDmaList[card].dmaMsgQ);
    }
    
    /* wait for callbacks to return and threads to terminate */
    for (card = 0; card <  MAX_PEV_CARDS; card++)
    {
        dmaChannelMask = pevDmaControllerMask;
        dmaChannel = 0;
        while (dmaChannelMask)
        {
            char threadName [16];

            sprintf(threadName, "pevDma%d.%d", card, dmaChannel);
            while (epicsThreadGetId(threadName)) epicsThreadSleep(0.1);
        }
        /* free dma buffers */
        for (bufEntry = pevDmaList[card].dmaBufList; bufEntry; bufEntry = bufEntry->next)
        {
            if (pevDebug)
                printf("pevDmaExit(): releasing DMA buffer card %d base=%p size=0x%zx\n",
                    card, bufEntry->buf.u_addr, bufEntry->buf.size);
            pevx_buf_free(card, &bufEntry->buf);
        }
    }
    if (pevDebug)
        printf("pevDmaExit(): done\n");
}

int pevDmaInit()
{
    if (pevDebug)
        printf("pevDmaInit()\n");

    pevInstallMapInfo(pevDmaGetMapInfo);

    pevDmaListLock = epicsMutexCreate();
    if (!pevDmaListLock)
    {
        errlogPrintf("pevDmaInit(): epicsMutexCreate() failed\n");
        return S_dev_noMemory;
    }

    epicsAtExit(pevDmaExit, NULL);
    
    return S_dev_success;
}
