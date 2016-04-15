#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#include <errlog.h>
#include <devLib.h>
#include <iocsh.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include <epicsTime.h>
#include <epicsExit.h>
#include <epicsMessageQueue.h>
#include <symbolname.h>
#include <keypress.h>
#include <epicsExport.h>

#include "pev.h"
#include "pevPrivate.h"

/* typo in old macro names */
#ifndef DMA_START_CHAN
#define DMA_START_CHAN(chan) (chan << 4)
#endif
#define DMA_NUM_CHANNELS 2

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
    long requestCount;
    long handleCount;
    long timeoutCount;
    long lastCount;
    unsigned int busyChannels;
    long useCount[DMA_NUM_CHANNELS];
    size_t lastSize[DMA_NUM_CHANNELS];
    double lastDuration[DMA_NUM_CHANNELS];
    unsigned int lastTransferStatus[DMA_NUM_CHANNELS];
} pevDmaList[MAX_PEV_CARDS];

static epicsMutexId pevDmaListLock;

struct dmaReq {
    struct pev_ioctl_dma_req pev_dma;
    void (*callback)(void* usr, int status);
    void *usr;
};

const char* pevDmaSpaceName(unsigned int dma_space)
{
    switch (dma_space)
    {
        case DMA_SPACE_PCIE: return "PCIe";
        case DMA_SPACE_VME:  return "VME";
        case DMA_SPACE_SHM:  return "SHM";
        case DMA_SPACE_USR:  return "USR";
        case DMA_SPACE_USR1: return "USR1";
        case DMA_SPACE_USR2: return "USR2";
        case DMA_SPACE_BUF:  return "Buffer";

        case DMA_SPACE_PCIE | DMA_SPACE_WS: return "PCIe[word swap]";
        case DMA_SPACE_SHM  | DMA_SPACE_WS: return "SHM[word swap]";
        case DMA_SPACE_USR  | DMA_SPACE_WS: return "USR[word swap]";
        case DMA_SPACE_USR1 | DMA_SPACE_WS: return "USR1[word swap]";
        case DMA_SPACE_USR2 | DMA_SPACE_WS: return "USR2[word swap]";
        case DMA_SPACE_BUF  | DMA_SPACE_WS: return "Buffer[word swap]";

        case DMA_SPACE_PCIE | DMA_SPACE_DS: return "PCIe[dword swap]";
        case DMA_SPACE_SHM  | DMA_SPACE_DS: return "SHM[dword swap]";
        case DMA_SPACE_USR  | DMA_SPACE_DS: return "USR[dword swap]";
        case DMA_SPACE_USR1 | DMA_SPACE_DS: return "USR1[dword swap]";
        case DMA_SPACE_USR2 | DMA_SPACE_DS: return "USR2[dword swap]";
        case DMA_SPACE_BUF  | DMA_SPACE_DS: return "Buffer[dword swap]";

        case DMA_SPACE_PCIE | DMA_SPACE_QS: return "PCIe[qword swap]";
        case DMA_SPACE_SHM  | DMA_SPACE_QS: return "SHM[qword swap]";
        case DMA_SPACE_USR  | DMA_SPACE_QS: return "USR[qword swap]";
        case DMA_SPACE_USR1 | DMA_SPACE_QS: return "USR1[qword swap]";
        case DMA_SPACE_USR2 | DMA_SPACE_QS: return "USR2[qword swap]";
        case DMA_SPACE_BUF  | DMA_SPACE_QS: return "Buffer[qword swap]";

        case DMA_SPACE_VME  | DMA_VME_A16:    return "VME_A16";
        case DMA_SPACE_VME  | DMA_VME_A24:    return "VME_A24";
        case DMA_SPACE_VME  | DMA_VME_A32:    return "VME_A42";
        case DMA_SPACE_VME  | DMA_VME_BLT:    return "VME_BLT";
        case DMA_SPACE_VME  | DMA_VME_MBLT:   return "VME_MBLT";
        case DMA_SPACE_VME  | DMA_VME_2eVME:  return "VME_2eVME";
        case DMA_SPACE_VME  | DMA_VME_2eFAST: return "VME_2eFAST";
        case DMA_SPACE_VME  | DMA_VME_2e160:  return "VME_2eSST160";
        case DMA_SPACE_VME  | DMA_VME_2e233:  return "VME_2eSST233";
        case DMA_SPACE_VME  | DMA_VME_2e320:  return "VME_2eSST320";

        default: return "unknown";
    }
}

char* pevDmaPrintStatus(int status, char* buffer, size_t bufferlen)
/* needs propbably max 33 bytes buffer */
{
    snprintf (buffer, bufferlen, "%s%s%s%s%s%s%s%s",
        status & DMA_STATUS_RUN_RD0 ? "RUN_RD0 ": "",
        status & DMA_STATUS_RUN_RD1 ? "RUN_RD1 ": "",
        status & DMA_STATUS_RUN_WR0 ? "RUN_WR0 ": "",
        status & DMA_STATUS_RUN_WR1 ? "RUN_WR1 ": "",
        status & DMA_STATUS_ENDED ? "ENDED" : "WAITING",
        status & DMA_STATUS_DONE ? " DONE" : "",
        status & DMA_STATUS_TMO ? " TIMEOUT" : "",
        status & DMA_STATUS_ERR ? " ERROR" : "");
    return buffer;
}

int pevDmaHandleRequest(unsigned int card, struct pev_ioctl_dma_req* pev_dma)
{
    epicsTimeStamp dmaStartTime, dmaCompleteTime;
    double duration;
    int size;
    int channel;
    char buffer[40];
    int status;

    channel = (pev_dma->start_mode & 0xf0) >> 4; /* at the moment only 0 or 1 */
    size = pev_dma->size & 0x3fffffff; /* other bits are flags */
    if (pevDmaDebug >= 3)
        printf("pevDmaHandleRequest(card=%d, dmaChannel=%d): pevx_dma_move() 0x%x bytes %s:0x%lx -> %s:0x%lx\n",
            card, channel, size,
            pevDmaSpaceName(pev_dma->src_space), pev_dma->src_addr,
            pevDmaSpaceName(pev_dma->des_space), pev_dma->des_addr);
    pevDmaList[card].useCount[channel]++;
    epicsTimeGetCurrent(&dmaStartTime);
    status = pevx_dma_move(card, pev_dma); /* blocks ? */
    if (status != 0)
        errlogPrintf("pevDmaHandleRequest(card=%d, dmaChannel=%d): pevx_dma_move() 0x%x bytes %s:0x%lx -> %s:0x%lx failed: return = 0x%x errno=%s\n",
        card, channel, size,
        pevDmaSpaceName(pev_dma->src_space), pev_dma->src_addr,
        pevDmaSpaceName(pev_dma->des_space), pev_dma->des_addr,
        status, strerror(errno));
    epicsTimeGetCurrent(&dmaCompleteTime);
    pevDmaList[card].handleCount++;
    duration = epicsTimeDiffInSeconds(&dmaCompleteTime, &dmaStartTime);
    pevDmaList[card].lastSize[channel] = size;
    pevDmaList[card].lastDuration[channel] = duration;
    pevDmaList[card].lastTransferStatus[channel] = pev_dma->dma_status;
    if ((pev_dma->dma_status & (DMA_STATUS_ERR|DMA_STATUS_TMO|DMA_STATUS_DONE)) == DMA_STATUS_DONE)
    {
        if (pevDmaDebug >= 2)
            printf("pevDmaHandleRequest(card=%d, dmaChannel=%d) 0x%x bytes = %uKiB %s:0x%lx -> %s:0x%lx in %#.3gms = %.3gMiB/s status = 0x%x %s\n",
                card, channel, size, size >> 10,
                pevDmaSpaceName(pev_dma->src_space), pev_dma->src_addr,
                pevDmaSpaceName(pev_dma->des_space), pev_dma->des_addr,
                duration*1000, size/duration*1e-6,
                pev_dma->dma_status, pevDmaPrintStatus(pev_dma->dma_status, buffer, sizeof(buffer)));
        return S_dev_success;
    }
    errlogPrintf("pevDmaHandleRequest(card=%d, dmaChannel=%d): pevx_dma_move() 0x%x bytes %s:0x%lx -> %s:0x%lx failed: status = 0x%x %s\n",
        card, channel, size,
        pevDmaSpaceName(pev_dma->src_space), pev_dma->src_addr,
        pevDmaSpaceName(pev_dma->des_space), pev_dma->des_addr,
        pev_dma->dma_status, pevDmaPrintStatus(pev_dma->dma_status, buffer, sizeof(buffer)));
    return pev_dma->dma_status;
}

void pevDmaThread(void* usr)
{
    struct dmaReq dmaRequest;
    int status;
    char* fname = NULL;
    char* uname = NULL;

    unsigned int card = ((int)usr) >> 8;
    unsigned int dmaChannel = ((int)usr) & 0xff;
    unsigned int dmaChannelMask = 1 << dmaChannel;
    unsigned int dmaChannelFlag = DMA_START_CHAN(dmaChannel);
    if (pevDmaDebug)
        printf("pevDmaThread(card=%d, dmaChannel=%d) starting\n", card, dmaChannel);

    while (pevDmaControllerMask & dmaChannelMask)
    {
        if (pevDmaDebug >= 3)
            printf("pevDmaThread(card=%d, dmaChannel=%d) waiting for requests...\n", card, dmaChannel);
        pevDmaList[card].busyChannels &= ~dmaChannelMask;
        epicsMessageQueueReceive(
            pevDmaList[card].dmaMsgQ, &dmaRequest, sizeof(dmaRequest)); /* blocks */
        if (!(pevDmaControllerMask & dmaChannelMask)) break; /* see pevDmaExit */
        pevDmaList[card].busyChannels |= dmaChannelMask;

        if (pevDmaDebug >= 3)
            printf("pevDmaThread(card=%d, dmaChannel=%d) request 0x%x bytes %s:0x%lx -> %s:0x%lx\n",
                card, dmaChannel, dmaRequest.pev_dma.size & 0x3fffffff,
                pevDmaSpaceName(dmaRequest.pev_dma.src_space), dmaRequest.pev_dma.src_addr,
                pevDmaSpaceName(dmaRequest.pev_dma.des_space), dmaRequest.pev_dma.des_addr);

        dmaRequest.pev_dma.start_mode |= dmaChannelFlag;
        status = pevDmaHandleRequest(card, &dmaRequest.pev_dma);
        if (pevDmaDebug >= 3)
        {
            fname = symbolName(dmaRequest.callback, 0);
            uname = symbolName(dmaRequest.usr, 0);

            printf("pevDmaThread(card=%d, dmaChannel=%d) calling %s(%s, 0x%x)\n",
                card, dmaChannel, fname, uname, status);
        }
        dmaRequest.callback(dmaRequest.usr, status);
        if (pevDmaDebug >= 3)
        {
            printf("pevDmaThread(card=%d, dmaChannel=%d) callback %s(%s, 0x%x) complete\n",
                card, dmaChannel, fname, uname, status);
            free (uname);
            free (fname);
        }
    }

    if (pevDmaDebug)
        printf("pevDmaThread(card=%d, dmaChannel=%d) stopped\n", card, dmaChannel);
}

struct pevDmaEngine* pevDmaGetEngine(unsigned int card)
{
    unsigned int dmaChannel = 0;
    unsigned int dmaChannelMask = pevDmaControllerMask & ~(-1<<DMA_NUM_CHANNELS);
#define THREAD_NAME_LEN 16
    char threadName [THREAD_NAME_LEN];

    if (card >= MAX_PEV_CARDS)
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
    if (pevInitCard(card) != S_dev_success)
    {
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
            errlogPrintf("pevDmaGetEngine(card=%d): epicsThreadCreate(%s) failed\n",
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

    if (card >= MAX_PEV_CARDS)
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
        if (pevDmaDebug >= 3)
            printf("pevDmaUsrToBusAddr(card=%d, useraddr=%p): compare against %p\n",
                card, useraddr, bufEntry->buf.u_addr);
        if (useraddr >= bufEntry->buf.u_addr && useraddr < bufEntry->buf.u_addr + bufEntry->buf.size)
        {
            busaddr = useraddr - bufEntry->buf.u_addr + (size_t) bufEntry->buf.b_addr;
            epicsMutexUnlock(pevDmaList[card].bufListLock);
            if (pevDmaDebug >= 3)
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

    if (card >= MAX_PEV_CARDS)
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
                errlogPrintf("pevDmaRealloc(card=%d): Cannot create mutex\n",
                    card);
                epicsMutexUnlock(pevDmaListLock);
                return NULL;
            }
            if (pevInitCard(card) != S_dev_success)
            {
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
    unsigned int des_space, size_t des_addr, size_t size, unsigned int dont_use,
    unsigned int priority, void (*callback)(void* usr, int status), void *usr)
{
    struct dmaReq dmaRequest;
    int status;
    int channel;

    if (pevDmaDebug >= 3)
    {
        char* cbName = symbolName(callback, 0);
        printf("pevDmaTransfer(card=%d, src_space=0x%x=%s, src_addr=0x%zx, "
            "des_space=0x%x=%s, des_addr=0x%zx, size=0x%zx, dont_use=0x%x, "
            "priority=%d, callback=%s, usr=%p)\n",
            card, src_space, pevDmaSpaceName(src_space), src_addr,
            des_space, pevDmaSpaceName(des_space), des_addr,
            size, dont_use, priority, cbName, usr);
        free(cbName);
    }

    if (pevDmaControllerMask == 0)
    {
        errlogPrintf("pevDmaTransfer(card=%d, ...): no available DMA channel\n",
            card);
        return S_dev_noDevice;
    }
    if (dont_use != 0)
    {
        errlogPrintf("pevDmaTransfer(card=%d, ...): don't use argument 7\n",
            card);
    }
    
    /* for DMA_BLOCK_MODE max 0xff800 bytes, but we use DMA_PIPE_MODE  see PEV API p. 20
       for DMA_PIPE_MODE we have max 63 blocks of 4Kib = 258048 =  0x3f000 = 252KiB  is this correct?
       but the driver skips at                         16777215 = 0xffffff = 16Mib-1
    */
    if ((size & 0x3fffffff) > 0xffffff) /* 16MiB-1 */
    {
        errlogPrintf("pevDmaTransfer(card=%d, ...): size=0x%zx too big\n",
            card, size & 0x3fffffff);
        return S_dev_badArgument;
    }

    if ((src_space & DMA_SPACE_MASK) == DMA_SPACE_BUF)
    {
        size_t addr = pevDmaUsrToBusAddr(card, (void*)src_addr);
        if (!addr)
        {
            if (pevDmaDebug >= 3)
                printf("pevDmaTransfer(card=%d, ...): Error: source address 0x%zx is no DMA buffer!\n",
                    card, src_addr);
            return S_dev_badArgument;
        }
        if (pevDmaDebug >= 3)
            printf("pevDmaTransfer(card=%d, ...): source address 0x%zx maps to PCI address 0x%zx\n",
                card, src_addr, addr);
        src_addr = addr;
        src_space = (src_space & ~DMA_SPACE_MASK) | DMA_SPACE_PCIE;
    }
    if ((des_space & DMA_SPACE_MASK) == DMA_SPACE_BUF)
    {
        size_t addr = pevDmaUsrToBusAddr(card, (void*)des_addr);
        if (!addr)
        {
            if (pevDmaDebug >= 3)
                printf("pevDmaTransfer(card=%d, ...): Error: dest address 0x%zx is no DMA buffer!\n",
                    card, src_addr);
            return S_dev_badArgument;
        }
        if (pevDmaDebug >= 3)
            printf("pevDmaTransfer(card=%d, ...): dest address 0x%zx maps to PCI address 0x%zx\n",
                card, des_addr, addr);
        des_addr = addr;
        des_space &= (src_space & ~DMA_SPACE_MASK) | DMA_SPACE_PCIE;
    }

    pevDmaList[card].requestCount++;

    dmaRequest.pev_dma.src_space = src_space;
    dmaRequest.pev_dma.src_addr  = src_addr;
    dmaRequest.pev_dma.des_space = des_space;
    dmaRequest.pev_dma.des_addr  = des_addr;
    dmaRequest.pev_dma.size = size;

    dmaRequest.pev_dma.start_mode = DMA_MODE_PIPE; /* irrelevant for MAP_SPACE_SHM */
    dmaRequest.pev_dma.end_mode = 0;
    dmaRequest.pev_dma.intr_mode = DMA_INTR_ENA;
    dmaRequest.pev_dma.src_mode = DMA_PCIE_RR2;
    dmaRequest.pev_dma.des_mode = DMA_PCIE_RR2;

/*  Timeout is x * units
    x = 1 ... 15
    units = DMA_WAIT_1MS ... DMA_WAIT_100S
    wait_mode = DMA_WAIT_INTR | x << 4 | units
*/
    dmaRequest.pev_dma.wait_mode = DMA_WAIT_INTR | (1 << 4);

    if (callback)
    {
        if (!pevDmaGetEngine(card))
        {
            errlogPrintf("pevDmaTransfer(card=%d, ...): pevDmaGetEngine() failed\n",
                card);
            return S_dev_noMemory;
        }

        /* asynchronous DMA transfer with long timeout */
        dmaRequest.pev_dma.wait_mode |= DMA_WAIT_1S;
        dmaRequest.callback = callback;
        dmaRequest.usr = usr;
        if (pevDmaDebug >= 3)
        {
            char* cbName = symbolName(callback, 0);
            printf("pevDmaTransfer(card=%d) callback=%s: asynchronous 0x%x bytes %s:0x%lx -> %s:0x%lx\n",
            card, cbName, dmaRequest.pev_dma.size & 0x3fffffff,
            pevDmaSpaceName(dmaRequest.pev_dma.src_space), dmaRequest.pev_dma.src_addr,
            pevDmaSpaceName(dmaRequest.pev_dma.des_space), dmaRequest.pev_dma.des_addr);
            free(cbName);
        }
        return epicsMessageQueueTrySend(pevDmaList[card].dmaMsgQ, &dmaRequest, sizeof(dmaRequest));
    }

    /* find free channel */
    printf("looking for idle DMA channel\n");
    for (channel = 0; channel < DMA_NUM_CHANNELS; channel++)
    {
        if (!(pevDmaControllerMask & (1 << channel))) { printf("channel %d not allowed\n", channel); continue; }
        if (!(pevDmaList[card].busyChannels & (0x10001 << channel))) { printf ("channel %d idle\n", channel); break; }
    }
    if (channel >= DMA_NUM_CHANNELS) /* no free channel, use first allowed one and wait */
    {
        printf ("no idle channels found, use first allowed\n");
        dmaRequest.pev_dma.wait_mode = DMA_WAIT_INTR | (5 << 4); /* allow longer timeout */
        for (channel = 0; channel < DMA_NUM_CHANNELS; channel++)
        {
            if ((pevDmaControllerMask & (1 << channel))) { printf("channel %d allowed\n", channel); break; }
        }
    }

    /* synchronous DMA transfer with short timeout */
    dmaRequest.pev_dma.wait_mode |= DMA_WAIT_10MS;

    pevDmaList[card].busyChannels |= (0x10000 << channel);
    dmaRequest.pev_dma.start_mode |= DMA_START_CHAN(channel);
    if (pevDmaDebug >= 3)
    {
        printf("pevDmaTransfer(card=%d) channel=%d: synchronous 0x%x bytes %s:0x%lx -> %s:0x%lx\n",
            card, channel, dmaRequest.pev_dma.size & 0x3fffffff,
            pevDmaSpaceName(dmaRequest.pev_dma.src_space), dmaRequest.pev_dma.src_addr,
            pevDmaSpaceName(dmaRequest.pev_dma.des_space), dmaRequest.pev_dma.des_addr);
    }
    status = pevDmaHandleRequest(card, &dmaRequest.pev_dma);
    if (pevDmaDebug >= 2)
        printf("pevDmaTransfer(card=%d) status = 0x%x\n",
            card, status);
    pevDmaList[card].busyChannels &= ~(0x10000 << channel);
    return status;
}

int pevDmaGetMapInfo(const volatile void* address, struct pevMapInfo* info)
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
                info->base  = bufEntry->buf.u_addr,
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
    int period = 0;
    int anything_reported = 0;

    if (level < 0) printf("redrawing every %d seconds (press any key to stop)\n", -level);
    while (1)
    {
        for (card = 0; card < MAX_PEV_CARDS; card++)
        {
            if (pevDmaList[card].dmaMsgQ || pevDmaList[card].requestCount)
            {
                char buffer[40];

                printf("  card %d dmaMessageQueue", card);
                if (pevDmaList[card].dmaMsgQ)
                    epicsMessageQueueShow(pevDmaList[card].dmaMsgQ, level);
                else
                    printf(" not used yet\n");
                printf("   Requests: %lu, handled: %lu",
                    pevDmaList[card].requestCount, pevDmaList[card].handleCount);
                if (period)
                {
                    printf(", %.2f Hz", ((double)(pevDmaList[card].requestCount - pevDmaList[card].lastCount))/period);
                }
                pevDmaList[card].lastCount = pevDmaList[card].requestCount;
                printf ("\n");
                if (level != 0)
                {
                    struct pevDmaEngine copy;
                    int channel;

                    /* Race condition if dma finishes while we get the values. Bad luck. This is just info. */
                    copy=pevDmaList[card];

                    for (channel = 0; channel < DMA_NUM_CHANNELS; channel++)
                    {
                        printf("   Channel %d %s status=0x%x (%s) 0x%08x=%uKiB in %#.3gms = %.3gMiB/s count=%ld\n",
                            channel,
                            copy.busyChannels & (0x10000<<channel) ? "in use sync" :
                            copy.busyChannels & (0x1<<channel) ? "in use async" : "idle",
                            copy.lastTransferStatus[channel],
                            copy.lastTransferStatus[channel] == 0 ? "Never used" :
                                pevDmaPrintStatus(copy.lastTransferStatus[channel], buffer, sizeof(buffer)),
                            copy.lastSize[channel], copy.lastSize[channel] >> 10,
                            copy.lastDuration[channel]*1000, copy.lastSize[channel]/copy.lastDuration[channel]*1e-6,
                            copy.useCount[channel]);

                        if (level > 2)
                        {
                            struct pev_ioctl_dma_sts dmaStatus;
                            pevx_dma_status(card, channel, &dmaStatus);
                            printf("    DMA status: rd_csr    = 0x%08x\n",       dmaStatus.rd_csr);
                            printf("                rd_ndes   = %u\n",           dmaStatus.rd_ndes);
                            printf("                rd_cdes   = 0x%08x\n",       dmaStatus.rd_cdes);
                            printf("                rd_cnt    = 0x%08x\n",       dmaStatus.rd_cnt);
                            printf("                wr_csr    = 0x%08x\n",       dmaStatus.wr_csr);
                            printf("                wr_ndes   = %u\n",           dmaStatus.wr_ndes);
                            printf("                wr_cdes   = 0x%08x\n",       dmaStatus.wr_cdes);
                            printf("                wr_cnt    = 0x%08x\n",       dmaStatus.wr_cnt);
                            printf("          start.ctl       = 0x%08x\n",       dmaStatus.start.csr);
                            printf("          start.cnt       = 0x%08x=%s:%u\n", dmaStatus.start.cnt, pevDmaSpaceName(dmaStatus.start.cnt>>24), dmaStatus.start.cnt & 0xffffff);
                            printf("          start.shm_addr  = 0x%08x\n",       dmaStatus.start.shm_p);
                            printf("          start.next      = 0x%08x\n",       dmaStatus.start.desc_p);
                            printf("          start.rem_addr  = 0x%08x:%08x\n",  dmaStatus.start.addr_hp, dmaStatus.start.addr_lp);
                            printf("          start.status    = 0x%08x\n",       dmaStatus.start.usec);
                            printf("          start.msec      = %u\n",           dmaStatus.start.msec);
                            printf("             wr.ctl       = 0x%08x\n",       dmaStatus.wr.csr);
                            printf("             wr.cnt       = 0x%08x=%s:%u\n", dmaStatus.wr.cnt, pevDmaSpaceName(dmaStatus.wr.cnt>>24), dmaStatus.wr.cnt & 0xffffff);
                            printf("             wr.shm_addr  = 0x%08x\n",       dmaStatus.wr.shm_p);
                            printf("             wr.next      = 0x%08x\n",       dmaStatus.wr.desc_p);
                            printf("             wr.rem_addr  = 0x%08x:%08x\n",  dmaStatus.wr.addr_hp, dmaStatus.wr.addr_lp);
                            printf("             wr.status    = 0x%08x\n",       dmaStatus.wr.usec);
                            printf("             wr.msec      = %u\n",           dmaStatus.wr.msec);
                            printf("             rd.ctl       = 0x%08x\n",       dmaStatus.rd.csr);
                            printf("             rd.cnt       = 0x%08x=%s:%u\n", dmaStatus.rd.cnt, pevDmaSpaceName(dmaStatus.rd.cnt>>24), dmaStatus.rd.cnt & 0xffffff);
                            printf("             rd.shm_addr  = 0x%08x\n",       dmaStatus.rd.shm_p);
                            printf("             rd.next      = 0x%08x\n",       dmaStatus.rd.desc_p);
                            printf("             rd.rem_addr  = 0x%08x:%08x\n",  dmaStatus.rd.addr_hp, dmaStatus.rd.addr_lp);
                            printf("             rd.status    = 0x%08x\n",       dmaStatus.wr.usec);
                            printf("             rd.msec      = %u\n",           dmaStatus.wr.msec);
                        }
                    }
                }
                anything_reported = 1;
            }
            if (level > 1 && pevDmaList[card].dmaBufList)
            {
                printf ("  card %d DMA buffers:\n", card);
                for (bufEntry = pevDmaList[card].dmaBufList; bufEntry; bufEntry = bufEntry->next)
                {
                    int s;
                    char u;
                    if (bufEntry->buf.size & 0xfffff)
                    { s = bufEntry->buf.size>>10; u = 'K'; }
                    else
                    { s = bufEntry->buf.size>>20; u = 'M'; }
                    printf("   usr_addr=%p bus_addr=%p size=size=0x%zx=%u%ciB\n",
                        bufEntry->buf.u_addr, bufEntry->buf.b_addr, bufEntry->buf.size, s, u);
                }
                anything_reported = 1;
            }
        }
        if (level >= 0) break;
        period = -level;
        if (waitForKeypress(period)) break;
        printf ("\n");
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
            {
                int s;
                char u;
                if (bufEntry->buf.size & 0xfffff)
                { s = bufEntry->buf.size>>10; u = 'K'; }
                else
                { s = bufEntry->buf.size>>20; u = 'M'; }
                printf("pevDmaExit(): releasing DMA buffer card %d usr_addr=%p bus_addr=%p size=0x%zx=%u%ciB\n",
                    card, bufEntry->buf.u_addr, bufEntry->buf.b_addr, bufEntry->buf.size, s, u);
            }
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
