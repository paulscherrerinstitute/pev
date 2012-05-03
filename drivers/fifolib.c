/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : fifolib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the low level functions to drive the FIFO busses
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
#include "fifolib.h"

void
fifo_init( uint io_base)
{
  uint value;

  value = 0xf8000000;
  outl( value, io_base + 0x10);
  outl( value, io_base + 0x14);
  outl( value, io_base + 0x18);
  outl( value, io_base + 0x1c);

  return;
}

void
fifo_status( uint io_base,
	     uint idx,
	     uint *sts)
{

  *sts = inl( io_base + 0x10 + idx*4);

  return;
}

void
fifo_clear( uint io_base,
	    uint idx,
	    uint *sts)
{
  uint value;

  *sts = inl( io_base + 0x10 + idx*4);
  value = 0xf8000000;
  outl( value, io_base + 0x10 + idx*4);

  return;
}

int
fifo_read( uint io_base,
	   uint idx,
	   uint *data,
	   uint cnt,
	   uint *sts)
{
  uint wcnt, tot;

  if( idx > 3) return( -1);
  tot = 0;
  
  while (1)
  {
    *sts = inl( io_base + 0x10 + idx*4);
    wcnt = *sts & 0xff;
    if (!wcnt)
    {
      return(tot);
    }
    if (cnt == 0)    // Cannot type boolean OR
    {
      return(tot);
    }	
    while( wcnt--)
    {	
      *data++ = inl( io_base + idx*4);
      tot += 1;
      if (tot >= cnt)
      {
	*sts = inl( io_base + 0x10 + idx*4);
	return(tot);
      }
    }
  }
}

int
fifo_write( uint io_base,
	    uint idx,
	    uint *data,
	    uint cnt,
	    uint *sts)
{
  uint wcnt, tot;
  uint srcount; // Ensure wait for new status
  
  if( idx > 3) return( -1);
  tot = 0;
  while( 1)
  {
    srcount = 0;
    do
    {
      *sts = inl( io_base + 0x10 + idx*4);
      srcount++;
    } while (srcount < 0x2);
	
    wcnt = 0xff - (*sts & 0xff);
    if( !wcnt)
    {
      return( tot);
    }
    if( cnt == 0)
    {
      return( tot);
    }
    while( wcnt--)
    {
      *sts = inl( io_base + 0x10 + idx*4);
      outl(  *data++, io_base + idx*4);
      tot += 1;
      if( tot >= cnt)
      {
        srcount = 0;
	do
	{
	  *sts = inl( io_base + 0x10 + idx*4);
	  srcount++;
	} while (srcount < 0x2);
        return( tot);
      }
    }
  }
}
