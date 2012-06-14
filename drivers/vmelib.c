/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : vmelib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the low level functions to drive the VME64x interface
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
 * $Log: vmelib.c,v $
 * Revision 1.7  2012/06/14 14:00:05  kalantari
 * added support for r/w PCI_IO bus registers, also added read USR1 generic area per DMA and distribute the readout into individual records
 *
 * Revision 1.10  2011/03/03 15:42:15  ioxos
 * support for 1MBytes VME slave granularity [JFG]
 *
 * Revision 1.9  2011/01/25 13:59:11  ioxos
 * cleanup [JFG]
 *
 * Revision 1.8  2011/01/25 13:40:43  ioxos
 * support for VME RMW [JFG]
 *
 * Revision 1.7  2009/12/15 17:13:25  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.6  2009/04/06 10:31:49  ioxos
 * stop timer before start [JFG]
 *
 * Revision 1.5  2009/01/27 14:37:30  ioxos
 * remove ref to semaphore.h [JFG]
 *
 * Revision 1.4  2008/11/12 10:36:21  ioxos
 * add timer functions [JFG]
 *
 * Revision 1.3  2008/09/17 11:54:45  ioxos
 * add functions to get/set VME config and CRSCR [JFG]
 *
 * Revision 1.2  2008/07/04 07:40:13  ioxos
 * update address mapping functions [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
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
#include "vmelib.h"

#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif


int
vme_conf_get( struct vme_reg *reg,
	      struct vme_ctl *vme)
{
  vme->arb = inl( reg->ctl  + 0x00);
  vme->master = inl( reg->ctl + 0x04);
  vme->slave = inl( reg->ctl + 0x08);
  vme->itgen = inl( reg->ctl + 0x0c);
  vme->ader  = (inl( reg->ader + 0x00) & 0xff) << 24;
  vme->ader |= (inl( reg->ader + 0x04) & 0xff) << 16;
  vme->ader |= (inl( reg->ader + 0x08) & 0xff) << 8;
  vme->ader |= (inl( reg->ader + 0x0c) & 0xff);
  vme->crcsr = (inl( reg->csr + 0x08) & 0xff);
  vme->bar   = (inl( reg->csr + 0x0c) & 0xff);

  return( 0);
}
 
int
vme_conf_set( struct vme_reg *reg,
	      struct vme_ctl *vme)
{
  uint data;

  data = (vme->arb & 0x303) | 0x404;
  outl( data, reg->ctl + 0x00);
  data = vme->master;
  outl( data, reg->ctl + 0x04);
  data = vme->slave;
  outl( data, reg->ctl + 0x08);
  data = vme->itgen & 0x7ff;
  outl( data, reg->ctl + 0x0c);
  outl( 0, reg->ader + 0x0c);
  data = vme->ader >> 24;
  outl( data, reg->ader + 0x00);
  data = (vme->ader >> 16)&0xf0;
  outl( data, reg->ader + 0x04);

  return( 0);

}
 
int
vme_crcsr_set( uint io_base,
	       int crcsr)
{
  if( crcsr)
  {
    outl( crcsr, io_base + 0x08);
  }
  crcsr = inl( io_base + 0x08) & 0xff;
  return( crcsr);
}
 
int
vme_crcsr_clear( uint io_base,
	         int crcsr )
{
  if( crcsr)
  {
    outl( crcsr, io_base + 0x04);
  }
  crcsr = inl( io_base + 0x04) & 0xff;
  return( crcsr);
}
 
int
vme_itgen_set( uint io_base,
	       uint level,
	       uint vect)
{
  uint data;
  uint tmp;

  tmp = inl( io_base + 0x0c);

  /* set vector */
  if( vect) data = vect;
  else data = tmp & 0xff;

  /* set level */
  if( level) data |= (level&7)<<8;
  else data |= tmp & 0x700;

  data |= 0x1000;
  outl( data, io_base + 0x0c);

  return( tmp);

}

int
vme_itgen_clear( uint io_base)
{
  uint data;
  uint tmp;

  tmp = inl( io_base + 0x0c);

  data = (tmp  & 0x7ff) | 0x2000;
  outl( data, io_base + 0x0c);

  return( tmp);

}

int
vme_itgen_get( uint io_base)
{
  return( inl( io_base + 0x0c));
}

int
vme_it_ack( uint io_base)
{
  return( inl( io_base + 0x00));
}

void
vme_it_enable( uint io_base)
{
  outl( 1, io_base + 0x04);
  return;
}
void
vme_it_disable( uint io_base)
{
  outl( 1, io_base + 0x04);
  return;
}
void
vme_it_clear( uint io_base)
{
  outl( 2, io_base + 0x04);
  return;
}

void
vme_it_unmask( uint io_base,
	       uint mask)
{
  outl( mask, io_base + 0x08);
  return;
}

void
vme_it_mask( uint io_base,
	     uint mask)
{
  outl( mask, io_base + 0x0c);
  return;
}

void
vme_it_restart( uint io_base,
		uint ip)
{
  outl( ip, io_base + 0x00);
  return;
}

int
vme_timer_start( uint io_base,
		 uint mode,
		 struct vme_time *tm)
{
  uint csr;

  csr = inl( io_base + 0x00);
  /* if timer is running... */
  if( csr & 0x80000000)
  {
    /* stop it */
    outl( 0, io_base + 0x00);
  }
  csr = mode | 0x80000080;
  /* if start time... */
  if( tm)
  {
    /* set it */
    outl( tm->time, io_base + 0x0c);
  }
  /* start timer... */
  outl( csr, io_base + 0x00);
  return( 0);
}

void
vme_timer_restart( uint io_base)
{
  uint csr;

  csr = inl( io_base + 0x00);
  if( !( csr&0x80000000))
  {
    csr |=  0x80000000;
    outl( csr, io_base + 0x00);
  }
  return;
}

void
vme_timer_stop( uint io_base)
{
  uint csr;

  csr = inl( io_base + 0x00);
  csr &=  ~0x80000000;
  outl( csr, io_base + 0x00);
  return;
}

void
vme_timer_read( uint io_base,
		struct vme_time *tm)
{
  tm->time = inl( io_base + 0x0c);
  tm->utime = inl( io_base + 0x08);
  return;
}

uint
vme_cmp_swap( uint io_base,
	      uint addr,
              uint cmp,
              uint up,
              uint mode)
{
  int tmo;

  if( (mode&3) == 1)
  {
    uint alg;

    alg = addr&3;
    if( (alg == 0) ||
        (alg == 2)    )
    {
      cmp = cmp<<8;
      up = up<<8;
    }
  }
  if( (mode&3) == 2)
  {
    char ci[2];
    char co[2];

    *(short *)ci = (short)cmp;
    co[0] = ci[1];
    co[1] = ci[0];
    cmp = *(short *)co;
    *(short *)ci = (short)up;
    co[0] = ci[1];
    co[1] = ci[0];
    up = *(short *)co;
  }
  if( (mode&3) == 3)
  {
    char ci[4];
    char co[4];

    *(uint *)ci = (uint)cmp;
    co[0] = ci[3];
    co[1] = ci[2];
    co[2] = ci[1];
    co[3] = ci[0];
    cmp = *(uint *)co;
    *(uint *)ci = (uint)up;
    co[0] = ci[3];
    co[1] = ci[2];
    co[2] = ci[1];
    co[3] = ci[0];
    up = *(uint *)co;
  }
  outl( addr, io_base + 0x2c);
  outl( cmp, io_base + 0x30);
  outl( up, io_base + 0x34);
  mode = 0x80000000 | mode;

  outl( mode, io_base + 0x28);
  tmo = 100000;
  while( (inl(io_base + 0x28) & 0x80000000) && --tmo);
  return( inl(io_base + 0x28));
}

uint
vme_lock( uint io_base,
	  uint addr,
	  uint mode)
{
  return( inl(io_base + 0x24));
}

uint
vme_unlock( uint io_base)
{
  return( inl(io_base + 0x24));
}
