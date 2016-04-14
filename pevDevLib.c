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
 *      Modified by Babak Kalantary 2012
 *      and Dirk Zimoch 2013
 *
 */

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/types.h>

#include <devLib.h>
#include <errlog.h>
#include <epicsTypes.h>
#include <symbolname.h>
#include <stdlib.h>
#include <epicsMutex.h>
#include <epicsTimer.h>
#include "pevPrivate.h"
#include <epicsExport.h>

#include "pev.h"

#define VME24_MAP_SIZE 	0x1000000      	/* 16 MiB A24 - fixed */
#define VMECR_MAP_SIZE 	0x1000000	/* 16 MiB CR/CSR - fixed */
#define VME16_MAP_SIZE 	0x10000		/* 64 KiB A16 - fixed */

#define PVME_ADDERR  0x8000041c
#define VME_BERR     0x80000000

#define PVME_MASCSR  0x80000404
#define VME_SUP_MODE 0x00000020

int pevDevLibDebug = 0;
epicsExportAddress(int, pevDevLibDebug);

/*
 * Interrupt handling
 */

int pevDevLibInterruptInUseVME(unsigned int vectorNumber)
{
    return 0;
}

long pevDevLibConnectInterruptVME(
    unsigned int vectorNumber,
    void (*pFunction)(),
    void  *parameter)
{
    if (pevDevLibDebug)
    {
        char *fName = symbolName(pFunction, 0);
        char *pName = symbolName(parameter, 0);
	printf("connecting VME Interrupt 0x%02x func=%s param=%s\n",
            vectorNumber, fName, pName);
        free(pName);
        free(fName);
    }
    
    if (vectorNumber > 0xff)
        return S_dev_badVector;

    /* connect to range VME IRQ-1 ... IRQ-7 */
    pevIntrConnect(0, EVT_SRC_VME_ANY_LEVEL, vectorNumber, pFunction, parameter);
    return S_dev_success;
}

long pevDevLibDisconnectInterruptVME(
    unsigned int vectorNumber,
    void (*pFunction)()
)
{
    if (pevDevLibDebug)
    {
        char *fName = symbolName(pFunction, 0);
	printf("disconnecting VME Interrupt 0x%02x func=%s\n",
            vectorNumber, fName);
        free(fName);
    }
    
    return pevIntrDisconnect(0, EVT_SRC_VME_ANY_LEVEL, vectorNumber, pFunction, NULL);
}

long pevDevLibEnableInterruptLevelVME(unsigned int level)
{
    if (pevDevLibDebug)
	printf("enabling VME Interrupt level 0x%02x\n", level);

    if (level < 1 || level > 7)
    {
        errlogPrintf("pevDevLibEnableInterruptLevelVME: Illegal VME interrupt level %d\n", level);
        return S_dev_badArgument;
    }
    return pevIntrEnable(0, EVT_SRC_VME+level);
}

long pevDevLibDisableInterruptLevelVME(unsigned int level)
{
    if (pevDevLibDebug)
	printf("disabling VME Interrupt level 0x%02x\n", level);

    if (level < 1 || level > 7)
    {
        errlogPrintf("pevDevLibEnableInterruptLevelVME: Illegal VME interrupt level %d\n", level);
        return S_dev_badArgument;
    }
    return pevIntrDisable(0, EVT_SRC_VME+level);
}

/* pevDevMapAddr
 * maps logical address to physical address, but does not detect
 * two device drivers that are using the same address range
 */

long pevDevLibMapAddr(epicsAddressType addrType, unsigned int options,
            size_t logicalAddress, size_t size, volatile void **ppPhysicalAddress)
{
    if (ppPhysicalAddress==NULL) {
        errlogPrintf("pevDevLibMapAddr: invalid argument! ppPhysicalAddress==NULL\n");
        return S_dev_badArgument;
    }

    *ppPhysicalAddress = NULL;

    if (pevDevLibDebug)
        printf("pevDevLibMapAddr(addrType=%d %s, options=0x%x, logicalAddress=0x%zx, size=0x%zx, ppPhysicalAddress=%p)\n",
            addrType, addrType<atLast ? epicsAddressTypeName[addrType] : "unknown", options, logicalAddress, size, ppPhysicalAddress);
    switch (addrType)
    {
        /* what is the use of this? */
        case atISA:
            if (pevDevLibDebug)
                printf("pevDevLibMapAddr: mapping NoConvert size = 0x%x at address 0x%x\n",
                    size, logicalAddress);
            *ppPhysicalAddress = (volatile void *) logicalAddress;
            return S_dev_success;

        /* info: pevMap re-uses existing maps if possible */
        case atVMECSR:
            if (pevDevLibDebug)
	        printf("pevDevLibMapAddr: mapping VME CR/CSR size = 0x%x at address 0x%x\n",
                    size, logicalAddress);
            if (logicalAddress + size > VMECR_MAP_SIZE)
            {
                errlogPrintf("pevDevMapAddr: requested range 0x%x - 0x%x does not fit in VME CR/CSR space\n",
                    logicalAddress, logicalAddress + size);
                return S_dev_badArgument;
            }
            *ppPhysicalAddress = pevMap(0, MAP_MASTER_32,
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_CR|MAP_VME_SP,
                logicalAddress, size);
            break;

        case atVMEA16:
            if (pevDevLibDebug)
	        printf("pevDevLibMapAddr: mapping VME 16 size = 0x%x at address 0x%x\n",
                    size, logicalAddress);
            if (logicalAddress + size > VME16_MAP_SIZE)
            {
                errlogPrintf("pevDevLibMapAddr: requested range 0x%x - 0x%x does not fit in VME A16 space\n",
                    logicalAddress, logicalAddress + size);
                return S_dev_badArgument;
            }
            *ppPhysicalAddress = pevMap(0, MAP_MASTER_32,  
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A16|MAP_VME_SP,
                logicalAddress, size);
            break;

        case atVMEA24:
            if (pevDevLibDebug)
	        printf("pevDevLibMapAddr: mapping VME 24 size = 0x%x at address 0x%x\n",
                    size, logicalAddress);
            if (logicalAddress + size > VME24_MAP_SIZE)
            {
                errlogPrintf("pevDevLibMapAddr: requested range 0x%x - 0x%x does not fit in VME A24 space\n",
                    logicalAddress, logicalAddress + size);
                return S_dev_badArgument;
            }
            *ppPhysicalAddress = pevMap(0, MAP_MASTER_32,
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A24|MAP_VME_SP,
                logicalAddress, size);
            break;

        case atVMEA32:
            if (pevDevLibDebug)
	        printf("pevDevLibMapAddr: mapping VME 32: size = 0x%x at address 0x%x\n",
                    size, logicalAddress);
            *ppPhysicalAddress = pevMap(0, MAP_MASTER_64,
                MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A32|MAP_VME_SP,
                logicalAddress, size);
            break;

        default:
            errlogPrintf("pevDevLibMapAddr: unknown address space 0x%x\n", addrType);
            return S_dev_addrMapFail;
    }
    return *ppPhysicalAddress ? S_dev_success : S_dev_addrMapFail;
}

static epicsMutexId pevDevLibProbeLock;
static jmp_buf pevDevLibProbeFail;

void pevDevLibProbeSigHandler(int sig, siginfo_t* info , void* ctx)
{
    struct pevMapInfo mapInfo;

    pevGetMapInfo(info->si_addr, &mapInfo);
    errlogPrintf("pevDevLibProbeSigHandler: access to unmapped address %p=%s+%#x\n",
        info->si_addr, mapInfo.name, mapInfo.addr);
    longjmp(pevDevLibProbeFail, 1);
}

void pevDevLibProbeTimeout(void* ptr)
{
    struct pevMapInfo mapInfo;

    pevGetMapInfo(ptr, &mapInfo);
    errlogPrintf("pevDevLibProbeTimeout: access to address %p=%s+%#x is hanging\n",
        ptr, mapInfo.name, mapInfo.addr);
}

long pevDevLibProbe(int write, unsigned int wordSize, volatile const void *ptr, void *pValue)
{
    struct sigaction sa = {{0}}, oldsa;
    int status = S_dev_success;
    struct pevMapInfo mapInfo = {0};
    static epicsTimerQueueId pevDevLibProbeTimerQueue;
    static epicsTimerId pevDevLibProbeTimer;
            
    /* serialize concurrent probes because we use global resources (jmp_buf, signals, PVME_ADDERR register) */
    epicsMutexMustLock(pevDevLibProbeLock);
    
    /* clear VME bus error */
    pevx_csr_rd(0, PVME_ADDERR);

    /* setup handler for unmapped addresses */
    sa.sa_sigaction = pevDevLibProbeSigHandler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, &oldsa);
    
    /* get info about mapped  pev resources */
    if (pevDevLibDebug)
        pevGetMapInfo(ptr, &mapInfo);

    /* setup 1 second timout for hanging access (due to TOSCA bug) */
    if (!pevDevLibProbeTimerQueue)
    {
        pevDevLibProbeTimerQueue = epicsTimerQueueAllocate(1, epicsThreadPriorityLow);
        pevDevLibProbeTimer = epicsTimerQueueCreateTimer(pevDevLibProbeTimerQueue, pevDevLibProbeTimeout, (void*)ptr);
        epicsTimerStartDelay(pevDevLibProbeTimer, 1.0);
    }
    
    if (setjmp(pevDevLibProbeFail) == 0)
    {
        if (write)
        {
            switch (wordSize)
            {
                case 1:
                    *(epicsUInt8 *)(ptr) = *(epicsUInt8 *)pValue;
                    break;
                case 2:
                    *(epicsUInt16 *)(ptr) = *(epicsUInt16 *)pValue;
                    break;
                case 4:
                    *(epicsUInt32 *)(ptr) = *(epicsUInt32 *)pValue;
                    break;
                default:
                    if (pevDevLibDebug)
                        printf("pevDevLibWriteProbe: illegal word size %u", wordSize);
                    status = S_dev_badArgument;
            }
            /* wait for writes to get posted */
            usleep(10);
        }
        else
        {
            switch (wordSize)
            {
                case 1:
                    *(epicsUInt8 *)pValue = *(epicsUInt8 *)(ptr);
                    break;
                case 2:
                    *(epicsUInt16 *)pValue = *(epicsUInt16 *)(ptr);
                    break;
                case 4:
                    *(epicsUInt32 *)pValue = *(epicsUInt32 *)(ptr);
                    break;
                default:
                    if (pevDevLibDebug)
                        printf("pevDevLibReadProbe: illegal word size %u", wordSize);
                    status = S_dev_badArgument;
            }
        }
        /* check VME bus error */
        status = pevx_csr_rd(0, PVME_ADDERR) & VME_BERR;

        if (pevDevLibDebug)
            printf("pevDevLib%sProbe(wordSize=%d, ptr=%p=%s+%#x): %#0*x%s\n",
                write ? "Write" : "Read",
                wordSize, ptr,
                mapInfo.name, mapInfo.addr,
                wordSize * 2,
                wordSize == 1 ? *(epicsUInt8*)pValue : wordSize == 2 ? *(epicsUInt16*)pValue : *(epicsUInt32*)pValue,
                status ? " bus error" : "");
        
        if (status) status = S_dev_noDevice;
    }
    else
    {
        errlogPrintf("pevDev%sProbe(wordSize=%d, ptr=%p): address not mapped\n", 
            write ? "Write" : "Read", wordSize, ptr);
        status = S_dev_noDevice;
    }

    epicsTimerCancel(pevDevLibProbeTimer);
    sigaction(SIGSEGV, &oldsa, NULL);
    epicsMutexUnlock(pevDevLibProbeLock);
    return status;
}

/* pevDevReadProbe
 * a bus error safe "wordSize" read at the specified address which returns
 * unsuccessful status if the device isnt present
 */
 
long pevDevLibReadProbe (unsigned int wordSize, volatile const void *ptr, void *pValue)
{
    return pevDevLibProbe(0, wordSize, ptr, pValue);
}

/* pevDevWriteProbe
 * a bus error safe "wordSize" write at the specified address which returns
 * unsuccessful status if the device isnt present
 * KB: does not currently work on ifc!
 *
 */

long pevDevLibWriteProbe (unsigned int wordSize, volatile void *ptr, const void *pValue)
{
    return pevDevLibProbe(1, wordSize, ptr, (void *)pValue);
}

/*devA24Malloc and devA24Free are not implemented*/
void *pevDevLibA24Malloc(size_t size) { return NULL;}
void pevDevLibA24Free(void *pBlock) {};
long pevDevLibInit(void);

/* PEV1100 specific initialization */
long pevDevLibInit(void)
{
    struct pev_ioctl_vme_conf vme_conf;

    if (pevDevLibDebug)
	printf("pevDevLibInit\n");

    if (!pevx_init(0))
    {
        printf("pevDevLibInit: pev_init(0) failed\n");
        return -1;
    }
    pevDevLibProbeLock = epicsMutexMustCreate();
    pevx_vme_conf_read(0, &vme_conf);
    if (vme_conf.mas_ena == 0 )
    {
        printf("VME master -> disabled\n");
        return -1;
    }
    
    /* Set VME master to supervisory mode */
    /* pev_csr_set(PVME_MASCSR, VME_SUP_MODE);*/

    return S_dev_success;
}

/* compatibility with different versions of EPICS 3.14 */
#if defined(pdevLibVirtualOS) && !defined(devLibVirtualOS)
#define devLibVirtualOS devLibVME
#endif

devLibVirtualOS pevVirtualOS = {
    pevDevLibMapAddr,
    pevDevLibReadProbe,
    pevDevLibWriteProbe,
    pevDevLibConnectInterruptVME,
    pevDevLibDisconnectInterruptVME,
    pevDevLibEnableInterruptLevelVME,
    pevDevLibDisableInterruptLevelVME,
    pevDevLibA24Malloc,
    pevDevLibA24Free,
    pevDevLibInit,
    pevDevLibInterruptInUseVME
};

static void pevDevLibRegistrar ()
{
    pdevLibVirtualOS = &pevVirtualOS;
}
   
epicsExportRegistrar(pevDevLibRegistrar);
