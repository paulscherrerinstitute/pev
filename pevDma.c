#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#include <pevxulib.h>

#include <errlog.h>
#include <devLib.h>
#include <iocsh.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include <epicsTime.h>
#include <epicsExit.h>
#include <epicsMessageQueue.h>
#include <symbolname.h>
#include <epicsExport.h>

#include "pev.h"
#include "pevPrivate.h"

/* typo in old macro names */
#ifndef DMA_START_CTRL_1
#define DMA_START_CTRL_1 DMA_START_CTLR_1
#endif

int pevDmaDebug = 0;
epicsExportAddress(int, pevDmaDebug);

volatile int pevDmaControllerMask = 3; /* use both DMA engines */
epicsExportAddress(int, pevDmaControllerMask);

int pevDmaPrio = 72;                   /* default priority */
epicsExportAddress(int, pevDmaPrio);

/* mental note: a hash table or a binary tree would be more efficient than a linked list */
struct pevDmaBufEntry {
    struct pevDmaBufEntry* next;
    struct pev_ioctl_buf buf;
};

static struct pevDmaEngine {
    epicsMessageQueueId dmaMsgQ;
    struct pevDmaBufEntry* dmaBufList;
    epicsMutexId bufListLock;
    unsigned int lastTransferStatus;
    size_t lastSize;
    double lastDuration;
} pevDmaList[MAX_PEV_CARDS];

static epicsMutexId pevDmaListLock;

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

char* pevDmaPrintStatus(int status, char* buffer, size_t bufferlen)
/* needs propbably max 21 bytes buffer */
{
    snprintf (buffer, bufferlen, "%s%s%s%s%s%s%s%s",
        status & DMA_STATUS_RUN_RD0 ? "Read0 ": "",
        status & DMA_STATUS_RUN_RD1 ? "Read1 ": "",
        status & DMA_STATUS_RUN_WR0 ? "Write0 ": "",
        status & DMA_STATUS_RUN_WR1 ? "Write1 ": "",
        status & DMA_STATUS_ENDED ? "Ended" : "Waiting",
        status & DMA_STATUS_DONE ? " Done" : "",
        status & DMA_STATUS_TMO ? " Timeout" : "",
        status & DMA_STATUS_ERR ? " Error" : "");
    return buffer;
}

int pevDmaHandleRequest(unsigned int card, struct pev_ioctl_dma_req* pev_dma)
{
    epicsTimeStamp dmaStartTime, dmaCompleteTime;
    double duration;
    char buffer[32];

    epicsTimeGetCurrent(&dmaStartTime);
    if (pevx_dma_move(card, pev_dma) != 0) /* blocks */
    {
        errlogPrintf("pevDmaHandleRequest(card=%d, dmaChannel=%d): pevx_dma_move() 0x%x bytes %s:0x%lx -> %s:0x%lx failed: status = 0x%x %s\n",
            card, !!(pev_dma->start_mode & DMA_START_CTRL_1), pev_dma->size,
            pevDmaSpaceName(pev_dma->src_space), pev_dma->src_addr,
            pevDmaSpaceName(pev_dma->des_space), pev_dma->des_addr,
            pev_dma->dma_status, pevDmaPrintStatus(pev_dma->dma_status, buffer, sizeof(buffer)));
        return pev_dma->dma_status;
    }
    epicsTimeGetCurrent(&dmaCompleteTime);
    duration = epicsTimeDiffInSeconds(&dmaCompleteTime, &dmaStartTime);
    pevDmaList[card].lastSize = pev_dma->size;
    pevDmaList[card].lastDuration = duration;
    pevDmaList[card].lastTransferStatus = pev_dma->dma_status;
    if (pevDmaDebug)
        printf("pevDmaHandleRequest(card=%d, dmaChannel=%d) 0x%x bytes = %ukB %s:0x%lx -> %s:0x%lx in %.3gms = %.3gMB/s\n",
            card, !!(pev_dma->start_mode & DMA_START_CTRL_1), pev_dma->size, pev_dma->size >> 10,
            pevDmaSpaceName(pev_dma->src_space), pev_dma->src_addr,
            pevDmaSpaceName(pev_dma->des_space), pev_dma->des_addr,
            duration*1000, pev_dma->size/duration*1e-6);
    if (pev_dma->dma_status & DMA_STATUS_DONE) return S_dev_success;
    return pev_dma->dma_status;    
}

void pevDmaThread(void* usr)
{
    struct dmaReq dmaRequest;

    unsigned int card = ((int)usr) >> 8;
    unsigned int dmaChannel = ((int)usr) & 0xff;
    unsigned int dmaChannelMask = 1 << dmaChannel;
    unsigned int dmaChannelFlag = dmaChannel * DMA_START_CTRL_1;
    
    if (pevDmaDebug)
        printf("pevDmaThread(card=%d, dmaChannel=%d) starting\n", card, dmaChannel);

    while (pevDmaControllerMask & dmaChannelMask)
    {
        epicsMessageQueueReceive(
            pevDmaList[card].dmaMsgQ, &dmaRequest, sizeof(dmaRequest)); /* blocks */
        if (!(pevDmaControllerMask & dmaChannelMask)) break; /* see pevDmaExit */  
            
        if (pevDmaDebug)
            printf("pevDmaThread(card=%d, dmaChannel=%d) request 0x%x bytes %s:0x%lx -> %s:0x%lx\n",
                card, dmaChannel, dmaRequest.pev_dma.size,
                pevDmaSpaceName(dmaRequest.pev_dma.src_space), dmaRequest.pev_dma.src_addr,
                pevDmaSpaceName(dmaRequest.pev_dma.des_space), dmaRequest.pev_dma.des_addr);

        dmaRequest.pev_dma.start_mode |= dmaChannelFlag;
        dmaRequest.callback(dmaRequest.usr, pevDmaHandleRequest(card, &dmaRequest.pev_dma));
    }

    if (pevDmaDebug)
        printf("pevDmaThread(card=%d, dmaChannel=%d) stopped\n", card, dmaChannel);
}

struct pevDmaEngine* pevDmaGetEngine(unsigned int card)
{
    unsigned int dmaChannel = 0;
    unsigned int dmaChannelMask = pevDmaControllerMask & 3; /* only 2 dma channels at the moment */
#define THREAD_NAME_LEN 16
    char threadName [THREAD_NAME_LEN];
    
    if (card > MAX_PEV_CARDS)
    {
        errlogPrintf("pevDmaGetEngine(card=%d): pev supports only %d cards\n", card, MAX_PEV_CARDS);
        return NULL;
    }
    if (pevDmaList[card].dmaMsgQ) return &pevDmaList[card];
 
    if (!pevDmaListLock)
    {
        errlogPrintf("pevDmaGetEngine(card=%d): pevDmaListLock is not initialized\n",
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
        errlogPrintf("pevDmaGetEngine(card=%d): pevx_init() failed\n",
            card);
        epicsMutexUnlock(pevDmaListLock);
        return NULL;
    }

    /* mental note: implement priority queue! */
    pevDmaList[card].dmaMsgQ = epicsMessageQueueCreate(1000, sizeof(struct dmaReq));
    if (!pevDmaList[card].dmaMsgQ)
    {
        errlogPrintf("pevDmaGetEngine(card=%d): Cannot create message queue\n",
            card);
        epicsMutexUnlock(pevDmaListLock);
        return NULL;
    }
    
    /* distribute DMA request between DMA channels */
    while (dmaChannelMask)
    {
        sprintf(threadName, "pev%d.DMA%d", card, dmaChannel);
        if (pevDmaDebug)
            printf("pevDmaGetEngine(card=%d): starting thread %s\n",
                card, threadName);
        if (!epicsThreadCreate(threadName, pevDmaPrio,
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

size_t pevDmaUsrToBusAddr(unsigned int card, void* useraddr)
{
    struct pevDmaBufEntry *bufEntry;
    size_t busaddr;

    if (card > MAX_PEV_CARDS)
    {
        errlogPrintf("pevDmaUsrToBusAddr(card=%d, useraddr=%p): pev supports only %d cards\n", card, useraddr, MAX_PEV_CARDS);
        return 0;
    }
    if (!pevDmaList[card].bufListLock)
    {
        if (pevDmaDebug)
            printf("pevDmaUsrToBusAddr(card=%d, useraddr=%p): no DMA buffers in use\n", card, useraddr);
        return 0;
    }
    epicsMutexLock(pevDmaList[card].bufListLock);
    for (bufEntry = pevDmaList[card].dmaBufList; bufEntry; bufEntry = bufEntry->next)
    {
        if (pevDmaDebug)
            printf("pevDmaUsrToBusAddr(card=%d, useraddr=%p): compare against %p\n",
                card, useraddr, bufEntry->buf.u_addr);
        if (useraddr >= bufEntry->buf.u_addr && useraddr < bufEntry->buf.u_addr + bufEntry->buf.size)
        {
            busaddr = useraddr - bufEntry->buf.u_addr + (size_t) bufEntry->buf.b_addr;
            epicsMutexUnlock(pevDmaList[card].bufListLock);
            if (pevDmaDebug)
                printf("pevDmaUsrToBusAddr(card=%d, useraddr=%p) busaddr=0x%zx\n",
                    card, useraddr, busaddr);            
            return busaddr;
        }
    }
    epicsMutexUnlock(pevDmaList[card].bufListLock);
    if (pevDmaDebug)
        printf("pevDmaUsrToBusAddr(card=%d, useraddr=%p): not a DMA buffer\n",
            card, useraddr);
    return 0;
}

void* pevDmaRealloc(unsigned int card, void* old, size_t size)
{
    struct pevDmaBufEntry** pbufEntry = NULL;
    void* addr;
    
    if (pevDmaDebug)
        printf("pevDmaRealloc(card=%d, old=%p, size=0x%zx)\n", card, old, size);

    if (card > MAX_PEV_CARDS)
    {
        errlogPrintf("pevDmaRealloc(card=%d, old=%p, size=0x%zx): pev supports only %d cards\n", card, old, size, MAX_PEV_CARDS);
        return NULL;
    }

    /* init card and buffer list */
    if (!pevDmaList[card].bufListLock)
    {
        if (!pevDmaListLock)
        {
            errlogPrintf("pevDmaRealloc(card=%d, old=%p, size=0x%zx): pevDmaListLock is not initialized\n",
                card, old, size);
            return NULL;
        }
        epicsMutexLock(pevDmaListLock);
        /* maybe we have been initialized by another thread meanwhile? */
        if (!pevDmaList[card].bufListLock)
        {
            pevDmaList[card].bufListLock = epicsMutexCreate();
            if (!pevDmaList[card].bufListLock)
            {
                errlogPrintf("pevDmaGetEngine(card=%d): Cannot create mutex\n",
                    card);
                epicsMutexUnlock(pevDmaListLock);
                return NULL;
            }
            if (!pevx_init(card))
            {
                errlogPrintf("pevDmaRealloc(card=%d, old=%p, size=0x%zx): pevx_init() failed\n",
                    card, old, size);
                epicsMutexUnlock(pevDmaListLock);
                return NULL;
            }
        }
        epicsMutexUnlock(pevDmaListLock);
    }
    
    epicsMutexLock(pevDmaList[card].bufListLock);
    for (pbufEntry = &pevDmaList[card].dmaBufList; *pbufEntry; pbufEntry = &(*pbufEntry)->next)
    {
        if (old == (*pbufEntry)->buf.u_addr)
        {
            if (size && size <= (*pbufEntry)->buf.size)
            {
                if (pevDmaDebug)
                    printf("pevDmaRealloc(card=%d, old=%p, size=0x%zx) new size fits in old block\n",
                        card, old, size);            
                epicsMutexUnlock(pevDmaList[card].bufListLock);
                return old;
            }
            pevx_buf_free(card, &(*pbufEntry)->buf);
            if (!size)
            {
                if (pevDmaDebug)
                    printf("pevDmaRealloc(card=%d, old=%p, size=0x%zx) releasing old block\n",
                        card, old, size);
                pbufEntry = &(*pbufEntry)->next;
                free(*pbufEntry);
                epicsMutexUnlock(pevDmaList[card].bufListLock);
                return NULL;     
            }
            old = NULL;
        }
    }
    if (old)
    {
        errlogPrintf("pevDmaRealloc(card=%d, old=%p, size=0x%zx) old block not found\n",
            card, old, size);            
    }
    if (!size)
    {
        if (pevDmaDebug)
            printf("pevDmaRealloc(card=%d, old=%p, size=0x%zx) no size, return NULL\n",
                card, old, size);            
        epicsMutexUnlock(pevDmaList[card].bufListLock);
        return NULL;
    }
    if (!*pbufEntry)
    {
        if (pevDmaDebug)
            printf("pevDmaRealloc(card=%d, old=%p, size=0x%zx) creating new block\n",
                card, old, size);            
        *pbufEntry = calloc(1,sizeof (struct pevDmaBufEntry));
        if (!*pbufEntry)
        {
            errlogPrintf("pevDmaBufEntry(): out of memory\n");        
            epicsMutexUnlock(pevDmaList[card].bufListLock);
            return NULL;
        }
    }
    /* pevx_buf_alloc ultimately calls __get_free_pages which works on 2^n pages */
    size_t allocsize = getpagesize();
    while (size > allocsize && allocsize != 0) allocsize<<=1;
    
    (*pbufEntry)->buf.size = allocsize;
    addr = pevx_buf_alloc(card, &(*pbufEntry)->buf);
    if (addr == NULL || addr == MAP_FAILED)
    {
        errlogPrintf("pevDmaBufEntry(): pevx_buf_alloc() failed: %s\n", strerror(errno));
        pbufEntry = &(*pbufEntry)->next;
        free(*pbufEntry);
        epicsMutexUnlock(pevDmaList[card].bufListLock);
        return NULL;
    }
    if (pevDmaDebug)
        printf("pevDmaRealloc(card=%d, size=0x%x) u_addr=%p, b_addr=%p, k_addr=%p, size=0x%x\n",
            card, size, (*pbufEntry)->buf.u_addr, (*pbufEntry)->buf.b_addr, (*pbufEntry)->buf.k_addr, (*pbufEntry)->buf.size);
    epicsMutexUnlock(pevDmaList[card].bufListLock);
    return addr;
}

int pevDmaTransfer(unsigned int card, unsigned int src_space, size_t src_addr,
    unsigned int des_space, size_t des_addr, size_t size, unsigned int swap_mode,
    unsigned int priority, void (*callback)(void* usr, int status), void *usr)
{
    struct dmaReq dmaRequest;
    
    if (pevDmaDebug)
    {
        char* cbName = symbolName(callback, 0);
        char* usrName = symbolName(usr, 0);
        printf("pevDmaTransfer(card=%d, src_space=0x%x=%s, src_addr=0x%zx, "
            "des_space=0x%x=%s, des_addr=0x%zx, size=0x%zx, swap_mode=0x%x, "
            "priority=%d, callback=%s, usr=%s)\n",
            card, src_space, pevDmaSpaceName(src_space), src_addr,
            des_space, pevDmaSpaceName(des_space), des_addr,
            size, swap_mode, priority, cbName, usrName);
        free(usrName);
        free(cbName);
    }
    if (src_space == DMA_SPACE_BUF)
    {
        size_t addr = pevDmaUsrToBusAddr(card, (void*)src_addr);
        if (!addr) return S_dev_badArgument;
        if (pevDmaDebug)
            printf("pevDmaTransfer(card=%d, ...): source address 0x%zx maps to PCI address 0x%zx\n",
                card, src_addr, addr);
        src_addr = addr;
        src_space = 0;
    }
    if (des_space == DMA_SPACE_BUF)
    {
        size_t addr = pevDmaUsrToBusAddr(card, (void*)des_addr);
        if (!addr) return S_dev_badArgument;
        if (pevDmaDebug)
            printf("pevDmaTransfer(card=%d, ...): dest address 0x%zx maps to PCI address 0x%zx\n",
                card, des_addr, addr);
        des_addr = addr;
        des_space = 0;
    }
    
    if (!pevDmaGetEngine(card))
    {
        errlogPrintf("pevDmaTransfer(card=%d, ...): pevDmaGetEngine() failed\n",
            card);
        return S_dev_noMemory;
    }

    dmaRequest.pev_dma.src_space = src_space;
    dmaRequest.pev_dma.src_addr  = src_addr;
    dmaRequest.pev_dma.des_space = des_space;
    dmaRequest.pev_dma.des_addr  = des_addr;
    dmaRequest.pev_dma.size = size;
    
    dmaRequest.pev_dma.start_mode = DMA_MODE_PIPE; /* irrelevant for MAP_SPACE_SHM */
    dmaRequest.pev_dma.end_mode = 0;
    dmaRequest.pev_dma.intr_mode = DMA_INTR_ENA;
    dmaRequest.pev_dma.src_mode = DMA_PCIE_RR2;
    dmaRequest.pev_dma.des_mode = DMA_PCIE_RR2 | swap_mode;

/*  Timeout is x * units
    x = 1 ... 15
    units = DMA_WAIT_1MS ... DMA_WAIT_100S
    wait_mode = DMA_WAIT_INTR | x << 4 | units
*/
    dmaRequest.pev_dma.wait_mode = DMA_WAIT_INTR | (1 << 4);
    
    if (callback)
    {
        /* asynchronous DMA transfer with long timeout */
        dmaRequest.pev_dma.wait_mode |= DMA_WAIT_100S;
        dmaRequest.callback = callback;
        dmaRequest.usr = usr;
        return epicsMessageQueueTrySend(pevDmaList[card].dmaMsgQ, &dmaRequest, sizeof(dmaRequest));
    }    
    
    /* synchronous DMA transfer with short timeout */
    dmaRequest.pev_dma.wait_mode |= DMA_WAIT_1MS;
    return pevDmaHandleRequest(card, &dmaRequest.pev_dma);
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
                info->start = (size_t)bufEntry->buf.u_addr, 
                info->size  = bufEntry->buf.size;
                return 1;
            }
        }
    }
    return 0;
}

void pevDmaShow(int level)
{
    unsigned int card;
    struct pevDmaBufEntry *bufEntry;
    int anything_reported = 0;
  
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        if (pevDmaList[card].dmaMsgQ)
        {
            char buffer[32];
            
            /* Race condition if dma finishes while we get the values. Bad luck. */
            unsigned int transferStatus = pevDmaList[card].lastTransferStatus;
            size_t size =  pevDmaList[card].lastSize;
            double duration = pevDmaList[card].lastDuration;

            printf("  card %d dmaMessageQueue", card);
            epicsMessageQueueShow(pevDmaList[card].dmaMsgQ, level);
            transferStatus = pevDmaList[card].lastTransferStatus;
            printf("   Last DMA transfer Status : 0x%x %s\n",
                transferStatus, transferStatus == 0 ? "Never used" :
                pevDmaPrintStatus(transferStatus, buffer, sizeof(buffer)));
            if (transferStatus)
                printf("     0x%08x=%ukB in %.3gms = %.3gMB/s\n",
                    size, size >> 10,
                    duration*1000, size/duration*1e-6);
            anything_reported = 1;
        }
        if (pevDmaList[card].dmaBufList)
        {
            printf ("  card %d DMA buffers:\n", card);
            for (bufEntry = pevDmaList[card].dmaBufList; bufEntry; bufEntry = bufEntry->next)
            {
                int s;
                char u;
                if (bufEntry->buf.size & 0xfffff)
                { s = bufEntry->buf.size>>10; u = 'k'; }
                else
                { s = bufEntry->buf.size>>20; u = 'M'; }
                printf("   usr_addr=%p bus_addr=%p size=size=0x%zx=%u%cB\n",
                    bufEntry->buf.u_addr, bufEntry->buf.b_addr, bufEntry->buf.size, s, u);
            }
            anything_reported = 1;
        }
    }
    if (!anything_reported)
    {
        printf("  No DMA in use\n");
    }
}

void pevDmaShowFunc(const iocshArgBuf *args)
{
    int level = args[0].ival;

    pevDmaShow(level);
}
static const iocshArg pevDmaShowArg0 = { "level", iocshArgInt };
static const iocshArg * const pevDmaShowArgs[] = {
    &pevDmaShowArg0,
};
static const iocshFuncDef pevDmaShowDef =
    { "pevDmaShow", 1, pevDmaShowArgs };

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
        if (pevDmaDebug)
            printf("pevDmaExit(): stopping DMA handling on card %d\n",
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
            char threadName[THREAD_NAME_LEN];

            sprintf(threadName, "pevDma%d.%d", card, dmaChannel);
            while (epicsThreadGetId(threadName)) epicsThreadSleep(0.1);
        }
        /* free dma buffers */
        for (bufEntry = pevDmaList[card].dmaBufList; bufEntry; bufEntry = bufEntry->next)
        {
            if (pevDmaDebug)
                printf("pevDmaExit(): releasing DMA buffer card %d usr_addr=%p bus_addr=%p size=0x%zx=%uMB\n",
                    card, bufEntry->buf.u_addr, bufEntry->buf.b_addr, bufEntry->buf.size, bufEntry->buf.size>>20);
            pevx_buf_free(card, &bufEntry->buf);
        }
    }
    if (pevDmaDebug)
        printf("pevDmaExit(): done\n");
}

void pevDmaStartHandler(const iocshArgBuf *args)
{
    int card = args[0].ival;

    pevDmaGetEngine(card);
}
static const iocshArg pevDmaStartHandlerArg0 = { "card", iocshArgInt };
static const iocshArg * const pevDmaStartHandlerArgs[] = {
    &pevDmaStartHandlerArg0,
};
static const iocshFuncDef pevDmaStartHandlerDef =
    { "pevDmaStartHandler", 1, pevDmaStartHandlerArgs };

int pevDmaInit(void)
{
    if (pevDmaDebug)
        printf("pevDmaInit()\n");

    pevInstallMapInfo(pevDmaGetMapInfo);

    pevDmaListLock = epicsMutexCreate();
    if (!pevDmaListLock)
    {
        errlogPrintf("pevDmaInit(): epicsMutexCreate() failed\n");
        return S_dev_noMemory;
    }

    epicsAtExit(pevDmaExit, NULL);

    iocshRegister(&pevDmaStartHandlerDef, pevDmaStartHandler);
    iocshRegister(&pevDmaShowDef, pevDmaShowFunc);
    
    return S_dev_success;
}
