#include <errlog.h>
#include <devLib.h>
#include <regDev.h>
#include <epicsTypes.h>
#include <epicsExport.h>

#include "pev.h"

static int pevCsrRegDevDebug = 0;
epicsExportAddress(int, pevCsrRegDevDebug);

#define pevCsrRegDevReport NULL
#define pevCsrRegDevGetInScanPvt NULL
#define pevCsrRegDevGetOutScanPvt NULL

int pevCsrRegDevRead (
        regDevice *device,
        size_t offset,
        unsigned int dlen,
        size_t nelem,
        void* pdata,
        int priority,
        regDevTransferComplete callback,
        char* user)
{
    int n;

    /* access is always 32 bit */
    if (dlen != 4)
    {
        errlogSevPrintf(errlogMajor,
            "pevCsrRegDevRead %s: data size must be 32 bit\n", user);
        return S_dev_badRequest;
    }
    if (offset & 3)
    {
        errlogSevPrintf(errlogMajor,
            "pevCsrRegDevRead %s: offset must be a multiple of 4\n", user);
        return S_dev_badRequest;
    }
    
    offset |= 0x80000000;
    
    for (n = 0; n < nelem; n++)
    {
        ((epicsUInt32*)pdata)[n] = (epicsUInt32) pevx_csr_rd(0, offset);
        offset += 4;
    }
    return SUCCESS;
}

int pevCsrRegDevWrite (
        regDevice *device,
        size_t offset,
        unsigned int dlen,
        size_t nelem,
        void* pdata,
        void* pmask,
        int priority,
        regDevTransferComplete callback,
        char* user)
{
    /* access is always 32 bit */
    if (dlen != 4)
    {
        errlogSevPrintf(errlogMajor,
            "pevCsrRegDevWrite %s: data size must be 32 bit\n", user);
        return S_dev_badRequest;
    }
    if (offset & 3)
    {
        errlogSevPrintf(errlogMajor,
            "pevCsrRegDevWrite %s: offset must be a multiple of 4\n", user);
        return S_dev_badRequest;
    }
    
    offset |= 0x80000000;  /* use bar 3 (CSR map) instead of bar 4 (I/O ports) */
    
    epicsUInt32* data32 = (epicsUInt32*)pdata;
    if (pmask)
    {
        int n;
        epicsUInt32 m = *(epicsUInt32*)pmask;

        for (n = 0; n < nelem; n++)
        {
            /* modify only masked bits and leave unmasked bits untouched */
            pevx_csr_wr(0, offset, (data32[n] & m) | (pevx_csr_rd(0, offset) & ~m));
            offset += 4;
        }
    }
    else
    {
        int n;

        for (n = 0; n < nelem; n++)
        {
            pevx_csr_wr(0, offset, data32[n]);
            offset += 4;
        }
    }
    return SUCCESS;
}

static regDevSupport pevCsrRegDevSupport = {
    pevCsrRegDevReport,
    pevCsrRegDevGetInScanPvt,
    pevCsrRegDevGetOutScanPvt,
    pevCsrRegDevRead,
    pevCsrRegDevWrite,
};

static void pevCsrRegDevRegistrar ()
{
    pevx_init(0);
    if (pevx_board_name(0)[0])
        regDevRegisterDevice("pev_csr", &pevCsrRegDevSupport,
            NULL,  /* no need to pass any driver private data: card number is always 0 */
            8*1024 /* is there any way to ask the PEV API for the size? 8k is the size of PCI bar 3 */
        );
}

epicsExportRegistrar(pevCsrRegDevRegistrar);
