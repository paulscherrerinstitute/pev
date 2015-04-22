#define _GNU_SOURCE  /* for strsignal */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <ucontext.h>
#include <execinfo.h>

#include <pevulib.h>

#include <errlog.h>
#include <devLib.h>
#include <epicsThread.h>
#include <epicsExit.h>
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

int pevInit()
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
    
    status = pevMapInit();
    if (status != S_dev_success) return status;
    
    status = pevIntrInit();
    if (status != S_dev_success) return status;
    
    status = pevDmaInit();
    if (status != S_dev_success) return status;

    return S_dev_success;
}

static void pevRegistrar ()
{
    pevInit();
}
epicsExportRegistrar(pevRegistrar);
