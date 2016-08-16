#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/mman.h>

#include <epicsExit.h>
#include <dbAccess.h>
#include <iocsh.h>
#include <epicsExport.h>

#include "pev.h"

/**
**  VME slave window setup
**  
**  addrSpace : "AM32" or "AM24"
**/

int pevVmeSlaveMainConfig(const char* addrSpace, unsigned int mainBase, unsigned int mainSize)
{
    struct pev_ioctl_vme_conf vme_conf;
    struct pev_ioctl_map_ctl vme_slv_map;
    int status;

    if (!addrSpace)
    {
        printf("usage: pevVmeSlaveMainConfig (\"A24\"|\"A32\", base, size)\n");
        return -1;
    }

    pev_init(0);

    pev_vme_conf_read(&vme_conf);

    if (strcmp(addrSpace, "AM32") != 0)
    {
        printf("pevVmeSlaveMainConfig(): ERROR, can map to AM32 only\n");
        return -1;
    }

    vme_slv_map.sg_id = MAP_SLAVE_VME;	/* first clear all default winodws */
    pev_map_clear(&vme_slv_map);

    vme_conf.a32_size = mainSize;
    vme_conf.a32_base = mainBase;
    vme_conf.slv_ena = mainSize == 0 ? 0 : mainSize <= 0x8000000 ? VME_SLV_1MB | VME_SLV_ENA : VME_SLV_ENA;

    status = pev_vme_conf_write(&vme_conf);
    pev_vme_conf_read(&vme_conf);

    if (status)
    {
        printf("pevVmeSlaveMainConfig(): ERROR, pev_vme_conf_write() failed\n");
    }

    if (vme_conf.a32_size < mainSize)
    {
        printf("pevVmeSlaveMainConfig(): ERROR, slave map too large, got only %uMiB instead of %uMiB\n",
            vme_conf.a32_size>>20, mainSize>>20);
        return -1;
    }
    if (vme_conf.a32_size > mainSize)
    {
        printf("pevVmeSlaveMainConfig(): Info: slave map size increased to 0x%x=%uMiB\n",
            vme_conf.a32_size, vme_conf.a32_size>>20);
    }
    if (vme_conf.a32_base != mainBase)
    {
        printf("pevVmeSlaveMainConfig(): ERROR, slave map not adjusted. Move base from 0x%x to 0x%x\n",
            mainBase, vme_conf.a32_base);
        return -1;
    }
    return 0;

}

/**
**  VME slave window setup
**  slaveAddrSpace 	:	"AM32" or "AM24"
**  winBase		:	baseOffset in main VME slave space  		
**  winSize		:	window size
**  protocol		:	"BLT","MBLT","2eVME","2eSST160","2eSST233","2eSST320"
**  target		: 	"SH_MEM", "PCIE", "USR1/2"
**  targetOffset	:	offset in the remote target
**  swapping		:	"WS", "DS" or "QS"
**/
 
int pevVmeSlaveTargetConfig(const char* slaveAddrSpace, unsigned int winBase,  unsigned int winSize, 
		const char* protocol, const char* target, unsigned int targetOffset, const char* swapping)
{
  struct pev_ioctl_vme_conf vme_conf;

  int mapMode = MAP_ENABLE | MAP_ENABLE_WR;
  if (!slaveAddrSpace || !protocol)
  {
    printf("usage: pevVmeSlaveTargetConfig (\"A24\"|\"A32\", base, size, \"BLT\"|\"MBLT\"|\"2eVME\"|\"2eSST160\"|\"2eSST233\"|\"2eSST320\", \"SH_MEM\"|\"PCIE\"|\"USR1/2\", offset, \"WS\"|\"DS\"|\"QS\")\n");
    return -1;
  }

  if (strcmp(slaveAddrSpace, "AM32") != 0)
  {
    printf("pevVmeSlaveTargetConfig(): ERROR, can map to AM32 only\n");
    return -1;
  }

  pev_init(0);
  pev_vme_conf_read(&vme_conf);
  if (!(vme_conf.slv_ena & VME_SLV_ENA))
  {
    printf("pevVmeSlaveTargetConfig(): ERROR, VME slave disabled, call pevVmeSlaveMainConfig first\n");
    return -1;
  }
  
  if (swapping && *swapping && strcmp(swapping, "0") != 0)
  {
      if (strcmp(swapping, "AUTO") == 0)
        mapMode |= MAP_SWAP_AUTO;
      else
      {
        printf("  pevVmeSlaveTargetConfig(): ERROR, invalid swapping \"%s\""
            "[valid options: AUTO]\n",
            swapping);
        return -1;
      }
  }

  if (protocol && *protocol)
  {
    printf("  pevVmeSlaveTargetConfig(): protocol parameter \"%s\" is useless and will be ignored\n", protocol);
  }

  if (strcmp(target, "SH_MEM") == 0)
    mapMode |= MAP_SPACE_SHM;
  else
  if (strcmp(target, "USR1") == 0)
    mapMode |= MAP_SPACE_USR1;
  else
  if (strcmp(target, "PCIE") == 0)
    mapMode |= MAP_SPACE_PCIE;
  else
  {
    printf("  pevVmeSlaveTargetConfig(): == ERROR == invalid target \"%s\""
        "[valid options: SH_MEM, USR1, PCIE]\n",
        target);
    return -1;
  }
  
  pevMapToAddr(0, MAP_SLAVE_VME, mapMode, targetOffset, winSize, winBase);

  return 0;
}

void pevVmeShow(void)
{
  struct pev_ioctl_vme_conf vme_conf;

  pev_init(0);
  pev_vme_conf_read(&vme_conf);
  printf("current VME settings:\n"
         " VME64x %s\n"
         " VME master %s\n"
         "  System Controller %s\n"
         "  Sysreset %s\n"
         "  Retry Timeout %d usec\n"
         "  Arbitration mode %s\n"
         "  Bus Timeout %d usec\n"
         "  VME request mode %d\n"
         "  VME request level %d\n"
         " VME slave %s granularity=%uMiB\n"
         "  A32 slave base=0x%08x size=0x%x=%uMiB\n"
         "  A24 slave base=  0x%06x size=0x%x=%uKiB\n"
         "  Slave retry %s\n"
         "  Slave burst %d\n",
         vme_conf.x64 ? "enabled" : "disabled",
         vme_conf.mas_ena ? "enabled" : "disabled",
         vme_conf.slot1 ? "yes" : "no",
         vme_conf.sysrst ? "enabled" : "disabled",
         vme_conf.rto,
         vme_conf.arb ? "yes" : "no",
         vme_conf.bto,
         vme_conf.req,
         vme_conf.level,
         vme_conf.slv_ena & VME_SLV_ENA ? "enabled" : "disabled",
         vme_conf.slv_ena & VME_SLV_1MB ? 1 : 16,
         vme_conf.a32_base, vme_conf.a32_size, vme_conf.a32_size>>20,
         vme_conf.a24_base, vme_conf.a24_size, vme_conf.a24_size>>10,
         vme_conf.slv_retry ? "no" : "yes",
         vme_conf.burst);
}

static const iocshArg pevVmeSlaveMainConfigArg0 = { "addrSpace", iocshArgString };
static const iocshArg pevVmeSlaveMainConfigArg1 = { "mainBase", iocshArgInt };
static const iocshArg pevVmeSlaveMainConfigArg2 = { "mainSize", iocshArgInt };

static const iocshArg * const pevVmeSlaveMainConfigArgs[] = {
    &pevVmeSlaveMainConfigArg0,
    &pevVmeSlaveMainConfigArg1,
    &pevVmeSlaveMainConfigArg2,
};

static const iocshFuncDef pevVmeSlaveMainConfigDef =
    { "pevVmeSlaveMainConfig", 3, pevVmeSlaveMainConfigArgs };
    
static void pevVmeSlaveMainConfigFunc (const iocshArgBuf *args)
{
    int status = pevVmeSlaveMainConfig(
        args[0].sval, args[1].ival, args[2].ival);
    if (status != 0 && !interruptAccept) epicsExit(1);
}
		
static const iocshArg pevVmeSlaveTargetConfigArg0 = { "slaveAddrSpace", iocshArgString };
static const iocshArg pevVmeSlaveTargetConfigArg1 = { "winBase", iocshArgInt };
static const iocshArg pevVmeSlaveTargetConfigArg2 = { "winSize", iocshArgInt };
static const iocshArg pevVmeSlaveTargetConfigArg3 = { "protocol", iocshArgString };
static const iocshArg pevVmeSlaveTargetConfigArg4 = { "target", iocshArgString };
static const iocshArg pevVmeSlaveTargetConfigArg5 = { "targetOffset", iocshArgInt };
static const iocshArg pevVmeSlaveTargetConfigArg6 = { "swapping", iocshArgString };
static const iocshArg * const pevVmeSlaveTargetConfigArgs[] = {
    &pevVmeSlaveTargetConfigArg0,
    &pevVmeSlaveTargetConfigArg1,
    &pevVmeSlaveTargetConfigArg2,
    &pevVmeSlaveTargetConfigArg3,
    &pevVmeSlaveTargetConfigArg4,
    &pevVmeSlaveTargetConfigArg5,
    &pevVmeSlaveTargetConfigArg6,
};
		
static const iocshFuncDef pevVmeSlaveTargetConfigDef =
    { "pevVmeSlaveTargetConfig", 7, pevVmeSlaveTargetConfigArgs };
    
static void pevVmeSlaveTargetConfigFunc (const iocshArgBuf *args)
{
    int status = pevVmeSlaveTargetConfig(
        args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].sval, args[5].ival, args[6].sval);
    if (status != 0 && !interruptAccept) epicsExit(1);
}

static const iocshFuncDef pevVmeShowDef =
    { "pevVmeShow", 0, NULL };
    
static void pevVmeShowFunc (const iocshArgBuf *args)
{
    pevVmeShow();
}
		
static void pevSlaveWindowRegistrar ()
{
    iocshRegister(&pevVmeSlaveMainConfigDef, pevVmeSlaveMainConfigFunc);
    iocshRegister(&pevVmeSlaveTargetConfigDef, pevVmeSlaveTargetConfigFunc);
    iocshRegister(&pevVmeShowDef, pevVmeShowFunc);
}

epicsExportRegistrar(pevSlaveWindowRegistrar);
