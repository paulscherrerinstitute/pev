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
#define I2CEXEC_OK	0x0200000
#define I2CEXEC_MASK	0x0300000


/*
static char cvsid_pev1100[] __attribute__((unused)) =
    "$Id: i2cDrv.c,v 1.8 2012/09/05 09:57:32 kalantari Exp $";
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
    epicsBoolean cmndOperation;			/* is it a command operation or not */
    int* opStat;				/* status of the operation */
} pevI2cReqMsg;


struct regDeviceAsyn {
    ELLNODE  node;                   /* Linked list node structure */
    unsigned long magic;
    const char* name;
    struct pev_node* pev;
    unsigned int i2cDevice; 
    epicsBoolean command; 
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
    int prio,
    int* rdStatus)
{
    pevI2cReqMsg pevI2cRequest;
    unsigned int i2cData;
    int status = 0;

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
    pevI2cRequest.cmndOperation = device->command;
    pevI2cRequest.opStat = rdStatus;
    
    if( !epicsMessageQueueSend(pevI2cMsgQueueId, (void*)&pevI2cRequest, sizeof(pevI2cReqMsg)) )
       return (1);  /* to tell regDev that this is first phase of record processing (to let recSupport set PACT to true) */
    else 
      {
    	 printf("pevI2cAsynRead(): sending I2C request failed! do normal & synchronous transfer\n");
      } 
	
    /* do synchronous i2c read here */
    status = pev_i2c_read( device->i2cDevice, offset, &i2cData);	    
    *(unsigned int*)pdata = i2cData;
    
    if( (status & I2CEXEC_MASK) != I2CEXEC_OK )
      return -1;
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
    pevI2cRequest.i2cDatSiz = dlen;
    pevI2cRequest.nelem = nelem; 
    pevI2cRequest.i2cCmd = offset;		/* reg address within an i2c device */
    pevI2cRequest.readOperation = epicsFalse;
    pevI2cRequest.cmndOperation = device->command;
        
    if( !epicsMessageQueueSend(pevI2cMsgQueueId, (void*)&pevI2cRequest, sizeof(pevI2cReqMsg)) )
       return (1);   /* to tell regDev that this is first phase of record processing (to let recSupport set PACT to true) */
    else 
      {
    	 printf("pevI2cAsynWrite(): sending I2C request failed! do normal & synchronous transfer\n");
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
*	pevAsynI2cConfigure(crate, name, i2cControlWord, command)  
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
    unsigned int i2cControlWord,
    unsigned int command)
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
  
  if(command == 1) device->command = epicsTrue;
  else 
  if(command == 0) device->command = epicsFalse;
  else  
  {
    printf("pevAsynI2cConfigure: illegal command value (must be 0 or 1)\n");
    free(device);
    exit( -1);
  }

  if( initHookpevI2cDone == epicsFalse )
  {
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
    case initHookAfterFinishDevSup:
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
   int status = 0;
   unsigned int readVal;
        
   while(1)
   {
     numByteRecvd = epicsMessageQueueReceive(pevI2cMsgQueueId, (pevI2cReqMsg*)&msgptr, sizeof(pevI2cReqMsg));
     if (numByteRecvd <= 0)
       {
         printf("pev_i2cRequetServer(): epicsMessageQueueReceive Failed! msg_size = %d \n",numByteRecvd);
	 continue;
       }

     /* we skip this for the moment until ioxox fixes the hang problem */
     if( msgptr.cmndOperation == epicsFalse )
     {
       i2cCtrl = (msgptr.i2cDevice&0xFFF3FFFF) | ((msgptr.i2cDatSiz-1) << 18);
       msgptr.i2cDevice = i2cCtrl;
     }
   

     if(msgptr.readOperation)
     {
       switch(msgptr.i2cDatSiz)
       {
     	 case 1:     /* 1 Byte */
     	   for (ii=0; ii<msgptr.nelem; ii++) 
	     {
     	       status = pev_i2c_read(msgptr.i2cDevice, msgptr.i2cCmd+ii, &readVal);
	       ((epicsUInt8*)msgptr.pi2cData)[ii] = (epicsUInt8)readVal;
	     }
     	   break;
     	 
     	 case 2:     /* 2 Byte */
     	   for (ii=0; ii<msgptr.nelem; ii++)
	     {
     	       status = pev_i2c_read(msgptr.i2cDevice, msgptr.i2cCmd+ii*2, &readVal);
	       ((epicsUInt16*)msgptr.pi2cData)[ii] = (epicsUInt16)readVal;
	     }
     	   break;
     	 
     	 case 4:     /* 4 Byte */
     	   for (ii=0; ii<msgptr.nelem; ii++)
	     {
     	       status = pev_i2c_read(msgptr.i2cDevice, msgptr.i2cCmd+ii*4, &readVal);
	       ((epicsUInt32*)msgptr.pi2cData)[ii] = (epicsUInt32)readVal;
	     }
     	   break;
       }       
       if( (status & I2CEXEC_MASK) != I2CEXEC_OK )
	 *(int*)msgptr.opStat = -1;
       else 
         *(int*)msgptr.opStat = 0; 
     }
     else
     {
       if( msgptr.cmndOperation == epicsTrue )
         pev_i2c_cmd( msgptr.i2cDevice, msgptr.i2cCmd);
       else
       switch(msgptr.i2cDatSiz)
       {
     	 case 1:     /* 1 Byte */
     	   for (ii=0; ii<msgptr.nelem; ii++)
             status = pev_i2c_write( msgptr.i2cDevice, msgptr.i2cCmd+ii, ((epicsUInt8*)msgptr.pi2cData)[ii]);
     	   break;
     	 
     	 case 2:     /* 2 Byte */
     	   for (ii=0; ii<msgptr.nelem; ii++)
             status = pev_i2c_write( msgptr.i2cDevice, msgptr.i2cCmd+ii*2, ((epicsUInt16*)msgptr.pi2cData)[ii]);
     	   break;
     	 
     	 case 4:     /* 4 Byte */
     	   for (ii=0; ii<msgptr.nelem; ii++)
             status = pev_i2c_write( msgptr.i2cDevice, msgptr.i2cCmd+ii*4, ((epicsUInt32*)msgptr.pi2cData)[ii]);
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

  pevDmaThreadId = epicsThreadCreate("pevI2cReqServer",epicsThreadPriorityMedium,
                                      epicsThreadGetStackSize(epicsThreadStackMedium),
                                      (EPICSTHREADFUNC)pev_i2cRequetServer,(int*)&crate);

  if( !pevDmaThreadId )
    {
      printf("ERROR; epicsThreadCreate failed. Thread id returned %p\n", pevDmaThreadId);
      return -1;
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
static const iocshArg pevAsynI2cConfigureArg3 = { "command", iocshArgInt };
static const iocshArg * const pevAsynI2cConfigureArgs[] = {
    &pevAsynI2cConfigureArg0,
    &pevAsynI2cConfigureArg1,
    &pevAsynI2cConfigureArg2,
    &pevAsynI2cConfigureArg3,
};

static const iocshFuncDef pevAsynI2cConfigureDef =
    { "pevAsynI2cConfigure", 4, pevAsynI2cConfigureArgs };
    
static void pevAsynI2cConfigureFunc (const iocshArgBuf *args)
{
    int status = pevAsynI2cConfigure(
        args[0].ival, args[1].sval, args[2].ival, args[3].ival);
    if (status != 0) epicsExit(1);
}

static void pevI2cRegistrar ()
{
    iocshRegister(&pevAsynI2cConfigureDef, pevAsynI2cConfigureFunc);
}

epicsExportRegistrar(pevI2cRegistrar);

#endif
