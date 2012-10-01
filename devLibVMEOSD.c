/*************************************************************************\
* Copyright (c) 2008 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* devLibOSD.c,v 1.1.2.7 2008/09/08 21:32:04 norume Exp */

/*      Linux/PEV1100 port by Timo Korhonen, <timo.korhonen@psi.ch>
 *            9/2010
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pevioctl.h>
#include <pevulib.h>

#include <devLib.h>
#include <errlog.h>
#include <epicsInterrupt.h>
#include <epicsExit.h>
#include <vmedefs.h>
#include <drvSup.h>            
#include <epicsExport.h>

#define VME32_MAP_SIZE 	0x40000000      /* 1024 MB A32 - fixed via PMEM */
#define VME24_MAP_SIZE 	0x1000000      	/* 8 MB A24 - fixed */
#define VMECR_MAP_SIZE 	0x1000000	/* 8 MB CR/CSR - fixed */
#define VME16_MAP_SIZE 	0x10000		/* 64 KB A16 - fixed */

#if defined(pdevLibVirtualOS) && !defined(devLibVirtualOS)
#define devLibVirtualOS devLibVME
#endif

typedef void    myISR (void *pParam);

static struct pev_node *pev;
static struct pev_ioctl_vme_conf vme_conf;
static struct pev_ioctl_map_pg vme_mas_map_a32;
static struct pev_ioctl_map_pg vme_mas_map_a24;
static struct pev_ioctl_map_pg vme_mas_map_a16;
static struct pev_ioctl_map_pg vme_mas_map_csr;

static void * map_a16_base;
static void * map_a24_base;
static void * map_a32_base;
static void * map_csr_base;

LOCAL myISR *isrFetch(unsigned vectorNumber, void **parg);
int pevDevLibVMEInit();

/*
 * this routine needs to be in the symbol table
 * for this code to work correctly
 */
void unsolicitedHandlerEPICS(int vectorNumber);

LOCAL myISR *defaultHandlerAddr[]={
    (myISR*)unsolicitedHandlerEPICS,
};

/*
 * Make sure that the CR/CSR addressing mode is defined.
 * (it may not be in some BSPs).
 */
#ifndef VME_AM_CSR
#  define VME_AM_CSR (0x2f)
#endif

/*
 * we use a translation between an EPICS encoding
 * and a vxWorks encoding here
 * to reduce dependency of drivers on vxWorks
 *
 * we assume that the BSP are configured to use these
 * address modes by default
 */
#define EPICSAddrTypeNoConvert -1
int EPICStovxWorksAddrType[] 
                = {
                VME_AM_SUP_SHORT_IO,
                VME_AM_STD_SUP_DATA,
                VME_AM_EXT_SUP_DATA,
                EPICSAddrTypeNoConvert,
                VME_AM_CSR
            };

/*
 * maps logical address to physical address, but does not detect
 * two device drivers that are using the same address range
 */
LOCAL long pevDevMapAddr (epicsAddressType addrType, unsigned options,
        size_t logicalAddress, size_t size, volatile void **ppPhysicalAddress);

/*
 * a bus error safe "wordSize" read at the specified address which returns 
 * unsuccessful status if the device isnt present
 */
long pevDevReadProbe (unsigned wordSize, volatile const void *ptr, void *pValue);

/*
 * a bus error safe "wordSize" write at the specified address which returns 
 * unsuccessful status if the device isnt present
 */
long pevDevWriteProbe (unsigned wordSize, volatile void *ptr, const void *pValue);

/* RTEMS specific init */

/*devA24Malloc and devA24Free are not implemented*/
LOCAL void *devA24Malloc(size_t size) { return 0;}
LOCAL void devA24Free(void *pBlock) {};
LOCAL long pevDevInit(void);

/*
 * used by bind in devLib.c
 */
devLibVirtualOS pevVirtualOS = {
    pevDevMapAddr, pevDevReadProbe, pevDevWriteProbe, 
    devConnectInterruptVME, devDisconnectInterruptVME,
    devEnableInterruptLevelVME, devDisableInterruptLevelVME,
    devA24Malloc,devA24Free,pevDevInit
};


/* 
*  clean-up  at epics exit
 */
static void pevDevLibAtexit(void)
{
  printf(">>>> pevDevLibAtexit exiting....\n");
  if( vme_mas_map_a32.win_size != 0 )
    pev_map_free(&vme_mas_map_a32);
  if( vme_mas_map_a16.win_size != 0 )
    pev_map_free(&vme_mas_map_a16);
  if( vme_mas_map_a24.win_size != 0 )
    pev_map_free(&vme_mas_map_a24);
  if( vme_mas_map_csr.win_size != 0 )
    pev_map_free(&vme_mas_map_csr);
  
/*
  if( map_a32_base )
    pev_map_clear(map_a32_base);
  if( map_a24_base )
    pev_map_clear(map_a24_base);
  if( map_a16_base )
    pev_map_clear(map_a16_base);
*/
  exit(0);
}

/* PEV1100 specific initialization */
LOCAL long
pevDevInit(void)
{
    /* assume the vme bridge has been initialized by bsp */
    /* init BSP extensions [memProbe etc.] */

  unsigned int crate = 0;
  pev = pev_init(crate);
  if( !pev)
  {
    printf("Cannot allocate data structures to control PEV1100\n");
    return( -1);
  }
  if( pev->fd < 0)
  {
    printf("Cannot find PEV1100 interface\n");
    return( -1);
  }

  /* ---> Enter your code here... */
  pev_vme_conf_read( &vme_conf);
  if( vme_conf.mas_ena == 0 )
  {
    printf("VME master -> disabled\n");
    return( -1);
  }


  vme_mas_map_a16.rem_addr = 0x0;
  vme_mas_map_a16.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A16;
  vme_mas_map_a16.flag = 0x0;
  vme_mas_map_a16.sg_id = MAP_MASTER_32;
  vme_mas_map_a16.size = VME16_MAP_SIZE;
  pev_map_alloc( &vme_mas_map_a16);

  vme_mas_map_a32.rem_addr = 0x0;
  vme_mas_map_a32.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A32;
  vme_mas_map_a32.flag = 0x0;
  vme_mas_map_a32.sg_id = MAP_MASTER_64;
  vme_mas_map_a32.size = VME32_MAP_SIZE;
  pev_map_alloc( &vme_mas_map_a32);
  
  vme_mas_map_a24.rem_addr = 0x0;		
  vme_mas_map_a24.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A24;
  vme_mas_map_a24.sg_id = MAP_MASTER_32;
  vme_mas_map_a24.size = VME24_MAP_SIZE;
  pev_map_alloc( &vme_mas_map_a24);
  
  vme_mas_map_csr.rem_addr = 0x0;		
  vme_mas_map_csr.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_CR;
  vme_mas_map_csr.sg_id = MAP_MASTER_32;
  vme_mas_map_csr.size = VMECR_MAP_SIZE;
  pev_map_alloc( &vme_mas_map_csr);
  
  if( !vme_mas_map_a16.win_size || !vme_mas_map_a24.win_size 
      || !vme_mas_map_a32.win_size || !vme_mas_map_csr.win_size )
  {
    pevDevLibAtexit();
    return S_dev_addrMapFail;
  }
  
  map_a16_base = pev_mmap(&vme_mas_map_a16);
  map_a24_base = pev_mmap(&vme_mas_map_a24);
  map_a32_base = pev_mmap(&vme_mas_map_a32);
  map_csr_base = pev_mmap(&vme_mas_map_csr);

  epicsAtExit((void*)pevDevLibAtexit, map_a32_base);  
  return 0; /*bspExtInit();*/
}

/*
 * devConnectInterruptVME
 *
 * wrapper to minimize driver dependency on OS
 */
long devConnectInterruptVME (
    unsigned vectorNumber,
    void (*pFunction)(),
    void  *parameter)
{
  /*  int status; */


    if (devInterruptInUseVME(vectorNumber)) {
        return S_dev_vectorInUse; 
    }
/*    status = pev_irq_register( pevStruct,
            vectorNumber,
            pFunction,
            parameter);       
    if (status) {
        return S_dev_vecInstlFail;
    }
*/
    return 0;
}

/*
 *
 *  devDisconnectInterruptVME()
 *
 *  wrapper to minimize driver dependency on OS
 *
 *  The parameter pFunction should be set to the C function pointer that 
 *  was connected. It is used as a key to prevent a driver from removing 
 *  an interrupt handler that was installed by another driver
 *
 */
long devDisconnectInterruptVME (
    unsigned vectorNumber,
    void (*pFunction)() 
)
{
    void (*psub)();
    void  *arg;
  /*  int status; */

    /*
     * If pFunction not connected to this vector
     * then they are probably disconnecting from the wrong vector
     */
    psub = isrFetch(vectorNumber, &arg);
    if(psub != pFunction){
        return S_dev_vectorNotInUse;
    }

/*
    status = BSP_removeVME_isr(
        vectorNumber,
        psub,
        arg) ||
     BSP_installVME_isr(
        vectorNumber,
        (BSP_VME_ISR_t)unsolicitedHandlerEPICS,
        (void*)vectorNumber);        
    if(status){
        return S_dev_vecInstlFail;
    }
*/
    return 0;
}

/*
 * enable VME interrupt level
 */
long devEnableInterruptLevelVME (unsigned level)
{
    return 0; /*BSP_enableVME_int_lvl(level);*/
}

/*
 * disable VME interrupt level
 */
long devDisableInterruptLevelVME (unsigned level)
{
    return 0; /*BSP_disableVME_int_lvl(level);*/
}

/*
 * pevDevMapAddr ()
 */
LOCAL long pevDevMapAddr (epicsAddressType addrType, unsigned options,
            size_t logicalAddress, size_t size, volatile void **ppPhysicalAddress)
{
    /* long status; */

    if (ppPhysicalAddress==NULL) {
        return S_dev_badArgument;
    }

    if (EPICStovxWorksAddrType[addrType] == EPICSAddrTypeNoConvert)
    {
        *ppPhysicalAddress = (void *) logicalAddress;
    }
    else
    {
	switch (addrType)
        {
          case atVMEA16:
	    if((logicalAddress + size)<=(vme_mas_map_a16.rem_base+vme_mas_map_a16.win_size)) {
              *ppPhysicalAddress = map_a16_base + logicalAddress;
              break;
            } else return S_dev_addrMapFail;
          case atVMEA24:
            printf("registering: size = %x at address %x, (available window %x at %lx)\n",size, logicalAddress,vme_mas_map_a24.win_size, vme_mas_map_a24.rem_base);
	    if((logicalAddress + size)<=(vme_mas_map_a24.rem_base+vme_mas_map_a24.win_size)) {
              *ppPhysicalAddress = map_a24_base + logicalAddress;
              break;
            } else return S_dev_addrMapFail;
          case atVMEA32:
            printf("registering: size = %x at address %x, (available window %x at %lx)\n",size, logicalAddress,vme_mas_map_a32.win_size, vme_mas_map_a32.rem_base);
	    if((logicalAddress + size)<=(vme_mas_map_a32.rem_base+vme_mas_map_a32.win_size)) {
              *ppPhysicalAddress = map_a32_base + logicalAddress;
              break;
            } else return S_dev_addrMapFail;
          case atVMECSR:
            printf("registering: size = %x at address %x, (available window %x at %lx)\n",size, logicalAddress,vme_mas_map_csr.win_size, vme_mas_map_csr.rem_base);
	    if((logicalAddress + size)<=(vme_mas_map_csr.rem_base+vme_mas_map_csr.win_size)) {
              *ppPhysicalAddress = map_csr_base + logicalAddress;
              break;
            } else return S_dev_addrMapFail;
          default:
            return S_dev_addrMapFail;
        }

    }

    return 0;
}

/*
 * a bus error safe "wordSize" read at the specified address which returns 
 * unsuccessful status if the device isnt present
 */

long pevDevReadProbe (unsigned wordSize, volatile const void *ptr, void *pValue)
{
 /*   long status; */
 
/*    status = bspExtMemProbe ((void*)ptr, 0, wordSize, pValue);
    if (status!=RTEMS_SUCCESSFUL) {
        return S_dev_noDevice;
    }
*/
    return 0;
}

/*
 * a bus error safe "wordSize" write at the specified address which returns 
 * unsuccessful status if the device isnt present
 */
long pevDevWriteProbe (unsigned wordSize, volatile void *ptr, const void *pValue)
{
 /*
   long status;
     status = bspExtMemProbe ((void*)ptr, 1, wordSize, (void*)pValue);
    if (status!=RTEMS_SUCCESSFUL) {
        return S_dev_noDevice;
    }
*/
    return 0;
}

/*
 *      isrFetch()
 */
LOCAL myISR *isrFetch(unsigned vectorNumber, void **parg)
{
    /*
     * fetch the handler or C stub attached at this vector
     */
    return 0; /*(myISR *) BSP_getVME_isr(vectorNumber,parg);*/
}

/*
 * determine if a VME interrupt vector is in use
 */
int devInterruptInUseVME (unsigned vectorNumber)
{
 /*   int i; */
    myISR *psub;
    void *arg;

    psub = isrFetch (vectorNumber,&arg);

    if (!psub)
        return FALSE;

    /*
     * its a C routine. Does it match a default handler?
     */
/*
    for (i=0; i<NELEMENTS(defaultHandlerAddr); i++) {
        if (defaultHandlerAddr[i] == psub) {
            return FALSE;
        }
    }
*/
    return TRUE;
}


/*
 *  unsolicitedHandlerEPICS()
 *      what gets called if they disconnect from an
 *  interrupt and an interrupt arrives on the
 *  disconnected vector
 *
 *  NOTE: RTEMS may pass additional arguments - hope
 *        this doesn't disturb this handler...
 *
 *  A cleaner way would be having a OS dependent
 *  macro to declare handler prototypes...
 *
 */
void unsolicitedHandlerEPICS(int vectorNumber)
{
    /*
     * call epicInterruptContextMessage()
     * and not errMessage()
     * so we are certain that printf()
     * does not get called at interrupt level
     *
     * NOTE: current RTEMS implementation only
     *       allows a static string to be passed
     */
    epicsInterruptContextMessage(
        "Interrupt to EPICS disconnected vector"
        );
}

/*
 * Some vxWorks convenience routines
 */
void
bcopyLongs(char *source, char *destination, int nlongs)
{
    const long *s = (long *)source;
    long *d = (long *)destination;
    while(nlongs--)
        *d++ = *s++;
}

/*

int pevDevLibVMEInit ()
{
	printf(">>>>>>>>>>>>>>>>> in pevDevLibVMEInit()\n");
	devLibVirtualOS *pdevLibVirtualOS = &pevVirtualOS;
	return 0;
}

drvet drvPevDevLib = {
    2, 
    NULL,     
    (DRVSUPFUN)pevDevLibVMEInit               
};

epicsExportAddress(drvet, drvPevDevLib);

int pevDevLibVMEInit ()
{
	printf(">>>>>>>>>>>>>>>>> in pevDevLibVMEInit()\n");
	devLibVirtualOS *pdevLibVirtualOS = &pevVirtualOS;
	return 0;
}

static int pevDevLibVMEInitLocal = pevDevLibVMEInit();
 */
int vmeMapShow(){
  printf("VME master windows:\n");
  printf("\tVME A16 base address = 0x%08lx [size 0x%x]\n", vme_mas_map_a16.rem_base, vme_mas_map_a16.win_size);
  printf("\tVME A24 base address = 0x%08lx [size 0x%x]\n", vme_mas_map_a24.rem_base, vme_mas_map_a24.win_size);
  printf("\tVME A32 base address = 0x%08lx [size 0x%x]\n", vme_mas_map_a32.rem_base, vme_mas_map_a32.win_size);
  printf("\tVME CSR base address = 0x%08lx [size 0x%x]\n", vme_mas_map_csr.rem_base, vme_mas_map_csr.win_size);
    
  printf("\tVME A16 mapped at user %p\n", map_a16_base);
  printf("\tVME A24 mapped at user %p\n", map_a24_base);
  printf("\tVME A32 mapped at user %p\n", map_a32_base);
  printf("\tVME CSR mapped at user %p\n", map_csr_base);
  return 0;
}
#include <iocsh.h>
#include <epicsExit.h>

static const iocshArg * const vmeMapShowArgs[] = {};

static const iocshFuncDef vmeMapShowDef =
    { "vmeMapShow", 0, vmeMapShowArgs };

static void vmeMapShowFunc (const iocshArgBuf *args)
{
    int status = vmeMapShow();
    if (status != 0) epicsExit(1);
}

static void vmeMapShowRegistrar ()
{
    iocshRegister(&vmeMapShowDef, vmeMapShowFunc);
}

epicsExportRegistrar(vmeMapShowRegistrar);
