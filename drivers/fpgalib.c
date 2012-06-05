/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : fpgalib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : november 25,2011
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the low level functions to load the FPGA device
 *    implemented on the IFC1210 through ELBC.
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
 *  $Log: fpgalib.c,v $
 *  Revision 1.4  2012/06/05 13:37:31  kalantari
 *  linux driver ver.4.12 with intr Handling
 *
 *  Revision 1.2  2012/01/27 13:13:04  ioxos
 *  prepare release 4.01 supporting x86 & ppc [JFG]
 *
 *  Revision 1.1  2012/01/27 08:53:41  ioxos
 *  first checkin [JFG]
 *
 *=============================< end file header >============================*/

#include <asm/uaccess.h>         // copy_to_user and copy_from_user
#include <linux/init.h>          // modules
#include <linux/module.h>        // module
#include <linux/types.h>         // dev_t type
#include <linux/fs.h>            // chrdev allocation
#include <linux/slab.h>          // kmalloc and kfree
#include <linux/cdev.h>          // struct cdev
#include <linux/errno.h>         // error codes
#include <linux/pci.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <asm/dma.h>

#ifdef XENOMAI
#include <rtdm/rtdm_driver.h>
#endif

#include "../include/pevioctl.h"

#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

#define FPGA_OFFSET     0x00
#define FPGA_INIT       0x01
#define FPGA_DONE       0x02
#define FPGA_PROG       0x04
#define FPGA_CSI        0x08
#define FPGA_RW         0x10
#define FPGA_ENABLE     0x80

int rdwr_swap_32( int); 
static int fpga_dev = 0;

void
fpga_set_dev( uint dev)
{
  fpga_dev = (dev & 1);
}

static void
fpga_write_io( uint data,
               volatile unsigned long reg_p)
{
  volatile uint tmp;

  *(uint *)reg_p = data;
  tmp = *(uint *)reg_p; /* force write cycle to be executed */

  return;
}


static uint
fpga_read_io( volatile unsigned long reg_p)
{
  volatile uint data;

  data = *(uint *)reg_p;

  return( data);
}

static int fpga_enable = 0; 
static int fpga_prog = 0;
static int fpga_init = 0;
static int fpga_done = 0;
static int fpga_count = 0;
static int fpga_mask = 0xffffffff;

int
fpga_load( unsigned long io_base,
	   unsigned char *buf,
	   uint len,
	   int first)
{
  unsigned long io_reg;
  int i, j;
  unsigned char *s;
  volatile int data, mask, tmo;

  io_reg = io_base + FPGA_OFFSET;
  if( first)
  {
    fpga_count   = 0;
    if( fpga_dev)
    {
      fpga_enable = 0x8000;
      fpga_init   = 0x0100;
      fpga_done   = 0x0200;
      fpga_prog   = 0x0400;
      fpga_mask   = ~0x800ff80;
    }
    else
    {
      fpga_enable = 0x80;
      fpga_init   = 0x01;
      fpga_done   = 0x02;
      fpga_prog   = 0x04;
      fpga_mask   = ~0x80080ff;
    }

    /* initialize load sequence */
    mask = fpga_read_io( io_reg) & fpga_mask;
    fpga_write_io( mask | fpga_enable | fpga_prog, io_reg);
    tmo = 100000;
    while( tmo--);
    fpga_read_io( io_reg);

    /* clear prog_bit */
    fpga_write_io( mask |  fpga_enable, io_reg);
    tmo = 100000;
    while( tmo--);
    fpga_read_io( io_reg);

    /* wait for INIT to go low */
    tmo = 500000;
    while( --tmo)
    {
      data =  fpga_read_io( io_reg);
      if( !( data & fpga_init))
      {
        break;
      }
    }
    if( !tmo)
    {
      fpga_write_io( fpga_enable | fpga_prog, io_reg); /* clear bit_prog */
      fpga_write_io( fpga_prog, io_reg); /*  disable acces to FPGA */
      return( -1);
    }

    /* set prog bit */
    fpga_write_io( mask | fpga_enable | fpga_prog, io_reg);

    /* wait for bit_init to go high */
    tmo = 5000;
    while( --tmo)
    {
      data = fpga_read_io( io_reg);
      if( data & fpga_init)
      {
        break;
      }
      data = 0;
    }
    if( !tmo)
    {
      fpga_write_io( mask | fpga_prog, io_reg); /*  disable acces to UFPGA */
      return( -1);
    }
  }

  s = buf;
  for( j = 0; j < len;)
  {
    if( fpga_dev)
    {
      data = (long)(*(short *)s);
      s +=2; j += 2;
      if( j == len) s = buf;
    }
    else
    {
      data =  *(long *)s;
      s += 4; j += 4;
      if( j == len) s = buf;
    }
    fpga_write_io( data, io_reg + 4); /* program dword */

    /* check end of programming sequence */
    data =  fpga_read_io( io_reg);
    if( data & fpga_done)
    {
      fpga_count += j;
      for( i = 0; i < 100; i++)
      {
	fpga_write_io( 0, io_reg + 4); /* program dword */
      }
      fpga_write_io( mask | fpga_prog, io_reg);  /*  disable acces to FPGA keeping bit_prog high */
      return(0);
    }

    /* verify that bit_init is still high */
    data =  fpga_read_io( io_reg);
    if( !( data & fpga_init))
    {
      fpga_write_io( mask | fpga_prog, io_reg); /*  disable acces to UFPGA */
      return( -1);
    }
  }
  fpga_count += j;
  return( 1);

}

