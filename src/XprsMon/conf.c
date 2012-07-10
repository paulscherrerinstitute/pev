/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : conf.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to configure
 *     the PEV1000 interface.
 *
 *----------------------------------------------------------------------------
 *  Copyright Notice
 *  
 *    Copyright and all other rights in this document are reserved by 
 *    IOxOS Technologies SA. This documents contains proprietary information    
 *    and is supplied on express condition that it may not be disclosed, 
 *    reproduced in whole or in part, or used for any other purpose other
 *    than that for which it is supplies, without the written consent of  
 *    IOxOS Technologies SA                                                        
 *
 *----------------------------------------------------------------------------
 *  Change History
 *  
 * $Log: conf.c,v $
 * Revision 1.7  2012/07/10 10:21:48  kalantari
 * added tosca driver release 4.15 from ioxos
 *
 * Revision 1.16  2012/07/03 14:34:42  ioxos
 * bug in PCI MEM/PME size calculation [JFG]
 *
 * Revision 1.15  2012/06/01 13:59:43  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.14  2012/05/09 09:49:21  ioxos
 * adaptation for IFC1210 [JFG]
 *
 * Revision 1.13  2012/04/30 08:53:58  ioxos
 * VCC not PEV [JFG]
 *
 * Revision 1.12  2012/04/30 08:47:11  ioxos
 * if VCC force SHM size to 256 MBytes [JFG]
 *
 * Revision 1.11  2012/03/21 11:27:46  ioxos
 * Voltage readout for IFC1210 [JFG]
 *
 * Revision 1.10  2012/03/15 15:11:38  ioxos
 * adapt to IFC1210  [JFG]
 *
 * Revision 1.9  2012/01/30 11:16:23  ioxos
 * cosmetics [JFG]
 *
 * Revision 1.8  2011/03/03 15:42:38  ioxos
 * support for 1MBytes VME slave granularity [JFG]
 *
 * Revision 1.7  2011/01/25 10:41:04  ioxos
 * support for autoID [JFG]
 *
 * Revision 1.6  2010/01/04 11:23:47  ioxos
 * correction in conf_show_fpga() and conf_show_shm() [JFG]
 *
 * Revision 1.5  2009/12/15 17:16:21  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.4  2008/11/12 13:30:18  ioxos
 * new static options + switch info [JFG]
 *
 * Revision 1.3  2008/08/08 12:46:02  ioxos
 *  reorganize code (cosmetic) [JFG]
 *
 * Revision 1.2  2008/07/18 14:14:51  ioxos
 * show more info [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: conf.c,v 1.7 2012/07/10 10:21:48 kalantari Exp $";
#endif

#define DEBUGno
#include <debug.h>

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cli.h>
#include <pevioctl.h>
#include <pevulib.h>

extern struct pev_reg_remap *reg_remap;
int conf_board = 0;

char *
conf_rcsid()
{
  return( rcsid);
}

void
conf_show_static( void)
{
  int d0;
  char c0, c1, c2, c3;

  d0 = pev_csr_rd( 0x00);
  printf("   Static Options [0x%08x]\n", d0);
  printf("      VME Interface\n");
  printf("         A24 Base Address  : %06x\n", (d0&0xf8)<<16);
  c0 = c1 = c2 = c3 = '-';
  if( d0 & (1<<2)) c0 = '+';
  if( d0 & (1<<1)) c1 = '+';
  if( d0 & (1<<0)) c2 = '+';
  if( d0 & (1<<0)) c2 = '+';
  printf("         System Controller : 64x%c Slot1%c SysRstEna%c\n", c0, c1, c2);
  if( (d0&0xfc) == 0xac)
  {
    printf("         Auto ID           : enabled\n");
  }
  else
  {
    printf("         Auto ID           : disabled\n");
  }
  c0 = c1 = c2 = '-';
  if( d0 & (1<<8)) c0 = '+';
  if( d0 & (1<<9)) c1 = '+';
  if( d0 & (1<<10)) c2 = '+';

  if(( conf_board == PEV_BOARD_PEV1100) ||
     ( conf_board == PEV_BOARD_IPV1102)    )
  {
    printf("      PLX8624 Switch\n");
    c0 = ( d0 >> 14) & 3;
    switch( c0)
    {
      case 0:
      {
        if( d0 & (1<<13))
        {
          printf("         Port0 [P3]        : Upstream : Local Clock\n");
        }
        else
        {
          printf("         Port0 [P3]        : Upstream : External Clock\n");
        }
        if( d0 & (1<<12))
        {
          printf("         Port1 [P4]        : Non Transparent\n");
        }
        else
        {
          printf("         Port1 [P4]        : Downstream\n");
        }
        printf("         Port5 [FPGA]      : Downstream\n");
        printf("         Port6 [PCI]       : Downstream\n");
        printf("         Port8 [XMC#1]     : Downstream\n");
        printf("         Port9 [XMC#2]     : Downstream\n");
        break;
      }
      case 1:
      {
        printf("         Port0 [P3]        : Downstream\n");
        if( d0 & (1<<12))
        {
          printf("         Port1 [P4]        : Non Transparent\n");
        }
        else
        {
          printf("         Port1 [P4]        : Downstream\n");
        }
        if( d0 & (1<<13))
        {
          printf("         Port5 [FPGA]      : Downstream\n");
          printf("         Port6 [PCI]       : Downstream\n");
          printf("         Port8 [XMC#1]     : Downstream\n");
          printf("         Port9 [XMC#2]     : Upstream\n");
        }
        else
        {
          printf("         Port5 [FPGA]      : Downstream\n");
          printf("         Port6 [PCI]       : Downstream\n");
          printf("         Port8 [XMC#1]     : Upstream\n");
          printf("         Port9 [XMC#2]     : Downstream\n");
        }
        break;
      }
      case 2:
      {
        printf("         Port0 [P3]        : Downstream\n");
        printf("         Port1 [P4]        : Upstream\n");
        printf("         Port5 [FPGA]      : Downstream\n");
        printf("         Port6 [PCI]       : Downstream\n");
        printf("         Port8 [XMC#1]     : Downstream\n");
        printf("         Port9 [XMC#2]     : Downstream\n");
        break;
      }
      case 3:
      {
        printf("         Disabled");
        break;
      }
    }
  }
  if(( conf_board == PEV_BOARD_PEV1100) ||
     ( conf_board == PEV_BOARD_IPV1102)    )
  {
    printf("      FPGA\n");
    if( d0 & (1<<16))
    {
      printf("         Configuration mode\n");
    }
    else
    {
      printf("         Bit Stream        : %d\n", ((d0 >> 9)&0x3));
    }
    if( d0 & (1<<17))
    {
      printf("         PON FSM           : Disabled\n");
    }
    else
    {
      printf("         PON FSM           : Enabled\n");
    }
    c0 = ( d0 >> 20) & 7;
    switch( c0)
    {
      case 0:
      {
        printf("         MEM size          : Disabled\n");
        printf("         PMEM size         : Disabled\n");
        break;
      }
      case 1:
      {
        printf("         MEM size          : %d MBytes\n", 64*(1+((d0>>18)&3)));
        printf("         PMEM size         : Disabled\n");
        break;
      }
      default:
      {
        printf("         MEM size          : %d MBytes\n", 0x40 << ((d0>>18)&3));
        printf("         PMEM size         : %d MBytes\n", 0x20 << c0);
        if( d0 & (1<<23))
        {
          printf("         PMEM mode         : A64\n");
        }
        else
        {
          printf("         PMEM mode         : A32\n");
	}
      }
    }
  }

  return;
}

void
conf_show_switch( void)
{
  int d0;

  d0 = pev_pex_read( 0x0);
  printf("   PCIe SWITCH Status\n");
  printf("      Identifier           : 0x%08x\n", d0);
  return;
}

void
conf_show_fpga( void)
{
  int d0;

  d0 = pev_csr_rd( reg_remap->iloc_sign);
  printf("   FPGA Status\n");
  printf("      Identifier           : 0x%08x\n", d0);
  d0 = pev_csr_rd( reg_remap->iloc_ctl);
  printf("      Bit stream loaded    : %d\n", d0&3);
  d0 = pev_csr_rd( reg_remap->pcie_mmu);
  return;
}

void
conf_show_pci_ep( void)
{
  int d0;

  d0 = pev_csr_rd( reg_remap->pcie_mmu);
  printf("   PCI End Point\n");
  if( d0 & (1<<23))
  {
    printf("     MEM size          : %d MBytes\n", 0x20 << ((d0>>18)&7));
  }
  else
  {
    printf("     MEM size          : disabled\n");
  }
  if( d0 & (1<<22))
  {
    printf("     MEM mode          : A64\n");
  }
  else
  {
    printf("     MEM mode          : A32\n");
  }
  if( d0 & (1<<31))
  {
    printf("     PMEM size         : %d MBytes\n", 0x20 << ((d0>>26)&7));
  }
  else
  {
    printf("     PMEM size         : disabled\n");
  }
  if( d0 & (1<<30))
  {
    printf("     PMEM mode         : A64\n");
  }
  else
  {
    printf("     PMEM mode         : A32\n");
  }
  return;
}

void
conf_show_shm( void)
{
  int d0;

  d0 = pev_csr_rd( reg_remap->iloc_ctl);
  printf("   Shared Memory\n");
  if(( conf_board == PEV_BOARD_VCC1104) ||
     ( conf_board == PEV_BOARD_VCC1105)    )
  {
    printf("      Size                 : %d MBytes\n", 256);
  }
  else
  {
    printf("      Size                 : %d MBytes\n", (((d0>>8)&3)+1)*256);
  }
  return;
}


void
conf_show_vme( void)
{
  int d0, d1, d2;
  char c0, c1, c2, c3;
  int autoid, x64;

  printf("   VME Interface\n");
  /* select VME registers */
  pev_csr_wr( reg_remap->iloc_base, PEV_SCSR_SEL_VME);
  d0 = pev_csr_rd( reg_remap->vme_base);
  autoid = 0;
  x64= 0;
  if( d0 & 0x8000000)
  {
    autoid = (d0 >> 27)&0x7;
  }
  if( d0 & 0x1000000)
  {
    x64 = 1;
  }
  printf("      System Controller    : ");
  if( d0 & 0x8)
  {
    printf("Enabled\n");
    printf("         Arbtration mode   : ");
    switch( d0&3)
    {
      case 0:
      {
	printf("PRI not pipelined\n");
	break;
      }
      case 1:
      {
	printf("RRS not pipelined\n");
	break;
      }
      case 2:
      {
	printf("PRI pipelined\n");
	break;
      }
      case 3:
      {
	printf("RRS pipelined\n");
	break;
      }
    }
    printf("         Bus Timeout       : %d usec\n", 16 << ((d0>>8)&3));
  }
  else
  {
    printf("Disabled\n");
  }
  d0 = pev_csr_rd( reg_remap->vme_base + 0x4);
  printf("      Master               : ");
  if( d0 & 0x80000000)
  {
    printf("Enabled\n");
    printf("         Request Mode      : ");
    switch( d0&3)
    {
      case 0:
      {
	printf("Release When Done\n");
	break;
      }
      case 1:
      {
	printf("Release On Request\n");
	break;
      }
      case 2:
      {
	printf("FAIR\n");
	break;
      }
      case 3:
      {
	printf("No Release\n");
	break;
      }
    }
    printf("         Request Level     : %d\n", (d0>>2)&3);
  }
  else
  {
    printf("Disabled\n");
  }
  d0 = pev_csr_rd( reg_remap->vme_base + 0x8);
  d1 = pev_csr_rd( reg_remap->vme_ader);
  printf("      Slave                : ");
  if( d0 & 0x80000000)
  {
    printf("Enabled\n");
  }
  else
  {
    printf("Disabled\n");
  }
  d2 = pev_csr_rd( reg_remap->vme_csr + 0xc);
  if( x64)
  {
    printf("         CR base address   : 0x%06x\n", (d2&0xfc)<<16);
  }
  else
  {
    printf("         A24 base address  : 0x%06x\n", (d2&0xfc)<<16);
  }
  d1 = (d1<<24) | (pev_csr_rd( reg_remap->vme_ader + 4) << 16);
  printf("         A32 base address  : 0x%08x\n", d1);
  if( d0 & VME_SLV_1MB)
  {
    printf("         A32 window size   : 0x%08x\n", 0x100000 << (d0&7));
  }
  else
  {
    printf("         A32 window size   : 0x%08x\n", 0x1000000 << (d0&7));
  }

  d2 = pev_csr_rd(  reg_remap->vme_csr + 0x4);
  printf("         CR/CSR            : ");
  c0 = c1 = c2 = '-';
  if( d2 & (1<<3)) c0 = '+';
  if( d2 & (1<<4)) c1 = '+';
  if( d2 & (1<<5)) c2 = '+';
  printf("Berr%c SlvEna%c SysFail%c ", c0, c1, c2);
  c0 = c1 = c2 = c3 = '-';
  if( d2 & (1<<6)) c1 = '+';
  if( d2 & (1<<7)) c2 = '+';
  if( autoid) c3 = '+';
  printf("SysFailEna%c Reset%c AutoID%c\n", c1, c2, c3);
  if( autoid)
  {
     printf("         AutoID            : ");
     switch( autoid)
     {
       case 1:
       {
	 printf("Idle\n");
	 break;
       }
       case 3:
       {
	 printf("IRQ#2 Pending\n");
	 break;
       }
       case 5:
       {
	 printf("Waiting\n");
	 break;
       }
       case 7:
       {
	 printf("Completed\n");
	 break;
       }
     }
  }
  d0 = pev_csr_rd( reg_remap->vme_base + 0xc);
  printf("      Interrupt Generator\n");
  printf("         Vector            : %02x\n", d0&0xff);
  printf("         Level             : %d\n", (d0>>8)&7);
  printf("         Mode              : ");
  if( d0 & 0x800000)
  {
    printf("FIFO [%d]\n", (d0>>16)&0xf);
  }
  else
  {
    printf("Register\n");
  }
  printf("         Status            : ");
  if( d0 & 0x800)
  {
    printf("Pending\n");
  }
  else
  {
    printf("Cleared\n");
  }
  return;
}

void
conf_show_smon( void)
{
  int d0, d1, d2;
  float f0, f1, f2;

  pev_smon_wr( 0x41, 0x3000);
  printf("   FPGA System Monitor\n");
  d0 = pev_smon_rd( 0x00) >> 6;
  f0 = (((float)d0*503.975)/1024.) - 273.15;
  d1 = pev_smon_rd( 0x20) >> 6;
  f1 = (((float)d1*503.975)/1024.) - 273.15;
  d2 = pev_smon_rd( 0x24) >> 6;
  f2 = (((float)d2*503.975)/1024.) - 273.15;
  printf("      Temperature          : %.2f [%.2f - %.2f]\n", f0, f1, f2);
  d0 = pev_smon_rd( 0x01) >> 6;
  f0 = (((float)d0*3.0)/1024.);
  d1 = pev_smon_rd( 0x21) >> 6;
  f1 = (((float)d1*3.0)/1024.);
  d2 = pev_smon_rd( 0x25) >> 6;
  f2 = (((float)d2*3.0)/1024.);
  printf("      VCCint               : %.2f [%.2f - %.2f]\n", f0, f1, f2);
  d0 = pev_smon_rd( 0x02) >> 6;
  f0 = (((float)d0*3.0)/1024.);
  d1 = pev_smon_rd( 0x22) >> 6;
  f1 = (((float)d1*3.0)/1024.);
  d2 = pev_smon_rd( 0x26) >> 6;
  f2 = (((float)d2*3.0)/1024.);
  printf("      VCCaux               : %.2f [%.2f - %.2f]\n", f0, f1, f2);
  pev_smon_wr( 0x40, 0x3);
  usleep( 100000);

  if(( conf_board == PEV_BOARD_PEV1100) ||
     ( conf_board == PEV_BOARD_IPV1102)    )
  {
    d0 = pev_smon_rd( 0x03) >> 6;
    f0 = ((float)d0*0.001925);
    printf("      VCC1.8-INT           : %.2f\n", f0);
    pev_smon_wr( 0x40, 0x1b);
    usleep( 100000);
    d0 = pev_smon_rd( 0x1b) >> 6;
    f0 = ((float)d0*0.00504);
    printf("      VCC3.3-INT           : %.2f\n", f0);
    pev_smon_wr( 0x40, 0x1c);
    usleep( 100000);
    d0 = pev_smon_rd( 0x1c) >> 6;
    f0 = ((float)d0*0.00510);
    printf("      VCC5.0-VME           : %.2f\n", f0);
    pev_smon_wr( 0x40, 0x1d);
    usleep( 100000);
    d0 = pev_smon_rd( 0x1d) >> 6;
    f0 = ((float)d0*0.00510);
    printf("      VCC3.3-VME           : %.2f\n", f0);
  }
  if( conf_board == PEV_BOARD_IFC1210)
  {
    d0 = pev_smon_rd( 0x03) >> 6;
    f0 = ((float)d0*0.0025);
    printf("      FMC_Vadj             : %.2f [%x]\n", f0, d0);
  }
  return;
}

int 
xprs_conf_show( struct cli_cmd_para *c)
{

  int retval;
  int cnt, i;

  retval = -1;
  cnt = c->cnt;
  i = 0;
  conf_board = pev_board();

  if( !cnt)
  {
    conf_show_static();
    conf_show_fpga();
    conf_show_pci_ep();
    conf_show_shm();
    conf_show_vme();
    conf_show_smon();
    return(0);;
  }

  if( cnt--)
  {
    retval = 0;
    if( !strcmp( "show", c->para[i]))
    {
      int show_set;

      show_set = 0x0;
      if( !cnt)
      {
	show_set = 0x3f;
      }
      while( cnt--)
      {
	i++;

        if( !strcmp( "all", c->para[i]))
        {
	  show_set |= 0x3f;
	}
        if( !strcmp( "static", c->para[i]))
        {
	  show_set |= 0x1;
	}
        if( !strcmp( "switch", c->para[i]))
        {
	  show_set |= 0x2;
	}
        if( !strcmp( "fpga", c->para[i]))
        {
	  show_set |= 0x4;
	}
        if( !strcmp( "shm", c->para[i]))
        {
	  show_set |= 0x8;
	}
        if( !strcmp( "vme", c->para[i]))
        {
	  show_set |= 0x10;
	}
        if( !strcmp( "smon", c->para[i]))
        {
	  show_set |= 0x20;
	}
        if( !strcmp( "pci", c->para[i]))
        {
	  show_set |= 0x40;
	}
      }
      printf("PEV1100 Configuration\n");
      if( show_set & 0x1) conf_show_static();
      if( show_set & 0x2) conf_show_switch();
      if( show_set & 0x4) conf_show_fpga();
      if( show_set & 0x8) conf_show_shm();
      if( show_set & 0x10) conf_show_vme();
      if( show_set & 0x20) conf_show_smon();
      if( show_set & 0x40) conf_show_pci_ep();
      return(0); 
    }
  }

  return(retval);
}

