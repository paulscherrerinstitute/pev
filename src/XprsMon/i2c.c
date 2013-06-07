/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : i2c.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : november 14,2011
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to perform read
 *     or write cycles through the I"C interface.
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
 * $Log: i2c.c,v $
 * Revision 1.12  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.7  2012/09/03 13:19:05  ioxos
 * adapt pec_i2c_xx(), pev_pex_xx() and pev_bmr_xx() to new FPGA and library [JFG]
 *
 * Revision 1.6  2012/08/28 13:40:46  ioxos
 * cleanup + update i2c status + reset [JFG]
 *
 * Revision 1.5  2012/06/01 13:59:44  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.4  2012/02/14 16:06:43  ioxos
 * add support for FMC [JFG]
 *
 * Revision 1.3  2012/02/03 16:27:37  ioxos
 * dynamic use of elbc for i2c [JFG]
 *
 * Revision 1.2  2012/01/27 15:55:44  ioxos
 * prepare release 4.01 supporting x86 & ppc [JFG]
 *
 * Revision 1.1  2012/01/27 13:39:15  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: i2c.c,v 1.12 2013/06/07 14:59:54 zimoch Exp $";
#endif

#define DEBUGno
#include <debug.h>

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cli.h>
#include <pevioctl.h>
#include <pevulib.h>

struct pev_i2c_devices i2c_devices_ifc[] =
{
  { "max5970",  0x41000030},
  { "bmr463_0", 0x40048053},
  { "bmr463_1", 0x4004805b},
  { "bmr463_2", 0x40048063},
  { "bmr463_3", 0x40048024},
  { "lm95255_1",0x0100004c},
  { "lm95255_2",0x01000018},
  { "idt8n4q01",0xe100006e},
  { "pes32nt",  0xc1000075},
  { "plx8624",  0x010f0069},
  { "vmep0",    0x60000000},
  { "fmc1",     0x80000000},
  { "fmc2",     0xa000000},
  { NULL,       0x00000000}
};

struct pev_i2c_devices i2c_devices_ipv[] =
{
  { "plx8624",  0x010f0069},
  { NULL,       0x00000000}
};

struct pev_i2c_devices i2c_devices_pev[] =
{
  { "plx8624",  0x010f0069},
  { NULL,       0x00000000}
};

char *
i2c_rcsid()
{
  return( rcsid);
}

int 
xprs_i2c( struct cli_cmd_para *c)
{
  struct pev_ioctl_i2c i2c;
  struct pev_i2c_devices *i2d;
  uint addr;
  char *i2d_name, *p;

  if( c->cnt < 2)
  {
    printf("i2c command needs more arguments\n");
    printf("usage: i2c <dev> <op> <reg> [<data>]\n");
    printf("       i2c <dev>:<addr> <op> <reg> [<data>]\n");
    printf("i2c device list:\n");
    i2d = &i2c_devices_ifc[0];
    while( i2d->name)
    {
      printf("   - %s\n", i2d->name);
      i2d++;
    }
    return(-1);
  }
  i2d = &i2c_devices_ifc[0];
  i2c.device = 0;
  printf("scanning i2d dev table\n");
  i2d_name = strtok( c->para[0],":");
  while( i2d->name)
  {
    if( !strcmp( i2d->name, i2d_name))
    {
      i2c.device = i2d->id;
      break;
    }
    i2d++;
  }
  p = strtok(0,".");
  addr = 0;
  if( p)
  {
    sscanf( p,"%x", &addr);
    printf("device address : %s - %x\n", p, addr);
  }
  if( !i2c.device)
  {
    if( sscanf( c->para[0],"0x%x", &i2c.device) != 1)
    {
      printf("wrong device name\n");
      printf("usage: i2c <dev> <op> <reg> [<data>]\n");
      printf("i2c device list:\n");
      i2d = &i2c_devices_ifc[0];
      while( i2d->name)
      {
        printf("   - %s\n", i2d->name);
        i2d++;
      }
      return(-1);
    }
  }
  if( addr)
  {
    i2c.device |= (addr & 0x7f) | ((addr & 0x380) << 1);
  }
  if( c->ext) 
  {
    /* on IFC1210 use ELBC */
    if( ( c->ext[0] == 'e') && (pev_board() == PEV_BOARD_IFC1210))
    {
      i2c.device |= 0x80;
    }
  }
  if( c->cnt > 2)
  {
    if( sscanf( c->para[2],"%x", &i2c.cmd) != 1)
    {
      printf("wrong register number\n");
      printf("usage: i2c <dev> <op> <reg> [<data>]\n");
      return(-1);
    }
  }
  printf("device: %08x\n", i2c.device);
  if( !strcmp( "cmd", c->para[1]))
  {
    i2c.data = pev_i2c_cmd( i2c.device, i2c.cmd);
  }
  if( !strcmp( "read", c->para[1]))
  {
    int status;

    if( i2d->id == 0x010f0069) /* PEX8624 */
    {
      status = pev_pex_read( i2c.cmd, &i2c.data);
      if( status & I2C_CTL_ERR)
      {
	printf("%s: reg=%x -> error = %08x\n", i2d->name, i2c.cmd, status);
      }
      else
      {
	printf("%s: reg=%x -> data = %x\n", i2d->name, i2c.cmd, i2c.data);
      }
    } 
    else
    {
      status = pev_i2c_read( i2c.device, i2c.cmd, &i2c.data);
      if( status & I2C_CTL_ERR)
      {
	printf("%s: reg=%x -> error = %08x\n", i2d->name, i2c.cmd, status);
      }
      else
      {
        switch(( i2c.device >> 18) & 0x3)
        {
          case 0:
  	  {
            printf("%s: reg=%x -> data = %02x\n", i2d->name, i2c.cmd, (uchar)i2c.data);
	    break;
  	  }
          case 1:
	  {
            printf("%s: reg=%x -> data = %04x\n", i2d->name, i2c.cmd, (ushort)i2c.data);
	    break;
	  }
          case 2:
	  {
            printf("%s: reg=%x -> data = %06x\n", i2d->name, i2c.cmd, (uint)i2c.data & 0xffffff);
	    break;
	  }
          case 3:
	  {
            printf("%s: reg=%x -> data = %08x\n", i2d->name, i2c.cmd, (uint)i2c.data);
	    break;
	  }
	}
      }
    }
  }
  if( !strcmp( "write", c->para[1]))
  {
    if( c->cnt < 4)
    {
      printf("mpc i2c write command needs more arguments\n");
      printf("usage: i2c <dev> write <reg> <data>\n");
      return(-1);
    }
    if( sscanf( c->para[3],"%x", &i2c.data) != 1)
    {
      printf("wrong data value\n");
      return(-1);
    }
    if( i2d->id == 0x010f0069) /* PEX8624 */
    {
      pev_pex_write( i2c.cmd, i2c.data);
    }
    else 
    {
       pev_i2c_write( i2c.device, i2c.cmd, i2c.data);
    } 
  }
  if( !strcmp( "reset", c->para[1]))
  {
    printf("resetting I2C bus 0x%08x\n", i2c.device & 0xe0000000);
    pev_i2c_reset( i2c.device);
  }
  return(0);

}
