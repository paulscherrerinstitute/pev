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


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pevioctl.h>
#include <pevulib.h>




#define MAGIC 		1100 		/*  pev1100 */
#define DMA_Q_SIZE	2000
#define MAX_PEVS	21		/* taken from VME 21 slot. makes sense ??? */
#define ELB_I2C_CTL	0x30		/* ELB_I2C_CTL address on ELB bus (4-bytes) */


/*
static char cvsid_pev1100[] __attribute__((unused)) =
    "$Id: i2cDrv.c,v 1.2 2012/05/01 10:47:44 kalantari Exp $";
*/
static void pevI2cHookFunc(initHookState state);
epicsBoolean initHookpevI2cDone = epicsFalse;
int pev_i2cQueue_init(int crate);
epicsMessageQueueId pevI2cMsgQueueId = 0;


typedef struct pevI2cReqMsg {
    CALLBACK* pCallBack;			/* record callback structure */
    unsigned int i2cDevice; 
    unsigned int i2cCmd;
    unsigned int i2cDatSiz;			/* I2C_DATSIZ in ELB_I2C_CTL register */
    unsigned int nelem;				/* number of elements */
    void *pi2cData;
    epicsBoolean readOperation;			/* is it read or not = write operation */
} pevI2cReqMsg;


struct regDeviceAsyn {
    ELLNODE  node;                   /* Linked list node structure */
    unsigned long magic;
    const char* name;
    struct pev_node* pev;
    unsigned int i2cDevice; 
};

int pevI2cDebug = 0;

/******** Support functions *****************************/ 

int pevI2cAsynRead(
    regDeviceAsyn *device,
    unsigned int offset,
    unsigned int dlen,
    unsigned int nelem,
    void* pdata,
    CALLBACK* cbStruct,
    int prio)
{
    pevI2cReqMsg pevI2cRequest;

    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevI2cAsynRead: illegal device handle\n");
        return -1;
    }

    pevI2cRequest.i2cDevice = device->i2cDevice;
    pevI2cRequest.pCallBack = cbStruct;
    pevI2cRequest.pi2cData = pdata;
    pevI2cRequest.i2cDatSiz = dlen;
    pevI2cRequest.nelem = nelem; 
    pevI2cRequest.i2cCmd = offset;		/* reg address within an i2c device */
    pevI2cRequest.readOperation = epicsTrue;
    
    if( !epicsMessageQueueSend(pevI2cMsgQueueId, (void*)&pevI2cRequest, sizeof(pevI2cReqMsg)) )
       return (1);  /* to tell regDev that this is first phase of record processing (to let recSupport set PACT to true) */
    else 
      {
    	 printf("pevI2cAsynRead(): sending I2C request failed! do normal & synchronous transfer\n");
      } 
	
    /* do synchronous i2c read here */
    *(unsigned int*)pdata = pev_i2c_read( device->i2cDevice, offset);	    
    return 0;
}

int pevI2cAsynWrite(
    regDeviceAsyn *device,
    unsigned int offset,
    unsigned int dlen,
    unsigned int nelem,
    void* pdata,
    CALLBACK* cbStruct,
    void* pmask,
    int priority)
{
  pevI2cReqMsg pevI2cRequest;
   	
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevI2cAsynWrite: illegal device handle\n");
        return -1;
    }

    pevI2cRequest.i2cDevice = device->i2cDevice;
    pevI2cRequest.pCallBack = cbStruct;
    pevI2cRequest.pi2cData = pdata;
    pevI2cRequest.i2cCmd = offset;		/* reg address within an i2c device */
    pevI2cRequest.readOperation = epicsFalse;
    
    if( !epicsMessageQueueSend(pevI2cMsgQueueId, (void*)&pevI2cRequest, sizeof(pevI2cReqMsg)) )
       return (1);   /* to tell regDev that this is first phase of record processing (to let recSupport set PACT to true) */
    else 
      {
    	 printf("pevI2cAsynWrite(): sending DMA request failed! do normal & synchronous transfer\n");
      }
	
    /* do the i2c write here */
    pev_i2c_write( device->i2cDevice, offset, *(unsigned int*)pdata);	    
    return 0;
}

void pevI2cAsynReport(
    regDeviceAsyn *device,
    int level)
{
    if (device && device->magic == MAGIC)
    {
        printf("pevI2cAsyn driver: for i2cDevice 0x\"%8x\" \n",
            device->i2cDevice);
    }
}


static regDevAsyncSupport pevI2cAsynSupport = {
    pevI2cAsynReport,
    NULL,
    NULL,
    pevI2cAsynRead,
    pevI2cAsynWrite,	
    NULL
};


static ELLLIST pevRegDevAsynList;                    /* Linked list of Async register devices */


/**
*	pevAsynI2cConfigure(crate, name, i2cControlWord)  
*
* 	crate: 		normally only 1; inceremnts if there are more crates in PCIe tree
*
*	name: 		virtual device name to refer to in EPICS record links (INP/OUT)
*
*  	i2cControlWord  all information to select target bus, address, access speed, data width, operation,.. 
*
*
**/

int pevAsynI2cConfigure(
    unsigned int crate,
    const char* name,
    unsigned int i2cControlWord)
{
    
  regDeviceAsyn* device;    
  char* tmpStrCpy;
  struct pev_node *pev;
   
  if( regDevFind(name) ) 
  {
    printf("pevAsynI2cConfigure: ERROR, device \"%s\" on an IFC/PEV already configured as synchronous device\n", 
    		name);
    exit( -1);
  }

  /* call PEV1100 user library initialization function */
  pev = pev_init( crate);
  if( !pev)
  {
    printf("pevAsynI2cConfigure: Cannot allocate data structures to control PEV1100\n");
    exit( -1);
  }
  /* verify if the PEV1100 is accessible */
  if( pev->fd < 0)
  {
    printf("pevAsynI2cConfigure: Cannot find PEV1100 interface\n");
    exit( -1);
  }
  
  if( !ellFirst(&pevRegDevAsynList) )
    ellInit (&pevRegDevAsynList);
  
  device = (regDeviceAsyn*)malloc(sizeof(regDeviceAsyn));
  if (device == NULL)
  {
      errlogSevPrintf(errlogFatal,
  	  "pevAsynI2cConfigure() %s: out of memory\n",
  	  name);
      return errno;
  };
  
  device->pev = pev;
  device->magic = MAGIC;
  tmpStrCpy = malloc(strlen(name)+1);
  strcpy(tmpStrCpy, name);
  device->name = tmpStrCpy;
  device->i2cDevice = i2cControlWord;
  

  if( initHookpevI2cDone == epicsFalse )
  {
    printf("========= we at registering pevI2cHookFunc()\n");
    initHookRegister((initHookFunction)pevI2cHookFunc);
    initHookpevI2cDone = epicsTrue;
  }
 	
  regDevAsyncRegisterDevice(name, &pevI2cAsynSupport, device);
  ellAdd (&pevRegDevAsynList, &device->node);
  return 0;
}


static void pevI2cHookFunc(initHookState state)
{

  switch(state) {
    case initHookAtEnd:
      printf("========= we are at case initHookAtBeginning\n");
      pev_i2cQueue_init(1);
      break;
      
    default:
      break;
  }
  return;
}

/**
*
* i2c multi-user request handling
*
**/

void *pev_i2cRequetServer(int *crate)
{
   pevI2cReqMsg msgptr;           /* -> allocated message space */
   int numByteRecvd = 0;
   unsigned int i2cCtrl = 0;
   int ii=0;
        
   while(1)
   {
     numByteRecvd = epicsMessageQueueReceive(pevI2cMsgQueueId, (pevI2cReqMsg*)&msgptr, sizeof(pevI2cReqMsg));
     if (numByteRecvd <= 0)
       {
         printf("pev_i2cRequetServer(): epicsMessageQueueReceive Failed! msg_size = %d \n",numByteRecvd);
	 continue;
       }

     i2cCtrl = (msgptr.i2cDevice&0xFFF3FFFF) | ((msgptr.i2cDatSiz-1) << 18);
     msgptr.i2cDevice = i2cCtrl;

     if(msgptr.readOperation)
     {
       switch(msgptr.i2cDatSiz)
       {
     	 case 1:     /* 1 Byte */
     	   for (ii=0; ii<msgptr.nelem; ii++)
     	     ((epicsUInt8*)msgptr.pi2cData)[ii] = pev_i2c_read(msgptr.i2cDevice, msgptr.i2cCmd+ii);
     	   break;
     	 
     	 case 2:     /* 2 Byte */
     	   for (ii=0; ii<msgptr.nelem; ii++)
     	     ((epicsUInt16*)msgptr.pi2cData)[ii] = pev_i2c_read(msgptr.i2cDevice, msgptr.i2cCmd+ii*2); 
     	   break;
     	 
     	 case 4:     /* 4 Byte */
     	   for (ii=0; ii<msgptr.nelem; ii++)
     	     ((epicsUInt32*)msgptr.pi2cData)[ii] = pev_i2c_read(msgptr.i2cDevice, msgptr.i2cCmd+ii*4);
     	   break;
       }       
     }
     else
     {
       switch(msgptr.i2cDatSiz)
       {
     	 case 1:     /* 1 Byte */
     	   for (ii=0; ii<msgptr.nelem; ii++)
             pev_i2c_write( msgptr.i2cDevice, msgptr.i2cCmd+ii, ((epicsUInt8*)msgptr.pi2cData)[ii]);
     	   break;
     	 
     	 case 2:     /* 2 Byte */
     	   for (ii=0; ii<msgptr.nelem; ii++)
             pev_i2c_write( msgptr.i2cDevice, msgptr.i2cCmd+ii*2, ((epicsUInt16*)msgptr.pi2cData)[ii]);
     	   break;
     	 
     	 case 4:     /* 4 Byte */
     	   for (ii=0; ii<msgptr.nelem; ii++)
             pev_i2c_write( msgptr.i2cDevice, msgptr.i2cCmd+ii*4, ((epicsUInt32*)msgptr.pi2cData)[ii]);
     	   break;
       }
     }  
	
     callbackRequest((CALLBACK*)msgptr.pCallBack);
   } /* endWhile */
   
   free(&msgptr);
   printf("Thread exiting...\n");
   exit(0);
}


int pev_i2cQueue_init(int crate)
{
epicsThreadId pevDmaThreadId;

 printf("========= we are at pev_i2cQueue_init()\n");
 /* 
  * create a message queue to listen to this pev
  * and check for correct creation??????????
  */
  
 pevI2cMsgQueueId = epicsMessageQueueCreate( DMA_Q_SIZE, sizeof(pevI2cReqMsg));
 if( !pevI2cMsgQueueId )
    {
      printf("ERROR; epicsMessageQueueCreate failed\n");
      return -1;
    }
  else
    {
       printf("===== created epicsMessageQueue with id %p \n",  pevI2cMsgQueueId);
    }

  pevDmaThreadId = epicsThreadCreate("pevDmaReqHandler",epicsThreadPriorityMedium,
                                      epicsThreadGetStackSize(epicsThreadStackMedium),
                                      (EPICSTHREADFUNC)pev_i2cRequetServer,(int*)&crate);

  if( !pevDmaThreadId )
    {
      printf("ERROR; epicsThreadCreate failed. Thread id returned %p\n", pevDmaThreadId);
      return -1;
    }
  else
    {
       printf("===== created epicsThread with id %p for pev_i2cRequetServer()\n",  pevDmaThreadId);
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
static const iocshArg pevAsynI2cConfigureArg0 = { "crate", iocshArgInt };
static const iocshArg pevAsynI2cConfigureArg1 = { "name", iocshArgString };
static const iocshArg pevAsynI2cConfigureArg2 = { "i2cControlWord", iocshArgInt };
static const iocshArg * const pevAsynI2cConfigureArgs[] = {
    &pevAsynI2cConfigureArg0,
    &pevAsynI2cConfigureArg1,
    &pevAsynI2cConfigureArg2,
};

static const iocshFuncDef pevAsynI2cConfigureDef =
    { "pevAsynI2cConfigure", 3, pevAsynI2cConfigureArgs };
    
static void pevAsynI2cConfigureFunc (const iocshArgBuf *args)
{
    int status = pevAsynI2cConfigure(
        args[0].ival, args[1].sval, args[2].ival);
    if (status != 0) epicsExit(1);
}

static void pevI2cRegistrar ()
{
    iocshRegister(&pevAsynI2cConfigureDef, pevAsynI2cConfigureFunc);
}

epicsExportRegistrar(pevI2cRegistrar);

#endif
