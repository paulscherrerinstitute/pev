#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <devLib.h>
#include "../regDev/regDev.h"

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

 /* 
#include <pthread.h>
#include <mqueue.h>    message queue stuff */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pevioctl.h>
#include <pevulib.h>




#define MAGIC 		1100 /*  pev1100 */
#define SHMEM_MAP_SIZE 	0x10000000     	/*  256 MB DDR2 shared memory */
#define VME32_MAP_SIZE 	0xDF00000      	/*  223 MB A32 */
#define VME24_MAP_SIZE 	0x1000000      	/* 16 MB A24 */
#define VMECR_MAP_SIZE 	0x1000000	/* 16 MB CR/CSR */
#define VME16_MAP_SIZE 	0x10000		/* 64 KB A16 */
#define PCIE_MAP_SIZE  	0x1000		/* ????????? */
#define DMA_BUF_SIZE   	0x10000
#define NO_DMA_SPACE	0xFF		/* DMA space not specified */
#define RNGBUF_SIZE 	1000 
#define DMA_Q_SIZE	1000
#define DMA_MAXMSG_SIZE 500


/*
static char cvsid_pev1100[] __attribute__((unused)) =
    "$Id: pevDrv.c,v 1.1 2012/02/06 09:53:06 kalantari Exp $";
*/
static void pevHookFunc(initHookState state);
int pev_dmaQueue_init(int crate);
epicsMessageQueueId pevDmaMsgQueueId = 0;
epicsBoolean initHookregDone = epicsFalse;


typedef struct pevDmaReqMsg {
    struct pev_ioctl_dma_req pev_dmaReq; 
    CALLBACK* pCallBack;			/* record callback structure */
    unsigned int dlen; 
    unsigned int nelem;
    void *pdata;
    int swap;
    ulong pev_dmaBuf_usr;
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
    epicsBoolean dmaAllocFailed;
    int dmaStatus;
    epicsUInt8 dmaSpace;
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
    epicsBoolean dmaAllocFailed;
    int dmaStatus;
    epicsUInt8 dmaSpace;
};

int pevDebug = 0;

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

IOSCANPVT mmapGetInScanPvt(
    regDevice *device,
    unsigned int offset)
{
    if (!device || device->magic != MAGIC)
    { 
        errlogSevPrintf(errlogMajor,
            "mmapGetInScanPvt: illegal device handle\n");
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
  
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevRead: illegal device handle\n");
        return -1;
    }
    if (offset > device->pev_rmArea_map.win_size)
    {
        errlogSevPrintf(errlogMajor,
            "pevRead %s: offset %d out of range (0-%d)\n",
            device->name, offset, device->pev_rmArea_map.win_size);
        return -1;
    }
    if (offset+dlen*nelem > device->pev_rmArea_map.win_size)
    {
        errlogSevPrintf(errlogMajor,
            "pevRead %s: offset %d + %d bytes length exceeds mapped size %d by %d bytes\n",
            device->name, offset, nelem, device->pev_rmArea_map.win_size,
            offset+dlen*nelem - device->pev_rmArea_map.win_size);
        return -1;
    }

    if( device->pev_rmArea_map.mode & MAP_SPACE_VME)
      swap = 1;
      
    if( nelem > 100 ) 		/* do DMA */
    {  
    if(device->dmaSpace == NO_DMA_SPACE) 
    	printf("pevRead(): \"%s\" DMA SPACE not specified! continue with normal transfer \n", device->name);
    else
    {
      device->pev_dmaReq.src_addr = device->baseOffset + offset;  		/* src bus address */ 
      device->pev_dmaReq.des_addr = (ulong)device->pev_dmaBuf.b_addr;       	/* des bus address */
      device->pev_dmaReq.size = nelem*dlen;              				
      device->pev_dmaReq.src_space = device->dmaSpace;		
      device->pev_dmaReq.des_space = DMA_SPACE_PCIE;
      device->pev_dmaReq.src_mode = 0;
      device->pev_dmaReq.des_mode = 0;
      device->pev_dmaReq.start_mode = DMA_MODE_BLOCK;
      device->pev_dmaReq.end_mode = 0;
      device->pev_dmaReq.intr_mode = DMA_INTR_ENA;
      device->pev_dmaReq.wait_mode = DMA_WAIT_INTR;
      if( pev_dma_move(&device->pev_dmaReq) )
        {
          printf("pevRead(): Invalid DMA transfer parameters! do normal transfer\n");
	}
      else 
        {
	 if( device->pev_dmaReq.dma_status )
           printf("pevRead(): DMA transfer failed! do normal transfer\n");
	 else
	  {
             regDevCopy(dlen, nelem, device->pev_dmaBuf.u_addr, pdata, NULL, swap);
	     printf("Dma Done!!!!!! device->pev_dmaBuf.u_addr = 0x%08lx\n", (ulong)device->pev_dmaBuf.u_addr);
             return 0;
	   }
        }
      }
    }
      
    regDevCopy(dlen, nelem, device->uPtrMapRes + offset, pdata, NULL, swap); 
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
   	
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevRead: illegal device handle\n");
        return -1;
    }

    if (offset > device->pev_rmArea_map.win_size)
    {
        errlogSevPrintf(errlogMajor,
            "pevRead %s: offset %d out of range (0-%d)\n",
            device->name, offset, device->pev_rmArea_map.win_size);
        return -1;
    }
    if (offset+dlen*nelem > device->pev_rmArea_map.win_size)
    {
        errlogSevPrintf(errlogMajor,
            "pevRead %s: offset %d + %d bytes length exceeds mapped size %d by %d bytes\n",
            device->name, offset, nelem, device->pev_rmArea_map.win_size,
            offset+dlen*nelem - device->pev_rmArea_map.win_size);
        return -1;
    }
    if( device->pev_rmArea_map.mode & MAP_SPACE_VME)
      swap = 1;
    
    if(nelem > 100)
    {
    if(device->dmaSpace == NO_DMA_SPACE) 
    	printf("pevWrite(): \"%s\" DMA SPACE not specified! continue with normal transfer \n", device->name);
    else
     {	
      /****
      * 	workaround: first copy the src data to kernel allocated buffer  
      ***/
      regDevCopy(dlen, nelem, pdata, device->pev_dmaBuf.u_addr, NULL, swap); 

      device->pev_dmaReq.src_addr = (ulong)device->pev_dmaBuf.b_addr;                   
      device->pev_dmaReq.des_addr = device->baseOffset + offset;       /* (ulong)pdata destination is DMA buffer    */
      device->pev_dmaReq.size = nelem*dlen;                  
      device->pev_dmaReq.src_space = DMA_SPACE_PCIE;
      device->pev_dmaReq.des_space = device->dmaSpace;
      device->pev_dmaReq.src_mode = 0;
      device->pev_dmaReq.des_mode = 0;
      device->pev_dmaReq.start_mode = DMA_MODE_BLOCK;
      device->pev_dmaReq.end_mode = 0;
      device->pev_dmaReq.intr_mode = DMA_INTR_ENA;
      device->pev_dmaReq.wait_mode = DMA_WAIT_INTR;
      if( pev_dma_move(&device->pev_dmaReq) )
        {
          printf("pevRead(): Invalid DMA transfer parameters! do normal transfer\n");
	}
      else 
        {
	 if( device->pev_dmaReq.dma_status )
           printf("pevRead(): DMA transfer failed! do normal transfer\n");
	 else
	  {
             regDevCopy(dlen, nelem, device->pev_dmaBuf.u_addr, pdata, NULL, swap);
	     printf("Dma Done!!!!!! \n");
             return 0;
	   }
        }
     }
    }
      
    regDevCopy(dlen, nelem, pdata, device->uPtrMapRes + offset, NULL, swap); 
    return 0;
}
	

static regDevSupport pevSupport = {
    pevReport,
    NULL,
    NULL,
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
    int prio)
{
  int swap = 0;
  pevDmaReqMsg pevDmaRequest;
  
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevAsynRead: illegal device handle\n");
        return -1;
    }
    if (offset > device->pev_rmArea_map.win_size)
    {
        errlogSevPrintf(errlogMajor,
            "pevAsynRead %s: offset %d out of range (0-%d)\n",
            device->name, offset, device->pev_rmArea_map.win_size);
        return -1;
    }
    if (offset+dlen*nelem > device->pev_rmArea_map.win_size)
    {
        errlogSevPrintf(errlogMajor,
            "pevAsynRead %s: offset %d + %d bytes length exceeds mapped size %d by %d bytes\n",
            device->name, offset, nelem, device->pev_rmArea_map.win_size,
            offset+dlen*nelem - device->pev_rmArea_map.win_size);
        return -1;
    }

    if( device->pev_rmArea_map.mode & MAP_SPACE_VME)
      swap = 1;
      
    if( nelem > 100 ) 		/* do DMA */
    {  
    if(device->dmaSpace == NO_DMA_SPACE && !cbStruct) 
    	printf("pevAsynRead(): \"%s\" DMA SPACE not specified! continue with normal transfer \n", device->name);
    else
    {	        
      device->pev_dmaReq.src_addr = device->baseOffset + offset;  		/* src bus address */ 
      device->pev_dmaReq.des_addr = (ulong)device->pev_dmaBuf.b_addr;       	/* des bus address */
      device->pev_dmaReq.size = nelem*dlen;              				
      device->pev_dmaReq.src_space = device->dmaSpace;		
      device->pev_dmaReq.des_space = DMA_SPACE_PCIE;
      device->pev_dmaReq.src_mode = 0;
      device->pev_dmaReq.des_mode = 0;
      device->pev_dmaReq.start_mode = DMA_MODE_BLOCK;
      device->pev_dmaReq.end_mode = 0;
      device->pev_dmaReq.intr_mode = DMA_INTR_ENA;
      device->pev_dmaReq.wait_mode = DMA_WAIT_INTR;
      
      pevDmaRequest.pev_dmaReq = device->pev_dmaReq;
      pevDmaRequest.pCallBack = cbStruct;
      pevDmaRequest.dlen = dlen;
      pevDmaRequest.nelem = nelem;
      pevDmaRequest.pdata = pdata;
      pevDmaRequest.swap = swap;
      pevDmaRequest.pev_dmaBuf_usr = (ulong)device->pev_dmaBuf.u_addr;
      
      if( !epicsMessageQueueSend(pevDmaMsgQueueId, (void*)&pevDmaRequest, sizeof(pevDmaReqMsg)) )
         return (1);   /* to tell regDev that this is first phase of record processing (to let recSupport set PACT to true) */
      else 
        {
           printf("pevAsynRead(): sending DMA request failed! do normal & synchronous transfer\n");
	}
      }
    }
    
    regDevCopy(dlen, nelem, device->uPtrMapRes + offset, pdata, NULL, swap); 
    return 0;

}

void pevAsynReport(
    regDeviceAsyn *device,
    int level)
{
    if (device && device->magic == MAGIC)
    {
        printf("pev driver: for resource \"%s\" \n",
            device->resource);
    }
}

static regDevAsyncSupport pevAsynSupport = {
    pevAsynReport,
    NULL,
    NULL,
    pevAsynRead,
    pevWrite	/* asyn write must be implemented */
};

static void pevDrvAtexit(regDevice* device)
{
  struct pev_ioctl_map_ctl pev_ctl;
  pev_ctl.sg_id = device->pev_rmArea_map.sg_id; 
  printf("pevDrvAtexit(): we are at ioc exit**********\n");
  
  if(pev_ctl.sg_id)
  { 
    if(pev_map_clear(&pev_ctl))
    	printf("pevDrvAtexit(): pev_map_clear(&pev_ctl) failed**********\n");
  
    if(&device->pev_rmArea_map)
    {
       pev_munmap( &device->pev_rmArea_map);
       pev_map_free( &device->pev_rmArea_map);
    }
  }
  else
     printf("pevDrvAtexit(): no mapping found. sg_id = %d **********\n", pev_ctl.sg_id);

  if(!device->dmaAllocFailed) 
  	pev_buf_free( &device->pev_dmaBuf);
	
  pev_exit( device->pev);
  free(device);
  exit(0);
}

static ELLLIST pevRegDevList;                        /* Linked list of sync register devices */
static ELLLIST pevRegDevAsynList;                    /* Linked list of Async register devices */



static struct pev_ioctl_map_pg null_pev_rmArea_map = {0,0,0,0,0,0,0,0,0,0,0,0};
/*****
*	findMappedPevResource():
*
*	Desc.: finds already mapped resource be it synchronous or asynchronous device.
*
*****/
static struct pev_ioctl_map_pg findMappedPevResource(struct pev_node *pev, const char* resource, epicsBoolean* mapStat) {
  regDevice* device;    
  regDeviceAsyn* asdevice;    

  for (device = (regDevice *)ellFirst(&pevRegDevList);
  	device != NULL;
  	device = (regDevice *)ellNext(&device->node)) {

       if ( pev == device->pev && !strcmp(device->resource, resource) ) {
  	   /* pev_rmArea_map = device->pev_rmArea_map*/;
	   *mapStat = epicsTrue; 
           printf ("findMappedPevResource(): resource \"%s\" already mapped.. continue\n", device->resource);
  	   return(device->pev_rmArea_map);
       }

  }
  
  for (asdevice = (regDeviceAsyn *)ellFirst(&pevRegDevAsynList);
  	asdevice != NULL;
  	asdevice = (regDeviceAsyn *)ellNext(&asdevice->node)) {

       if ( pev == asdevice->pev && !strcmp(asdevice->resource, resource) ) {
  	   /* pev_rmArea_map = device->pev_rmArea_map*/;
	   *mapStat = epicsTrue; 
           printf ("findMappedPevResource(): resource \"%s\" already mapped.. continue\n", asdevice->resource);
  	   return(asdevice->pev_rmArea_map);
       }

  }
  
  return null_pev_rmArea_map;
}

/**
*	pevConfigure(crate, name, resource, offset)  
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
**/

int pevConfigure(
    unsigned int crate,
    const char* name,
    const char* resource,
    unsigned int offset,
    char* vmeProtocol)
{
    
  regDevice* device;    
  char* tmpStrCpy;
  struct pev_node *pev;
  struct pev_ioctl_map_pg pev_rmArea_map = null_pev_rmArea_map;
  epicsBoolean mapExists = epicsFalse;
   
  /* see if this device already exists */
  device = regDevFind(name);
  if( device ) 
  {
    printf("pevConfigure: ERROR, device \"%s\" on pev1100 in %s already in use\n", 
    		device->name, device->resource);
    exit( -1);
  }

  /* call PEV1100 user library initialization function */
  pev = pev_init( crate);
  if( !pev)
  {
    printf("Cannot allocate data structures to control PEV1100\n");
    exit( -1);
  }
  /* verify if the PEV1100 is accessible */
  if( pev->fd < 0)
  {
    printf("Cannot find PEV1100 interface\n");
    exit( -1);
  }
  
  if( !ellFirst(&pevRegDevList) )
    ellInit (&pevRegDevList);

  pev_rmArea_map = findMappedPevResource(pev, resource, &mapExists);
  
  device = (regDevice*)malloc(sizeof(regDevice));
  if (device == NULL)
  {
      errlogSevPrintf(errlogFatal,
  	  "pevConfigure %s: out of memory\n",
  	  name);
      return errno;
  }
  
  device->pev = pev;
  device->magic = MAGIC;
  tmpStrCpy = malloc(strlen(name)+1);
  strcpy(tmpStrCpy, name);
  device->name = tmpStrCpy;
  tmpStrCpy = malloc(strlen(resource)+1);
  strcpy(tmpStrCpy, resource);
  device->resource = tmpStrCpy;
  device->dmaSpace = NO_DMA_SPACE;
  
  /* get the current VME configuration */
  pev_vme_conf_read( &device->vme_conf);
  printf("VME A32 base address = 0x%08x [0x%x]", device->vme_conf.a32_base, device->vme_conf.a32_size);
  if( device->vme_conf.mas_ena)
  {
    printf(" -> enabled\n");
  }
  else
  {
    printf(" -> disabled\n");
  }
  
  if( mapExists )
    {
      device->pev_rmArea_map = pev_rmArea_map;
      printf ("pevConfigure: skip mapping...\n");
      goto SKIP_PEV_RESMAP;
    }
      
  epicsAtExit((void*)pevDrvAtexit, device);
  
  device->pev_dmaBuf.size = DMA_BUF_SIZE;
  if( pev_buf_alloc( &device->pev_dmaBuf)==MAP_FAILED || !device->pev_dmaBuf.b_addr )
    {
      printf("pevConfigure: ERROR, could not allocate dma buffer; bus %p user %p\n", device->pev_dmaBuf.b_addr, device->pev_dmaBuf.u_addr);
      device->dmaAllocFailed = epicsTrue;
      exit(0);
    }
  else
    device->dmaAllocFailed = epicsFalse;
  
  
  /* create an address translation window in the VME slave port */
  /* pointing to the PEV1100 Shared Memory                      */

 /* "SH_MEM", "PCIE", "VME_A16/24/32/BLT/MBLT/2eSST" */
   if( strcmp(resource, "SH_MEM")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_SHM;
  	device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 	/* shared memory base address */
  	device->pev_rmArea_map.size = SHMEM_MAP_SIZE;
	if( offset > SHMEM_MAP_SIZE) 
	  {
	    printf("pevConfigure: ERROR, too big offset\n");
      	    exit(0);
	  }
	device->dmaSpace = DMA_SPACE_SHM;
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
	     exit(0);
	  }
	device->dmaSpace = DMA_SPACE_PCIE;
     }
   else			/*******************   To BE DONE ***********************/
   if( strcmp(resource, "USR")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_USR;
  	/* device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 		
  	device->pev_rmArea_map.size = VME16_MAP_SIZE;
	if( offset > VME16_MAP_SIZE) 
	  {
	    printf("pevConfigure: ERROR, too big offset\n");
	    exit(0);
	  }*/
     }
   else
   if( strcmp(resource, "VME_A16")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_VME|MAP_VME_A16;
  	device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 		/* VME space */
  	device->pev_rmArea_map.size = VME16_MAP_SIZE;
	if( offset > VME16_MAP_SIZE) 
	  {
	    printf("pevConfigure: ERROR, too big offset\n");
	    exit(0);
	  }
     }
   else
   if( strcmp(resource, "VME_A24")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_VME|MAP_VME_A24;
  	device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 		/* VME space */
  	device->pev_rmArea_map.size = VME24_MAP_SIZE;
	if( offset > VME24_MAP_SIZE) 
	  {
	    printf("pevConfigure: ERROR, too big offset\n");
	    exit(0);
	  }
     }
   else
   if( strcmp(resource, "VME_A32")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_VME|MAP_VME_A32;
  	device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 		/* VME space */
  	device->pev_rmArea_map.size = VME32_MAP_SIZE;
	if( offset > VME32_MAP_SIZE) 
	  {
	    printf("pevConfigure: ERROR, too big offset\n");
	    exit(0);
	  }
     }
    else 
      { 
    	printf("Unknown PCIe remote area - valid options: SH_MEM, PCIE, VME_A16/24/32/\n");
    	exit( -1);
      }
   
  device->pev_rmArea_map.mode |= MAP_ENABLE|MAP_ENABLE_WR;
  device->pev_rmArea_map.flag = 0x0;
  pev_map_alloc( &device->pev_rmArea_map);
  printf("offset in PCI MEM window to access %s @ 0x%08X size: 0x%08x\n", resource, (uint)device->pev_rmArea_map.loc_addr, device->pev_rmArea_map.win_size);
  printf("perform the mapping in user's space : ");

SKIP_PEV_RESMAP:

  device->uPtrMapRes  = (char*)pev_mmap( &device->pev_rmArea_map);
  printf("%p", device->uPtrMapRes);
  if( device->uPtrMapRes == MAP_FAILED)
  {
    printf(" ->Failed\n");
    exit(0);
  }
  printf(" -> Done\n");
  
  device->uPtrMapRes += offset;
  device->baseOffset = offset;
  
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
 	
  regDevRegisterDevice(name, &pevSupport, device);
  ellAdd (&pevRegDevList, &device->node);
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
**/

int pevAsynConfigure(
    unsigned int crate,
    const char* name,
    const char* resource,
    unsigned int offset,
    char* vmeProtocol)
{
    
  regDeviceAsyn* device;    
  char* tmpStrCpy;
  struct pev_node *pev;
  struct pev_ioctl_map_pg pev_rmArea_map = null_pev_rmArea_map;
  epicsBoolean mapExists = epicsFalse;
   
  /* see if this device already exists */
  device = regDevAsynFind(name);
  if( device ) 
  {
    printf("pevAsynConfigure: ERROR, device \"%s\" on pev1100 in %s already in use\n", 
    		device->name, device->resource);
    exit( -1);
  }

  /* call PEV1100 user library initialization function */
  pev = pev_init( crate);
  if( !pev)
  {
    printf("pevAsynConfigure: Cannot allocate data structures to control PEV1100\n");
    exit( -1);
  }
  /* verify if the PEV1100 is accessible */
  if( pev->fd < 0)
  {
    printf("pevAsynConfigure: Cannot find PEV1100 interface\n");
    exit( -1);
  }
  
  if( !ellFirst(&pevRegDevAsynList) )
    ellInit (&pevRegDevAsynList);

  pev_rmArea_map = findMappedPevResource(pev, resource, &mapExists);
  
  device = (regDeviceAsyn*)malloc(sizeof(regDeviceAsyn));
  if (device == NULL)
  {
      errlogSevPrintf(errlogFatal,
  	  "pevAsynConfigure() %s: out of memory\n",
  	  name);
      return errno;
  };
  
  device->pev = pev;
  device->magic = MAGIC;
  tmpStrCpy = malloc(strlen(name)+1);
  strcpy(tmpStrCpy, name);
  device->name = tmpStrCpy;
  tmpStrCpy = malloc(strlen(resource)+1);
  strcpy(tmpStrCpy, resource);
  device->resource = tmpStrCpy;
  device->dmaSpace = NO_DMA_SPACE;
  
  /* get the current VME configuration */
  pev_vme_conf_read( &device->vme_conf);
  printf("VME A32 base address = 0x%08x [0x%x]", device->vme_conf.a32_base, device->vme_conf.a32_size);
  if( device->vme_conf.mas_ena)
  {
    printf(" -> enabled\n");
  }
  else
  {
    printf(" -> disabled\n");
  }
  
  if( mapExists )
    {
      device->pev_rmArea_map = pev_rmArea_map;
      printf ("pevConfigure: skip mapping...\n");
      goto SKIP_PEV_RESMAP;
    }
      
  epicsAtExit((void*)pevDrvAtexit, device);
  
  device->pev_dmaBuf.size = DMA_BUF_SIZE;
  if( pev_buf_alloc( &device->pev_dmaBuf)==MAP_FAILED || !device->pev_dmaBuf.b_addr )
    {
      printf("pevConfigure: ERROR, could not allocate dma buffer; bus %p user %p\n", device->pev_dmaBuf.b_addr, device->pev_dmaBuf.u_addr);
      device->dmaAllocFailed = epicsTrue;
      exit(0);
    }
  else
    device->dmaAllocFailed = epicsFalse;
  
  
  /* create an address translation window in the VME slave port */
  /* pointing to the PEV1100 Shared Memory                      */

 /* "SH_MEM", "PCIE", "VME_A16/24/32/BLT/MBLT/2eSST" */
   if( strcmp(resource, "SH_MEM")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_SHM;
  	device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 	/* shared memory base address */
  	device->pev_rmArea_map.size = SHMEM_MAP_SIZE;
	if( offset > SHMEM_MAP_SIZE) 
	  {
	    printf("pevConfigure: ERROR, too big offset\n");
      	    exit(0);
	  }
	device->dmaSpace = DMA_SPACE_SHM;
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
	     exit(0);
	  }
	device->dmaSpace = DMA_SPACE_PCIE;
     }
   else			/*******************   To BE DONE ***********************/
   if( strcmp(resource, "USR")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_USR;
  	/* device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 		
  	device->pev_rmArea_map.size = VME16_MAP_SIZE;
	if( offset > VME16_MAP_SIZE) 
	  {
	    printf("pevConfigure: ERROR, too big offset\n");
	    exit(0);
	  }*/
     }
   else
   if( strcmp(resource, "VME_A16")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_VME|MAP_VME_A16;
  	device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 		/* VME space */
  	device->pev_rmArea_map.size = VME16_MAP_SIZE;
	if( offset > VME16_MAP_SIZE) 
	  {
	    printf("pevConfigure: ERROR, too big offset\n");
	    exit(0);
	  }
     }
   else
   if( strcmp(resource, "VME_A24")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_VME|MAP_VME_A24;
  	device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 		/* VME space */
  	device->pev_rmArea_map.size = VME24_MAP_SIZE;
	if( offset > VME24_MAP_SIZE) 
	  {
	    printf("pevConfigure: ERROR, too big offset\n");
	    exit(0);
	  }
     }
   else
   if( strcmp(resource, "VME_A32")==0 ) 
     {
  	device->pev_rmArea_map.mode = MAP_SPACE_VME|MAP_VME_A32;
  	device->pev_rmArea_map.sg_id = MAP_MASTER_32;
  	device->pev_rmArea_map.rem_addr = 0x000000; 		/* VME space */
  	device->pev_rmArea_map.size = VME32_MAP_SIZE;
	if( offset > VME32_MAP_SIZE) 
	  {
	    printf("pevAsynConfigure: ERROR, too big offset\n");
	    exit(0);
	  }
     }
    else 
      { 
    	printf("Unknown PCIe remote area - valid options: SH_MEM, PCIE, VME_A16/24/32/\n");
    	exit( -1);
      }
   
  device->pev_rmArea_map.mode |= MAP_ENABLE|MAP_ENABLE_WR;
  device->pev_rmArea_map.flag = 0x0;
  pev_map_alloc( &device->pev_rmArea_map);
  printf("offset in PCI MEM window to access %s @ 0x%08X size: 0x%08x\n", resource, (uint)device->pev_rmArea_map.loc_addr, device->pev_rmArea_map.win_size);
  printf("perform the mapping in user's space : ");

SKIP_PEV_RESMAP:

  device->uPtrMapRes  = (char*)pev_mmap( &device->pev_rmArea_map);
  printf("%p", device->uPtrMapRes);
  if( device->uPtrMapRes == MAP_FAILED)
  {
    printf(" ->Failed\n");
    exit(0);
  }
  printf(" -> Done\n");
  
  device->uPtrMapRes += offset;
  device->baseOffset = offset;
    
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

  if( initHookregDone == epicsFalse )
  {
    printf("========= we at registering pevHookFunc()\n");
    initHookRegister((initHookFunction)pevHookFunc);
    initHookregDone = epicsTrue;
  }
 	
  regDevAsyncRegisterDevice(name, &pevAsynSupport, device);
  ellAdd (&pevRegDevAsynList, &device->node);
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
    case initHookAtEnd:
      printf("========= we are at case initHookAtBeginning\n");
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

void *pev_dmaRequetServer(void *crate)
{
   pevDmaReqMsg msgptr;           /* -> allocated message space */
   int numByteRecvd = 0;
   struct pev_ioctl_dma_sts dmaStatus;
   int dmaStatRet;
   
   
   if( *(int*)crate == 0)
   {
       printf("pev_dmaRequetServer(): ERROR, invalid crate num (%d) passed!\n", *(int*)crate);
    	exit(0);
   }
  
   while(1)
   {
     numByteRecvd = epicsMessageQueueReceive(pevDmaMsgQueueId, (pevDmaReqMsg*)&msgptr, sizeof(pevDmaReqMsg));
     if (numByteRecvd <= 0)
       {
         printf("pev_dmaRequetServe(): epicsMessageQueueReceive Failed! msg_size = %d \n",numByteRecvd);
	 continue;
       }

     if( pev_dma_move(&msgptr.pev_dmaReq) )
       {
     	 printf("pevRead(): Invalid DMA transfer parameters! do normal transfer\n");
       }
     else 
       {
        dmaStatRet = pev_dma_status( &dmaStatus );
        if( dmaStatRet < 0 )
     	  { 
	    printf("pevRead(): DMA transfer failed! do normal transfer; dma stat = 0x%x\n", dmaStatRet);
	    /**  
	    	TODO: DO NORMAL TRANSFER **/
	  }
        else
          {
     	    regDevCopy(msgptr.dlen, msgptr.nelem, (void*)msgptr.pev_dmaBuf_usr, msgptr.pdata, NULL, msgptr.swap);
	    callbackRequest((CALLBACK*)msgptr.pCallBack);
          }
       }     
   } /* endWhile */
   
   free(&msgptr);
   printf("Thread exiting...\n");
   exit(0);
}


int pev_dmaQueue_init(int crate)
{
epicsThreadId pevDmaThreadId;

 printf("========= we are at pev_dmaQueue_init()\n");
 /* 
  * create a message queue to listen to this pev
  * and check for correct creation??????????
  */
  
 pevDmaMsgQueueId = epicsMessageQueueCreate( DMA_Q_SIZE, sizeof(pevDmaReqMsg));
 if( !pevDmaMsgQueueId )
    {
      printf("ERROR; epicsMessageQueueCreate failed\n");
      return -1;
    }
  else
    {
       printf("===== created epicsMessageQueue with id %p \n",  pevDmaMsgQueueId);
    }

  pevDmaThreadId = epicsThreadCreate("pevDmaReqHandler",epicsThreadPriorityMedium,
                                      epicsThreadGetStackSize(epicsThreadStackMedium),
                                      (EPICSTHREADFUNC)pev_dmaRequetServer,(void*)&crate);

  if( !pevDmaThreadId )
    {
      printf("ERROR; epicsThreadCreate failed. Thread id returned %p\n", pevDmaThreadId);
      return -1;
    }
  else
    {
       printf("===== created epicsThread with id %p for pev_dmaRequetServer()\n",  pevDmaThreadId);
    }
   
return 0;
}
 
/**
*
* end of dma multi-user request handling
*
**/

#ifdef EPICS_3_14

#include <iocsh.h>
static const iocshArg mmapConfigureArg0 = { "crate", iocshArgInt };
static const iocshArg mmapConfigureArg1 = { "name", iocshArgString };
static const iocshArg mmapConfigureArg2 = { "resource", iocshArgString };
static const iocshArg mmapConfigureArg3 = { "offset", iocshArgInt };
static const iocshArg mmapConfigureArg4 = { "dmaSpace", iocshArgString };
static const iocshArg * const pevConfigureArgs[] = {
    &mmapConfigureArg0,
    &mmapConfigureArg1,
    &mmapConfigureArg2,
    &mmapConfigureArg3,
    &mmapConfigureArg4,
};

static const iocshFuncDef pevConfigureDef =
    { "pevConfigure", 5, pevConfigureArgs };
    
static void pevConfigureFunc (const iocshArgBuf *args)
{
    int status = pevConfigure(
        args[0].ival, args[1].sval, args[2].sval, args[3].ival, args[4].sval);
    if (status != 0) epicsExit(1);
}

static void pevRegistrar ()
{
    iocshRegister(&pevConfigureDef, pevConfigureFunc);
}

epicsExportRegistrar(pevRegistrar);


static const iocshFuncDef pevAsynConfigureDef =
    { "pevAsynConfigure", 5, pevConfigureArgs };
    
static void pevAsynConfigureFunc (const iocshArgBuf *args)
{
    int status = pevAsynConfigure(
        args[0].ival, args[1].sval, args[2].sval, args[3].ival, args[4].sval);
    if (status != 0) epicsExit(1);
}

static void pevAsynRegistrar ()
{
    iocshRegister(&pevAsynConfigureDef, pevAsynConfigureFunc);
}

epicsExportRegistrar(pevAsynRegistrar);

#endif
