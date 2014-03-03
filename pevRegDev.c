#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/mman.h>
#include <pevulib.h>
#include <pev.h>
#include <regDev.h>

#include <devLib.h>
#include <epicsExit.h>
#include <errlog.h>
#include <epicsTypes.h>
#include <dbAccess.h>
#include <epicsExport.h>

#define MAGIC 		1100 		/*  pev1100 */
#define NO_DMA_SPACE	0xFF		/* DMA space not specified */


static char cvsid_pev1100[] __attribute__((unused)) =
    "$Id: pevRegDev.c,v 1.3 2014/03/03 17:08:53 zimoch Exp $";

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

/* VME slave */
typedef struct pevVmeSlaveMap {
    unsigned int a32_base; 
    unsigned int a32_size;
    unsigned int a24_base; 
    unsigned int a24_size;
} pevVmeSlaveMap;

static pevVmeSlaveMap glbVmeSlaveMap = {0, 0, 0, 0};	/* global slave map */

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
    char* user)
{
    int swap = 0;
  
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevRead %s: illegal device handle\n", user);
        return S_dev_noDevice;
    }
    if (pevDrvDebug & DBG_IN)
        printf("pevRead(device=%s, offset=%d, dlen=%d, nelem=%d, pdata=@%p, prio=%d, callback=@%p, user=%s)\n",
            device->name, offset, dlen, nelem, pdata, prio, callback, user);
    
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
           block mode is used and this is the trigger record
       or  this is a large array read except if in block mode this is not the trigger record.
       The first case copies to local buffer from which other records read triggered by I/O Intr.
       The second case includes asynchonous device support.
    */
    if (device->localBuffer) /* block mode */
    {
        if (prio == 2) /* prio == HIGH: this is the trigger */
        {
            /* transfer complete device buffer into local buffer */
            
            if (pevDrvDebug & DBG_IN)
                printf("pevRead %s %s: 0x%x bytes DMA to local buffer\n",
                    user, device->name, device->size);

            int status = pevDmaToBufferWait(device->card, device->dmaSpace | device->swap, device->baseOffset,
                device->localBuffer, device->size | device->vmePktSize, device->vmeSwap);
            if (status != S_dev_success)
            {
                errlogPrintf("pevRead %s %s: DMA block transfer failed! status = 0x%x\n",
                    user, device->name, status);
                regDevCopy(dlen, device->size/dlen, device->baseAddress,
                    device->localBuffer, NULL, swap);
            }
        }
        /* read from local buffer */
        regDevCopy(dlen, nelem, device->localBuffer + offset, pdata, NULL, swap);
        
        if (prio == 2)
        {
            /* notify other records about new data */
            scanIoRequest(device->ioscanpvt);
        }
        return S_dev_success;
    }
    
    if (nelem>100 && device->dmaSpace != NO_DMA_SPACE)  /* large array transfer */
    {
        int status;

        if (pevDrvDebug & DBG_IN)
                printf("pevRead %s %s: 0x%x bytes DMA of array data\n",
                    user, device->name, nelem * dlen);
        /* handle swapping here ! */
        
        status = pevDmaToBuffer(device->card, device->dmaSpace | device->swap, device->baseOffset + offset, pdata,
            nelem * dlen | device->vmePktSize, device->vmeSwap, prio, (pevDmaCallback) callback, user);

        if (status == S_dev_success) return ASYNC_COMPLETION;
        if (status != S_dev_badArgument) /* S_dev_badArgument = not a DMA buffer */
            errlogPrintf("pevRead %s %s: sending DMA request failed! status = 0x%x. Do normal & synchronous transfer\n",
                user, device->name, status);
    }
    
    if (pevDrvDebug & DBG_IN)
            printf("pevRead %s %s: 0x%x bytes normal copy\n",
                user, device->name, nelem * dlen);
    
    regDevCopy(dlen, nelem, device->baseAddress + offset, pdata, NULL, swap);
    
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
    int swap = 0;
   	
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevWrite: illegal device handle\n");
        return S_dev_noDevice;
    }
    if (pevDrvDebug & DBG_OUT)
        printf("pevWrite(device=%s, offset=%d, dlen=%d, nelem=%d, pdata=@%p, pmask=@%p, prio=%d, callback=@%p, user=%s)\n",
            device->name, offset, dlen, nelem, pdata, pmask, prio, callback, user);

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
        /* write into local buffer */
        regDevCopy(dlen, nelem, pdata, device->localBuffer + offset, pmask, swap);

        if (prio == 2) /* prio == HIGH: this is the trigger */
        {
            /* write complete buffer to device */
            int status = pevDmaFromBufferWait(device->card, device->localBuffer, device->dmaSpace | device->swap,
                device->baseOffset, device->size | device->vmePktSize, device->vmeSwap);
            if (status != S_dev_success)
            {
                errlogPrintf("pevWrite %s %s: DMA block transfer failed! status = 0x%x. Do normal & synchronous transfer\n",
                    user, device->name, status);
                regDevCopy(dlen, device->size/dlen, device->localBuffer,
                    device->baseAddress, NULL, swap);
            }
        }
        return S_dev_success;
    }

    if (nelem>100 && device->dmaSpace != NO_DMA_SPACE && !pmask) /* large array transfer */
    {
        int status = pevDmaFromBuffer(device->card, pdata, device->dmaSpace | device->swap, device->baseOffset + offset,
            nelem * dlen | device->vmePktSize, device->vmeSwap, prio, (pevDmaCallback) callback, user);

        if (status == S_dev_success) return ASYNC_COMPLETION;
        if (status != S_dev_badArgument) /* S_dev_badArgument = not a DMA buffer */
            errlogPrintf("pevRead %s %s: sending DMA request failed! status = 0x%x. Do normal & synchronous transfer\n",
                user, device->name, status);
    }

    regDevCopy(dlen, nelem, pdata, device->baseAddress + offset, pmask, swap);

    return S_dev_success;
}

/**
*	pevConfigure(card, name, resource, offset, vmeProtocol, intrVec, mapSize, blockMode)  
*
* 	card: 		normally 0; inceremnts if there are more cards in PCIe tree
*
*	name: 		virtual device name to refer to in EPICS record links (INP/OUT)
*
*	resource: 	PCIe resource; shared mem, VME space, FPGA user area -
*			- options: "SH_MEM", "PCIE", "VME_A16/24/32" -  ("USR" is missing!!!)
*
*	offset		offset in bytes within the resource space
*
*	vmeProtocol	VME transfer protocol for DMA. -
*                       - valid ptions: "BLT","MBLT","2eVME","2eSST160","2eSST233","2eSST320"
*
*
*	size 		? this parameter might be added for how much kernel memory to be allocated for DMA
*
*	 blockMode	does a bock read from a mapped resource to a kernel buffer using DMA (epics records read from kernel buffer)		
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
    char* vmeProtocol,
    int intrVec,
    int mapSize,
    int blockMode,
    const char* swap,
    int vmePktSize)
{
    regDevice* device;

    if (!name || !resource || !mapSize)
    {
        printf("usage: pevConfigure (card, \"name\", "
            "\"SH_MEM\"|\"PCIE\"|\"VME_A16\"|\"VME_A24\"|\"VME_A32\", "
            "offset, \"BLT\"|\"MBLT\"|\"2eVME\"|\"2eSST160\"|\"2eSST233\"|\"2eSST320\", "
            "intrVec, dmaSize, blockMode, \"WS\"|\"DS\"|\"QS\", vmePktSize)\n");
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
    if (strcmp(resource, "SH_MEM") == 0)
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
    if (strcmp(resource, "VME_A16") == 0)
    {
        device->mode = MAP_SPACE_VME;
        device->baseAddress = pevMap(card, MAP_MASTER_32,
            MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A16,
            offset, mapSize);
    }
    else
    if (strcmp(resource, "VME_A24") == 0) 
    {
        device->mode = MAP_SPACE_VME;
        device->baseAddress = pevMap(card, MAP_MASTER_32,
            MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A24,
            offset, mapSize);
    }
    else
    if (strcmp(resource, "VME_A32") == 0) 
    {
        device->mode = MAP_SPACE_VME;
        device->baseAddress = pevMap(card, MAP_MASTER_64,
            MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A32,
            offset, mapSize);
    }
    else
    if (strcmp(resource, "VME_CSR") == 0) 
    {
        device->mode = MAP_SPACE_VME;
        device->baseAddress = pevMap(card, MAP_MASTER_32,
            MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_CR,
            offset, mapSize);
    }
    else 
    { 
        errlogSevPrintf(errlogFatal,
            "pevConfigure %s: Unknown PCIe remote area %s - valid options: SH_MEM, PCIE, VME_A16/24/32/CSR\n",
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

    if (strcmp(vmeProtocol, "NODMA") == 0)
    {   
        device->dmaSpace = NO_DMA_SPACE;
    }
    
    if (device->mode == MAP_SPACE_VME && vmeProtocol && *vmeProtocol)
    {
        if (strcmp(vmeProtocol, "BLT") == 0) 
        {
            device->dmaSpace = DMA_SPACE_VME|DMA_VME_BLT;
        }
        else if (strcmp(vmeProtocol, "MBLT") == 0) 
        {
            device->dmaSpace = DMA_SPACE_VME|DMA_VME_MBLT;
        }
        else if (strcmp(vmeProtocol, "2eVME") == 0) 
        {
            device->dmaSpace = DMA_SPACE_VME|DMA_VME_2eVME;
        }
        else if (strcmp(vmeProtocol, "2eSST160") == 0) 
        {
            device->dmaSpace = DMA_SPACE_VME|DMA_VME_2e160;
        }
        else if (strcmp(vmeProtocol, "2eSST233") == 0) 
        {
            device->dmaSpace = DMA_SPACE_VME|DMA_VME_2e233;
        }
        else if (strcmp(vmeProtocol, "2eSST320") == 0) 
        {
            device->dmaSpace = DMA_SPACE_VME|DMA_VME_2e320;
        }
        else
        {
            errlogSevPrintf(errlogFatal,
                "pevConfigure %s: Unknown vme protocol %s - valid options: BLT, MBLT, 2eVME, 2eSST160, 2eSST233, 2eSST320\n",
                name, resource);
            return S_dev_badFunction;
        }
    }
  
    if (device->mode != MAP_SPACE_VME)
    {
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
        }
    }
    else
    {
        if (swap && *swap)
        {
            if (strcmp(swap, "WS") == 0)
                device->vmeSwap = DMA_SPACE_WS;
            else
            if (strcmp(swap, "DS") == 0)
                device->vmeSwap = DMA_SPACE_DS;
            else
            if (strcmp(swap, "QS") == 0)
                device->vmeSwap = DMA_SPACE_QS;
        }
        else
        {
            device->vmeSwap = DMA_VME_SWAP;
        }

        if (device->dmaSpace != NO_DMA_SPACE)
        {
            switch (vmePktSize) 
            {
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
                        "pevConfigure: vmePktSize parameter not specified/wrong - set to DMA_SIZE_PKT_128\n");
                    device->vmePktSize = DMA_SIZE_PKT_128;
            }
        }
    }

    if (intrVec)
    {
        int src_id = 0;

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
                            "pevConfigure: intrVec out of range (1<=VME<=255)\n");
                        return S_dev_badVector;
                    }
                    src_id = EVT_SRC_VME;
                    break;
                case MAP_SPACE_USR1:
                    if (intrVec<1 || intrVec>16)
                    {
                        errlogSevPrintf(errlogFatal,
                            "pevConfigure: intrVec out of range (1<=USR1<=16)\n");
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
    }
 	
    regDevRegisterDevice(name, &pevSupport, device, mapSize);
    regDevRegisterDmaAlloc(device, pevDrvDmaAlloc);

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
*	vmeProtocol	VME transfer protocol for DMA. -
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
    char* vmeProtocol,
    int intrVec,
    int mapSize,
    int blockMode,
    const char* swap,
    int vmePktSize)
{
    return pevConfigure(card, name, resource, offset, vmeProtocol, intrVec, mapSize, 0, swap, vmePktSize);
}

/**
*
* EPICS shell debug utility
*
**/
int pevExpertReport(int level)
{ 
 printf("pevExpertReport:");
 if(glbVmeSlaveMap.a32_size == 0)
   printf("\n\t Main VME slave window (A32) has been turned OFF!\n");
 else
   printf("\n\t Main VME slave window (A32) at 0x%x [size = 0x%x]\n", glbVmeSlaveMap.a32_base, glbVmeSlaveMap.a32_size);
 
    if (level > 0)
    {
        printf("DMA:");
        pevDmaReport(level);
    }
    return 0;
}
 
/**
*
* end of dma multi-user request handling
*
**/

/**
**  VME slave window setup
**  
**  addrSpace : "AM32" or "AM24"
**/

int pevVmeSlaveMainConfig(const char* addrSpace, unsigned int mainBase, unsigned int mainSize)
{
  struct pev_ioctl_vme_conf vme_conf;
  struct pev_ioctl_map_ctl vme_slv_map;
  
  if (!addrSpace)
  {
    printf("usage: pevVmeSlaveMainConfig (\"A24\"|\"A32\", base, size)\n");
    return -1;
  }
  
  pev_vme_conf_read(&vme_conf);

  if (strcmp(addrSpace, "AM32") == 0)
  { 
    if (glbVmeSlaveMap.a32_size != 0)
    {
      printf("pevVmeSlaveMainConfig(): ERROR, Main Config can be done only once, current a32_size = 0x%x a32_base = 0x%x \n", 
      				glbVmeSlaveMap.a32_size, glbVmeSlaveMap.a32_base);
      return -1;
    }
    vme_slv_map.sg_id = MAP_SLAVE_VME;	/* first clear all default winodws */
    pev_map_clear(&vme_slv_map);
    vme_conf.a32_size = mainSize;
    vme_conf.a32_base = mainBase;
    glbVmeSlaveMap.a32_size = vme_conf.a32_size;
    glbVmeSlaveMap.a32_base = vme_conf.a32_base;
    if (mainSize == 0)			/* turn off slave window */
    {
      vme_conf.slv_ena = 0;
      printf("\t => pevVmeSlaveMainConfig(): turnning OFF the whole vme slave window !!\n");
    }
    else 
      vme_conf.slv_ena = 1; 
  }
  else
  if (strcmp(addrSpace, "AM24") == 0)
  { 
    if (glbVmeSlaveMap.a24_size != 0)
    {
      printf("\t => pevVmeSlaveMainConfig(): ERROR, Main Config can be done only once, current \
              a24_size = 0x%x a24_base = 0x%x \n", glbVmeSlaveMap.a24_size, glbVmeSlaveMap.a24_base);
      return -1;
    }
    vme_conf.a24_size = mainSize;
    vme_conf.a24_base = mainBase;
    glbVmeSlaveMap.a24_size = vme_conf.a24_size;
    glbVmeSlaveMap.a24_base = vme_conf.a24_base;
  }
  else
  {
    printf("\t => pevVmeSlaveMainConfig(): ERROR, invalid addrSpace (must be \"AM32\" or \"AM24\")\n");
    return -1;
  }
  pev_vme_conf_write(&vme_conf);
  pev_vme_conf_read(&vme_conf);
  if (strcmp(addrSpace, "AM32") == 0 && vme_conf.slv_ena && (vme_conf.a32_base != mainBase || vme_conf.a32_size != mainSize))
  {
    printf("  pevVmeSlaveMainConfig(): == ERROR == mapping failed! \n\t current A32 slaveBase 0x%x [size 0x%x] \
    		\n\t Address/Size Granularity must be 16MB=0x1000000\n", vme_conf.a32_base, vme_conf.a32_size);
    return -1;
  }

  return 0;
}

/**
**  VME slave window setup
**  slaveAddrSpace 	:	"AM32" or "AM24"
**  winBase		:	baseOffset in main VME slave space  		
**  winSize		:	window size
**  vmeProtocol		:	"BLT","MBLT","2eVME","2eSST160","2eSST233","2eSST320"
**  target		: 	"SH_MEM", "PCIE", "USR1/2"
**  targetOffset	:	offset in the remote target
**  swapping		:	"WS", "DS" or "QS"
**/
 
int pevVmeSlaveTargetConfig(const char* slaveAddrSpace, unsigned int winBase,  unsigned int winSize, 
		const char* vmeProtocol, const char* target, unsigned int targetOffset, const char* swapping)
{
  struct pev_ioctl_map_pg vme_slv_map;
  unsigned int mainSlaveBase = 0;
  epicsAddressType addrType; int dummyMap;
  epicsUInt8 dmaSpace = 0;		/*  must be used also to contain user provided DMA_SPACE_W/D/QS */

  if (!slaveAddrSpace || !vmeProtocol || !swapping)
  {
    printf("usage: pevVmeSlaveTargetConfig (\"A24\"|\"A32\", base, size, \"BLT\"|\"MBLT\"|\"2eVME\"|\"2eSST160\"|\"2eSST233\"|\"2eSST320\", \"SH_MEM\"|\"PCIE\"|\"USR1/2\", offset, \"WS\"|\"DS\"|\"QS\")\n");
    return -1;
  }
  
  if (strcmp(slaveAddrSpace, "AM32") == 0)
  { 
    if (glbVmeSlaveMap.a32_size == 0)
    {
      printf("pevVmeSlaveTargetConfig(): ERROR, pevVmeSlaveMainConfig() has to be called first, Main \
              a32_size = 0x%x \n", glbVmeSlaveMap.a32_size);
      return -1;
    }
    addrType = atVMEA32;
    mainSlaveBase = glbVmeSlaveMap.a32_base;
  }
  else
  if (strcmp(slaveAddrSpace, "AM24") == 0)
  { 
    if (glbVmeSlaveMap.a24_size == 0)
    {
      printf("pevVmeSlaveTargetConfig(): ERROR, pevVmeSlaveMainConfig() has to be called first, Main \
              a24_size = 0x%x \n", glbVmeSlaveMap.a24_size);
      return -1;
    }
    addrType = atVMEA24;
    mainSlaveBase = glbVmeSlaveMap.a24_base;
  }
  else
  {
    printf("pevVmeSlaveTargetConfig(): ERROR, invalid slaveAddrSpace (must be \"AM32\" or \"AM24\")\n");
    return -1;
  }
  if (strcmp(swapping, "WS") == 0)
    dmaSpace = DMA_SPACE_WS;
  if (strcmp(swapping, "DS") == 0)
    dmaSpace = DMA_SPACE_DS;
  if (strcmp(swapping, "QS") == 0)
    dmaSpace = DMA_SPACE_QS;
  
/*
  if(strcmp(vmeProtocol, "BLT") == 0) 
    dmaSpace = DMA_SPACE_VME|DMA_VME_BLT;
  else if(strcmp(vmeProtocol, "MBLT") == 0) 
    dmaSpace = DMA_SPACE_VME|DMA_VME_MBLT;
  else if(strcmp(vmeProtocol, "2eVME") == 0) 
    dmaSpace = DMA_SPACE_VME|DMA_VME_2eVME;
  else if(strcmp(vmeProtocol, "2eSST160") == 0) 
    dmaSpace = DMA_SPACE_VME|DMA_VME_2e160;
  else if(strcmp(vmeProtocol, "2eSST233") == 0) 
    dmaSpace = DMA_SPACE_VME|DMA_VME_2e233;
  else if(strcmp(vmeProtocol, "2eSST320") == 0) 
    dmaSpace = DMA_SPACE_VME|DMA_VME_2e320;
*/    
  vme_slv_map.loc_addr = winBase;	       /* offset within main VME slave window */
  vme_slv_map.rem_addr = targetOffset;  	       /* offset within target space */
  vme_slv_map.mode = MAP_ENABLE|MAP_ENABLE_WR;
  vme_slv_map.flag = MAP_FLAG_FORCE;
  vme_slv_map.sg_id = MAP_SLAVE_VME;
  vme_slv_map.size = winSize;
  if (strcmp(target, "SH_MEM") == 0)
    vme_slv_map.mode |= MAP_SPACE_SHM | dmaSpace;
  else
  if (strcmp(target, "USR1") == 0)
    vme_slv_map.mode |= MAP_SPACE_USR1 | dmaSpace;
  else
  if (strcmp(target, "PCIE") == 0)
    vme_slv_map.mode |= MAP_SPACE_PCIE | dmaSpace;
  else
  {
    printf("  pevVmeSlaveTargetConfig(): == ERROR == invalid target [valid options: SH_MEM, USR1, PCIE]\n");
    return -1;
  }

  pevMap(0, MAP_SLAVE_VME, MAP_ENABLE|MAP_ENABLE_WR, winBase, winSize);

  if (pev_map_alloc(&vme_slv_map) || vme_slv_map.win_size!=winSize)
  {
    printf("  pevVmeSlaveTargetConfig(): == ERROR == translation window allocation failed! \
    		\n\t [allocated mapped size 0x%x] \n\t Address/Size Granularity must be 1MB=0x100000 \n", vme_slv_map.win_size);
    return -1;
  }

  if (devRegisterAddress (
  	     target,			
  	     addrType,  		    
  	     mainSlaveBase+winBase,		       
  	     winSize,		 
  	     (volatile void **)(void *)&dummyMap) != 0) {
      printf("  pevVmeSlaveTargetConfig(): == ERROR == devRegisterAddress() failed for slave at 0x%x\n", mainSlaveBase+winBase);
      return -1;
  }
  printf("\t Mapped offset 0x%x in \"%s\" to VMEslave [%s] at 0x%x [size 0x%x]\n"
  		, targetOffset, target, slaveAddrSpace, mainSlaveBase+winBase, vme_slv_map.win_size);
  return 0;
}


#ifdef EPICS_3_14

#include <iocsh.h>
static const iocshArg pevConfigureArg0 = { "card", iocshArgInt };
static const iocshArg pevConfigureArg1 = { "name", iocshArgString };
static const iocshArg pevConfigureArg2 = { "resource", iocshArgString };
static const iocshArg pevConfigureArg3 = { "offset", iocshArgInt };
static const iocshArg pevConfigureArg4 = { "dmaSpace", iocshArgString };
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

static const iocshArg pevExpertReportArg0 = { "level", iocshArgInt };

static const iocshArg * const pevExpertReportArgs[] = {
    &pevExpertReportArg0
};

static const iocshFuncDef pevExpertReportDef =
    { "pevExpertReport", 1, pevExpertReportArgs };
    
static void pevExpertReportFunc (const iocshArgBuf *args)
{
    int status = pevExpertReport(
        args[0].ival);
    if (status != 0 && !interruptAccept) epicsExit(1);
}

static const iocshArg pevVmeSlaveMainConfigArg0 = { "addrSpace", iocshArgString };
static const iocshArg pevVmeSlaveMainConfigArg1 = { "mainBase", iocshArgInt };
static const iocshArg pevVmeSlaveMainConfigArg2 = { "mainSize", iocshArgInt };

static const iocshArg * const pevVmeSlaveMainConfigArgs[] = {
    &pevVmeSlaveMainConfigArg0,
    &pevVmeSlaveMainConfigArg1,
    &pevVmeSlaveMainConfigArg2,
};

static const iocshFuncDef pevVmeSlaveMainConfigDef =
    { "pevVmeSlaveMainConfig", 3, pevVmeSlaveMainConfigArgs };
    
static void pevVmeSlaveMainConfigFunc (const iocshArgBuf *args)
{
    int status = pevVmeSlaveMainConfig(
        args[0].sval, args[1].ival, args[2].ival);
    if (status != 0 && !interruptAccept) epicsExit(1);
}
		
static const iocshArg pevVmeSlaveTargetConfigArg0 = { "slaveAddrSpace", iocshArgString };
static const iocshArg pevVmeSlaveTargetConfigArg1 = { "winBase", iocshArgInt };
static const iocshArg pevVmeSlaveTargetConfigArg2 = { "winSize", iocshArgInt };
static const iocshArg pevVmeSlaveTargetConfigArg3 = { "vmeProtocol", iocshArgString };
static const iocshArg pevVmeSlaveTargetConfigArg4 = { "target", iocshArgString };
static const iocshArg pevVmeSlaveTargetConfigArg5 = { "targetOffset", iocshArgInt };
static const iocshArg pevVmeSlaveTargetConfigArg6 = { "swapping", iocshArgString };
static const iocshArg * const pevVmeSlaveTargetConfigArgs[] = {
    &pevVmeSlaveTargetConfigArg0,
    &pevVmeSlaveTargetConfigArg1,
    &pevVmeSlaveTargetConfigArg2,
    &pevVmeSlaveTargetConfigArg3,
    &pevVmeSlaveTargetConfigArg4,
    &pevVmeSlaveTargetConfigArg5,
    &pevVmeSlaveTargetConfigArg6,
};
		
static const iocshFuncDef pevVmeSlaveTargetConfigDef =
    { "pevVmeSlaveTargetConfig", 7, pevVmeSlaveTargetConfigArgs };
    
static void pevVmeSlaveTargetConfigFunc (const iocshArgBuf *args)
{
    int status = pevVmeSlaveTargetConfig(
        args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].sval, args[5].ival, args[6].sval);
    if (status != 0 && !interruptAccept) epicsExit(1);
}

static void pevDrvRegistrar ()
{
    iocshRegister(&pevConfigureDef, pevConfigureFunc);
    iocshRegister(&pevAsynConfigureDef, pevAsynConfigureFunc);
    iocshRegister(&pevExpertReportDef, pevExpertReportFunc);
    iocshRegister(&pevVmeSlaveMainConfigDef, pevVmeSlaveMainConfigFunc);
    iocshRegister(&pevVmeSlaveTargetConfigDef, pevVmeSlaveTargetConfigFunc);
}

epicsExportRegistrar(pevDrvRegistrar);

#endif
