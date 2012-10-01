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
#define I2C_CTL_RESET    0x004000
#define I2C_CTL_MASK     0xC00000

uint i2c_elb = 0;

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


void
i2c_set_elb( uint elb)
{
  i2c_elb = elb;
}

void
i2c_write_io( uint data,
              volatile ulong reg_p)
{
  volatile uint tmp;

  //printk("i2c_write_io before: %08lx -> %08x\n", reg_p, data);
  if( i2c_elb)
  {
    tmp = *(uint *)reg_p;
    *(uint *)reg_p = data;
    tmp = *(uint *)reg_p;
  }
  else
  {
    outl( data, reg_p);
    tmp = inl( reg_p);
  }
  //printk("i2c_write_io after: %08lx -> %08x\n", reg_p, tmp);
  return;
}

uint
i2c_read_io(  ulong reg_p)
{
  volatile uint data;

  if( i2c_elb)
  {
    data = *(uint *)reg_p;
  }
  else
  {
    data = inl( reg_p);
  }
  //printk("i2c_read_io after: %08lx -> %08x\n", reg_p, data);

  return( data);
}

struct semaphore i2c_sem;

uint
i2c_wait( uint io_base,
	  uint tmo)
{
  uint ctl;
  uint sts;
  uint sem;

  ctl = i2c_read_io( io_base + I2C_CTL_OFFSET);
  sem = tmo & 0x80000000;
  tmo &= ~0x80000000;
  while( !(ctl & 0x300000))
  {
    ctl = i2c_read_io( io_base + I2C_CTL_OFFSET);
    if( !tmo--)
    {
      printk(" I2C timeout\n");
      break;
    }
    if( sem)
    {
      sema_init( &i2c_sem, 0);
      sts = down_timeout( &i2c_sem, 1);
    }
  }
  return( ctl);
}
uint
i2c_cmd( uint io_base,
	 uint dev,
	 uint cmd)
{
  /* load command in register */
  i2c_write_io( cmd, io_base + I2C_CMD_OFFSET);
  /* trig command cycle */
  i2c_write_io( (~I2C_CTL_MASK & dev), io_base + I2C_CTL_OFFSET);
  i2c_write_io( (I2C_CTL_CMD | dev), io_base + I2C_CTL_OFFSET);

  return( 0);
}

uint
i2c_read( uint io_base,
	  uint dev,
	  uint *sts_p,
	  uint tmo)
{


  uint data;

  /* trig read cycle */
  i2c_write_io( (~I2C_CTL_MASK & dev), io_base + I2C_CTL_OFFSET);
  i2c_write_io( (I2C_CTL_READ | dev), io_base + I2C_CTL_OFFSET);
  *sts_p = i2c_wait( io_base, tmo);
  /* get data */
  data = i2c_read_io( io_base + I2C_READ_OFFSET);

  return( data);
}

uint
i2c_write( uint io_base,
	   uint dev,
	   uint cmd,
	   uint data)
{
  /* load command register */
  i2c_write_io( cmd, io_base + I2C_CMD_OFFSET);
  /* load data register */
  i2c_write_io( data, io_base + I2C_WRITE_OFFSET);
  /* trig command cycle */
  i2c_write_io( (~I2C_CTL_MASK & dev), io_base + I2C_CTL_OFFSET);
  i2c_write_io(  (I2C_CTL_WRITE | dev), io_base + I2C_CTL_OFFSET);

  return( 0);
}

uint
i2c_reset( uint io_base,
	   uint dev)
{
  /* kill any pending transaction on  I2C bus*/
  dev &= 0xe0000000; /* select I"C bus */
  i2c_write_io( dev, io_base + I2C_CTL_OFFSET);
  i2c_write_io( (I2C_CTL_RESET | dev), io_base + I2C_CTL_OFFSET);

  return( 0);
}


