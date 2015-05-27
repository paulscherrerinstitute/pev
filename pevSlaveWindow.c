#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/mman.h>
#include <pevulib.h>

#include <epicsExit.h>
#include <dbAccess.h>
#include <iocsh.h>
#include <epicsExport.h>

#include "pev.h"

/* VME slave */
typedef struct pevVmeSlaveMap {
    unsigned int a32_base; 
    unsigned int a32_size;
    unsigned int a24_base; 
    unsigned int a24_size;
} pevVmeSlaveMap;

static pevVmeSlaveMap glbVmeSlaveMap = {0, 0, 0, 0};	/* global slave map */

/**
**  VME slave window setup
**  
**  addrSpace : "AM32" or "AM24"
**/

int pevVmeSlaveMainConfig(const char* addrSpace, unsigned int mainBase, unsigned int mainSize)
{
  struct pev_ioctl_vme_conf vme_conf;
  struct pev_ioctl_map_ctl vme_slv_map;
  
  if (!addrSpace)
  {
    printf("usage: pevVmeSlaveMainConfig (\"A24\"|\"A32\", base, size)\n");
    return -1;
  }
  
  if( !pev_init(0) )
  {
    printf("pevVmeSlaveMainConfig(): ERROR, Cannot allocate data structures to control IFC\n");
    return -1;
  }
  pev_vme_conf_read(&vme_conf);

  if (strcmp(addrSpace, "AM32") == 0)
  { 
    if (glbVmeSlaveMap.a32_size != 0)
    {
      printf("pevVmeSlaveMainConfig(): ERROR, Main Config can be done only once, current a32_size = 0x%x a32_base = 0x%x \n", 
      				glbVmeSlaveMap.a32_size, glbVmeSlaveMap.a32_base);
      return -1;
    }
    vme_slv_map.sg_id = MAP_SLAVE_VME;	/* first clear all default winodws */
    pev_map_clear(&vme_slv_map);
    vme_conf.a32_size = mainSize;
    vme_conf.a32_base = mainBase;
    glbVmeSlaveMap.a32_size = vme_conf.a32_size;
    glbVmeSlaveMap.a32_base = vme_conf.a32_base;
    if (mainSize == 0)			/* turn off slave window */
    {
      vme_conf.slv_ena = 0;
      printf("\t => pevVmeSlaveMainConfig(): turnning OFF the whole vme slave window !!\n");
    }
    else 
      vme_conf.slv_ena = 1; 
  }
  else
  if (strcmp(addrSpace, "AM24") == 0)
  { 
    if (glbVmeSlaveMap.a24_size != 0)
    {
      printf("\t => pevVmeSlaveMainConfig(): ERROR, Main Config can be done only once, current \
              a24_size = 0x%x a24_base = 0x%x \n", glbVmeSlaveMap.a24_size, glbVmeSlaveMap.a24_base);
      return -1;
    }
    vme_conf.a24_size = mainSize;
    vme_conf.a24_base = mainBase;
    glbVmeSlaveMap.a24_size = vme_conf.a24_size;
    glbVmeSlaveMap.a24_base = vme_conf.a24_base;
  }
  else
  {
    printf("\t => pevVmeSlaveMainConfig(): ERROR, invalid addrSpace (must be \"AM32\" or \"AM24\")\n");
    return -1;
  }
  pev_vme_conf_write(&vme_conf);
  pev_vme_conf_read(&vme_conf);
  if (strcmp(addrSpace, "AM32") == 0 && vme_conf.slv_ena && (vme_conf.a32_base != mainBase || vme_conf.a32_size != mainSize))
  {
    printf("  pevVmeSlaveMainConfig(): == ERROR == mapping failed! \n\t current A32 slaveBase 0x%x [size 0x%x] \
    		\n\t Address/Size Granularity must be 16MB=0x1000000\n", vme_conf.a32_base, vme_conf.a32_size);
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
  int mapMode = MAP_ENABLE | MAP_ENABLE_WR;
  if (!slaveAddrSpace || !protocol)
  {
    printf("usage: pevVmeSlaveTargetConfig (\"A24\"|\"A32\", base, size, \"BLT\"|\"MBLT\"|\"2eVME\"|\"2eSST160\"|\"2eSST233\"|\"2eSST320\", \"SH_MEM\"|\"PCIE\"|\"USR1/2\", offset, \"WS\"|\"DS\"|\"QS\")\n");
    return -1;
  }
  
  if (strcmp(slaveAddrSpace, "AM32") == 0)
  { 
    if (glbVmeSlaveMap.a32_size == 0)
    {
      printf("pevVmeSlaveTargetConfig(): ERROR, pevVmeSlaveMainConfig() has to be called first, Main \
              a32_size = 0x%x \n", glbVmeSlaveMap.a32_size);
      return -1;
    }
  }
  else
  if (strcmp(slaveAddrSpace, "AM24") == 0)
  { 
    if (glbVmeSlaveMap.a24_size == 0)
    {
      printf("pevVmeSlaveTargetConfig(): ERROR, pevVmeSlaveMainConfig() has to be called first, Main \
              a24_size = 0x%x \n", glbVmeSlaveMap.a24_size);
      return -1;
    }
  }
  else
  {
    printf("pevVmeSlaveTargetConfig(): ERROR, invalid slaveAddrSpace \"%s\""
        "(must be \"AM32\" or \"AM24\")\n",
        slaveAddrSpace);
    return -1;
  }

  if (swapping && *swapping)
  {
      if (strcmp(swapping, "AUTO") == 0)
        mapMode |= MAP_SWAP_AUTO;
      else
      {
        printf("  pevVmeSlaveTargetConfig(): == ERROR == invalid swapping \"%s\""
            "[valid options: AUTO]\n",
            swapping);
        return -1;
      }
  }
  

  if (protocol && *protocol)
  {
  
      if(strcmp(protocol, "BLT")==0) 
        mapMode |= DMA_VME_BLT;
      else if(strcmp(protocol, "MBLT")==0) 
        mapMode |= DMA_VME_MBLT;
      else if(strcmp(protocol, "2eVME")==0) 
        mapMode |= DMA_VME_2eVME;
      else if(strcmp(protocol, "2eSST160")==0) 
        mapMode |= DMA_VME_2e160;
      else if(strcmp(protocol, "2eSST233")==0) 
        mapMode |= DMA_VME_2e233;
      else if(strcmp(protocol, "2eSST320")==0) 
        mapMode |= DMA_VME_2e320;
      else
        mapMode|=strtol(protocol, NULL, 0);
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

void pevVmeSlaveShow(int level)
{
    printf("  Main VME slave window (A32) ");
    if(glbVmeSlaveMap.a32_size == 0)
        printf("turned OFF!\n");
    else
        printf("base=0x%08x size=0x%08x %dMB\n",
            glbVmeSlaveMap.a32_base, glbVmeSlaveMap.a32_size, glbVmeSlaveMap.a32_size>>20);
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

static void pevSlaveWindowRegistrar ()
{
    iocshRegister(&pevVmeSlaveMainConfigDef, pevVmeSlaveMainConfigFunc);
    iocshRegister(&pevVmeSlaveTargetConfigDef, pevVmeSlaveTargetConfigFunc);
}

epicsExportRegistrar(pevSlaveWindowRegistrar);
