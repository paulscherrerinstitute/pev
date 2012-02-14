/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : i2clib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the low level functions to drive the I2C busses
 *    implemented on the PEV1100.
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
 *  $log: $
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

#include "../include/pevioctl.h"

#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

#define I2C_CTL_OFFSET    0x0
#define I2C_CMD_OFFSET    0x4
#define I2C_WRITE_OFFSET  0x8
#define I2C_READ_OFFSET   0xC

#define I2C_CTL_PEX 0x10f0069

#define I2C_CTL_RUNNING  0x100000
#define I2C_CTL_DONE     0x200000
#define I2C_CTL_ERROR    0x300000
#define I2C_CTL_CMD      0x400000
#define I2C_CTL_WRITE    0x800000
#define I2C_CTL_READ     0xC00000

int
i2c_swap_32( int data)
{
  char ci[4];
  char co[4];

  *(int *)ci = data;
  co[0] = ci[3];
  co[1] = ci[2];
  co[2] = ci[1];
  co[3] = ci[0];

  return( *(int *)co);
}


uint
i2c_wait( uint io_base,
	  uint tmo)
{
  uint ctl;
  ctl = inl( io_base + I2C_CTL_OFFSET);
  while( ctl & 0x100000)
  {
    ctl = inl( io_base + I2C_CTL_OFFSET);
    if( !tmo--) return( -1);
  }
  inl( io_base + I2C_CTL_OFFSET);
  return( ctl);
}
uint
i2c_cmd( uint io_base,
	 uint dev,
	 uint cmd)
{
  uint ctl;

  switch(dev)
  {
    case I2C_DEV_PEX:
    {
      ctl = I2C_CTL_PEX;
      break;
    }
    default:
    {
      return(-1);
    }
  }
  /* load command in register */
  cmd = i2c_swap_32( cmd);
  outl( cmd, io_base + I2C_CMD_OFFSET);
  /* trig command cycle */
  ctl |= I2C_CTL_CMD; 
  outl( ctl, io_base + I2C_CTL_OFFSET);

  return( 0);
}

uint
i2c_read( uint io_base,
	  uint dev)
{
  uint data;
  uint ctl;

  switch(dev)
  {
    case I2C_DEV_PEX:
    {
      ctl = I2C_CTL_PEX;
      break;
    }
    default:
    {
      return(-1);
    }
  }
  /* trig read cycle */
  ctl |= I2C_CTL_READ; 
  outl( ctl, io_base + I2C_CTL_OFFSET);
  ctl = i2c_wait( io_base, 10000);
  /* get data */
  data = inl( io_base + I2C_READ_OFFSET);
  data = i2c_swap_32( data);

  return( data);
}

uint
i2c_write( uint io_base,
	   uint dev,
	   uint cmd,
	   uint data)
{
  uint ctl;

  switch(dev)
  {
    case I2C_DEV_PEX:
    {
      ctl = I2C_CTL_PEX;
      break;
    }
    default:
    {
      return(-1);
    }
  }
  /* load command register */
  cmd = i2c_swap_32( cmd);
  outl( cmd, io_base + I2C_CMD_OFFSET);
  /* load data register */
  data = i2c_swap_32( data);
  outl( data, io_base + I2C_WRITE_OFFSET);
  /* trig command cycle */
  ctl |= I2C_CTL_WRITE; 
  outl( ctl, io_base + I2C_CTL_OFFSET);

  return( 0);
}



