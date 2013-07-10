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
#include <epicsTypes.h>
#include <epicsAssert.h>
#include <epicsThread.h>
#include <epicsExport.h>

#define VME32_MAP_SIZE 	0x40000000      /* 1024 MB A32 - max via PMEM */
#define VME24_MAP_SIZE 	0x1000000      	/* 16 MB A24 - fixed */
#define VMECR_MAP_SIZE 	0x1000000	/* 16 MB CR/CSR - fixed */
#define VME16_MAP_SIZE 	0x10000		/* 64 KB A16 - fixed */
#define MAP_PAGE_SIZE   0x100000	/* 1 MB - fixed */
#define ISRC_VME	0x10		/* ITC_SRC */

static const char* vme_addr_name(int mode)
{
    switch (mode & (MAP_VME_CR|MAP_VME_A16|MAP_VME_A24|MAP_VME_A32))
    {
        case MAP_VME_CR:  return "CR/CSR";
        case MAP_VME_A16: return "A16";
        case MAP_VME_A24: return "A24";
        case MAP_VME_A32: return "A32";
        default:          return "unknown";
    }
}

#if defined(pdevLibVirtualOS) && !defined(devLibVirtualOS)
#define devLibVirtualOS devLibVME
#endif

static struct pev_node *pev;

struct pev_map_element {
    struct pev_map_element* next;
    struct pev_ioctl_map_pg vme_mas_map;
};

static struct pev_map_element* pev_map_list = NULL;

int pevDevLibVMEInit();
LOCAL struct pev_ioctl_evt *pevDevLibIntrEvent;
LOCAL void (*pevIsrFuncTable[256])(void*);
LOCAL void*  pevIsrParmTable[256];
LOCAL void pevDevIntrThread(void*);

int pevDevLibDebug = 0;
epicsExportAddress(int, pevDevLibDebug);

LOCAL epicsBoolean pevDevLibAtexitCalled = epicsFalse;

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

/*
 * Interrupt handling
 */

LOCAL long pevDevConnectInterruptVME (unsigned vectorNumber, void (*pFunction)(), void  *parameter);
LOCAL long pevDevDisconnectInterruptVME (unsigned vectorNumber, void (*pFunction)());
LOCAL int pevDevInterruptInUseVME (unsigned vectorNumber);
LOCAL long pevDevEnableInterruptLevelVME(unsigned level);
LOCAL long pevDevDisableInterruptLevelVME(unsigned level);
LOCAL void pevUnsolicitedIntrHandler(void* vectorNumber);

/* pev specific init */

/*devA24Malloc and devA24Free are not implemented*/
LOCAL void *devA24Malloc(size_t size) { return NULL;}
LOCAL void devA24Free(void *pBlock) {};
LOCAL long pevDevInit(void);

/*
 * used by bind in devLib.c
 */
devLibVirtualOS pevVirtualOS = {
    pevDevMapAddr,
    pevDevReadProbe,
    pevDevWriteProbe,
    pevDevConnectInterruptVME,
    pevDevDisconnectInterruptVME,
    pevDevEnableInterruptLevelVME,
    pevDevDisableInterruptLevelVME,
    devA24Malloc,
    devA24Free,
    pevDevInit,
    pevDevInterruptInUseVME
};

/*
*  clean-up  at epics exit
 */

void pevDevLibAtexit(void)
{
    struct pev_map_element* map;

    if (pevDevLibAtexitCalled) return;

    pevDevLibAtexitCalled = epicsTrue;

    printf("pevDevLibAtexit....\n");

    /* stop interrupts */
    pev_evt_queue_disable(pevDevLibIntrEvent);
    pev_evt_queue_free(pevDevLibIntrEvent);

    for (map = pev_map_list; map; map = map->next)
    {
        printf("cleaning up %s base=0x%08lx size=0x%x\n",
            vme_addr_name(map->vme_mas_map.mode),
            map->vme_mas_map.rem_base,map->vme_mas_map.win_size);
        if (map->vme_mas_map.usr_addr != MAP_FAILED && map->vme_mas_map.usr_addr != NULL)
            pev_munmap(&map->vme_mas_map);
        if (map->vme_mas_map.win_size != 0)
            pev_map_free(&map->vme_mas_map);
    }
    printf("pevDevLibAtexit done\n");
}

LOCAL void pevDevLibSigHandler(int sig, siginfo_t* info , void* ctx)
{
    /* Signal handler for "terminate" signals:
       SIGHUP, SIGINT, SIGPIPE, SIGALRM, SIGTERM
       as well as "core dump" signals:
       SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGSEGV
    */
    
    /* try to clean up before exit */
    printf ("\npevDevLibSigHandler %s\n", strsignal(sig));

    if (sig == SIGSEGV)
    {
        printf ("SEGV @ %p\n", info->si_addr);
    }

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
    struct pev_ioctl_vme_conf vme_conf;
    unsigned int crate = 0, i = 0 ;
    struct sigaction sa;

    if (pev != NULL) return S_dev_success; /* we have been called already */
    printf("pev_init(%d)\n", crate);
    pev = pev_init(crate);
    if (pev == NULL)
    {
        printf("Cannot allocate data structures to control PEV1100\n");
        return -1;
    }
    if (pev->fd < 0)
    {
        printf("Cannot find PEV1100 interface\n");
        return -1;
    }

    printf("pev_vme_conf_read\n");
    pev_vme_conf_read(&vme_conf);
    if (vme_conf.mas_ena == 0 )
    {
        printf("VME master -> disabled\n");
        return -1;
    }
    
    /* Set VME master to supervisory mode */
    pev_csr_set(0x404, 1<<5);
    
    /* make sure that even at abnormal termination the clean up is called */
    sa.sa_sigaction = pevDevLibSigHandler;
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGHUP,  &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGILL,  &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE,  &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);

    printf("pevDevInit interrupt setup\n");
    /* intr setup start */
    for (i=0; i<256; i++)
    {
        pevIsrFuncTable[i] = pevUnsolicitedIntrHandler;
        pevIsrParmTable[i] = (void*)i;
    }

    epicsThreadCreate("pevDevIntr", epicsThreadPriorityMax,
        epicsThreadGetStackSize(epicsThreadStackSmall),
        pevDevIntrThread, (void*)SIGUSR1);
    /* intr setup end */

    printf("epicsAtExit setup\n");
    epicsAtExit((void*)pevDevLibAtexit, NULL);
    return S_dev_success; /*bspExtInit();*/
}


LOCAL void pevDevIntrThread(void* signumber)
{
    sigset_t sigset;
    int signal = (int) signumber;

    sigemptyset(&sigset);
    sigaddset(&sigset, signal);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    printf("pev_evt_queue_alloc(%s)\n", strsignal(signal));
    pevDevLibIntrEvent = pev_evt_queue_alloc(signal);
    pevDevLibIntrEvent->wait = -1;

    while (sigwait(&sigset, &signal) == 0)
    {
        do
        {
            pev_evt_read(pevDevLibIntrEvent, 0);
            if (pevDevLibIntrEvent->src_id & ISRC_VME)	
            {
                if (pevDevLibDebug)
                    printf("pevDevIntrHandler(): src_id = 0x%x vec_id = 0x%x evt_count = %d \n",
                        pevDevLibIntrEvent->src_id, pevDevLibIntrEvent->vec_id, pevDevLibIntrEvent->evt_cnt);
                if (*pevIsrFuncTable[pevDevLibIntrEvent->vec_id] )
                    (*pevIsrFuncTable[pevDevLibIntrEvent->vec_id])(pevIsrParmTable[pevDevLibIntrEvent->vec_id]);
                pev_evt_clear(pevDevLibIntrEvent,  pevDevLibIntrEvent->src_id);
                pev_evt_unmask(pevDevLibIntrEvent, pevDevLibIntrEvent->src_id);
            }
        } while (pevDevLibIntrEvent->evt_cnt);
    }
    printf("pevDevIntrHandler(): sigwait failed! Cannot do interrupts\n");
}

/*
 * devConnectInterruptVME
 *
 * wrapper to minimize driver dependency on OS
 */

long pevDevConnectInterruptVME(
    unsigned vectorNumber,
    void (*pFunction)(),
    void  *parameter)
{
    if (vectorNumber <=0 || vectorNumber > 256)
        return S_dev_badVector;
    if (pevDevInterruptInUseVME(vectorNumber))
        return S_dev_vectorInUse;

    pevIsrParmTable[vectorNumber] = parameter;
    pevIsrFuncTable[vectorNumber] = pFunction;

    return S_dev_success;
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
long pevDevDisconnectInterruptVME(
    unsigned vectorNumber,
    void (*pFunction)()
)
{
    if (vectorNumber <=0 || vectorNumber > 256)
        return S_dev_badVector;
    if (pFunction != pevIsrFuncTable[vectorNumber])
        return S_dev_badVector;

    pevIsrFuncTable[vectorNumber] = pevUnsolicitedIntrHandler;
    pevIsrParmTable[vectorNumber] = (void*)vectorNumber;

    return S_dev_success;
}

/*
 * enable VME interrupt level
 */
long pevDevEnableInterruptLevelVME(unsigned level)
{
    if (level <=0 || level > 7)
        return S_dev_intEnFail;

    pev_evt_register(pevDevLibIntrEvent, ISRC_VME + level);
    pev_evt_queue_enable(pevDevLibIntrEvent);
    return S_dev_success;
}

/*
 * disable VME interrupt level
 */
long pevDevDisableInterruptLevelVME (unsigned level)
{
    if (level <=0 || level > 7)
      return S_dev_intEnFail;

    pev_evt_unregister(pevDevLibIntrEvent, ISRC_VME + level);
    return S_dev_success;
}

/*
 * determine if a VME interrupt vector is in use
 */
LOCAL int pevDevInterruptInUseVME (unsigned vectorNumber)
{
    if (pevIsrFuncTable[vectorNumber] == pevUnsolicitedIntrHandler )
      return FALSE;

    return TRUE;
}


/*
 *  pevUnsolicitedIntrHandler()
 *      what gets called if they disconnect from an
 *  interrupt and an interrupt arrives on the
 *  disconnected vector
 *
 */
void pevUnsolicitedIntrHandler(void* vectorNumber)
{
    errlogPrintf("VME interrupt to disconnected vector %d",
        (int)vectorNumber);
}

/*
 * pevDevMapAddr ()
 */
LOCAL long pevDevMapAddr (epicsAddressType addrType, unsigned options,
            size_t logicalAddress, size_t size, volatile void **ppPhysicalAddress)
{
    struct pev_ioctl_map_pg vme_mas_map;
    struct pev_map_element* map;

    if (ppPhysicalAddress==NULL) {
        fprintf (stderr, "pevDevMapAddr: invalid argument! ppPhysicalAddress==NULL\n");
        return S_dev_badArgument;
    }

    *ppPhysicalAddress = NULL;
    memset (&vme_mas_map, 0, sizeof(vme_mas_map));

    switch (addrType)
    {

        case atISA:
            if (pevDevLibDebug)
                printf("registering NoConvert: size = %x at address %x\n",
                    size, logicalAddress);
            *ppPhysicalAddress = (volatile void *) logicalAddress;
            return S_dev_success;

        /* for A16, A24 and CR/CSR map complete address space first time it is used */
        case atVMEA16:
            if (pevDevLibDebug)
	        printf("registering VME 16: size = %x at address %x\n",
                    size, logicalAddress);
            vme_mas_map.size = VME16_MAP_SIZE;
            vme_mas_map.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A16|MAP_VME_SP;
            vme_mas_map.sg_id = MAP_MASTER_32;
            break;

        case atVMEA24:
            if (pevDevLibDebug)
	        printf("registering VME 24: size = %x at address %x\n",
                    size, logicalAddress);
            vme_mas_map.size = VME24_MAP_SIZE;
            vme_mas_map.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A24|MAP_VME_SP;
            vme_mas_map.sg_id = MAP_MASTER_32;
            break;

        case atVMECSR:
            if (pevDevLibDebug)
	        printf("registering VME CR/CSR: size = %x at address %x\n",
                    size, logicalAddress);
            vme_mas_map.size = VMECR_MAP_SIZE;
            vme_mas_map.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_CR|MAP_VME_SP;
            vme_mas_map.sg_id = MAP_MASTER_32;
            break;

        /* for A32 map only requested 1MB pages */
        case atVMEA32:
            if (pevDevLibDebug)
	        printf("registering VME 32: size = %x at address %x\n",
                    size, logicalAddress);
            /* round down start address to page boundary */
            vme_mas_map.rem_addr = logicalAddress & ~(MAP_PAGE_SIZE - 1);
            size += logicalAddress - vme_mas_map.rem_addr;
            /* round up end address to page boundary */
            vme_mas_map.size = ((size - 1) & ~(MAP_PAGE_SIZE - 1)) + MAP_PAGE_SIZE;
            vme_mas_map.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A32|MAP_VME_SP;
            vme_mas_map.sg_id = MAP_MASTER_64;
            break;

        default:
            fprintf (stderr, "pevDevMapAddr: unknown address space %x\n", addrType);
            return S_dev_addrMapFail;
    }

    /* Note: the following list handling happens usually in single threaded context.
       Thus it is not protected with a mutex
    */
    for (map = pev_map_list; map; map = map->next)
    {
        if (map->vme_mas_map.mode != vme_mas_map.mode) continue;
        if (map->vme_mas_map.sg_id != vme_mas_map.sg_id) continue;

        if (map->vme_mas_map.rem_base <= logicalAddress &&
            map->vme_mas_map.rem_base + map->vme_mas_map.win_size >= logicalAddress + size)
        {
            *ppPhysicalAddress = logicalAddress - map->vme_mas_map.rem_base + (char*)map->vme_mas_map.usr_addr;
            return S_dev_success;
        }
    }

    map = (struct pev_map_element*) calloc(1, sizeof(struct pev_map_element));
    if (map == NULL)
    {
        printf("pevDevMapAddr: out of memory for new VME map structure\n");
        return S_dev_noMemory;
    }
    /* insert at list head for inverse removal at exit */
    map->next = pev_map_list;
    pev_map_list = map;
    memcpy (&map->vme_mas_map, &vme_mas_map, sizeof(vme_mas_map));

    pev_map_alloc(&map->vme_mas_map);
    if (map->vme_mas_map.win_size < vme_mas_map.size)
    {
        printf ("allocating VME space failed\n");
        return S_dev_addrMapFail;
    }
    if (pev_mmap(&map->vme_mas_map) == MAP_FAILED)
    {
        printf ("mapping VME space to user space failed\n");
        return S_dev_addrMapFail;
    }
    *ppPhysicalAddress = (logicalAddress - map->vme_mas_map.rem_addr + (char*)map->vme_mas_map.usr_addr);
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
    sts = pev_csr_rd(0x41c | 0x80000000);
    if (sts & 0x80000000)
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
    sts = pev_csr_rd(0x41c | 0x80000000);
    if (sts & 0x80000000)
      return S_dev_noDevice;

    return S_dev_success;
}

/*
 * Some VME convenience routines
 */

int vmeMapShow(){
    struct pev_map_element* map;
    
    printf("VME master windows:\n");
    for (map = pev_map_list; map; map = map->next)
    {
        printf("  VME %s:\tbase=0x%08lx size=0x%x mapped to %p\n",
            vme_addr_name(map->vme_mas_map.mode), map->vme_mas_map.rem_base,
            map->vme_mas_map.win_size, map->vme_mas_map.usr_addr);
    }
    return S_dev_success;
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
    pevDevInit();
    iocshRegister(&vmeMapShowDef, vmeMapShowFunc);
}

epicsExportRegistrar(vmeMapShowRegistrar);
