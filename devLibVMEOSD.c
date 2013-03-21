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


#define _GNU_SOURCE  /* for strsignal */
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <unistd.h>
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
#include <epicsTypes.h>
#include <epicsAssert.h>

#define VME32_MAP_SIZE 	0x40000000      /* 1024 MB A32 - fixed via PMEM */
#define VME24_MAP_SIZE 	0x1000000      	/* 8 MB A24 - fixed */
#define VMECR_MAP_SIZE 	0x1000000	/* 8 MB CR/CSR - fixed */
#define VME16_MAP_SIZE 	0x10000		/* 64 KB A16 - fixed */
#define ISRC_VME	0x10		/* ITC_SRC */

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

int pevDevLibVMEInit();
LOCAL struct pev_ioctl_evt *pevDevLibIntrEvent;
LOCAL void (*pevIsrFuncTable[256])(void*);
LOCAL void*  pevIsrParmTable[256];
LOCAL void pevDevIntrHandler(int sig);

LOCAL int pevDevLibDebug = 0;
LOCAL epicsBoolean pevDevLibAtexitCalled = epicsFalse;

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
LOCAL long pevDevReadProbe (unsigned wordSize, volatile const void *ptr, void *pValue);

/*
 * a bus error safe "wordSize" write at the specified address which returns 
 * unsuccessful status if the device isnt present
 */
LOCAL long pevDevWriteProbe (unsigned wordSize, volatile void *ptr, const void *pValue);

LOCAL long pevDevConnectInterruptVME (unsigned vectorNumber, void (*pFunction)(), void  *parameter);
LOCAL long pevDevDisconnectInterruptVME ( unsigned vectorNumber, void (*pFunction)());
LOCAL int pevDevInterruptInUseVME (unsigned vectorNumber);
LOCAL long pevDevEnableInterruptLevelVME(unsigned level);
LOCAL long pevDevDisableInterruptLevelVME(unsigned level);


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
    pevDevConnectInterruptVME, pevDevDisconnectInterruptVME,
    pevDevEnableInterruptLevelVME, pevDevDisableInterruptLevelVME,
    devA24Malloc,devA24Free,pevDevInit
};


/* 
*  clean-up  at epics exit
 */
 
void pevDevLibAtexit(void)
{
  struct pev_ioctl_map_ctl pev_ctl; 
  
  if( pevDevLibAtexitCalled )
    return;

  pev_evt_queue_disable(pevDevLibIntrEvent);
  pev_evt_queue_free(pevDevLibIntrEvent);

  printf("pevDevLibAtexit....\n");
  if( vme_mas_map_a32.win_size != 0 )
  {
    pev_ctl.sg_id = vme_mas_map_a32.sg_id;
    pev_map_clear( &pev_ctl );
    pev_munmap(&vme_mas_map_a32);
    pev_map_free(&vme_mas_map_a32);
  }
  if( vme_mas_map_a16.win_size != 0 )
  {
    pev_ctl.sg_id = vme_mas_map_a16.sg_id;
    pev_map_clear( &pev_ctl );
    pev_munmap(&vme_mas_map_a16);
    pev_map_free(&vme_mas_map_a16);
  }
  if( vme_mas_map_a24.win_size != 0 )
  {
    pev_ctl.sg_id = vme_mas_map_a24.sg_id;
    pev_map_clear( &pev_ctl );
    pev_munmap(&vme_mas_map_a24);
    pev_map_free(&vme_mas_map_a24);
  } 
  if( vme_mas_map_csr.win_size != 0 )
  {
    pev_ctl.sg_id = vme_mas_map_csr.sg_id;
    pev_map_clear( &pev_ctl );
    pev_munmap(&vme_mas_map_csr);
    pev_map_free(&vme_mas_map_csr);
  } 
  
  pevDevLibAtexitCalled = epicsTrue;
  
/*
  pev_exit( pev);
  exit(0);
  if( map_a32_base )
    pev_map_clear(map_a32_base);
  if( map_a24_base )
    pev_map_clear(map_a24_base);
  if( map_a16_base )
    pev_map_clear(map_a16_base);
  exit(0);
*/
}

LOCAL void pevDevLibSigHandler(int sig)
{
    /* Signal handler for "terminate" signals:
       SIGHUP, SIGINT, SIGPIPE, SIGALRM, SIGTERM
       as well as "core dump" signals:
       SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGSEGV         
    */
    
    /* try to clean up before exit */
    printf ("\npevDevLibSigHandler %s\n", strsignal(sig));
    epicsExitCallAtExits();
    signal(sig, SIG_DFL);
    raise(sig);
}
 
/* PEV1100 specific initialization */
LOCAL long
pevDevInit(void)
{
    /* assume the vme bridge has been initialized by bsp */
    /* init BSP extensions [memProbe etc.] */

  unsigned int crate = 0, i = 0 ;
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

  /* make sure that even at abnormal termination the clean up is called */
  signal(SIGHUP, pevDevLibSigHandler);
  signal(SIGINT, pevDevLibSigHandler);
  signal(SIGPIPE, pevDevLibSigHandler);
  signal(SIGALRM, pevDevLibSigHandler);
  signal(SIGTERM, pevDevLibSigHandler);
  signal(SIGQUIT, pevDevLibSigHandler);
  signal(SIGILL, pevDevLibSigHandler);
  signal(SIGABRT, pevDevLibSigHandler);
  signal(SIGFPE, pevDevLibSigHandler);
  signal(SIGSEGV, pevDevLibSigHandler);
  
  vme_mas_map_a16.rem_addr = 0x0;
  vme_mas_map_a16.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A16;
  vme_mas_map_a16.flag = 0x0;
  vme_mas_map_a16.sg_id = MAP_MASTER_32;
  vme_mas_map_a16.size = VME16_MAP_SIZE;
  assert (pev_map_alloc( &vme_mas_map_a16) == 0);

  vme_mas_map_a32.rem_addr = 0x0;
  vme_mas_map_a32.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A32;
  vme_mas_map_a32.flag = 0x0;
  vme_mas_map_a32.sg_id = MAP_MASTER_64;
  vme_mas_map_a32.size = VME32_MAP_SIZE;
  assert (pev_map_alloc( &vme_mas_map_a32) == 0);
  
  vme_mas_map_a24.rem_addr = 0x0;		
  vme_mas_map_a24.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A24;
  vme_mas_map_a24.sg_id = MAP_MASTER_32;
  vme_mas_map_a24.size = VME24_MAP_SIZE;
  assert (pev_map_alloc( &vme_mas_map_a24) == 0);
  
  vme_mas_map_csr.rem_addr = 0x0;		
  vme_mas_map_csr.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_CR;
  vme_mas_map_csr.sg_id = MAP_MASTER_32;
  vme_mas_map_csr.size = VMECR_MAP_SIZE;
  assert (pev_map_alloc( &vme_mas_map_csr) == 0);
  
  if( !vme_mas_map_a16.win_size || !vme_mas_map_a24.win_size 
      || !vme_mas_map_a32.win_size || !vme_mas_map_csr.win_size )
  {
    pevDevLibAtexit();
    return S_dev_addrMapFail;
  }
  
  map_a16_base = pev_mmap(&vme_mas_map_a16);
  assert (map_a16_base);
  map_a24_base = pev_mmap(&vme_mas_map_a24);
  assert (map_a24_base);
  map_a32_base = pev_mmap(&vme_mas_map_a32);
  assert (map_a32_base);
  map_csr_base = pev_mmap(&vme_mas_map_csr);
  assert (map_csr_base);
  
  /* intr setup start */
  for(i=0; i<256; i++)
  {
    pevIsrFuncTable[i] = 0;
    pevIsrParmTable[i] = 0;
  }
  pevDevLibIntrEvent = pev_evt_queue_alloc( SIGUSR1);  
  pevDevLibIntrEvent->wait = -1;
  signal(pevDevLibIntrEvent->sig, pevDevIntrHandler);
  /* intr setup end */
  
  epicsAtExit((void*)pevDevLibAtexit, NULL);
  return 0; /*bspExtInit();*/
}



/*
 * devConnectInterruptVME
 *
 * wrapper to minimize driver dependency on OS
 */


LOCAL void pevDevIntrHandler(int sig)
{ 
  do
  {
    pev_evt_read( pevDevLibIntrEvent, 0);
    if(pevDevLibIntrEvent->src_id & ISRC_VME)	
    {
      if(pevDevLibDebug)
        printf("pevDevIntrHandler(): src_id = 0x%x vec_id = 0x%x evt_count = %d \n", pevDevLibIntrEvent->src_id, pevDevLibIntrEvent->vec_id, pevDevLibIntrEvent->evt_cnt);
      if( *pevIsrFuncTable[pevDevLibIntrEvent->vec_id] )
          (*pevIsrFuncTable[pevDevLibIntrEvent->vec_id])(pevIsrParmTable[pevDevLibIntrEvent->vec_id]);
      pev_evt_clear(pevDevLibIntrEvent,  pevDevLibIntrEvent->src_id);
      pev_evt_unmask( pevDevLibIntrEvent, pevDevLibIntrEvent->src_id);
    }
  } while(pevDevLibIntrEvent->evt_cnt);
  return;
}



long pevDevConnectInterruptVME (
    unsigned vectorNumber,
    void (*pFunction)(),
    void  *parameter)
{

    if ( vectorNumber <=0 || vectorNumber > 256) {
        return S_dev_badVector; 
    }
    if (pevDevInterruptInUseVME(vectorNumber)) {
        return S_dev_vectorInUse; 
    }
    
    pevIsrFuncTable[vectorNumber] = pFunction;
    pevIsrParmTable[vectorNumber] = parameter;

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
long pevDevDisconnectInterruptVME (
    unsigned vectorNumber,
    void (*pFunction)() 
)
{
    if( pFunction ==  pevIsrFuncTable[vectorNumber] ) 
      pevIsrFuncTable[vectorNumber] = 0;
    else 
      return S_dev_badVector;
    
    return 0;
}

/*
 * enable VME interrupt level
 */
long pevDevEnableInterruptLevelVME (unsigned level)
{
    if(level <=0 || level > 7)
      return S_dev_intEnFail; 
    
    pev_evt_register( pevDevLibIntrEvent, ISRC_VME + level);
    pev_evt_queue_enable(pevDevLibIntrEvent);
    return 0; 
}

/*
 * disable VME interrupt level
 */
long pevDevDisableInterruptLevelVME (unsigned level)
{
    if(level <=0 || level > 7)
      return S_dev_intEnFail; 
    
    pev_evt_unregister( pevDevLibIntrEvent, ISRC_VME + level);
    return 0; 
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
            if( pevDevLibDebug )
	      printf("registering atVMEA16: size = %x at address %x, (available window %x at %lx)\n",size, logicalAddress,vme_mas_map_a16.win_size, vme_mas_map_a16.rem_base);
	    if((logicalAddress + size)<=(vme_mas_map_a16.rem_base+vme_mas_map_a16.win_size)) {
              *ppPhysicalAddress = map_a16_base + logicalAddress;
              break;
            } else return S_dev_addrMapFail;
          case atVMEA24:
            if( pevDevLibDebug )
	      printf("registering atVMEA24: size = %x at address %x, (available window %x at %lx)\n",size, logicalAddress,vme_mas_map_a24.win_size, vme_mas_map_a24.rem_base);
	    if((logicalAddress + size)<=(vme_mas_map_a24.rem_base+vme_mas_map_a24.win_size)) {
              *ppPhysicalAddress = map_a24_base + logicalAddress;
              break;
            } else return S_dev_addrMapFail;
          case atVMEA32:
            if( pevDevLibDebug )
	      printf("registering atVMEA32: size = %x at address %x, (available window %x at %lx)\n",size, logicalAddress,vme_mas_map_a32.win_size, vme_mas_map_a32.rem_base);
	    if((logicalAddress + size)<=(vme_mas_map_a32.rem_base+vme_mas_map_a32.win_size)) {
              *ppPhysicalAddress = map_a32_base + logicalAddress;
              break;
            } else return S_dev_addrMapFail;
          case atVMECSR:
            if( pevDevLibDebug )
	      printf("registering atVMECSR: size = %x at address %x, (available window %x at %lx)\n",size, logicalAddress,vme_mas_map_csr.win_size, vme_mas_map_csr.rem_base);
	    if((logicalAddress + size)<=(vme_mas_map_csr.rem_base+vme_mas_map_csr.win_size)) {
              *ppPhysicalAddress = map_csr_base + logicalAddress;
              break;
            } else return S_dev_addrMapFail;
          default:
            return S_dev_addrMapFail;
        }

    }

    return S_dev_success;
}

/*
 * a bus error safe "wordSize" read at the specified address which returns 
 * unsuccessful status if the device isnt present
 */
 
jmp_buf pevDevReadProbeFail;

void pevDevReadProbeSigHandler(int sig)
{
    longjmp(pevDevReadProbeFail,1);
}

long pevDevReadProbe (unsigned wordSize, volatile const void *ptr, void *pValue)
{
    volatile uint sts; 
    struct sigaction sa, oldsa;

    /* clear BERR */
    sts = pev_csr_rd(0x41c | 0x80000000);

    /* check address */
    sa.sa_handler = pevDevReadProbeSigHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_restorer = NULL;
    sigaction(SIGSEGV, &sa, &oldsa);
    if (setjmp(pevDevReadProbeFail) == 0)
    switch(wordSize)
    {
      case 1: *(char *)pValue = *(char *)(ptr);
        break;
      case 2: *(unsigned short *)pValue = *(unsigned short *)(ptr);
        break;
      case 4: *(unsigned long *)pValue = *(unsigned long *)(ptr);
        break;
      default:
        sigaction(SIGSEGV, &oldsa, NULL);
	return S_dev_badArgument;
    }
    else
    {
        printf ("pevDevReadProbe: address %p not mapped\n", pValue);
        sigaction(SIGSEGV, &oldsa, NULL);
        return S_dev_noDevice;
    }
    sigaction(SIGSEGV, &oldsa, NULL);
    
    /* check BERR */
    sts = pev_csr_rd( 0x41c | 0x80000000);
    if( sts & 0x80000000)
      return S_dev_noDevice;

    return S_dev_success;
}

/*
 * a bus error safe "wordSize" write at the specified address which returns 
 * unsuccessful status if the device isnt present
 * KB: does not currently work on ifc!
 *
 */

jmp_buf pevDevWriteProbeFail;

void pevDevWriteProbeSigHandler(int sig)
{
    longjmp(pevDevWriteProbeFail,1);
}

long pevDevWriteProbe (unsigned wordSize, volatile void *ptr, const void *pValue)
{
    volatile uint sts; 
    struct sigaction sa, oldsa;

    /* clear BERR */
    sts = pev_csr_rd(0x41c | 0x80000000);

    /* check address */
    sa.sa_handler = pevDevWriteProbeSigHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_restorer = NULL;
    sigaction(SIGSEGV, &sa, &oldsa);
    if (setjmp(pevDevWriteProbeFail) == 0)
    switch(wordSize)
    {
      case 1: *(char *)(ptr) = *(char *)pValue;
        break;
      case 2: *(unsigned short *)(ptr) = *(unsigned short *)pValue;
        break;
      case 4: *(unsigned long *)(ptr) = *(unsigned long *)pValue;
        break;
      default:
        sigaction(SIGSEGV, &oldsa, NULL);
	return S_dev_badArgument;
    }
    else
    {
        printf ("pevDevWriteProbe: address %p not mapped\n", pValue);
        sigaction(SIGSEGV, &oldsa, NULL);
        return S_dev_noDevice;
    }
    /* wait for writes to get posted */
    usleep(10);
    
    sigaction(SIGSEGV, &oldsa, NULL);
    
    /* check BERR */
    sts = pev_csr_rd( 0x41c | 0x80000000);
    if( sts & 0x80000000)
      return S_dev_noDevice;

    return S_dev_success;
}


/*
 * determine if a VME interrupt vector is in use
 */
LOCAL int pevDevInterruptInUseVME (unsigned vectorNumber)
{
    if( pevIsrFuncTable[vectorNumber] == NULL )
      return FALSE;
   
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
