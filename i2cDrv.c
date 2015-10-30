#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <devLib.h>
#include <regDev.h>

#include <epicsTypes.h>
#include <epicsExit.h>
#include <epicsMutex.h>
#include <epicsRingPointer.h>
#include <ellLib.h>           
#include <errlog.h>
#include <dbAccess.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <symbolname.h>
#include <epicsExport.h>

#include "pev.h"

#define MAGIC                 1100                 /*  pev1100 */
#define DMA_Q_SIZE        2000
#define MAX_PEVS        21                /* taken from VME 21 slot. makes sense ??? */
#define ELB_I2C_CTL        0x30                /* ELB_I2C_CTL address on ELB bus (4-bytes) */
#define I2CEXEC_OK        0x0200000
#define I2CEXEC_MASK        0x0300000


/*
static char cvsid_pev1100[] __attribute__((unused)) =
    "$Id: i2cDrv.c,v 1.26 2015/05/19 14:12:03 zimoch Exp $";
*/

struct regDevice {
    unsigned long magic;
    const char* name;
    struct pev_node* pev;
    unsigned int i2cDevice; 
    epicsBoolean command; 
};

int pevI2cDebug = 0;

/******** Support functions *****************************/ 

int pevI2cRead(
    regDevice *device,
    unsigned int offset,
    unsigned int dlen,
    unsigned int nelem,
    void* pdata,
    int prio,
    regDevTransferComplete callback,
    char* user)
{
    unsigned int i;
    unsigned int val;
    int status;

    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevI2cRead %s: illegal device handle\n", user);
        return S_dev_noDevice;
    }
    if (pevI2cDebug & 2)
    {
        char* cbName = symbolName(callback, 0);
        char* dataName = symbolName(pdata, 0);
        printf("pevI2cRead(device=%s, offset=%d, dlen=%d, nelem=%d, pdata=@%s, prio=%d, callback=%s, user=%s)\n",
            device->name, offset, dlen, nelem, dataName, prio, cbName, user);
        free(dataName);
        free(cbName);
    }

    switch (dlen)
    {
        case 1:
            for (i = 0; i < nelem; i++) 
            {
                status = pev_i2c_read(device->i2cDevice, offset, &val);
                if ((status & I2CEXEC_MASK) != I2CEXEC_OK)
                    break;
                ((epicsUInt8*)pdata)[i] = val;
                offset += 1;
            }
            return 0;

        case 2:
            for (i = 0; i < nelem; i++)
            {
                status = pev_i2c_read(device->i2cDevice, offset, &val);
                if ((status & I2CEXEC_MASK) != I2CEXEC_OK)
                    break;
                ((epicsUInt16*)pdata)[i] = val;
                offset += 2;
            }
            return 0;

        case 4:
            for (i = 0; i < nelem; i++)
            {
                status = pev_i2c_read(device->i2cDevice, offset, &val);
                if ((status & I2CEXEC_MASK) != I2CEXEC_OK)
                    break;
                ((epicsUInt32*)pdata)[i] = val;
                offset += 4;
            }
            return 0;
        default:
            errlogPrintf("%s i2cRead(device->i2cDevice=%d, offset=%d, dlen=%d, nelem=%d, pdata=@%p): illegal data size\n",
                user, device->i2cDevice, offset, dlen, nelem, pdata);
            return -1;
    }
    /* clear data on error */
    errlogPrintf("%s i2cRead(device->i2cDevice=%d, offset=%d, dlen=%d, nelem=%d, pdata=@%p): pev_i2c_read() faild on offset %d status=%#x\n",
        user, device->i2cDevice, offset, dlen, nelem, pdata, offset, status);
    memset(pdata, 0, nelem * dlen);
    return status;    
}

int pevI2cWrite(
    regDevice *device,
    unsigned int offset,
    unsigned int dlen,
    unsigned int nelem,
    void* pdata,
    void* pmask,
    int prio,
    regDevTransferComplete callback,
    char* user)
{
    unsigned int val;
    unsigned int i;
    int status;
           
    if (!device || device->magic != MAGIC)
    {
        errlogSevPrintf(errlogMajor,
            "pevI2cWrite %s: illegal device handle\n", user);
        return -1;
    }

    if (pevI2cDebug & 2)
    {
        char* cbName = symbolName(callback, 0);
        char* dataName = symbolName(pdata, 0);
        printf("pevI2cWrite(device=%s, offset=%d, dlen=%d, nelem=%d, pdata=@%s, pmask=@%p, prio=%d, callback=%s, user=%s)\n",
            device->name, offset, dlen, nelem, dataName, pmask, prio, cbName, user);
        free(dataName);
        free(cbName);
    }

    switch (dlen)
    {
        case 1:
            if (!pmask)
                for (i = 0; i < nelem; i++)
                {
                    status = pev_i2c_write(device->i2cDevice, offset, ((epicsUInt8*)pdata)[i]);
                    if ((status & I2CEXEC_MASK) != I2CEXEC_OK)
                        break;
                    offset += 1;
                }
            else
                for (i = 0; i < nelem; i++)
                {
                    status = pev_i2c_read(device->i2cDevice, offset, &val);
                    if ((status & I2CEXEC_MASK) != I2CEXEC_OK)
                        goto readfail;
                    val &= ~*(epicsUInt8*)pmask;
                    val |= ((epicsUInt8*)pdata)[i] & *(epicsUInt8*)pmask;
                    status = pev_i2c_write(device->i2cDevice, offset, (epicsUInt8)val);
                    if ((status & I2CEXEC_MASK) != I2CEXEC_OK)
                        break;
                    offset += 1;
                }
            return 0;

        case 2:
            if (!pmask)
                for (i = 0; i < nelem; i++)
                {
                    status = pev_i2c_write(device->i2cDevice, offset, ((epicsUInt16*)pdata)[i]);
                    if ((status & I2CEXEC_MASK) != I2CEXEC_OK)
                        break;
                    offset += 2;
                }
            else
                for (i = 0; i < nelem; i++)
                {
                    status = pev_i2c_read(device->i2cDevice, offset, &val);
                    if ((status & I2CEXEC_MASK) != I2CEXEC_OK)
                        goto readfail;
                    val &= ~*(epicsUInt16*)pmask;
                    val |= ((epicsUInt16*)pdata)[i] & *(epicsUInt16*)pmask;
                    status = pev_i2c_write(device->i2cDevice, offset, (epicsUInt16)val);
                    if ((status & I2CEXEC_MASK) != I2CEXEC_OK)
                        break;
                    offset += 2;
                }
            return 0;
        case 4:
            if (!pmask)
                for (i = 0; i < nelem; i++)
                {
                    status = pev_i2c_write(device->i2cDevice, offset, ((epicsUInt32*)pdata)[i]);
                    if ((status & I2CEXEC_MASK) != I2CEXEC_OK)
                        break;
                    offset += 4;
                }
            else
                for (i = 0; i < nelem; i++)
                {
                    status = pev_i2c_read(device->i2cDevice, offset, &val);
                    if ((status & I2CEXEC_MASK) != I2CEXEC_OK)
                        goto readfail;
                    val &= ~*(epicsUInt32*)pmask;
                    val |= ((epicsUInt32*)pdata)[i] & *(epicsUInt32*)pmask;
                    status = pev_i2c_write(device->i2cDevice, offset, (epicsUInt32)val);
                    if ((status & I2CEXEC_MASK) != I2CEXEC_OK)
                        break;
                    offset += 4;
                }
            return 0;
        default:
            errlogPrintf("%s i2cWrite(device->i2cDevice=%d, offset=%d, dlen=%d, nelem=%d, pdata=@%p, pmask=@%p): illegal data size\n",
                user, device->i2cDevice, offset, dlen, nelem, pdata, pmask);
            return -1;
    }
    errlogPrintf("%s i2cWrite(device->i2cDevice=%d, offset=%d, dlen=%d, nelem=%d, pdata=@%p, pmask=@%p): pev_i2c_write() faild on offset %d status=%#x\n",
        user, device->i2cDevice, offset, dlen, nelem, pdata, pmask, offset, status);
    return status;
readfail:
    errlogPrintf("%s i2cWrite(device->i2cDevice=%d, offset=%d, dlen=%d, nelem=%d, pdata=@%p, pmask=@%p): pev_i2c_read() faild on offset %d status=%#x\n",
        user, device->i2cDevice, offset, dlen, nelem, pdata, pmask, offset, status);
    return status;
}

void pevI2cReport(
    regDevice *device,
    int level)
{
    if (device && device->magic == MAGIC)
    {
        printf("pevI2cAsyn driver: for i2cDevice %#010x\n",
            device->i2cDevice);
    }
}

static regDevSupport pevI2cSupport = {
    pevI2cReport,
    NULL,
    NULL,
    pevI2cRead,
    pevI2cWrite
};

/**
*        pevI2cConfigure(crate, name, i2cControlWord, command)  
*
*         crate:                 normally only 0; inceremnts if there are more crates in PCIe tree
*
*        name:                 virtual device name to refer to in EPICS record links (INP/OUT)
*
*          i2cControlWord  all information to select target bus, address, access speed, data width, operation,.. 
*
*
**/

int pevI2cConfigure(
    unsigned int crate,
    const char* name,
    unsigned int i2cControlWord,
    unsigned int command)
{
    
  regDevice* device;    
  char* tmpStrCpy;
  struct pev_node *pev;
   
  if (!name)
  {
    printf("usage: pevI2cConfigure (crate, name, i2cControlWord, command)\n");
    return -1;
  }

  if (regDevFind(name)) 
  {
    printf("pevI2cConfigure: ERROR, device \"%s\" on an IFC/PEV already configured as synchronous device\n", 
                    name);
    return -1;
  }

  /* call PEV1100 user library initialization function */
  pev = pev_init( crate);
  if (!pev)
  {
    printf("pevI2cConfigure: Cannot allocate data structures to control PEV1100\n");
    return -1;
  }
  /* verify if the PEV1100 is accessible */
  if (pev->fd < 0)
  {
    printf("pevI2cConfigure: Cannot find PEV1100 interface\n");
    return -1;
  }
  
  device = (regDevice*)malloc(sizeof(regDevice));
  if (device == NULL)
  {
      errlogSevPrintf(errlogFatal,
            "pevI2cConfigure() %s: out of memory\n",
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
    printf("pevI2cConfigure: illegal command value (must be 0 or 1)\n");
    free(device);
    return -1;
  }

  regDevRegisterDevice(name, &pevI2cSupport, device, 0);
  regDevInstallWorkQueue(device, 100);
  return 0;
}


/**
*
* end of dma multi-user request handling
*
**/

#ifdef EPICS_3_14

#include <iocsh.h>
static const iocshArg pevI2cConfigureArg0 = { "crate", iocshArgInt };
static const iocshArg pevI2cConfigureArg1 = { "name", iocshArgString };
static const iocshArg pevI2cConfigureArg2 = { "i2cControlWord", iocshArgInt };
static const iocshArg pevI2cConfigureArg3 = { "command", iocshArgInt };
static const iocshArg * const pevI2cConfigureArgs[] = {
    &pevI2cConfigureArg0,
    &pevI2cConfigureArg1,
    &pevI2cConfigureArg2,
    &pevI2cConfigureArg3,
};

static const iocshFuncDef pevI2cConfigureDef =
    { "pevI2cConfigure", 4, pevI2cConfigureArgs };
static const iocshFuncDef pevAsynI2cConfigureDef =
    { "pevAsynI2cConfigure", 4, pevI2cConfigureArgs };
    
static void pevI2cConfigureFunc (const iocshArgBuf *args)
{
    int status = pevI2cConfigure(
        args[0].ival, args[1].sval, args[2].ival, args[3].ival);
    if (status != 0 && !interruptAccept) epicsExit(1);
}

static void pevI2cRegistrar ()
{
    iocshRegister(&pevI2cConfigureDef, pevI2cConfigureFunc);
    iocshRegister(&pevAsynI2cConfigureDef, pevI2cConfigureFunc);
}

epicsExportRegistrar(pevI2cRegistrar);

#endif
