#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <devLib.h>
#include <regDev.h>

#include <epicsTypes.h>
#include <epicsExit.h>
#include <epicsMutex.h>
#include <epicsExport.h>
#include <epicsRingPointer.h>
#include <ellLib.h>           
#include <errlog.h>
#include <initHooks.h>
#include <epicsThread.h>
#include <epicsMessageQueue.h>
#include <dbAccess.h>


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pevioctl.h>
#include <pevulib.h>




#define MAGIC 		1100 		/*  pev1100 */
#define SHMEM_MAP_SIZE 	0     		/* All SHMEM through DMA */
#define VME32_MAP_SIZE 	0x40000000      /* 1024 MB A32 - fixed via PMEM */
#define VME24_MAP_SIZE 	0x1000000      	/* 8 MB A24 - fixed */
#define VMECR_MAP_SIZE 	0x1000000	/* 8 MB CR/CSR - fixed */
#define VME16_MAP_SIZE 	0x10000		/* 64 KB A16 - fixed */
#define USR1_MAP_SIZE 	0x100000	/* 1 MB - fixed  */
#define USR2_MAP_SIZE 	0x100000	/* 1 MB - fixed  */
#define PCIE_MAP_SIZE  	0x1000		/* ????????? */
#define DMA_BUF_SIZE   	0x100
#define NO_DMA_SPACE	0xFF		/* DMA space not specified */
#define DMA_Q_SIZE	100000
#define MAX_PEVS	21		/* taken from VME 21 slot. makes sense ??? */
#define ISRC_PCI	0x00		/* ITC_SRC bits 13:12 of ITC_IACK*/
#define ISRC_VME	0x10		/* ITC_SRC */
#define ISRC_SHM	0x20		/* ITC_SRC */
#define ISRC_USR	0x40		/* ITC_SRC actually  USR1 */
#define ISRC_NONE	-1		/* ITC_SRC actually  USR1 */
#define FLAG_BLKMD	1		/* BlockMode flag */
#define FLAG_OFF	0		/* flag not defined (default) */
#define DMA_WAIT_RES    DMA_WAIT_1S	/* DMA timeout resolution */


/*
static char cvsid_pev1100[] __attribute__((unused)) =
    "$Id: pevDrv.c,v 1.38 2013/03/21 12:27:40 zimoch Exp $";
*/
static void pevHookFunc(initHookState state);
int pev_dmaQueue_init(int crate);
static IOSCANPVT pevGetIoScanPvt(regDevice *device, unsigned int offset);
static IOSCANPVT pevAsynGetIoScanPvt(regDeviceAsyn *device, unsigned int offset);
static int pevIntrRegister(int mapMode);
static void pevIntrHandler(int sig);
static int pevIntrHanlder_init();
epicsMessageQueueId pevDmaMsgQueueId = 0;
epicsMutexId pevDmaReqLock = 0;
epicsBoolean initHookregDone = epicsFalse;
static epicsBoolean pevIntrHandlerInitialized = epicsFalse;
static IOSCANPVT* pevIntrTable = 0; 
static struct pev_ioctl_evt *pevIntrEvent;
int pevExpertReport(int, int);
static int pevDrvDebug = 0;
static int dmaServerDebug = 0;
static unsigned int dmaLastTransferStat = 0;

typedef struct pevDmaReqMsg {
    struct pev_ioctl_dma_req pev_dmaReq; 
    CALLBACK* pCallBack;			/* record callback structure */
    unsigned int dlen; 
    unsigned int nelem;
    void *pdata;
    int swap;
    ulong pev_dmaBuf_usr;
    epicsBoolean readReq;
    int* reqStatus;
} pevDmaReqMsg;

struct regDevice {
    ELLNODE  node;                   /* Linked list node structure */
    unsigned long magic;
    const char* name;
    const char* resource;
    struct pev_node* 	pev;
    struct pev_ioctl_map_pg pev_rmArea_map; 	/* remote area (resource) */
    struct pev_ioctl_dma_req pev_dmaReq; 
    struct pev_ioctl_buf pev_dmaBuf;
    struct pev_ioctl_vme_conf vme_conf;		/* vme configuration */
    char* uPtrMapRes;			/* pointer to mapped resource in Linux user space */
    long  baseOffset;  		
    IOSCANPVT ioscanpvt;
    int flags;
    int dmaStatus;
    epicsUInt8 dmaSpace;		/*  must be used also to contain user provided DMA_SPACE_W/D/QS */
    epicsUInt8 vmeSwap;
    epicsUInt32 vmePktSize;
};

struct regDeviceAsyn {
    ELLNODE  node;                   /* Linked list node structure */
    unsigned long magic;
    const char* name;
    const char* resource;
    struct pev_node* 	pev;
    struct pev_ioctl_map_pg pev_rmArea_map; 	/* remote area (resource) */
    struct pev_ioctl_dma_req pev_dmaReq; 
    struct pev_ioctl_buf pev_dmaBuf;
    struct pev_ioctl_vme_conf vme_conf;		/* vme configuration */
    char* uPtrMapRes;			/* pointer to mapped resource in Linux user space */
    long  baseOffset;  		
    IOSCANPVT ioscanpvt;
    int flags;
    CALLBACK* callback;			/* callback struct pointer of record */
    int dmaStatus;
    epicsUInt8 dmaSpace;
    epicsUInt8 vmeSwap;
    epicsUInt32 vmePktSize;
};

static int pevDebug = 0;
static int currConfCrates = 0;

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
        printf("pev driver: for resource \"%s\" \n",
            device->resource);
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
    int prio)
{
  int swap = 0;
  unsigned short srcMode = 0;
  unsigned short desMode = 0;
  
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevRead: illegal device handle\n");
        return -1;
    }
    if (offset > device->pev_rmArea_map.win_size && !(device->dmaSpace & DMA_SPACE_SHM) 
    						&& (device->pev_rmArea_map.mode & MAP_SPACE_VME))
    {
        errlogSevPrintf(errlogMajor,
            "pevRead %s: offset %d out of range (0-%d)\n",
            device->name, offset, device->pev_rmArea_map.win_size);
        return -1;
    }
    if (offset+dlen*nelem > device->pev_rmArea_map.win_size && !(device->dmaSpace & DMA_SPACE_SHM)
    						&& (device->pev_rmArea_map.mode & MAP_SPACE_VME))
    {
        errlogSevPrintf(errlogMajor,
            "pevRead %s: offset %d + %d bytes length exceeds mapped size %d by %d bytes\n",
            device->name, offset, nelem, device->pev_rmArea_map.win_size,
            offset+dlen*nelem - device->pev_rmArea_map.win_size);
        return -1;
    }

    if( device->pev_rmArea_map.mode & MAP_SPACE_VME )
    {
      desMode = device->vmeSwap;
#ifdef powerpc
      srcMode = DMA_SWAP;
      swap = 0;
#else
      srcMode = 0;
      swap = 1;
#endif
    }
    if( device->pev_rmArea_map.mode & MAP_SPACE_USR1)
      swap = 1;
      
    if( (nelem>100 && device->flags!=FLAG_BLKMD ) || (device->flags==FLAG_BLKMD && prio==2) ) 		/* do DMA, prio_2=HIGH */
    {  
    if(device->dmaSpace == NO_DMA_SPACE) 
    	printf("pevRead(): \"%s\" DMA SPACE not specified! continue with normal transfer \n", device->name);
    else
    {
      if(device->flags==FLAG_BLKMD)
        {
          device->pev_dmaReq.size = device->pev_dmaBuf.size;
          device->pev_dmaReq.src_addr = device->baseOffset;  		
	  device->pev_dmaReq.des_addr = (ulong)device->pev_dmaBuf.b_addr; 
	}
      else
        {
          device->pev_dmaReq.size = nelem*dlen | device->vmePktSize;              				
          device->pev_dmaReq.src_addr = device->baseOffset + offset;  		
          device->pev_dmaReq.des_addr = (ulong)pdata;       	
	}
      device->pev_dmaReq.src_space = device->dmaSpace;		
      device->pev_dmaReq.des_space = DMA_SPACE_PCIE | desMode;
      device->pev_dmaReq.src_mode = srcMode;
      device->pev_dmaReq.des_mode = 0;
      if(device->pev_rmArea_map.mode & MAP_SPACE_SHM)
        device->pev_dmaReq.start_mode = DMA_MODE_BLOCK;
      else
        device->pev_dmaReq.start_mode = DMA_MODE_PIPE;
      /* device->pev_dmaReq.start_mode = DMA_MODE_BLOCK; */
      device->pev_dmaReq.end_mode = 0;
      device->pev_dmaReq.intr_mode = DMA_INTR_ENA;
      device->pev_dmaReq.wait_mode = DMA_WAIT_INTR | DMA_WAIT_RES | (1<<4);
      if( pev_dma_move(&device->pev_dmaReq) )
        {
	  printf("pevRead(): DMA transfer failed! dma status = 0x%x \n", device->pev_dmaReq.dma_status);
          return -1;
	}
      else 
        {
	 if( !(device->pev_dmaReq.dma_status & DMA_STATUS_ENDED) )
	   {
             printf("pevRead(): DMA transfer not completed! status = 0x%x\n", device->pev_dmaReq.dma_status);
	     return -1;
	   }
	 else
	  {
             if( device->flags==FLAG_BLKMD ) 
	       scanIoRequest(device->ioscanpvt);
             return 0;
	  }
        }
      }
    }
    
    if( device->flags==FLAG_BLKMD )
      regDevCopy(dlen, nelem, device->pev_dmaBuf.u_addr + offset, pdata, NULL, 0);
    else
      {
        if( !(device->pev_rmArea_map.mode & MAP_SPACE_VME) )
	  regDevCopy(dlen, nelem, device->uPtrMapRes + device->baseOffset + offset, pdata, NULL, swap); 
	else
	  regDevCopy(dlen, nelem, device->uPtrMapRes + offset, pdata, NULL, swap); 	  
      }
    return 0;

}

int pevWrite(
    regDevice *device,
    unsigned int offset,
    unsigned int dlen,
    unsigned int nelem,
    void* pdata,
    void* pmask,
    int priority)
{
  int swap = 0;
  unsigned short destMode = 0;
  unsigned short srcMode = 0;
  	
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevWrite: illegal device handle\n");
        return -1;
    }

    if (offset > device->pev_rmArea_map.win_size && !(device->dmaSpace & DMA_SPACE_SHM)
    						&& (device->pev_rmArea_map.mode & MAP_SPACE_VME))
    {
        errlogSevPrintf(errlogMajor,
            "pevWrite %s: offset %d out of range (0-%d)\n",
            device->name, offset, device->pev_rmArea_map.win_size);
        return -1;
    }
    if (offset+dlen*nelem > device->pev_rmArea_map.win_size && !(device->dmaSpace & DMA_SPACE_SHM)
    						&& (device->pev_rmArea_map.mode & MAP_SPACE_VME))
    {
        errlogSevPrintf(errlogMajor,
            "pevWrite %s: offset %d + %d bytes length exceeds mapped size %d by %d bytes\n",
            device->name, offset, nelem, device->pev_rmArea_map.win_size,
            offset+dlen*nelem - device->pev_rmArea_map.win_size);
        return -1;
    }
    if( device->pev_rmArea_map.mode & MAP_SPACE_VME)
    {
      srcMode = device->vmeSwap;
#ifdef powerpc
      destMode = DMA_SWAP;
      swap = 0;
#else
      destMode = 0;
      swap = 1;
#endif
    }
    if( device->pev_rmArea_map.mode & MAP_SPACE_USR1)
      swap = 1;
      
    
    if( (nelem>100 && device->flags!=FLAG_BLKMD ) || (device->flags==FLAG_BLKMD && priority==2) ) 	/* do DMA, prio_2=HIGH */
    {
    if(device->dmaSpace == NO_DMA_SPACE) 
    	printf("pevWrite(): \"%s\" DMA SPACE not specified! continue with normal transfer \n", device->name);
    else
     {	

      if(device->flags==FLAG_BLKMD)
        {
          device->pev_dmaReq.size = device->pev_dmaBuf.size;
          device->pev_dmaReq.src_addr = (ulong)device->pev_dmaBuf.b_addr;                   
          device->pev_dmaReq.des_addr = device->baseOffset;       	
	}
      else
        {
          device->pev_dmaReq.src_addr = (ulong)pdata;
          device->pev_dmaReq.size = nelem*dlen | device->vmePktSize;                  
          device->pev_dmaReq.des_addr = device->baseOffset + offset;      
	}
      device->pev_dmaReq.src_space = DMA_SPACE_PCIE | srcMode;
      device->pev_dmaReq.des_space = device->dmaSpace;
      device->pev_dmaReq.src_mode = 0;
      device->pev_dmaReq.des_mode = destMode;
      if(device->pev_rmArea_map.mode & MAP_SPACE_SHM)
        device->pev_dmaReq.start_mode = DMA_MODE_BLOCK;
      else
        device->pev_dmaReq.start_mode = DMA_MODE_PIPE;
      /* device->pev_dmaReq.start_mode = DMA_MODE_BLOCK; */
      device->pev_dmaReq.end_mode = 0;
      device->pev_dmaReq.intr_mode = DMA_INTR_ENA;
      device->pev_dmaReq.wait_mode = DMA_WAIT_INTR | DMA_WAIT_RES | (1<<4);
      if( pev_dma_move(&device->pev_dmaReq) )
        {
	  printf("pevWrite(): DMA transfer failed! dma status = 0x%x \n", device->pev_dmaReq.dma_status);
          return -1;
	}
      else 
        {
	 if( !(device->pev_dmaReq.dma_status & DMA_STATUS_ENDED) )
           {
	     printf("pevWrite(): DMA transfer not completed! dma status = 0x%x \n", device->pev_dmaReq.dma_status);
             return -1;
	   }
	 else
          return 0;
        }
     }
    }

    if( device->flags==FLAG_BLKMD )
      regDevCopy(dlen, nelem, pdata, device->pev_dmaBuf.u_addr + offset, NULL, 0); 
    else
      {  
        if( !(device->pev_rmArea_map.mode & MAP_SPACE_VME) )
          regDevCopy(dlen, nelem, pdata, device->uPtrMapRes + device->baseOffset + offset, NULL, swap);
	else
	  regDevCopy(dlen, nelem, pdata, device->uPtrMapRes + offset, NULL, swap);
      }
    return 0;
}
	

static regDevSupport pevSupport = {
    pevReport,
    pevGetIoScanPvt,
    pevGetIoScanPvt,
    pevRead,
    pevWrite
};


int pevAsynRead(
    regDeviceAsyn *device,
    unsigned int offset,
    unsigned int dlen,
    unsigned int nelem,
    void* pdata,
    CALLBACK* cbStruct,
    int prio,
    int* rdStat)
{
  int swap = 0;
  unsigned short srcMode = 0;
  unsigned short desMode = 0;
  pevDmaReqMsg pevDmaRequest;
  
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevAsynRead: illegal device handle\n");
        return -1;
    }
    if (offset > device->pev_rmArea_map.win_size && !(device->dmaSpace & DMA_SPACE_SHM)
    						&& !(device->pev_rmArea_map.mode & MAP_SPACE_VME))
    {
        errlogSevPrintf(errlogMajor,
            "pevAsynRead %s: offset %d out of range (0-%d)\n",
            device->name, offset, device->pev_rmArea_map.win_size);
        return -1;
    }
    if (offset+dlen*nelem > device->pev_rmArea_map.win_size && !(device->dmaSpace & DMA_SPACE_SHM)
    						&& !(device->pev_rmArea_map.mode & MAP_SPACE_VME))
    {
        errlogSevPrintf(errlogMajor,
            "pevAsynRead %s: offset %d + %d bytes length exceeds mapped size %d by %d bytes\n",
            device->name, offset, nelem, device->pev_rmArea_map.win_size,
            offset+dlen*nelem - device->pev_rmArea_map.win_size);
        return -1;
    }

    if( device->pev_rmArea_map.mode & MAP_SPACE_VME )
    {
      desMode = device->vmeSwap;
#ifndef powerpc
      srcMode = DMA_SWAP;
      swap = 1;
#else		/* X86_32 can probably used for stronegr check */
      srcMode = 0;
      swap = 0;
#endif
    }
    if( device->pev_rmArea_map.mode & MAP_SPACE_USR1)
      swap = 1;
      
    if( nelem > 100 ) 		/* do DMA */
    {  
    if(device->dmaSpace == NO_DMA_SPACE && !cbStruct) 
    	printf("pevAsynRead(): \"%s\" DMA SPACE not specified! continue with normal transfer \n", device->name);
    else
    {	
      epicsMutexLock(pevDmaReqLock);       
      device->pev_dmaReq.src_addr = device->baseOffset + offset;  		/* src bus address */ 
      device->pev_dmaReq.des_addr = (ulong)pdata;       			 /*  des bus address */
      device->pev_dmaReq.size = nelem*dlen | device->vmePktSize;              				
      device->pev_dmaReq.src_space = device->dmaSpace;		
      device->pev_dmaReq.des_space = DMA_SPACE_PCIE | desMode;
      device->pev_dmaReq.src_mode = srcMode;
      device->pev_dmaReq.des_mode = 0;
      if(device->pev_rmArea_map.mode & MAP_SPACE_SHM)
        device->pev_dmaReq.start_mode = DMA_MODE_BLOCK;
      else
        device->pev_dmaReq.start_mode = DMA_MODE_PIPE;
      /* device->pev_dmaReq.start_mode = DMA_MODE_BLOCK; */
      device->pev_dmaReq.end_mode = 0;
      device->pev_dmaReq.intr_mode = DMA_INTR_ENA;
      device->pev_dmaReq.wait_mode = DMA_WAIT_INTR | DMA_WAIT_RES | (1<<4);
      
      pevDmaRequest.pev_dmaReq = device->pev_dmaReq;
      pevDmaRequest.pCallBack = cbStruct;
      pevDmaRequest.dlen = dlen;
      pevDmaRequest.nelem = nelem;
      pevDmaRequest.pdata = pdata;
      pevDmaRequest.swap = swap;
      pevDmaRequest.readReq = epicsTrue;
      pevDmaRequest.reqStatus = rdStat;
      pevDmaRequest.pev_dmaBuf_usr = (ulong)device->pev_dmaBuf.u_addr;
      epicsMutexUnlock(pevDmaReqLock);       
      
      if( !epicsMessageQueueSend(pevDmaMsgQueueId, (void*)&pevDmaRequest, sizeof(pevDmaReqMsg)) )
         return (1);   /* to tell regDev that this is first phase of record processing (to let recSupport set PACT to true) */
      else 
        {
           printf("pevAsynRead(): sending DMA request failed! do normal & synchronous transfer\n");
	}
      }
    }
    if( device->dmaSpace & DMA_SPACE_SHM )	/* this is SH_MEM */
    {
      printf("pevAsynRead(): not allowed to do normal transfer for SH_MEM\n");
      return -1;
    }
      
    if( !(device->pev_rmArea_map.mode & MAP_SPACE_VME) )
      regDevCopy(dlen, nelem, device->uPtrMapRes + device->baseOffset + offset, pdata, NULL, swap);        
    else
      regDevCopy(dlen, nelem, device->uPtrMapRes + offset, pdata, NULL, swap); 
    
    return 0;
}

int pevAsynWrite(
    regDeviceAsyn *device,
    unsigned int offset,
    unsigned int dlen,
    unsigned int nelem,
    void* pdata,
    CALLBACK* cbStruct,
    void* pmask,
    int priority,
    int* wrStat)
{
  int swap = 0;
  unsigned short destMode = 0;
  unsigned short srcMode = 0;
  pevDmaReqMsg pevDmaRequest;
   	
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevAsynWrite: illegal device handle\n");
        return -1;
    }

    if (offset > device->pev_rmArea_map.win_size && !(device->dmaSpace & DMA_SPACE_SHM)
    						&& (device->pev_rmArea_map.mode & MAP_SPACE_VME))
    {
        errlogSevPrintf(errlogMajor,
            "pevAsynWrite %s: offset %d out of range (0-%d)\n",
            device->name, offset, device->pev_rmArea_map.win_size);
        return -1;
    }
    if (offset+dlen*nelem > device->pev_rmArea_map.win_size && !(device->dmaSpace & DMA_SPACE_SHM)
    						&& (device->pev_rmArea_map.mode & MAP_SPACE_VME))
    {
        errlogSevPrintf(errlogMajor,
            "pevAsynWrite %s: offset %d + %d bytes length exceeds mapped size %d by %d bytes\n",
            device->name, offset, nelem, device->pev_rmArea_map.win_size,
            offset+dlen*nelem - device->pev_rmArea_map.win_size);
        return -1;
    }
    if( device->pev_rmArea_map.mode & MAP_SPACE_VME)
    {
      srcMode = device->vmeSwap;
#ifndef powerpc
      destMode = DMA_SWAP;
      swap = 1;
#else
      destMode = 0;
      swap = 0;
#endif
    }
    if( device->pev_rmArea_map.mode & MAP_SPACE_USR1)
      swap = 1;
    
    
    if(nelem > 100)
    {
    if(device->dmaSpace == NO_DMA_SPACE) 
    	printf("pevAsynWrite(): \"%s\" DMA SPACE not specified! continue with normal transfer \n", device->name);
    else
     {	
      /****
      * 	workaround: first copy the src data to kernel allocated buffer  
      regDevCopy(dlen, nelem, pdata, device->pev_dmaBuf.u_addr, NULL, swap); 
      ***/

       /* device->pev_dmaReq.src_addr = (ulong)device->pev_dmaBuf.b_addr;   */                   
      epicsMutexLock(pevDmaReqLock);       
      device->pev_dmaReq.src_addr = (ulong)pdata;                   
      device->pev_dmaReq.des_addr = device->baseOffset + offset;       /* (ulong)pdata destination is DMA buffer    */
      device->pev_dmaReq.size = nelem*dlen | device->vmePktSize;                  
      device->pev_dmaReq.src_space = DMA_SPACE_PCIE | srcMode;
      device->pev_dmaReq.des_space = device->dmaSpace;
      device->pev_dmaReq.src_mode = DMA_PCIE_RR2;
      device->pev_dmaReq.des_mode = destMode | DMA_PCIE_RR2;
      if(device->pev_rmArea_map.mode & MAP_SPACE_SHM)
        device->pev_dmaReq.start_mode = DMA_MODE_BLOCK;
      else
        device->pev_dmaReq.start_mode = DMA_MODE_PIPE;
      /* device->pev_dmaReq.start_mode = DMA_MODE_BLOCK; */
      device->pev_dmaReq.end_mode = 0;
      device->pev_dmaReq.intr_mode = DMA_INTR_ENA;
      device->pev_dmaReq.wait_mode = DMA_WAIT_INTR | DMA_WAIT_RES | (1<<4);
      
      pevDmaRequest.pev_dmaReq = device->pev_dmaReq;
      pevDmaRequest.pCallBack = cbStruct;
      pevDmaRequest.dlen = dlen;
      pevDmaRequest.nelem = nelem;
      pevDmaRequest.pdata = pdata;
      pevDmaRequest.swap = swap;
      pevDmaRequest.reqStatus = wrStat;
      pevDmaRequest.readReq = epicsFalse;
      pevDmaRequest.pev_dmaBuf_usr = (ulong)device->pev_dmaBuf.u_addr;
      epicsMutexUnlock(pevDmaReqLock);       
      
      if( !epicsMessageQueueSend(pevDmaMsgQueueId, (void*)&pevDmaRequest, sizeof(pevDmaReqMsg)) )
         return (1);   /* to tell regDev that this is first phase of record processing (to let recSupport set PACT to true) */
      else 
        {
           printf("pevAsynWrite(): sending DMA request failed! do normal & synchronous transfer\n");
	}
     }
    }
    if( device->dmaSpace & DMA_SPACE_SHM )	/* this is SH_MEM */
    {
      printf("pevAsynWrite(): not allowed to do normal transfer for SH_MEM\n");
      return -1;
    }
      
    if( !(device->pev_rmArea_map.mode & MAP_SPACE_VME) )
      regDevCopy(dlen, nelem, pdata, device->uPtrMapRes + device->baseOffset + offset, NULL, swap); 
    else
      regDevCopy(dlen, nelem, pdata, device->uPtrMapRes + offset, NULL, swap);   
      
    return 0;
}

void pevAsynReport(
    regDeviceAsyn *device,
    int level)
{
    if (device && device->magic == MAGIC)
    {
        printf("pevAsyn driver: for resource \"%s\" \n",
            device->resource);
    }
}

static IOSCANPVT pevAsynGetIoScanPvt(regDeviceAsyn *device, unsigned int offset)
{
    if (!device || device->magic != MAGIC)
    { 
        errlogSevPrintf(errlogMajor,
            "pevAsynGetIoScanPvt: illegal device handle\n");
        return NULL;
    }
    return device->ioscanpvt;
}

int pevAlloc(
       	void** usrBufPtr, 
       	void** busBufPtr, 
    	unsigned int size)
{
  struct pev_ioctl_buf pev_dma_Buf;
  int status = 0;
  
  pev_dma_Buf.size = size;
  if( pev_buf_alloc( &pev_dma_Buf)==MAP_FAILED )
    {
      printf("pevAlloc: ERROR, could not allocate dma buffer; bus %p user %p\n", pev_dma_Buf.b_addr, pev_dma_Buf.u_addr);
      status = -1;
    }
  else
    {
      *(unsigned long*)usrBufPtr = (unsigned long)pev_dma_Buf.u_addr;
      *(unsigned long*)busBufPtr = (unsigned long)pev_dma_Buf.b_addr;	
   }
  return status;
} 


static regDevAsyncSupport pevAsynSupport = {
    pevAsynReport,
    pevAsynGetIoScanPvt,
    pevAsynGetIoScanPvt,
    pevAsynRead,
    pevAsynWrite,	/* asyn write must be implemented */
    pevAlloc
};


static ELLLIST pevRegDevList;                        /* Linked list of sync register devices */
static ELLLIST pevRegDevAsynList;                    /* Linked list of Async register devices */

static struct pev_ioctl_map_pg null_pev_rmArea_map = {0,0,0,0,0,0,0,0,0,0,0,0};
/*****
*	findMappedPevResource():
*
*	Desc.: finds already mapped resource be it synchronous or asynchronous device.
*       if clearAll == true then clear all mapped areas
*
*****/
static struct pev_ioctl_map_pg findMappedPevResource(struct pev_node *pev, const char* resource, 
							epicsBoolean* mapStat, long* baseOffset, char** uPtr, epicsBoolean clearAll) {
  regDevice* device;    
  regDeviceAsyn* asdevice;    
  struct pev_ioctl_map_ctl pev_ctl; 

  for (device = (regDevice *)ellFirst(&pevRegDevList);
  	device != NULL;
  	device = (regDevice *)ellNext(&device->node)) {
	
       if(clearAll)
       { 
	  pev_ctl.sg_id = device->pev_rmArea_map.sg_id;
	  pev_map_clear( &pev_ctl );
          pev_munmap( &device->pev_rmArea_map );
          pev_map_free( &device->pev_rmArea_map );
          if(device->pev_dmaBuf.k_addr) pev_buf_free( &device->pev_dmaBuf );
       }
       else
       if ( pev == device->pev && !strcmp(device->resource, resource) ) {
  	   /* pev_rmArea_map = device->pev_rmArea_map*/;
	   *mapStat = epicsTrue;
	   *baseOffset = device->baseOffset;
	   *uPtr = device->uPtrMapRes;
           if(pevDrvDebug)
	     printf ("findMappedPevResource(): resource \"%s\" already mapped.. continue\n", device->resource);
  	   return(device->pev_rmArea_map);
       }

  }
  
  for (asdevice = (regDeviceAsyn *)ellFirst(&pevRegDevAsynList);
  	asdevice != NULL;
  	asdevice = (regDeviceAsyn *)ellNext(&asdevice->node)) {

       if(clearAll)
       { 
	  pev_ctl.sg_id = asdevice->pev_rmArea_map.sg_id;
	  pev_map_clear( &pev_ctl );
          pev_munmap( &asdevice->pev_rmArea_map );
          pev_map_free( &asdevice->pev_rmArea_map );
          if(asdevice->pev_dmaBuf.k_addr) pev_buf_free( &asdevice->pev_dmaBuf );
       }
       else
       if ( pev == asdevice->pev && !strcmp(asdevice->resource, resource) ) {
  	   /* pev_rmArea_map = device->pev_rmArea_map*/;
	   *mapStat = epicsTrue; 
	   *baseOffset = asdevice->baseOffset;
	   *uPtr = asdevice->uPtrMapRes;
           if(pevDrvDebug)
	     printf ("findMappedPevResource(): resource \"%s\" already mapped.. continue\n", asdevice->resource);
  	   return(asdevice->pev_rmArea_map);
       }

  }
  
  return null_pev_rmArea_map;
}

extern void pevDevLibAtexit(void);

static void pevDrvAtexit(void)
{
  int i=0;
  struct pev_node* pev; 
  regDevice* device;
  regDeviceAsyn* deviceAsyn;

/*  pevDevLibAtexit(); */
  printf("pevDrvAtexit...\n");
  pev_evt_queue_disable(pevIntrEvent);
  pev_evt_queue_free(pevIntrEvent);
  free(pevIntrTable);
  
  findMappedPevResource(0, 0, 0, 0, 0, epicsTrue);
}

/**
*	pevConfigure(crate, name, resource, offset, vmeProtocol, intrVec, mapSize, blockMode)  
*
* 	crate: 		normally only 1; inceremnts if there are more crates in PCIe tree
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

int pevConfigure(
    unsigned int crate,
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
  struct pev_node *pev;
  struct pev_ioctl_map_pg pev_rmArea_map = null_pev_rmArea_map;
  epicsBoolean mapExists = epicsFalse;
  long otherBaseOffset;
  char* usrMappedResource = 0;
  epicsAddressType addrType;
   
  if( !name || !resource || !swap)
  {
    printf("usage: pevConfigure (crate, \"name\", "
        "\"SH_MEM\"|\"PCIE\"|\"VME_A16\"|\"VME_A24\"|\"VME_A32\", "
        "offset, \"BLT\"|\"MBLT\"|\"2eVME\"|\"2eSST160\"|\"2eSST233\"|\"2eSST320\", "
        "intrVec, dmaSize, blockMode, \"WS\"|\"DS\"|\"QS\", vmePktSize)\n");
    return -1;
  }

  if(  regDevAsynFind(name) ) 
  {
    printf("pevConfigure: ERROR, device \"%s\" on an IFC/PEV already configured as asynchronous device\n", 
    		name);
    return -1;
  }
  /* see if this device already exists */
  device = regDevFind(name);
  if( device ) 
  {
    printf("pevConfigure: ERROR, device \"%s\" on IFC/PEV in %s already configured as synchronous device\n", 
    		device->name, device->resource);
    return -1;
  }

  /* call PEV1100 user library initialization function */
  pev = pev_init( crate);
  if( !pev)
  {
    printf("Cannot allocate data structures to control PEV1100\n");
    return -1;
  }
  /* verify if the PEV1100 is accessible */
  if( pev->fd < 0)
  {
    printf("Cannot find PEV1100 interface\n");
    return -1;
  }
  
  if( !ellFirst(&pevRegDevList) )
    ellInit (&pevRegDevList);

  if (strncmp(resource, "VME_", 4) != 0 )
    pev_rmArea_map = findMappedPevResource(pev, resource, &mapExists, &otherBaseOffset, &usrMappedResource, epicsFalse);  
/*
  if( mapExists == epicsTrue && otherBaseOffset == offset)
  {
    printf("pevConfigure: Another device for the same recource (%s) with the same offset (0x%x) exists!\n", resource, offset);
    return -1;
  }
*/
  
  device = (regDevice*)calloc(1,sizeof(regDevice));
  if (device == NULL)
  {
      errlogSevPrintf(errlogFatal,
  	  "pevConfigure %s: out of memory\n",
  	  name);
      return errno;
  }
  
  device->pev = pev;
  device->magic = MAGIC;
  device->name = strdup(name);
  device->resource = strdup(resource);
  device->dmaSpace = NO_DMA_SPACE;
  device->vmePktSize = 0;
  if( blockMode == 1 )
   {
    device->flags = FLAG_BLKMD;
   }
  else
    device->flags = FLAG_OFF;
  
  /* get the current VME configuration */
  pev_vme_conf_read( &device->vme_conf);
  if(pevDebug)
  {
    printf("VME A32 base address = 0x%08x [0x%x]", device->vme_conf.a32_base, device->vme_conf.a32_size);
    if( device->vme_conf.mas_ena)
    {
      printf(" -> enabled\n");
    }
    else
    {
      printf(" -> disabled\n");
    }
  }
  
  
  if( device->flags == FLAG_BLKMD )	/* use blockMode */
  {
    device->pev_dmaBuf.size = mapSize;	/* TODO: need to double-check size restrication with mapped resource size */
    if( pev_buf_alloc( &device->pev_dmaBuf) == MAP_FAILED )
      {
        printf("pevConfigure: ERROR, could not allocate dma buffer; bus %p user %p\n", device->pev_dmaBuf.b_addr, device->pev_dmaBuf.u_addr);
        return -1;
      }
   }
  
  if( mapExists )
    {
      device->pev_rmArea_map = pev_rmArea_map;
      device->uPtrMapRes = usrMappedResource;
      if(pevDebug)  
        printf ("pevConfigure: skip mapping...\n");
  
      if( strcmp(resource, "USR1")==0 )
       device->dmaSpace = DMA_SPACE_USR1;
      else
      if( strcmp(resource, "SH_MEM")==0 )
       device->dmaSpace = DMA_SPACE_SHM;
      goto SKIP_PEV_RESMAP;
    }
      
  

 /* "SH_MEM", "PCIE", "VME_A16/24/32/BLT/MBLT/2eSST" */
   if( strcmp(resource, "SH_MEM")==0 ) 
     {
	device->dmaSpace = DMA_SPACE_SHM;
	device->flags = FLAG_BLKMD;	/* only DMA access */
        goto TESTJUMP;
     }
   else
   if( strcmp(resource, "PCIE")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_PCIE;
  	/* device->pev_rmArea_map.sg_id = MAP_MASTER_32 */
  	device->pev_rmArea_map.rem_addr = 0x000000;		/* PCIe tree base address */
  	device->pev_rmArea_map.size = PCIE_MAP_SIZE;
	if( offset > PCIE_MAP_SIZE) 
	  {
	    printf("pevConfigure: ERROR, too big offset\n");
	    return -1;
	  }
	device->dmaSpace = DMA_SPACE_PCIE;
     }
   else			
   if( strcmp(resource, "USR1")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_USR1;
  	device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 		
  	device->pev_rmArea_map.size = USR1_MAP_SIZE;
	if( offset > VME16_MAP_SIZE) 
	  {
	    printf("pevConfigure: ERROR, too big offset\n");
	    return -1;
	  }
	device->dmaSpace = DMA_SPACE_USR1;
     }
   else			
   if( strcmp(resource, "USR2")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_USR2;
  	device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 		
  	device->pev_rmArea_map.size = USR2_MAP_SIZE;
	if( offset > VME16_MAP_SIZE) 
	  {
	    printf("pevConfigure: ERROR, too big offset\n");
	    return -1;
	  }
	  device->dmaSpace = DMA_SPACE_USR2;
     }
   else
   if( strncmp(resource, "VME_", 4)==0 ) 
     {  
        device->pev_rmArea_map.mode = MAP_SPACE_VME;
        if( strcmp(resource, "VME_A16")==0 ) 
	  addrType = atVMEA16;
        if( strcmp(resource, "VME_A24")==0 ) 
	  addrType = atVMEA24;
        if( strcmp(resource, "VME_A32")==0 ) 
	  addrType = atVMEA32;
        if( strcmp(resource, "VME_CSR")==0 ) 
	  addrType = atVMECSR;
	
        /* no not use devRegisterAddress here in order to allow multiple maps on the same address */
	if( devBusToLocalAddr (
                   addrType,                      
                   offset,                   
                   (volatile void **)(void *)&device->uPtrMapRes) != 0 ) {
	    printf("pevConfigure: ERROR, devBusToLocalAddr() failed\n");
	    return -1;
        }
/*
	if( devRegisterAddress (
                   name,                      
                   addrType,                      
                   offset,                   
                   mapSize,            
                   (volatile void **)(void *)&device->uPtrMapRes) != 0 ) {
	    printf("pevConfigure: ERROR, devRegisterAddress() failed\n");
	    return -1;
        }
*/
	goto SKIP_PEV_RESMAP;
     }
    else 
      { 
    	printf("Unknown PCIe remote area - valid options: SH_MEM, PCIE, VME_A16/24/32/\n");
    	return -1;
      }
   
  device->pev_rmArea_map.mode |= MAP_ENABLE|MAP_ENABLE_WR;
  device->pev_rmArea_map.flag = 0x0;
  
  pev_map_alloc( &device->pev_rmArea_map);
  printf("offset in PCI MEM window to access %s @ 0x%08X size: 0x%08x\n", resource, (uint)device->pev_rmArea_map.loc_addr, device->pev_rmArea_map.win_size);
  printf("perform the mapping in user's space : ");

  device->uPtrMapRes  = (char*)pev_mmap( &device->pev_rmArea_map);
  printf("%p", device->uPtrMapRes);
  if( device->uPtrMapRes == MAP_FAILED)
  {
    printf(" ->Failed\n");
    return -1;
  }
  printf(" -> Done\n");
  /* device->uPtrMapRes += offset; */ /** uPtrMapRes for each space is fixed, baseoffset must be added for each device **/

SKIP_PEV_RESMAP:

  /* goto TESTJUMP;
   set VME block transfer protocol*/
  if(!vmeProtocol)  
  	goto TESTJUMP;
	
  if(strcmp(vmeProtocol, "BLT")==0) 
  {
    device->dmaSpace = DMA_SPACE_VME|DMA_VME_BLT;
  }
  else if(strcmp(vmeProtocol, "MBLT")==0) 
  {
    device->dmaSpace = DMA_SPACE_VME|DMA_VME_MBLT;
  }
  else if(strcmp(vmeProtocol, "2eVME")==0) 
  {
    device->dmaSpace = DMA_SPACE_VME|DMA_VME_2eVME;
  }
  else if(strcmp(vmeProtocol, "2eSST160")==0) 
  {
    device->dmaSpace = DMA_SPACE_VME|DMA_VME_2e160;
  }
  else if(strcmp(vmeProtocol, "2eSST233")==0) 
  {
    device->dmaSpace = DMA_SPACE_VME|DMA_VME_2e233;
  }
  else if(strcmp(vmeProtocol, "2eSST320")==0) 
  {
    device->dmaSpace = DMA_SPACE_VME|DMA_VME_2e320;
  }

TESTJUMP: 
  
  if( strncmp(resource, "VME_", 4)!=0 )
  {
  if( strcmp(swap, "WS")==0 )
    device->dmaSpace |= DMA_SPACE_WS;
  else if( strcmp(swap, "DS")==0 )
    device->dmaSpace |= DMA_SPACE_DS;
  else if( strcmp(swap, "QS")==0 )
    device->dmaSpace |= DMA_SPACE_QS;
  }
  else
  {
    if( strcmp(swap, "WS")==0 )
      device->vmeSwap = DMA_SPACE_WS;
    else if( strcmp(swap, "DS")==0 )
      device->vmeSwap = DMA_SPACE_DS;
    else if( strcmp(swap, "QS")==0 )
      device->vmeSwap = DMA_SPACE_QS;
    else
      device->vmeSwap = 0;
      
    switch(vmePktSize) 
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
        printf("pevConfigure: vmePktSize parameter not specified/wrong - set to DMA_SIZE_PKT_128\n");
        break;
    }
  }

  device->baseOffset = offset;
  if( initHookregDone == epicsFalse )
  {
    initHookRegister((initHookFunction)pevHookFunc);
    initHookregDone = epicsTrue;
    epicsAtExit((void*)pevDrvAtexit, NULL);
  }

  if( intrVec )
  {
    if( intrVec<0 || intrVec>255 || 
         ((device->pev_rmArea_map.mode & MAP_SPACE_USR1) && (intrVec>16)) ||
         ((device->pev_rmArea_map.mode & MAP_SPACE_SHM) && (intrVec>1))  || 
	 ((device->pev_rmArea_map.mode & MAP_SPACE_PCIE) && (intrVec>1)) )
    {
      printf("pevConfigure: ERROR, intrVec out of range (1<=VME<=255, 1<=USR1<=16, SH_MEM=1, PCIE=1)\n");
      return errno;
    }	
    if( !pevIntrHandlerInitialized ) 
    {
      pevIntrHanlder_init();
    }
    pev_evt_queue_disable(pevIntrEvent);
    if(strncmp(resource, "VME_", 4)==0)
      device->ioscanpvt = pevIntrTable[intrVec];
      
    if(device->pev_rmArea_map.mode & MAP_SPACE_USR1)
      device->ioscanpvt = pevIntrTable[255+intrVec];
    
    if(device->pev_rmArea_map.mode & MAP_SPACE_SHM)
      device->ioscanpvt = pevIntrTable[272];
    
    if(device->pev_rmArea_map.mode & MAP_SPACE_PCIE)
      device->ioscanpvt = pevIntrTable[273];
    
    pevIntrRegister(device->pev_rmArea_map.mode); 
    pevIntrEvent->wait = -1;
    signal(pevIntrEvent->sig, pevIntrHandler);
    pev_evt_queue_enable(pevIntrEvent);
  }
  else 
  if( device->flags == FLAG_BLKMD )
  {
    scanIoInit(&device->ioscanpvt);
  }    
 	
  regDevRegisterDevice(name, &pevSupport, device);
  ellAdd (&pevRegDevList, &device->node);
  if(crate > currConfCrates)
    currConfCrates = crate;
  return 0;
}

/**
*	pevAsynConfigure(crate, name, resource, offset)  
*
* 	crate: 		normally only 1; inceremnts if there are more crates in PCIe tree
*
*	name: 		virtual device name to refer to in EPICS record links (INP/OUT)
*
*	resource: 	PCIe resource; shared mem, VME space, FPGA user area -
*			- options: "SH_MEM", "PCIE", "VME_A16/24/32/USR1/2" -  
*
*	offset		offset in bytes within the resource space
*
*	vmeProtocol	VME transfer protocol for DMA. -
*                       - valid ptions: "BLT","MBLT","2eVME","2eSST160","2eSST233","2eSST320"
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
    unsigned int crate,
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
    
  regDeviceAsyn* device;    
  struct pev_node *pev;
  struct pev_ioctl_map_pg pev_rmArea_map = null_pev_rmArea_map;
  epicsBoolean mapExists = epicsFalse;
  long otherBaseOffset;
  char* usrMappedResource = 0;
  epicsAddressType addrType;
   
  if( !name || !resource || !swap)
  {
    printf("usage: pevAsynConfigure (crate, \"name\", "
        "\"SH_MEM\"|\"PCIE\"|\"VME_A16\"|\"VME_A24\"|\"VME_A32\", "
        "offset, \"BLT\"|\"MBLT\"|\"2eVME\"|\"2eSST160\"|\"2eSST233\"|\"2eSST320\", "
        "intrVec, dmaSize, blockMode, \"WS\"|\"DS\"|\"QS\", vmePktSize)\n");
    return -1;
  }

  if( regDevFind(name) ) 
  {
    printf("pevAsynConfigure: ERROR, device \"%s\" on an IFC/PEV already configured as synchronous device\n", 
    		name);
    return -1;
  }
  /* see if this device already exists */
  device = regDevAsynFind(name);
  if( device ) 
  {
    printf("pevAsynConfigure: ERROR, device \"%s\" on IFC/PEV %s already configured\n", 
    		device->name, device->resource);
    return -1;
  }

  /* call PEV1100 user library initialization function */
  pev = pev_init( crate);
  if( !pev)
  {
    printf("pevAsynConfigure: Cannot allocate data structures to control PEV1100\n");
    return -1;
  }
  /* verify if the PEV1100 is accessible */
  if( pev->fd < 0)
  {
    printf("pevAsynConfigure: Cannot find PEV1100 interface\n");
    return -1;
  }
  
  if( !ellFirst(&pevRegDevAsynList) )
    ellInit (&pevRegDevAsynList);

  if (strncmp(resource, "VME_", 4) != 0 )
    pev_rmArea_map = findMappedPevResource(pev, resource, &mapExists, &otherBaseOffset, &usrMappedResource, epicsFalse);
/*
  if( mapExists == epicsTrue && otherBaseOffset == offset)
  {
    printf("pevAsynConfigure: Another device for the same recource (%s) with the same offset (0x%x) exists!\n", resource, offset);
    return -1;
  }
*/
  
  device = (regDeviceAsyn*)calloc(1,sizeof(regDeviceAsyn));
  if (device == NULL)
  {
      errlogSevPrintf(errlogFatal,
  	  "pevAsynConfigure() %s: out of memory\n",
  	  name);
      return errno;
  };
  
  device->pev = pev;
  device->magic = MAGIC;
  device->name = strdup(name);
  device->resource = strdup(resource);
  device->dmaSpace = NO_DMA_SPACE;
  device->vmePktSize = 0;
  
  /* get the current VME configuration */
  pev_vme_conf_read( &device->vme_conf);
  if(pevDebug)
  {
    printf("VME A32 base address = 0x%08x [0x%x]", device->vme_conf.a32_base, device->vme_conf.a32_size);
    if( device->vme_conf.mas_ena)
    {
      printf(" -> enabled\n");
    }
    else
    {
      printf(" -> disabled\n");
    }
  }

  if( mapExists )
    {
      device->pev_rmArea_map = pev_rmArea_map;
      device->uPtrMapRes = usrMappedResource;
      if(pevDebug) 
        printf ("pevAsynConfigure: skip mapping...\n");
  
      if( strcmp(resource, "USR1")==0 )
        device->dmaSpace = DMA_SPACE_USR1;
      else
      if( strcmp(resource, "SH_MEM")==0 )
        device->dmaSpace = DMA_SPACE_SHM;
      goto SKIP_PEV_RESMAP;
    }
     
  
  device->pev_dmaBuf.size = DMA_BUF_SIZE;
  if( pev_buf_alloc( &device->pev_dmaBuf)==MAP_FAILED /*|| !device->pev_dmaBuf.b_addr*/ )
    {
      printf("pevAsynConfigure: ERROR, could not allocate dma buffer; bus %p user %p\n", device->pev_dmaBuf.b_addr, device->pev_dmaBuf.u_addr);
      return -1;
    }  
  

 /* "SH_MEM", "PCIE", "VME_A16/24/32/BLT/MBLT/2eSST" */
   if( strcmp(resource, "SH_MEM")==0 ) 
     {
	device->dmaSpace = DMA_SPACE_SHM;
	goto TESTJUMP;
     }
   else
   if( strcmp(resource, "PCIE")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_PCIE;
  	/* device->pev_rmArea_map.sg_id = MAP_MASTER_32 */
  	device->pev_rmArea_map.rem_addr = 0x000000;		/* PCIe tree base address */
  	device->pev_rmArea_map.size = PCIE_MAP_SIZE;
	if( offset > PCIE_MAP_SIZE) 
	  {
	    printf("pevAsynConfigure: ERROR, too big offset\n");
	    return -1;
	  }
	device->dmaSpace = DMA_SPACE_PCIE;
     }
   else			
   if( strcmp(resource, "USR1")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_USR1;
  	device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 		
  	device->pev_rmArea_map.size = USR1_MAP_SIZE;
	if( offset > VME16_MAP_SIZE) 
	  {
	    printf("pevAsynConfigure: ERROR, too big offset\n");
	    return -1;
	  }/**/
	device->dmaSpace = DMA_SPACE_USR1;
     }
   else			
   if( strcmp(resource, "USR2")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_USR2;
  	 device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 		
  	device->pev_rmArea_map.size = USR2_MAP_SIZE;
	if( offset > VME16_MAP_SIZE) 
	  {
	    printf("pevAsynConfigure: ERROR, too big offset\n");
	    return -1;
	  }/**/
	device->dmaSpace = DMA_SPACE_USR2;
     }
   else
   if( strncmp(resource, "VME_", 4)==0 ) 
     {
        device->pev_rmArea_map.mode = MAP_SPACE_VME;
        if( strcmp(resource, "VME_A16")==0 ) 
	  addrType = atVMEA16;
        if( strcmp(resource, "VME_A24")==0 ) 
	  addrType = atVMEA24;
        if( strcmp(resource, "VME_A32")==0 ) 
	  addrType = atVMEA32;
        if( strcmp(resource, "VME_CSR")==0 ) 
	  addrType = atVMECSR;
	  
        /* no not use devRegisterAddress here in order to allow multiple maps on the same address */
	if( devBusToLocalAddr (
                   addrType,                      
                   offset,                   
                   (volatile void **)(void *)&device->uPtrMapRes) != 0 ) {
	    printf("pevAsynConfigure: ERROR, devBusToLocalAddr() failed\n");
	    return -1;
        }
/*
	if( devRegisterAddress (
                   name,                      
                   addrType,                      
                   offset,                   
                   mapSize,            
                   (volatile void **)(void *)&device->uPtrMapRes) != 0 ) {
	    printf("pevAsynConfigure: ERROR, devRegisterAddress() failed\n");
	    return -1;
        }
*/
	goto SKIP_PEV_RESMAP;
     }
    else 
      { 
    	printf("pevAsynConfigure: Unknown PCIe remote area - valid options: SH_MEM, PCIE, VME_A16/24/32/\n");
    	return -1;
      }
   
  device->pev_rmArea_map.mode |= MAP_ENABLE|MAP_ENABLE_WR;
  device->pev_rmArea_map.flag = 0x0;
  pev_map_alloc( &device->pev_rmArea_map);
  printf("pevAsynConfigure: offset in PCI MEM window to access %s @ 0x%08X size: 0x%08x\n", resource, (uint)device->pev_rmArea_map.loc_addr, device->pev_rmArea_map.win_size);
  printf("pevAsynConfigure: perform the mapping in user's space : ");

  device->uPtrMapRes  = (char*)pev_mmap( &device->pev_rmArea_map);
  printf("%p", device->uPtrMapRes);
  if( device->uPtrMapRes == MAP_FAILED)
  {
    printf(" ->Failed\n");
    return -1;
  }
  printf(" -> Done\n");
  
  /* device->uPtrMapRes += offset; */ /** uPtrMapRes for each space is fixed, baseoffset must be added for each device **/

SKIP_PEV_RESMAP:
    
  /* set up VME block transfer protocol*/
  if(!vmeProtocol)  
  	goto TESTJUMP;
	
  if(strcmp(vmeProtocol, "BLT")==0) 
  {
    device->dmaSpace = DMA_SPACE_VME|DMA_VME_BLT;
  }
  else if(strcmp(vmeProtocol, "MBLT")==0) 
  {
    device->dmaSpace = DMA_SPACE_VME|DMA_VME_MBLT;
  }
  else if(strcmp(vmeProtocol, "2eVME")==0) 
  {
    device->dmaSpace = DMA_SPACE_VME|DMA_VME_2eVME;
  }
  else if(strcmp(vmeProtocol, "2eSST160")==0) 
  {
    device->dmaSpace = DMA_SPACE_VME|DMA_VME_2e160;
  }
  else if(strcmp(vmeProtocol, "2eSST233")==0) 
  {
    device->dmaSpace = DMA_SPACE_VME|DMA_VME_2e233;
  }
  else if(strcmp(vmeProtocol, "2eSST320")==0) 
  {
    device->dmaSpace = DMA_SPACE_VME|DMA_VME_2e320;
  }
  
TESTJUMP: 
  if( strncmp(resource, "VME_", 4)!=0 )
  {
  if( strcmp(swap, "WS")==0 )
    device->dmaSpace |= DMA_SPACE_WS;
  else if( strcmp(swap, "DS")==0 )
    device->dmaSpace |= DMA_SPACE_DS;
  else if( strcmp(swap, "QS")==0 )
    device->dmaSpace |= DMA_SPACE_QS;
  }
  else
  {
    if( strcmp(swap, "WS")==0 )
      device->vmeSwap = DMA_SPACE_WS;
    else if( strcmp(swap, "DS")==0 )
      device->vmeSwap = DMA_SPACE_DS;
    else if( strcmp(swap, "QS")==0 )
      device->vmeSwap = DMA_SPACE_QS;
    else
      device->vmeSwap = 0;
      
    switch(vmePktSize) 
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
        printf("pevConfigure: vmePktSize parameter wrong/not specified - set to DMA_SIZE_PKT_128\n");
        break;
    }
  }
    
  device->baseOffset = offset;
  if( initHookregDone == epicsFalse )
  {
    initHookRegister((initHookFunction)pevHookFunc);
    initHookregDone = epicsTrue;
    epicsAtExit((void*)pevDrvAtexit, NULL);
  }

  if( intrVec )
  {
    if( intrVec<0 || intrVec>255 || 
         ((device->pev_rmArea_map.mode & MAP_SPACE_USR1) && (intrVec>16)) ||
         ((device->pev_rmArea_map.mode & MAP_SPACE_SHM) && (intrVec>1))  || 
	 ((device->pev_rmArea_map.mode & MAP_SPACE_PCIE) && (intrVec>1)) )
    {
      printf("pevAsynConfigure: ERROR, intrVec out of range (1<=VME<=255, 1<=USR1<=16, SH_MEM=1, PCIE=1)\n");
      return errno;
    }	
    if( !pevIntrHandlerInitialized ) 
    {
      pevIntrHanlder_init();
    }
    pev_evt_queue_disable(pevIntrEvent);
    if(strncmp(resource, "VME_", 4)==0)
      device->ioscanpvt = pevIntrTable[intrVec];
      
    if(device->pev_rmArea_map.mode & MAP_SPACE_USR1)
    {
      device->ioscanpvt = pevIntrTable[255+intrVec];
    }
    
    if(device->pev_rmArea_map.mode & MAP_SPACE_SHM)
      device->ioscanpvt = pevIntrTable[272];
    
    if(device->pev_rmArea_map.mode & MAP_SPACE_PCIE)
      device->ioscanpvt = pevIntrTable[273];
    
    pevIntrRegister(device->pev_rmArea_map.mode); 
    pevIntrEvent->wait = -1;
    signal(pevIntrEvent->sig, pevIntrHandler);
    pev_evt_queue_enable(pevIntrEvent);
  }
 	
  regDevAsyncRegisterDevice(name, &pevAsynSupport, device);
  ellAdd (&pevRegDevAsynList, &device->node);
  if(crate > currConfCrates)
    currConfCrates = crate;
  return 0;
}


static void pevHookFunc(initHookState state)
{
  /*  
    initHookAfterFinishDevSup,
    initHookAfterScanInit,
	must prepare dma msg_queue and spawn dma_request_handler thread
	which waits on the msg_queue for incoming requests and processes them
	one by one
  */
      
  switch(state) {
    case initHookAfterFinishDevSup:
      pev_dmaQueue_init(1);
      break;
      
    default:
      break;
  }
  return;
}

/**
*
* dma multi-user request handling
*
**/

void *pev_dmaRequetServer(int *crate)
{
   pevDmaReqMsg msgptr;           /* -> allocated message space */
   int numByteRecvd = 0;
   struct pev_ioctl_dma_sts dmaStatus;
   int dmaStatRet;
   
   
   /* NOTE: we assume always there is only one crate with number = 1
   if( *(int*)crate == 0)
   {
       printf("pev_dmaRequetServer(): ERROR, invalid crate num (%d) passed!\n", *(int*)crate);
       return -1;
   }
   */
  
   while(1)
   {
     dmaServerDebug = 4;
     numByteRecvd = epicsMessageQueueReceive(pevDmaMsgQueueId, (pevDmaReqMsg*)&msgptr, sizeof(pevDmaReqMsg));
     if(pevDrvDebug)
       {
         printf("pev_dmaRequetServer(): msgptr.pCallBack = %p \n", (CALLBACK*)msgptr.pCallBack);
       }
     if (numByteRecvd <= 0)
       {
         printf("pev_dmaRequetServer(): WARNING: received empty message, size = %d \n",numByteRecvd);
	 continue;
       }
     dmaServerDebug = 1;
     /* usleep(1); */
     if( (*(msgptr.reqStatus) = pev_dma_move(&msgptr.pev_dmaReq)) )
       {
     	 printf("pev_dmaRequetServer(): DMA failure or Timeout! dma status = 0x%x src = ",
	 msgptr.pev_dmaReq.dma_status);
     	 if(msgptr.pev_dmaReq.src_space &  DMA_SPACE_VME) printf(" \"VME\" ");
     	 else if(msgptr.pev_dmaReq.src_space &  DMA_SPACE_PCIE) printf(" \"PCIE\" ");
     	 else if(msgptr.pev_dmaReq.src_space &  DMA_SPACE_USR1) printf(" \"USR1\" ");
     	 else if(msgptr.pev_dmaReq.src_space &  DMA_SPACE_SHM) printf(" \"SHM\" ");
	 printf(", dest = ");
     	 if(msgptr.pev_dmaReq.des_space &  DMA_SPACE_VME) printf(" \"VME\"\n");
     	 else if(msgptr.pev_dmaReq.des_space &  DMA_SPACE_PCIE) printf(" \"PCIE\"\n");
     	 else if(msgptr.pev_dmaReq.des_space &  DMA_SPACE_USR1) printf(" \"USR1\"\n");
     	 else if(msgptr.pev_dmaReq.des_space &  DMA_SPACE_SHM) printf(" \"SHM\"\n");
	 printf("\n");
       }
     else
       {
        dmaServerDebug = 2;
        dmaStatRet = pev_dma_status(0, &dmaStatus );
        if( dmaStatRet < 0 )
     	  { 
	    printf("pev_dmaRequetServer(): DMA transfer failed! dma status = 0x%x\n", dmaStatRet);
	  }
       }

     dmaLastTransferStat = msgptr.pev_dmaReq.dma_status;
     dmaServerDebug = 3;
     callbackRequest((CALLBACK*)msgptr.pCallBack);
   } /* endWhile */
   
   free(&msgptr);
   printf("Thread exiting...\n");
   exit(0);
}


int pev_dmaQueue_init(int crate)
{
epicsThreadId pevDmaThreadId;

 /* 
  * create a message queue to listen to this pev
  * and check for correct creation??????????
  */
 pevDmaReqLock = epicsMutexCreate();
 if( !pevDmaReqLock )
    {
      printf("ERROR; epicsMutexCreate for pevDmaReqLock failed\n");
      return -1;
    }
 
 
 pevDmaMsgQueueId = epicsMessageQueueCreate( DMA_Q_SIZE, sizeof(pevDmaReqMsg));
 if( !pevDmaMsgQueueId )
    {
      printf("ERROR; epicsMessageQueueCreate failed\n");
      return -1;
    }

  pevDmaThreadId = epicsThreadCreate("pevDmaReqHandler",99,
                                      epicsThreadGetStackSize(epicsThreadStackMedium),
                                      (EPICSTHREADFUNC)pev_dmaRequetServer,(int*)&crate);

  if( !pevDmaThreadId )
    {
      printf("ERROR; epicsThreadCreate failed. Thread id returned %p\n", pevDmaThreadId);
      return -1;
    }
   
return 0;
}

/**
*
* EPICS shell debug utility
*
**/
int pevExpertReport(int level, int debug)
{
 struct pev_ioctl_dma_sts dmaStatus;
 
 printf("pevExpertReport:");
 if(glbVmeSlaveMap.a32_size == 0)
   printf("\n\t Main VME slave window (A32) has been turned OFF!\n");
 else
   printf("\n\t Main VME slave window (A32) at 0x%x [size = 0x%x]\n", glbVmeSlaveMap.a32_base, glbVmeSlaveMap.a32_size);
 
 if(level > 1)
 {
   printf("\t dmaMessageQueue: ");
   epicsMessageQueueShow(pevDmaMsgQueueId, level);
   pevDrvDebug = debug;
   pev_dma_status(0, &dmaStatus );
 
   printf("\n\t dmaServerDebug = %d dmaStat = %d",dmaServerDebug, pev_dma_status(0, &dmaStatus ));
   printf("\n\t Last DMA transfer Status : 0x%x = ", dmaLastTransferStat);
   if(dmaLastTransferStat & DMA_STATUS_WAITING) printf(" START_WAITING, ");
   if(dmaLastTransferStat & DMA_STATUS_TMO) printf("TIMEOUT, ");
   if(dmaLastTransferStat & DMA_STATUS_ENDED) printf("ENDED_WAITING, ");
   if(dmaLastTransferStat & DMA_STATUS_DONE) printf("DONE, ");
 }
 printf("\n");

 return 0;
}
 
/**
*
* end of dma multi-user request handling
*
**/

/**
* interrupt handling stuff:
* we keep a table for 255 VME + 16 SUSER + 1 SHMEM + 1 PCIE = 273 total
* VME: 1-255, SUSER: 256-271, SHMEM: 272, PCIE: 273 
**/
int pevIntrHanlder_init() 
{
  int iSrcAndVec;
  
  if( (pevIntrTable = malloc(274*sizeof(IOSCANPVT)))==NULL )
    return errno;
   
  for(iSrcAndVec=0; iSrcAndVec<274; iSrcAndVec++)
  {
    scanIoInit(&pevIntrTable[iSrcAndVec]);
  }
  pevIntrEvent = pev_evt_queue_alloc( SIGUSR2);  
  pevIntrHandlerInitialized = epicsTrue;
  return 0;
}

int pevIntrRegister(int mapMode)
{    
  int src_id = ISRC_NONE, i, idMax;
   
  if( mapMode & MAP_SPACE_VME)
    {
    src_id = ISRC_VME;
    idMax = 8;		/* all 7 VME intr levels */
    }
  else
  if( mapMode & MAP_SPACE_USR1)
    {
    src_id = ISRC_USR;
    idMax = 16;		/* all 16 USER1 intr sources */
    }
  else
  if( mapMode & MAP_SPACE_SHM)
    {
    src_id = ISRC_SHM;
    idMax = 1;
    }
  else
  if( mapMode & MAP_SPACE_PCIE)
    {
    src_id = ISRC_PCI;
    idMax = 1;
    }
  
  if(src_id != ISRC_NONE)
    for( i = 0; i < idMax; i++)		
    {
      pev_evt_register( pevIntrEvent, src_id++);
    }
  return 0;
}

void pevIntrHandler(int sig)
{ 
  int intrEntry = 0;
  /* printf("pevIntrHandler(): in pevIntrHandle...%d\n", sig); */
  do
  {
    pev_evt_read( pevIntrEvent, 0);
    if(pevIntrEvent->src_id)	/* probably wrong since 0 is ISRC_PCI */
    {
      /* printf("%x - %x - %d\n", pevIntrEvent->src_id, pevIntrEvent->vec_id, pevIntrEvent->evt_cnt); */
      if(pevIntrEvent->src_id & ISRC_VME)
      	intrEntry = pevIntrEvent->vec_id;
      else
      if(pevIntrEvent->src_id & ISRC_USR)
      	intrEntry = 256 + (pevIntrEvent->src_id & 0x0f);
      else
      if(pevIntrEvent->src_id & ISRC_SHM)
      	intrEntry = 272;
      else
      if(pevIntrEvent->src_id & ISRC_PCI)
      	intrEntry = 273;
            
      scanIoRequest(pevIntrTable[intrEntry]);
      pev_evt_unmask( pevIntrEvent, pevIntrEvent->src_id);
    }
    else
    {
      printf("pevIntrEvent queue empty... count = %d, src_id = 0x%x \n", pevIntrEvent->evt_cnt, pevIntrEvent->src_id);
    }
  } while(pevIntrEvent->evt_cnt);
  return;
}

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
  
  if( !pev_init(0) )
  {
    printf("pevVmeSlaveMainConfig(): ERROR, Cannot allocate data structures to control IFC\n");
    return -1;
  }
  pev_vme_conf_read(&vme_conf);

  if( strcmp(addrSpace, "AM32")==0 )
  { 
    if( glbVmeSlaveMap.a32_size != 0 )
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
    if( mainSize == 0 )			/* turn off slave window */
    {
      vme_conf.slv_ena = 0;
      printf("\t => pevVmeSlaveMainConfig(): turnning OFF the whole vme slave window !!\n");
    }
    else 
      vme_conf.slv_ena = 1; 
  }
  else
  if( strcmp(addrSpace, "AM24")==0 )
  { 
    if( glbVmeSlaveMap.a24_size != 0 )
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
  if( strcmp(addrSpace, "AM32")==0 && vme_conf.slv_ena && (vme_conf.a32_base != mainBase || vme_conf.a32_size != mainSize) )
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
  regDevice* device = (regDevice*)malloc(sizeof(regDevice));
  epicsUInt8 dmaSpace = 0;		/*  must be used also to contain user provided DMA_SPACE_W/D/QS */

  if (!slaveAddrSpace || !vmeProtocol || !swapping)
  {
    printf("usage: pevVmeSlaveTargetConfig (\"A24\"|\"A32\", base, size, \"BLT\"|\"MBLT\"|\"2eVME\"|\"2eSST160\"|\"2eSST233\"|\"2eSST320\", \"SH_MEM\"|\"PCIE\"|\"USR1/2\", offset, \"WS\"|\"DS\"|\"QS\")\n");
    return -1;
  }
  
  if( strcmp(slaveAddrSpace, "AM32")==0 )
  { 
    if( glbVmeSlaveMap.a32_size == 0 )
    {
      printf("pevVmeSlaveTargetConfig(): ERROR, pevVmeSlaveMainConfig() has to be called first, Main \
              a32_size = 0x%x \n", glbVmeSlaveMap.a32_size);
      return -1;
    }
    addrType = atVMEA32;
    mainSlaveBase = glbVmeSlaveMap.a32_base;
  }
  else
  if( strcmp(slaveAddrSpace, "AM24")==0 )
  { 
    if( glbVmeSlaveMap.a24_size == 0 )
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
  if( strcmp(swapping, "WS")==0 )
    dmaSpace = DMA_SPACE_WS;
  if( strcmp(swapping, "DS")==0 )
    dmaSpace = DMA_SPACE_DS;
  if( strcmp(swapping, "QS")==0 )
    dmaSpace = DMA_SPACE_QS;
  
/*
  if(strcmp(vmeProtocol, "BLT")==0) 
    dmaSpace = DMA_SPACE_VME|DMA_VME_BLT;
  else if(strcmp(vmeProtocol, "MBLT")==0) 
    dmaSpace = DMA_SPACE_VME|DMA_VME_MBLT;
  else if(strcmp(vmeProtocol, "2eVME")==0) 
    dmaSpace = DMA_SPACE_VME|DMA_VME_2eVME;
  else if(strcmp(vmeProtocol, "2eSST160")==0) 
    dmaSpace = DMA_SPACE_VME|DMA_VME_2e160;
  else if(strcmp(vmeProtocol, "2eSST233")==0) 
    dmaSpace = DMA_SPACE_VME|DMA_VME_2e233;
  else if(strcmp(vmeProtocol, "2eSST320")==0) 
    dmaSpace = DMA_SPACE_VME|DMA_VME_2e320;
*/    
  vme_slv_map.loc_addr = winBase;	       /* offset within main VME slave window */
  vme_slv_map.rem_addr = targetOffset;  	       /* offset within target space */
  vme_slv_map.mode = MAP_ENABLE|MAP_ENABLE_WR;
  vme_slv_map.flag = MAP_FLAG_FORCE;
  vme_slv_map.sg_id = MAP_SLAVE_VME;
  vme_slv_map.size = winSize;
  if( strcmp(target, "SH_MEM")==0 )
    vme_slv_map.mode |= MAP_SPACE_SHM | dmaSpace;
  else
  if( strcmp(target, "USR1")==0 )
    vme_slv_map.mode |= MAP_SPACE_USR1 | dmaSpace;
  else
  if( strcmp(target, "PCIE")==0 )
    vme_slv_map.mode |= MAP_SPACE_PCIE | dmaSpace;
  else
  {
    printf("  pevVmeSlaveTargetConfig(): == ERROR == invalid target [valid options: SH_MEM, USR1, PCIE]\n");
    return -1;
  }

  /** this is needed for cleanup at exit **/
  device->pev_rmArea_map = vme_slv_map;
  ellAdd (&pevRegDevList, &device->node);
  
  if( pev_map_alloc( &vme_slv_map) || vme_slv_map.win_size!=winSize )
  {
    printf("  pevVmeSlaveTargetConfig(): == ERROR == translation window allocation failed! \
    		\n\t [allocated mapped size 0x%x] \n\t Address/Size Granularity must be 1MB=0x100000 \n", vme_slv_map.win_size);
    return -1;
  }

  if( devRegisterAddress (
  	     target,			
  	     addrType,  		    
  	     mainSlaveBase+winBase,		       
  	     winSize,		 
  	     (volatile void **)(void *)&dummyMap) != 0 ) {
      printf("  pevVmeSlaveTargetConfig(): == ERROR == devRegisterAddress() failed for slave at 0x%x\n", mainSlaveBase+winBase);
      return -1;
  }
  printf("\t Mapped offset 0x%x in \"%s\" to VMEslave [%s] at 0x%x [size 0x%x]\n"
  		, targetOffset, target, slaveAddrSpace, mainSlaveBase+winBase, vme_slv_map.win_size);
  return 0;
}



#ifdef EPICS_3_14

#include <iocsh.h>
static const iocshArg pevConfigureArg0 = { "crate", iocshArgInt };
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

static void pevRegistrar ()
{
    iocshRegister(&pevConfigureDef, pevConfigureFunc);
}

epicsExportRegistrar(pevRegistrar);


static const iocshFuncDef pevAsynConfigureDef =
    { "pevAsynConfigure", 10, pevConfigureArgs };
    
static void pevAsynConfigureFunc (const iocshArgBuf *args)
{
    int status = pevAsynConfigure(
        args[0].ival, args[1].sval, args[2].sval, args[3].ival, args[4].sval, args[5].ival,args[6].ival, args[7].ival, args[8].sval, args[9].ival);
    if (status != 0 && !interruptAccept) epicsExit(1);
}

static void pevAsynRegistrar ()
{
    iocshRegister(&pevAsynConfigureDef, pevAsynConfigureFunc);
}

epicsExportRegistrar(pevAsynRegistrar);

static const iocshArg pevExpertReportArg0 = { "level", iocshArgInt };
static const iocshArg pevExpertReportArg1 = { "debug", iocshArgInt };

static const iocshArg * const pevExpertReportArgs[] = {
    &pevExpertReportArg0,
    &pevExpertReportArg1,
};

static const iocshFuncDef pevExpertReportDef =
    { "pevExpertReport", 2, pevExpertReportArgs };
    
static void pevExpertReportFunc (const iocshArgBuf *args)
{
    int status = pevExpertReport(
        args[0].ival, args[1].ival);
    if (status != 0 && !interruptAccept) epicsExit(1);
}

static void pevExpertReportRegistrar ()
{
    iocshRegister(&pevExpertReportDef, pevExpertReportFunc);
}

epicsExportRegistrar(pevExpertReportRegistrar);

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

static void pevVmeSlaveMainConfigRegistrar ()
{
    iocshRegister(&pevVmeSlaveMainConfigDef, pevVmeSlaveMainConfigFunc);
}

epicsExportRegistrar(pevVmeSlaveMainConfigRegistrar);
		
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

static void pevVmeSlaveRegistrar ()
{
    iocshRegister(&pevVmeSlaveTargetConfigDef, pevVmeSlaveTargetConfigFunc);
}

epicsExportRegistrar(pevVmeSlaveRegistrar);

#endif
