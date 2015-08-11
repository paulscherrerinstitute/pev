#define _GNU_SOURCE  /* for strsignal */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <ucontext.h>
#include <execinfo.h>
#include <endian.h>

#include <pevxulib.h>

#include <errlog.h>
#include <devLib.h>
#include <drvSup.h>
#include <epicsThread.h>
#include <epicsExit.h>
#include <iocsh.h>
#include <epicsExport.h>

#include "pev.h"
#include "pevPrivate.h"

int pevSigDebug = 0;
epicsExportAddress(int, pevSigDebug);

LOCAL struct pevMapInfoEntry {
    struct pevMapInfoEntry* next;
    int (*getInfo) (const void* address, struct pevMapInfo* info);
} *pevMapInfoList = NULL;


int pevInstallMapInfo(int (*getInfo)(const void* address, struct pevMapInfo* info))
{
    struct pevMapInfoEntry* infoEntry;
    
    infoEntry = malloc(sizeof (struct pevMapInfoEntry));
    if (!infoEntry) return S_dev_noMemory;
    infoEntry->next = NULL;
    infoEntry->getInfo = getInfo;
    pevMapInfoList = infoEntry;
    return 0;
}
    
int pevGetMapInfo(const void* address, struct pevMapInfo* info)
{
    struct pevMapInfoEntry* infoEntry;

    for (infoEntry = pevMapInfoList; infoEntry; infoEntry = infoEntry->next)
    {
        if (infoEntry->getInfo(address, info)) return 1;
    }
    return 0;
}
    
LOCAL void pevSigHandler(int sig, siginfo_t* info , void* ctx)
{
    /* Signal handler for "terminate" signals:
       SIGHUP, SIGINT, SIGPIPE, SIGALRM, SIGTERM
       as well as "core dump" signals:
       SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGSEGV
    */
    
    if (pevSigDebug)
    {
        void *array[50];
        size_t size;

        errlogPrintf("\npevSigHandler: %s", strsignal(sig));

        switch (info->si_code)
        {
            case SI_USER:
                errlogPrintf(" sent by kill() or sigsend() or raise()");
                break;
            case SI_KERNEL:
                errlogPrintf(" sent by kernel");
                break;
            case SI_QUEUE:
                errlogPrintf(" sent by sigqueue()");
                break;
            case SI_TIMER:
                errlogPrintf(" sent by timer");
                break;
            case SI_MESGQ:
                errlogPrintf(" sent by message queue");
                break;
            case SI_ASYNCIO:
                errlogPrintf(" sent by async I/O");
                break;
            case SI_SIGIO:
                errlogPrintf(" queued SIGIO");
                break;
            case SI_TKILL:
                errlogPrintf(" sent by tkill() or tgkill()");
                break;
        }
        switch (sig)
        {
            case SIGILL:
                switch (info->si_code)
                {
                    case ILL_ILLOPC:
                        errlogPrintf(", illegal opcode");
                        break;
                    case ILL_ILLOPN:
                        errlogPrintf(", illegal operand");
                         break;
                    case ILL_ILLADR:
                        errlogPrintf(", illegal addressing mode");
                        break;
                    case ILL_ILLTRP:
                        errlogPrintf(", illegal trap");
                        break;  
                    case ILL_PRVOPC:
                        errlogPrintf(", priviledged opcode");
                        break;
                    case ILL_PRVREG:
                        errlogPrintf(", priviledged register");
                        break;
                    case ILL_COPROC:
                        errlogPrintf(", coprocessor error");
                        break;
                    case ILL_BADSTK:
                        errlogPrintf(", internal stack error");
                        break;
                }
                break;
            case SIGFPE:
                switch (info->si_code)
                {
                    case FPE_INTDIV:
                        errlogPrintf(", integer divide by zero");
                        break;  
                    case FPE_INTOVF:
                        errlogPrintf(", integer overflow");
                        break;  
                    case FPE_FLTDIV:
                        errlogPrintf(", floating point divide by zero");
                        break;  
                    case FPE_FLTOVF:
                        errlogPrintf(", floating point overflow");
                        break;  
                    case FPE_FLTUND:
                        errlogPrintf(", floating point underflow");
                        break;  
                    case FPE_FLTINV:
                        errlogPrintf(", floating point invalid operation");
                        break;  
                    case FPE_FLTSUB:
                        errlogPrintf(", subscript out of range");
                        break;
                }
                break;
            case SIGSEGV:
                switch (info->si_code)
                {
                    case SEGV_MAPERR:
                        errlogPrintf(", address %p not mapped", info->si_addr);
                        break;  
                    case SEGV_ACCERR:
                        errlogPrintf(", invalid permissions for mapped object at address %p", info->si_addr);
                        break;
                }
                break;
            case SIGBUS:
                switch (info->si_code)
                {  
                    case BUS_ADRALN:
                        errlogPrintf(", invalid alignment at address %p", info->si_addr);
                        break;  
                    case BUS_ADRERR:
                        errlogPrintf(", non-existent physical address %p", info->si_addr);
                        break;  
                    case BUS_OBJERR:
                        errlogPrintf(", object specific hardware error at address %p", info->si_addr);
                        break;
                }
                break;
        }
        errlogPrintf ("\n");
        if (sig != SIGINT || pevSigDebug > 1)
        {
            errlogPrintf ("Stack trace:\n--------------------\n");

#ifdef __i386__
#define FAULT_INSTRUCTION_ADDRESS eip
#define WASTEINDEX 1
#endif

#ifdef __arm__
#define FAULT_INSTRUCTION_ADDRESS arm_pc
#define WASTEINDEX 1
#endif

#ifdef __powerpc__
#define FAULT_INSTRUCTION_ADDRESS regs->link /* probably wrong */
#define WASTEINDEX 2
#endif

            size = backtrace(array, 50);
            array[WASTEINDEX] = (void *) ((ucontext_t *)ctx)->uc_mcontext.FAULT_INSTRUCTION_ADDRESS;
            backtrace_symbols_fd(array+WASTEINDEX, size-WASTEINDEX, STDERR_FILENO);
            errlogPrintf ("--------------------\n\n");
        }
    }

    switch (sig)
    {
        case SIGBUS:
        case SIGSEGV:
        {
            struct pevMapInfo mapInfo;
            
            if (pevGetMapInfo(info->si_addr, &mapInfo))
            {
                if (pevSigDebug)
                {
                    errlogPrintf("Access to already unmapped %s on card %d base=%#x size=%#x\n",
                        mapInfo.name, mapInfo.card, mapInfo.start, mapInfo.size);
                    errlogPrintf("Thread %s stopped\n", epicsThreadGetNameSelf());
                }
                epicsThreadSuspendSelf();
                /* does not return */
            }
        }
    }
        
    /* try to clean up before exit */
    epicsExitCallAtExits();
    signal(sig, SIG_DFL);
    raise(sig);
}

/**
*
* EPICS shell debug utility
*
**/

void pevVersionShow(int level)
{
    unsigned int card;
    const char *board_name;
    volatile unsigned int* usr1_data;
    unsigned int appdata[16];
    int i;

    pevx_init(0);
    printf("  pev library version  : %s\n", pevx_get_lib_version());
    printf("  kernel driver version: %s %s\n", pevx_get_driver_version(),  pevx_id());
    if (level < 1) return;
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        pevx_init(card);
        board_name = pevx_board_name(card);
        if (board_name == NULL) continue;
        printf("  card %-2d              : %s\n", card, board_name);
        if (level < 2) continue;
        usr1_data = pevMap(card, MAP_PCIE_MEM, MAP_SPACE_USR1 | MAP_ENABLE, 0, 0x100);
        if (usr1_data == NULL)  continue;
        /* convert application data from little endian */
        for (i=0; i < 20; i++) appdata[i] = le32toh(usr1_data[i]);
        printf("    firmware ID: %#x (%.32s) version %#x built %x.%x.%x\n    expected FMC1: %.16s\n    expected FMC2: %.16s\n",
            appdata[0]&0xffff, (char*)(appdata+2), (appdata[0]>>16)&0xffff, 
            appdata[1]&0xff, (appdata[1]>>8)&0xff, (appdata[1]>>16)&0xffff,
            (char*)(appdata+10), (char*)(appdata+14));
    }
}

static const iocshArg pevVersionShowArg0 = { "level", iocshArgInt };

static const iocshArg * const pevVersionShowArgs[] = {
    &pevVersionShowArg0
};

static const iocshFuncDef pevVersionShowDef =
    { "pevVersionShow", 1, pevVersionShowArgs };
    
static void pevVersionShowFunc (const iocshArgBuf *args)
{
    pevVersionShow(args[0].ival);
}

void pevExpertReport(int level)
{ 
    printf(" pevExpertReport:\n");
 
    printf(" == Version ==\n");
    pevVersionShow(level);
    
    printf(" == Maps ==\n");
    pevMapShow(0);

    printf(" == DMA ==\n");
    pevDmaShow(level);

    printf(" == Interrupts ==\n");
    pevIntrShow(level);
    
    printf(" == VME ==\n");
    pevVmeShow();
}

static const iocshArg pevExpertReportArg0 = { "level", iocshArgInt };

static const iocshArg * const pevExpertReportArgs[] = {
    &pevExpertReportArg0
};

static const iocshFuncDef pevExpertReportDef =
    { "pevExpertReport", 1, pevExpertReportArgs };
    
static void pevExpertReportFunc (const iocshArgBuf *args)
{
    pevExpertReport(args[0].ival);
}

int pevInit(void)
{
    int status;
    struct sigaction sa;

    /* make sure that even at abnormal termination the clean up is called */
    sa.sa_sigaction = pevSigHandler;
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
    
    status = pevIntrInit();
    if (status != S_dev_success) return status;
    
    status = pevDmaInit();
    if (status != S_dev_success) return status;

    status = pevMapInit();
    if (status != S_dev_success) return status;
    
    iocshRegister(&pevVersionShowDef, pevVersionShowFunc);
    iocshRegister(&pevExpertReportDef, pevExpertReportFunc);

    return S_dev_success;
}

static void pevRegistrar (void)
{
    pevInit();
}
   
epicsExportRegistrar(pevRegistrar);

/* for dbior */

long pev_dbior(int level)
{
    if (level == 0) pevVersionShow(0);
    else pevExpertReport(level);
    return 0;
}

struct {
    long number;
    long (*report) (int level);
    long (*init) ();
} pev = {
    2,
    pev_dbior,
    NULL,
};
epicsExportAddress(drvet, pev);
