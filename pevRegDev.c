#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/mman.h>
#include <pevulib.h>

#include <devLib.h>
#include <epicsExit.h>
#include <errlog.h>
#include <dbAccess.h>
#include <symbolname.h>
#include <iocsh.h>

#include <regDev.h>
#include <epicsExport.h>

#include "pev.h"

#define MAGIC 		1100 		/*  pev1100 */
#define NO_DMA_SPACE	0xFF		/* DMA space not specified */


static char cvsid_pev1100[] __attribute__((unused)) =
    "$Id: pevRegDev.c,v 1.19 2015/07/14 12:24:33 zimoch Exp $";

static int pevDrvDebug = 0;
epicsExportAddress(int, pevDrvDebug);

struct regDevice {
    unsigned long magic;
    const char* name;
    unsigned int card;
    const char* resource;
    long  baseOffset;                   /* device base in bus address space */
    volatile char* baseAddress;         /* device base in user address space */
    void* localBuffer;                  /* for block mode */
    IOSCANPVT ioscanpvt;                /* input available trigger for block mode */
    epicsUInt8 dmaSpace;                /* must be used also to contain user provided DMA_SPACE_W/D/QS */
    epicsUInt8 swap;
    epicsUInt8 vmeSwap;
    epicsUInt32 vmePktSize;
    size_t size;
    int mode;
};

/******** Support functions *****************************/ 

void pevReport(
    regDevice *device,
    int level)
{
    if (device && device->magic == MAGIC)
    {
        printf("pev %s:0x%lx\n",
            device->resource, device->baseOffset);
    }
}

static IOSCANPVT pevGetIoScanPvt(regDevice *device, unsigned int offset)
{
    if (!device || device->magic != MAGIC)
    { 
        errlogSevPrintf(errlogMajor,
            "pevGetIoScanPvt: illegal device handle\n");
        return NULL;
    }
    return device->ioscanpvt;
}

int pevRead(
    regDevice *device,
    unsigned int offset,
    unsigned int dlen,
    unsigned int nelem,
    void* pdata,
    int prio,
    regDevTransferComplete callback,
    char* user);

struct pevBlockReadContext
{
    regDevice *device;
    unsigned int offset;
    unsigned int dlen;
    unsigned int nelem;
    void* pdata;
    regDevTransferComplete callback;
    char* user;
};

void pevBlockReadCallback(struct pevBlockReadContext* ctx, int status)
{
    if (pevDrvDebug & DBG_IN)
        printf("pevBlockReadCallback %s %s: DMA block transfer complete. status = 0x%x%s.\n",
            ctx->user, ctx->device->name, status,  status & DMA_STATUS_TMO ? " TIMEOUT" : "");
    if (status == S_dev_success)
    {
        pevRead(ctx->device, ctx->offset, ctx->dlen, ctx->nelem, ctx->pdata, 0, NULL, ctx->user);
        /* notify other records about new data */
        scanIoRequest(ctx->device->ioscanpvt);
    }
    ctx->callback(ctx->user, status);
    free(ctx);
}

int pevRead(
    regDevice *device,
    unsigned int offset,
    unsigned int dlen,
    unsigned int nelem,
    void* pdata,
    int prio,
    regDevTransferComplete callback,
    char* user)
{
    int status;
  
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevRead %s: illegal device handle\n", user);
        return S_dev_noDevice;
    }
    if (!device->size) 
    {
        errlogSevPrintf(errlogMajor,
            "pevRead %s: cannot read zero size map\n", user);
        return S_dev_badRequest;
    }
    if (pevDrvDebug & DBG_IN)
    {
        char* cbName = symbolName(callback, 0);
        char* dataName = symbolName(pdata, 0);
        printf("pevRead(device=%s, offset=%d, dlen=%d, nelem=%d, pdata=@%p, prio=%d, callback=%s, user=%s)\n",
            device->name, offset, dlen, nelem, dataName, prio, cbName, user);
        free(dataName);
        free(cbName);
    }
    
    /* Use DMA if it is available and
           block mode is used and this is the trigger record
       or  this is a large array read except if in block mode this is not the trigger record.
       The first case copies to local buffer from which other records read triggered by I/O Intr.
       The second case includes asynchonous device support.
    */
    if (device->localBuffer) /* block mode */
    {
        if (prio == 2) /* prio == HIGH: this is the trigger */
        {
            struct pevBlockReadContext* ctx;

            /* transfer complete device buffer into local buffer */
            if (pevDrvDebug & DBG_IN)
                printf("pevRead %s %s: 0x%x bytes DMA to local buffer, swap=%d\n",
                    user, device->name, device->size, device->swap);
                    
            if (callback)
            {
                ctx = malloc(sizeof(struct pevBlockReadContext));
                if (ctx)
                {
                    ctx->device = device;
                    ctx->offset = offset;
                    ctx->dlen = dlen;
                    ctx->nelem = nelem;
                    ctx->pdata = pdata;
                    ctx->callback = callback;
                    ctx->user = user;
                    status = pevDmaTransfer(device->card, device->dmaSpace | device->swap, device->baseOffset,
                        DMA_SPACE_BUF, (size_t)device->localBuffer, device->size | device->vmePktSize, 0,
                        prio, (pevDmaCallback)pevBlockReadCallback, ctx);
                    if (status == S_dev_success) return ASYNC_COMPLETION; /* continue asyncronously with callback */
                }
                errlogPrintf("pevRead %s %s: out of memory for async DMA transfer! Wait for completition.\n",
                    user, device->name);
            }

            status = pevDmaTransferWait(device->card, device->dmaSpace | device->swap, device->baseOffset,
                DMA_SPACE_BUF, (size_t)device->localBuffer, device->size | device->vmePktSize, 0);
            if (status != S_dev_success)
            {
                unsigned int bufferDlen;
                unsigned int bufferNelem;
                char buffer [32];

                errlogPrintf("pevRead %s %s: DMA block transfer failed! status = 0x%x%s\n",
                    user, device->name, status, pevDmaPrintStatus(status, buffer, sizeof(buffer)));
                if (status & DMA_STATUS_TMO) return status;

                /* emulate swapping as used in DMA */
                bufferDlen = dlen;
                bufferNelem = device->size/dlen;
                switch (device->swap)
                {
                    case DMA_SPACE_WS:
                        if (dlen == 2) break;
                        if (pevDrvDebug & DBG_IN)
                            printf("pevRead %s %s: manual swap fix for WS when dlen=%d\n",
                                user, device->name, dlen);
                        bufferNelem = device->size >> 1;
                        bufferDlen = 2;
                        break;
                    case DMA_SPACE_DS:
                        if (dlen == 4) break;
                        if (pevDrvDebug & DBG_IN)
                            printf("pevRead %s %s: manual swap fix for DS when dlen=%d\n",
                                user, device->name, dlen);
                        bufferNelem = device->size >> 2;
                        bufferDlen = 4;
                        break;
                    case DMA_SPACE_QS:
                        if (dlen == 8) break;
                        if (pevDrvDebug & DBG_IN)
                            printf("pevRead %s %s: manual swap fix for QS when dlen=%d\n",
                                user, device->name, dlen);
                        bufferNelem = device->size >> 3;
                        bufferDlen = 8;
                        break;
                }

                regDevCopy(bufferDlen, bufferNelem, device->baseAddress, device->localBuffer, NULL, device->swap != 0);
            }
        }

        /* read from local buffer */
        if (pevDrvDebug & DBG_IN)
            printf("pevRead %s %s: read from local buffer (dlen=%d nelem=%"Z"d)\n",
                user, device->name, dlen, nelem);

        regDevCopy(dlen, nelem, device->localBuffer + offset, pdata, NULL, 0);
        
        if (prio == 2) /* prio == HIGH: this is the trigger */
        {
            /* notify other records about new data */
            scanIoRequest(device->ioscanpvt);
        }
        return S_dev_success;
    }
    
    /* 50 us overhead = circa 10kB on VME bus */
    if (nelem>100 && device->dmaSpace != NO_DMA_SPACE)  /* large array transfer */
    {
        if (pevDrvDebug & DBG_IN)
            printf("pevRead %s %s: 0x%x bytes DMA receive of array data, swap=%d\n",
                user, device->name, nelem * dlen, device->swap);
        
        status = pevDmaTransfer(device->card, device->dmaSpace | device->swap, device->baseOffset + offset,
            DMA_SPACE_BUF, (size_t)pdata, nelem * dlen | device->vmePktSize, 0,
            prio, (pevDmaCallback) callback, user);

        if (status == S_dev_success) return ASYNC_COMPLETION; /* continue asyncronously with callback */
        
        if (pevDrvDebug & DBG_IN && status == S_dev_badArgument)
            printf("pevRead %s %s: Address %p is not a DMA buffer. Using normal transfer\n",
                user, device->name, pdata);
        
        if (status != S_dev_badArgument) /* S_dev_badArgument = not a DMA buffer */
            errlogPrintf("pevRead %s %s: sending DMA request failed! status = 0x%x. Do normal & synchronous transfer\n",
                user, device->name, status);
    }
    
    if (pevDrvDebug & DBG_IN)
            printf("pevRead %s %s: 0x%x bytes normal copy, swap=%d\n",
                user, device->name, nelem * dlen, device->swap);
    
    /* emulate swapping as used in DMA */
    switch (device->swap)
    {
        case DMA_SPACE_WS:
            if (dlen == 2) break;
            if (pevDrvDebug & DBG_IN)
                printf("pevRead %s %s: manual swap fix for WS when dlen=%d\n",
                    user, device->name, dlen);
            nelem = (nelem * dlen) >> 1;
            dlen = 2;
            break;
        case DMA_SPACE_DS:
            if (dlen == 4) break;
            if (pevDrvDebug & DBG_IN)
                printf("pevRead %s %s: manual swap fix for DS when dlen=%d\n",
                user, device->name, dlen);
            nelem = (nelem * dlen) >> 2;
            dlen = 4;
            break;
        case DMA_SPACE_QS:
            if (dlen == 8) break;
            if (pevDrvDebug & DBG_IN)
                printf("pevRead %s %s: manual swap fix for QS when dlen=%d\n",
                user, device->name, dlen);
            nelem = (nelem * dlen) >> 3;
            dlen = 8;
            break;
    }
    
    regDevCopy(dlen, nelem, device->baseAddress + offset, pdata, NULL, device->swap != 0);
    return S_dev_success;
}

int pevWrite(
    regDevice *device,
    unsigned int offset,
    unsigned int dlen,
    unsigned int nelem,
    void* pdata,
    void* pmask,
    int prio,
    regDevTransferComplete callback,
    char* user)
{
    int status;
    int swap = 0;
   	
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevWrite: illegal device handle\n");
        return S_dev_noDevice;
    }
    if (!device->size) 
    {
        errlogSevPrintf(errlogMajor,
            "pevRead %s: cannot write zero size map\n", user);
        return S_dev_badRequest;
    }
    if (pevDrvDebug & DBG_OUT)
    {
        char* cbName = symbolName(callback, 0);
        char* dataName = symbolName(pdata, 0);
        printf("pevWrite(device=%s, offset=%d, dlen=%d, nelem=%d, pdata=@%p, pmask=@%p, prio=%d, callback=%s, user=%s)\n",
            device->name, offset, dlen, nelem, dataName, pmask, prio, cbName, user);
        free(dataName);
        free(cbName);
    }

    /* do swapping compatible with DMA swap */
    switch (device->swap)
    {
        case DMA_SPACE_WS:
            swap = 1;
            if (dlen == 2) break;
            if (pevDrvDebug & DBG_IN)
                printf("pevRead %s %s: manual swap fix for WS\n", user, device->name);
            nelem = (nelem * dlen) >> 1;
            dlen = 2;
            break;
        case DMA_SPACE_DS:
            swap = 1;
            if (dlen == 4) break;
            if (pevDrvDebug & DBG_IN)
                printf("pevRead %s %s: manual swap fix for DS\n", user, device->name);
            nelem = (nelem * dlen) >> 2;
            dlen = 4;
            break;
        case DMA_SPACE_QS:
            swap = 1;
            if (dlen == 8) break;
            if (pevDrvDebug & DBG_IN)
                printf("pevRead %s %s: manual swap fix for QS\n", user, device->name);
            nelem = (nelem * dlen) >> 3;
            dlen = 8;
            break;
    }
          
    /* Use DMA if it is available and
           block mode (FLAG_BLKMD) is used and this is the trigger record (prio=HIGH)
       or  this is a large array write (> 100 elements) except if in block mode this is not the trigger record.
       The first case copies from local buffer to which other records have written before.
       The second case includes asynchonous device support.
    */

    if (device->localBuffer) /* block mode */
    {
        /* write through local buffer */
        if (pevDrvDebug & DBG_OUT)
            printf("pevRead %s %s: write through local buffer (dlen=%d nelem=%"Z"d)\n",
                user, device->name, dlen, nelem);

        regDevCopy(dlen, nelem, pdata, device->localBuffer + offset, pmask, swap);

        if (prio == 2) /* prio == HIGH: this is the trigger */
        {
            /* write complete buffer to device */
            if (pevDrvDebug & DBG_OUT)
                printf("pevRead %s %s: 0x%x bytes DMA from local buffer, swap=%d\n",
                    user, device->name, device->size, device->swap);

            status = pevDmaTransferWait(device->card, DMA_SPACE_BUF, (size_t)device->localBuffer,
                device->dmaSpace | device->swap, device->baseOffset, device->size | device->vmePktSize, 0);
            if (status != S_dev_success)
            {
                errlogPrintf("pevWrite %s %s: DMA block transfer failed! status = 0x%x.\n",
                    user, device->name, status);
                regDevCopy(dlen, device->size/dlen, device->localBuffer,
                    device->baseAddress, NULL, swap);
            }
            /* update block read records */
//            scanIoRequest(device->ioscanpvt);
            return S_dev_success;
        }
        
        /* fall through to normal write */
    }

    if (nelem>100 && device->dmaSpace != NO_DMA_SPACE && !pmask) /* large array transfer */
    {
        if (pevDrvDebug & DBG_OUT)
            printf("pevWrite %s %s: 0x%x bytes DMA send of array data, swap=%d\n",
                user, device->name, nelem * dlen, device->swap);
        status = pevDmaTransfer(device->card, DMA_SPACE_BUF, (size_t)device->localBuffer,
            device->dmaSpace | device->swap, device->baseOffset, device->size | device->vmePktSize, 0,
            prio, (pevDmaCallback) callback, user);

        if (status == S_dev_success) return ASYNC_COMPLETION;
        if (status != S_dev_badArgument) /* S_dev_badArgument = not a DMA buffer */
            errlogPrintf("pevWrite %s %s: sending DMA request failed! status = 0x%x. Do normal & synchronous transfer\n",
                user, device->name, status);
    }

    if (pevDrvDebug & DBG_OUT)
            printf("pevWrite %s %s: 0x%x bytes normal copy, swap=%d\n",
                user, device->name, nelem * dlen, device->swap);
    
    regDevCopy(dlen, nelem, pdata, device->baseAddress + offset, pmask, swap);
    /* update block read records */
//    scanIoRequest(device->ioscanpvt);
    return S_dev_success;
}

/**
*	pevConfigure(card, name, resource, offset, protocol, intrVec, mapSize, blockMode)  
*
* 	card: 		normally 0; inceremnts if there are more cards in PCIe tree
*
*	name: 		virtual device name to refer to in EPICS record links (INP/OUT)
*
*	resource: 	PCIe resource; shared mem, VME space, FPGA user area -
*			- options: "SH_MEM", "PCIE", "VME_A16/24/32", "USR", "USR1", "USR2"
*
*	offset		offset in bytes within the resource space
*
*	protocol	VME transfer protocol for DMA. -
*                       - valid ptions: "NODMA","BLT","MBLT","2eVME","2eSST160","2eSST233","2eSST320"
*
*	blockMode	does a bock read from a mapped resource to a kernel buffer using DMA (epics records read from kernel buffer)		
*
**/

void* pevDrvDmaAlloc(
    regDevice *device,
    void* old,
    size_t size)
{
    return pevDmaRealloc(device->card, old, size);
}


static regDevSupport pevSupport = {
    pevReport,
    pevGetIoScanPvt,
    pevGetIoScanPvt,
    pevRead,
    pevWrite
};

int pevConfigure(
    unsigned int card,
    const char* name,
    const char* resource,
    unsigned int offset,
    char* protocol,
    int intrVec,
    int mapSize,
    int blockMode,
    const char* swap,
    int vmePktSize)
{
    regDevice* device;

    if (!name || !resource)
    {
        printf("usage: pevConfigure (card, \"name\", \"resource\", offset, \"protocol\", "
            "intrVec, mapSize, blockMode, \"swap\", vmePktSize)\n");
        printf("  resource = SH_MEM|PCIE|USR[1|2]|VME_{A16|A24|A32|CSR}\n");
        printf("  protocol = NODMA|BLT|MBLT|2eVME|2eSST{160|233|320} (mainly for VME resources)\n");
        printf("  blockMode = 0|1\n");
        printf("  swap = WS|DS|QS|NS (or empty) for word|double word|quad word|no swap\n");
        printf("  vmePktSize = 128|256|512|1024 (only when protocol != NODMA)\n");
            
        return -1;
    }

    device = regDevFind(name);
    if (device) 
    {
        errlogSevPrintf(errlogFatal,
            "pevConfigure %s: device already configured\n", 
            device->name);
        return S_dev_multDevice;
    }

    device = (regDevice*)calloc(1,sizeof(regDevice));
    if (device == NULL)
    {
        errlogSevPrintf(errlogFatal,
            "pevConfigure %s: out of memory\n",
            name);
        return S_dev_noMemory;
    }
  
    device->magic = MAGIC;
    device->name = strdup(name);
    device->card = card;
    device->resource = strdup(resource);
    device->dmaSpace = NO_DMA_SPACE;
    device->baseOffset = offset;
    device->size = mapSize;
  
    if (mapSize)
    {
        /* mapSize == 0  =>  interrupt only */

        if (blockMode)
        {
            device->localBuffer = pevDmaRealloc(card, NULL, mapSize);
            if (!device->localBuffer)
            {
                errlogSevPrintf(errlogFatal,
                    "pevConfigure %s: could not allocate dma buffer size %#x\n",
                    device->name, mapSize);
                return S_dev_noMemory;
            }
            scanIoInit(&device->ioscanpvt);
        }

        /* "SH_MEM", "PCIE", "VME_A16/24/32/BLT/MBLT/2eSST" */
        if (strcmp(resource, "SH_MEM") == 0 || strcmp(resource, "SHM") == 0)
        {
            device->dmaSpace = DMA_SPACE_SHM;
            /* device->flags = FLAG_BLKMD; */

            /* Babak has only DMA here. Why? */

            device->baseAddress = pevMap(card, MAP_MASTER_32,
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_SHM,
                offset, mapSize);
        }
        else
        if (strcmp(resource, "PCIE") == 0) 
        {
            device->mode = MAP_SPACE_PCIE;
	    device->dmaSpace = DMA_SPACE_PCIE;
            device->baseAddress = pevMap(card, MAP_MASTER_32,
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_PCIE,
                offset, mapSize);
        }
        else			
        if (strcmp(resource, "USR") == 0) 
        {
            device->mode = MAP_SPACE_USR;
	    device->dmaSpace = DMA_SPACE_USR;
            device->baseAddress = pevMap(card, MAP_MASTER_32,
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_USR,
                offset, mapSize);
        }
        else			
        if (strcmp(resource, "USR1") == 0) 
        {
            device->mode = MAP_SPACE_USR1;
	    device->dmaSpace = DMA_SPACE_USR1;
            device->baseAddress = pevMap(card, MAP_MASTER_32,
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_USR1,
                offset, mapSize);
        }
        else			
        if (strcmp(resource, "USR2") == 0) 
        {
            device->mode = MAP_SPACE_USR2;
	    device->dmaSpace = DMA_SPACE_USR2;
            device->baseAddress = pevMap(card, MAP_MASTER_32,
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_USR2,
                offset, mapSize);
        }
        else
        if (strcmp(resource, "VME_A16") == 0 || strcmp(resource, "A16") == 0)
        {
            device->mode = MAP_SPACE_VME;
            device->baseAddress = pevMap(card, MAP_MASTER_32,
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A16,
                offset, mapSize);
        }
        else
        if (strcmp(resource, "VME_A24") == 0 || strcmp(resource, "A24") == 0) 
        {
            device->mode = MAP_SPACE_VME;
            device->baseAddress = pevMap(card, MAP_MASTER_32,
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A24,
                offset, mapSize);
        }
        else
        if (strcmp(resource, "VME_A32") == 0 || strcmp(resource, "A32") == 0) 
        {
            device->mode = MAP_SPACE_VME;
            device->baseAddress = pevMap(card, MAP_MASTER_64,
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A32,
                offset, mapSize);
        }
        else
        if (strcmp(resource, "VME_CSR") == 0 || strcmp(resource, "CSR") == 0) 
        {
            device->mode = MAP_SPACE_VME;
            device->baseAddress = pevMap(card, MAP_MASTER_32,
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_CR,
                offset, mapSize);
        }
        else 
        { 
            errlogSevPrintf(errlogFatal,
                "pevConfigure %s: Unknown resource %s\n  valid options: SH_MEM, PCIE, VME_A16, VME_A24, VME_A32, VME_CSR\n",
                name, resource);
    	    return S_dev_uknAddrType;
        }

        if (!device->baseAddress)
        {
    	    errlogSevPrintf(errlogFatal,
                "pevConfigure %s: address mapping failed.\n",
                name);
            return S_dev_addrMapFail;
        }

        if (strcmp(protocol, "NODMA") == 0)
        {   
            device->dmaSpace = NO_DMA_SPACE;
        }

        if (device->mode == MAP_SPACE_VME && protocol && *protocol)
        {
            if (strcmp(protocol, "BLT") == 0) 
            {
                device->dmaSpace = DMA_SPACE_VME|DMA_VME_BLT;
            }
            else if (strcmp(protocol, "MBLT") == 0) 
            {
                device->dmaSpace = DMA_SPACE_VME|DMA_VME_MBLT;
            }
            else if (strcmp(protocol, "2eVME") == 0) 
            {
                device->dmaSpace = DMA_SPACE_VME|DMA_VME_2eVME;
            }
            else if (strcmp(protocol, "2eSST160") == 0) 
            {
                device->dmaSpace = DMA_SPACE_VME|DMA_VME_2e160;
            }
            else if (strcmp(protocol, "2eSST233") == 0) 
            {
                device->dmaSpace = DMA_SPACE_VME|DMA_VME_2e233;
            }
            else if (strcmp(protocol, "2eSST320") == 0) 
            {
                device->dmaSpace = DMA_SPACE_VME|DMA_VME_2e320;
            }
            else
            {
                errlogSevPrintf(errlogFatal,
                    "pevConfigure %s: Unknown vme protocol %s\n  valid options: BLT, MBLT, 2eVME, 2eSST160, 2eSST233, 2eSST320\n",
                    name, protocol);
                return S_dev_badFunction;
            }
        }

        if (swap && *swap)
        {
            if (strcmp(swap, "WS") == 0)
                device->swap = DMA_SPACE_WS;
            else
            if (strcmp(swap, "DS") == 0)
                device->swap |= DMA_SPACE_DS;
            else
            if (strcmp(swap, "QS") == 0)
                device->swap |= DMA_SPACE_QS;
            else
            if (strcmp(swap, "NS") != 0)
                errlogSevPrintf(errlogInfo,
                    "pevConfigure %s: Unknown swap mode %s\n  valid options: WS, DS, QS, NS. Using No Swap now.\n",
                    name, swap);
        }

        if (device->mode == MAP_SPACE_VME)
        {
            if (device->dmaSpace != NO_DMA_SPACE)
            {
                switch (vmePktSize) 
                {
                    case 0:
                    case 128:  
                        device->vmePktSize = DMA_SIZE_PKT_128;
	                break;
                    case 256:  
                        device->vmePktSize = DMA_SIZE_PKT_256;
	                break;
                    case 512:  
                        device->vmePktSize = DMA_SIZE_PKT_512;
	                break;
                    case 1024:  
                        device->vmePktSize = DMA_SIZE_PKT_1K;
	                break;
                    default:
                        errlogSevPrintf(errlogInfo,
                            "pevConfigure %s: Invalid vmePktSize %d\n  valid options: 128, 256, 512, 1024. Using 128 now.\n",
                             name, vmePktSize);
                        device->vmePktSize = DMA_SIZE_PKT_128;
                }
            }
        }
    } /* if (mapSize) */

    if (intrVec)
    {
        int src_id = 0;
        int i;

        scanIoInit(&device->ioscanpvt);
        if (intrVec > 0xff)
        {
            src_id = (intrVec>>8) & 0xff;
            intrVec = intrVec & 0xff;
        }
        else
        {
            switch (device->mode & 0xF000)
            {
                case MAP_SPACE_VME:
                    if (intrVec<1)
                    {
                        errlogSevPrintf(errlogFatal,
                            "pevConfigure %s: Invalid intrVec %d\n  valid options: 1 ... 255\n",
                            name, intrVec);
                        return S_dev_badVector;
                    }
                    src_id = EVT_SRC_VME_ANY_LEVEL;
                    break;
                case MAP_SPACE_USR1:
                    if (intrVec<1 || intrVec>16)
                    {
                        errlogSevPrintf(errlogFatal,
                            "pevConfigure %s: Invalid intrVec %d\n  valid options: 1 ... 16\n",
                            name, intrVec);
                        return S_dev_badVector;
                    }
                    src_id = EVT_SRC_USR1+intrVec-1;
                    intrVec = 0;
                    break;
                case MAP_SPACE_SHM:
                    src_id = EVT_SRC_DMA;
                    break;
                default:
                    src_id = EVT_SRC_LOC;
            }
        }
        pevIntrConnect(card, src_id, intrVec, scanIoRequest, device->ioscanpvt);
        /* enable interrupt (range) */
        i = src_id & 0xff;
        do {
            pevIntrEnable(card, i);
        } while (++i < ((src_id >> 8) & 0xff));
    }
 	
    regDevRegisterDevice(name, &pevSupport, device, mapSize);
    regDevRegisterDmaAlloc(device, pevDrvDmaAlloc);
    
    if (pevDrvDebug & DBG_INIT)
        printf("pevConfigure name=%s, card=%d, resource=%s, baseOffset=0x%lx, baseAddress=%p, localBuffer=%p, ioscanpvt=%p,"
                " dmaSpace=0x%x, swap=%d, vmeSwap=%d, vmePktSize=0x%08x, mode=0x%x\n",
            device->name, device->card, device->resource, device->baseOffset, device->baseAddress, device->localBuffer, device->ioscanpvt,
            device->dmaSpace, device->swap, device->vmeSwap, device->vmePktSize, device->mode);

    return S_dev_success;
}

/**
*	pevAsynConfigure(card, name, resource, offset)  
*
* 	card: 		normally 0; inceremnts if there are more cards in PCIe tree
*
*	name: 		virtual device name to refer to in EPICS record links (INP/OUT)
*
*	resource: 	PCIe resource; shared mem, VME space, FPGA user area -
*			- options: "SH_MEM", "PCIE", "VME_A16/24/32/USR1/2" -  
*
*	offset		offset in bytes within the resource space
*
*	protocol	VME transfer protocol for DMA. -
*                       - valid options: "BLT","MBLT","2eVME","2eSST160","2eSST233","2eSST320"
*
*
*	intrVec 	for VME 0-255, for 
*
*	mapSize 	currently just used for VME 
*
*	blockMode	NOT used...
*
*	swap		"DS", "WS", "QS"
*
**/

int pevAsynConfigure(
    unsigned int card,
    const char* name,
    const char* resource,
    unsigned int offset,
    char* protocol,
    int intrVec,
    int mapSize,
    int blockMode,
    const char* swap,
    int vmePktSize)
{
    static int first=1;
    if (first)
    {
        first = 0;
        errlogSevPrintf(errlogInfo,
            "pevAsynConfigure is obsolete. It only calls pevConfigure() now.\n");
    }
    return pevConfigure(card, name, resource, offset, protocol, intrVec, mapSize, 0, swap, vmePktSize);
}
 
static const iocshArg pevConfigureArg0 = { "card", iocshArgInt };
static const iocshArg pevConfigureArg1 = { "name", iocshArgString };
static const iocshArg pevConfigureArg2 = { "resource", iocshArgString };
static const iocshArg pevConfigureArg3 = { "offset", iocshArgInt };
static const iocshArg pevConfigureArg4 = { "protocol", iocshArgString };
static const iocshArg pevConfigureArg5 = { "intrVec", iocshArgInt };
static const iocshArg pevConfigureArg6 = { "mapSize", iocshArgInt };
static const iocshArg pevConfigureArg7 = { "blockMode", iocshArgInt };
static const iocshArg pevConfigureArg8 = { "swap", iocshArgString };
static const iocshArg pevConfigureArg9 = { "vmePktSize", iocshArgInt };
static const iocshArg * const pevConfigureArgs[] = {
    &pevConfigureArg0,
    &pevConfigureArg1,
    &pevConfigureArg2,
    &pevConfigureArg3,
    &pevConfigureArg4,
    &pevConfigureArg5,
    &pevConfigureArg6,
    &pevConfigureArg7,
    &pevConfigureArg8,
    &pevConfigureArg9,
};

static const iocshFuncDef pevConfigureDef =
    { "pevConfigure", 10, pevConfigureArgs };
    
static void pevConfigureFunc (const iocshArgBuf *args)
{
    int status = pevConfigure(
        args[0].ival, args[1].sval, args[2].sval, args[3].ival, args[4].sval, args[5].ival, args[6].ival, args[7].ival, args[8].sval, args[9].ival);
    if (status != 0 && !interruptAccept) epicsExit(1);
}

static const iocshFuncDef pevAsynConfigureDef =
    { "pevAsynConfigure", 10, pevConfigureArgs };
    
static void pevAsynConfigureFunc (const iocshArgBuf *args)
{
    int status = pevAsynConfigure(
        args[0].ival, args[1].sval, args[2].sval, args[3].ival, args[4].sval, args[5].ival,args[6].ival, args[7].ival, args[8].sval, args[9].ival);
    if (status != 0 && !interruptAccept) epicsExit(1);
}

static void pevDrvRegistrar (void)
{
    iocshRegister(&pevConfigureDef, pevConfigureFunc);
    iocshRegister(&pevAsynConfigureDef, pevAsynConfigureFunc);
}

epicsExportRegistrar(pevDrvRegistrar);
