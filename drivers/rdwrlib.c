/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : rdwrlib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the low level functions to perform read/write
 *    operations on the resources implemented on the PEV1100.
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
 *  $Log: rdwrlib.c,v $
 *  Revision 1.1  2012/02/14 14:15:45  kalantari
 *  added IoxoS driver and module version 3_13 under drivers and modules
 *
 *  Revision 1.6  2009/04/06 09:49:06  ioxos
 *  remove pevdrvr.h [JFG]
 *
 *  Revision 1.5  2009/01/27 14:37:30  ioxos
 *  remove ref to semaphore.h [JFG]
 *
 *  Revision 1.4  2008/09/17 12:17:32  ioxos
 *  add support for read loop with data check [JFG]
 *
 *  Revision 1.3  2008/07/18 14:26:26  ioxos
 *  correct bug in block transfer of short (16 bit) [JFG]
 *
 *  Revision 1.2  2008/07/04 07:40:13  ioxos
 *  update address mapping functions [JFG]
 *
 *  Revision 1.1.1.1  2008/07/01 09:48:06  ioxos
 *  Import sources for PEV1100 project [JFG]
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

#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

#define I2C_CTL_OFFSET    0x100
#define I2C_CMD_OFFSET    0x104
#define I2C_WRITE_OFFSET  0x108
#define I2C_READ_OFFSET   0x10C

#define I2C_CTL_PEX 0x10f0069

#define I2C_CTL_RUNNING  0x100000
#define I2C_CTL_DONE     0x200000
#define I2C_CTL_ERROR    0x300000
#define I2C_CTL_CMD      0x400000
#define I2C_CTL_WRITE    0x800000
#define I2C_CTL_READ     0xC00000

long
rdwr_swap_64( long data)
{
  char ci[8];
  char co[8];

  *(long *)ci = data;
  co[0] = ci[7];
  co[1] = ci[6];
  co[2] = ci[5];
  co[3] = ci[4];
  co[4] = ci[3];
  co[5] = ci[2];
  co[6] = ci[1];
  co[7] = ci[0];

  return( *(long *)co);
}

int
rdwr_swap_32( int data)
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

short
rdwr_swap_16( short data)
{
  char ci[2];
  char co[2];

  *(short *)ci = data;
  co[0] = ci[1];
  co[1] = ci[0];

  return( *(short *)co);
}


int 
rdwr_cfg_wr( struct pci_dev *dev,
	     void *buf,
	     int offset,
	     struct pev_rdwr_mode *m)
{
  if( m->ds == RDWR_BYTE) /* 8 bit access */
  {
    char tmp;

    if( get_user( tmp, (char *)buf))
    {
      return( -EFAULT);
    }
    if( pci_write_config_byte( dev, offset, tmp))
    {
      return( -EIO);
    }
    return( 0);
  }
  if( m->ds == RDWR_SHORT) /* 16 bit access */
  {
    short tmp;

    if( get_user( tmp, (short *)buf))
    {
      return( -EFAULT);
    }
    if( m->swap) tmp = rdwr_swap_16( tmp);
    if( pci_write_config_word( dev, offset, tmp))
    {
      return( -EIO);
    }
    return( 0);
  }
  if( m->ds == RDWR_INT) /* 32 bit access */
  {
    int tmp;

    if( get_user( tmp, (int *)buf))
    {
      return( -EFAULT);
    }
    if( m->swap) tmp = rdwr_swap_32( tmp);
    if( pci_write_config_dword( dev, offset, tmp))
    {
      return( -EIO);
    }
    return( 0);
  }
  return( -EINVAL);
}

int 
rdwr_cfg_rd( struct pci_dev *dev,
	     void *buf,
	     int offset,
	     struct pev_rdwr_mode *m)
{
  if( m->ds == RDWR_BYTE) /* 8 bit access */
  {
    char tmp;

    if( pci_read_config_byte( dev, offset, &tmp))
    {
      return( -EIO);
    }
    if( put_user( tmp, (char *)buf))
    {
      return( -EFAULT);
    }
    return( 0);
  }
  if( m->ds == RDWR_SHORT) /* 16 bit access */
  {
    short tmp;

    if( pci_read_config_word( dev, offset, &tmp))
    {
      return( -EIO);
    }
    if( m->swap) tmp = rdwr_swap_16( tmp);
    if( put_user( tmp, (short *)buf))
    {
      return( -EFAULT);
    }
    return( 0);
  }
  if( m->ds == RDWR_INT) /* 32 bit access */
  {
    int tmp;

    if( pci_read_config_dword( dev, offset, &tmp))
    {
      return( -EIO);
    }
    if( m->swap) tmp = rdwr_swap_32( tmp);
    if( put_user( tmp, (int *)buf))
    {
      return( -EFAULT);
    }
    return( 0);
  }
  return( -EINVAL);
}

int 
rdwr_io_wr( uint io_base,
            void *buf,
	    int offset,
	    struct pev_rdwr_mode *m)
{
  if( m->ds == RDWR_BYTE) /* 8 bit access */
  {
    char tmp;

    if( get_user( tmp, (char *)buf))
    {
      return( -EFAULT);
    }
    outb( tmp, io_base + offset);
    return( 0);
  }
  if( m->ds == RDWR_SHORT) /* 16 bit access */
  {
    short tmp;

    if( get_user( tmp, (short *)buf))
    {
      return( -EFAULT);
    }
    if( m->swap) tmp = rdwr_swap_16( tmp);
    outw( tmp, io_base + offset);
    return( 0);
  }
  if( m->ds == RDWR_INT) /* 32 bit access */
  {
    int tmp;

    if( get_user( tmp, (int *)buf))
    {
      return( -EFAULT);
    }
    if( m->swap) tmp = rdwr_swap_32( tmp);
    outl( tmp, io_base + offset);
    return( 0);
  }
  return( -EINVAL);
}

int 
rdwr_io_rd( uint io_base,
	    void *buf,
	    int offset,
	    struct pev_rdwr_mode *m)
{
  if( m->ds == RDWR_BYTE) /* 8 bit access */
  {
    char tmp;

    tmp = inb( io_base + offset);
    if( put_user( tmp, (char *)buf))
    {
      return( -EFAULT);
    }
    return( 0);
  }
  if( m->ds == RDWR_SHORT) /* 16 bit access */
  {
    short tmp;

    tmp = inw( io_base + offset);
    if( m->swap) tmp = rdwr_swap_16( tmp);
    if( put_user( tmp, (short *)buf))
    {
      return( -EFAULT);
    }
    return( 0);
  }
  if( m->ds == RDWR_INT) /* 32 bit access */
  {
    int tmp;

    tmp = inl( io_base + offset);
    if( m->swap) tmp = rdwr_swap_32( tmp);
    if( put_user( tmp, (int *)buf))
    {
      return( -EFAULT);
    }
    return( 0);
  }
  return( -EINVAL);
}

int 
rdwr_wr_sgl( void *buf,
	     void *addr,
	     struct pev_rdwr_mode *m)
{
  if( m->ds == RDWR_BYTE) /* 8 bit access */
  {
    char tmp;

    if( get_user( tmp, (char *)buf))
    {
      return( -EFAULT);
    }
    *(char *)addr = tmp;
    return( 0);
  }
  if( m->ds == RDWR_SHORT) /* 16 bit access */
  {
    short tmp;

    if( get_user( tmp, (short *)buf))
    {
      return( -EFAULT);
    }
    if( m->swap) tmp = rdwr_swap_16( tmp);
    *(short *)addr = tmp;
    return( 0);
  }
  if( m->ds == RDWR_INT) /* 32 bit access */
  {
    int tmp;

    if( get_user( tmp, (int *)buf))
    {
      return( -EFAULT);
    }
    if( m->swap) tmp = rdwr_swap_32( tmp);
    *(int *)addr = tmp;
    return( 0);
  }
  if( m->ds == RDWR_LONG) /* 64 bit access */
  {
    long tmp;

    if( get_user( tmp, (long *)buf))
    {
      return( -EFAULT);
    }
    if( m->swap) tmp = rdwr_swap_64( tmp);
    *(long *)addr = tmp;
    return( 0);
  }
  return( -EINVAL);
}

int 
rdwr_rd_sgl( void *buf,
	     void *addr,
	     struct pev_rdwr_mode *m)
{
  if( m->ds == RDWR_BYTE) /* 8 bit access */
  {
    char tmp;

    tmp = *(char *)addr;
    if( put_user( tmp, (char *)buf))
    {
      return( -EFAULT);
    }
    return( 0);
  }
  if( m->ds == RDWR_SHORT) /* 16 bit access */
  {
    short tmp;

    tmp = *(short *)addr;
    if( m->swap) tmp = rdwr_swap_16( tmp);
    if( put_user( tmp, (short *)buf))
    {
      return( -EFAULT);
    }
    return( 0);
  }
  if( m->ds == RDWR_INT) /* 32 bit access */
  {
    int tmp;

    tmp = *(int *)addr;

    if( m->swap) tmp = rdwr_swap_32( tmp);
    if( put_user( tmp, (int *)buf))
    {
      return( -EFAULT);
    }
    return( 0);
  }
  if( m->ds == RDWR_LONG) /* 64 bit access */
  {
    long tmp;

    tmp = *(long *)addr;

    if( m->swap) tmp = rdwr_swap_64( tmp);
    if( put_user( tmp, (long *)buf))
    {
      return( -EFAULT);
    }
    return( 0);
  }
  return( -EINVAL);
}

int 
rdwr_wr_blk( void *u_buf,
	     void *addr,
	     int len,
	     struct pev_rdwr_mode *m)
{
  void *k_buf;
  int retval;
  int nbuf;
  int buf_cnt;
  int last_cnt;
  int i;

  buf_cnt = 0x1000;
  k_buf = kmalloc( buf_cnt, GFP_KERNEL);
  if( !k_buf)
  {
    return(-ENOMEM);
  }

  retval = 0;
  nbuf = len/buf_cnt;
  last_cnt = len % buf_cnt;

  if( m->ds == RDWR_BYTE) /* 8 bit access */
  {
    while( nbuf--)
    {
      if( copy_from_user( k_buf, u_buf, buf_cnt))
      {
	retval = -EIO;
	break;
      }
      for( i = 0; i < buf_cnt; i += m->ds)
      {
	*(char *)addr = *(char *)(k_buf+i);
	addr += m->ds;
      }
      u_buf += buf_cnt;
    }
    if( !retval && last_cnt)
    {
      if( !copy_from_user( k_buf, u_buf, last_cnt))
      {
        for( i = 0; i < last_cnt; i += m->ds)
        {
	  *(char *)addr = *(char *)(k_buf+i);
	  addr += m->ds;
	}
      }
      else
      {
	retval = -EIO;
      }
    }
  }
  if( m->ds == RDWR_SHORT) /* 16 bit access */
  {
    short tmp;

    while( nbuf--)
    {
      if( copy_from_user( k_buf, u_buf, buf_cnt))
      {
	retval = -EIO;
	break;
      }
      for( i = 0; i < buf_cnt; i += m->ds)
      {
	tmp = *(short *)(k_buf+i);
	if( m->swap) tmp = rdwr_swap_16( tmp);
	*(short *)addr = tmp;
	addr += m->ds;
      }
      u_buf += buf_cnt;
    }
    if( !retval && last_cnt)
    {
      if( !copy_from_user( k_buf, u_buf, last_cnt))
      {
        for( i = 0; i < last_cnt; i += m->ds)
        {
	  tmp = *(short *)(k_buf+i);
	  if( m->swap) tmp = rdwr_swap_16( tmp);
	  *(short *)addr = tmp;
	  addr += m->ds;
	}
      }
      else
      {
	retval = -EIO;
      }
    }
  }
  if( m->ds == RDWR_INT) /* 32 bit access */
  {
    int tmp;

    while( nbuf--)
    {
      if( copy_from_user( k_buf, u_buf, buf_cnt))
      {
	retval = -EIO;
	break;
      }
      for( i = 0; i < buf_cnt; i += m->ds)
      {
	tmp = *(int *)(k_buf+i);
	if( m->swap) tmp = rdwr_swap_32( tmp);
	*(int *)addr = tmp;
	addr += m->ds;
      }
      u_buf += buf_cnt;
    }
    if( !retval && last_cnt)
    {
      if( !copy_from_user( k_buf, u_buf, last_cnt))
      {
        for( i = 0; i < last_cnt; i += m->ds)
        {
  	  tmp = *(int *)(k_buf+i);
	  if( m->swap) tmp = rdwr_swap_32( tmp);
	  *(int *)addr = tmp;
	  addr += m->ds;
	}
      }
      else
      {
	retval = -EIO;
      }
    }
  }
  if( m->ds == RDWR_LONG) /* 64 bit access */
  {
    long tmp;

    while( nbuf--)
    {
      if( copy_from_user( k_buf, u_buf, buf_cnt))
      {
	retval = -EIO;
	break;
      }
      for( i = 0; i < buf_cnt; i += m->ds)
      {
	tmp = *(long *)(k_buf+i);
	if( m->swap) tmp = rdwr_swap_64( tmp);
	*(long *)addr = tmp;
	addr += m->ds;
      }
      u_buf += buf_cnt;
    }
    if( !retval && last_cnt)
    {
      if( !copy_from_user( k_buf, u_buf, last_cnt))
      {
        for( i = 0; i < last_cnt; i += m->ds)
        {
  	  tmp = *(long *)(k_buf+i);
	  if( m->swap) tmp = rdwr_swap_64( tmp);
	  *(long *)addr = tmp;
	  addr += m->ds;
	}
      }
      else
      {
	retval = -EIO;
      }
    }
  }
  kfree( k_buf);
  return( retval);
}

int 
rdwr_rd_blk( void *u_buf,
	     void *addr,
	     int len,
	     struct pev_rdwr_mode *m)
{
  void *k_buf;
  int retval;
  int nbuf;
  int buf_cnt;
  int last_cnt;
  int i;

  buf_cnt = 0x1000;
  k_buf = kmalloc( buf_cnt, GFP_KERNEL);
  if( !k_buf)
  {
    return( ENOMEM);
  }

  retval = 0;
  nbuf = len/buf_cnt;
  last_cnt = len % buf_cnt;

  if( m->ds == RDWR_BYTE) /* 8 bit access */
  {
    while( nbuf--)
    {
      for( i = 0; i < buf_cnt; i += m->ds)
      {
	*(char *)(k_buf+i) = *(char *)addr;
	addr += m->ds;
      }
      if( copy_to_user( u_buf, k_buf, buf_cnt))
      {
	retval = -EIO;
	break;
      }
      u_buf += buf_cnt;
    }
    if( !retval && last_cnt)
    {
      for( i = 0; i < last_cnt; i += m->ds)
      {
	*(char *)(k_buf+i) = *(char *)addr;
	addr += m->ds;
      }
      if( copy_to_user( u_buf, k_buf, last_cnt))
      {
	retval = -EIO;
      }
    }
  }
  if( m->ds == RDWR_SHORT) /* 16 bit access */
  {
    short tmp;
    while( nbuf--)
    {
      for( i = 0; i < buf_cnt; i += m->ds)
      {
	tmp = *(short *)addr;
	if( m->swap) tmp = rdwr_swap_16( tmp);
	*(short *)(k_buf+i) = tmp;
	addr += m->ds;
      }
      if( copy_to_user( u_buf, k_buf, buf_cnt))
      {
	retval = -EIO;
	break;
      }
      u_buf += buf_cnt;
    }
    if( !retval && last_cnt)
    {
      for( i = 0; i < last_cnt; i += m->ds)
      {
	tmp = *(short *)addr;
	if( m->swap) tmp = rdwr_swap_16( tmp);
	*(short *)(k_buf+i) = tmp;
	addr += m->ds;
      }
      if( copy_to_user( u_buf, k_buf, last_cnt))
      {
	retval = -EIO;
      }
    }
  }
  if( m->ds == RDWR_INT) /* 32 bit access */
  {
    int tmp;

    while( nbuf--)
    {
      /* move data from device to kernel buffer */
      for( i = 0; i < buf_cnt; i += m->ds)
      {
	tmp = *(int *)addr;
	if( m->swap) tmp = rdwr_swap_32( tmp);
	*(int *)(k_buf+i) = tmp;
	addr += m->ds;
      }
      /* move data from kernel buffer to user buffer */
      if( copy_to_user( u_buf, k_buf, buf_cnt))
      {
	retval = -EIO;
	break;
      }
      u_buf += buf_cnt;
    }
    if( !retval && last_cnt)
    {
      for( i = 0; i < last_cnt; i += m->ds)
      {
	/* move data from device to kernel buffer */
	tmp = *(int *)addr;
	if( m->swap) tmp = rdwr_swap_32( tmp);
	*(int *)(k_buf+i) = tmp;
	addr += m->ds;
      }
      /* move data from kernel buffer to user buffer */
      if( copy_to_user( u_buf, k_buf, last_cnt))
      {
	retval = -EIO;
      }
    }
  }
  if( m->ds == RDWR_LONG) /* 64 bit access */
  {
    long tmp;

    while( nbuf--)
    {
      /* move data from device to kernel buffer */
      for( i = 0; i < buf_cnt; i += m->ds)
      {
	tmp = *(long *)addr;
	if( m->swap) tmp = rdwr_swap_64( tmp);
	*(long *)(k_buf+i) = tmp;
	addr += m->ds;
      }
      /* move data from kernel buffer to user buffer */
      if( copy_to_user( u_buf, k_buf, buf_cnt))
      {
	retval = -EIO;
	break;
      }
      u_buf += buf_cnt;
    }
    if( !retval && last_cnt)
    {
      for( i = 0; i < last_cnt; i += m->ds)
      {
	/* move data from device to kernel buffer */
	tmp = *(long *)addr;
	if( m->swap) tmp = rdwr_swap_64( tmp);
	*(long *)(k_buf+i) = tmp;
	addr += m->ds;
      }
      /* move data from kernel buffer to user buffer */
      if( copy_to_user( u_buf, k_buf, last_cnt))
      {
	retval = -EIO;
      }
    }
  }
  return( retval);
}


int 
rdwr_loop( void *buf,
	   volatile void *addr,
	   int len,
	   struct pev_rdwr_mode *m)
{
  int retval;

  retval = 0;

  if( m->ds == RDWR_BYTE) /* 8 bit access */
  {
    volatile char tmp_rd;
    volatile char tmp_wr;


    switch( m->dir)
    {
      case RDWR_LOOP_READ:
      {
        while( len--)
        {
          tmp_rd = *(char *)addr;
        }
	break;
      }
      case RDWR_LOOP_WRITE:
      {
        if( get_user( tmp_wr, (char *)buf))
        {
          return( -EFAULT);
        }
        while( len--)
        {
          *(char *)addr = tmp_wr;
        }
	break;
      }
      case RDWR_LOOP_RDWR:
      {
        if( get_user( tmp_wr, (char *)buf))
        {
          return( -EFAULT);
        }
        while( len--)
        {
          tmp_rd = *(char *)addr;
          *(char *)addr = tmp_wr;
        }
	break;
      }
      case RDWR_LOOP_CHECK:
      {
        if( get_user( tmp_wr, (char *)buf))
        {
          return( -EFAULT);
        }
        while( len--)
        {
          tmp_rd = *(char *)addr;
	  if( tmp_rd != tmp_wr)
	  {
	    put_user( tmp_rd, (char *)buf);
	    return( -EFAULT);
	  }
        }
	break;
      }
    }
  }
  if( m->ds == RDWR_SHORT) /* 16 bit access */
  {
    volatile short tmp_rd;
    volatile short tmp_wr;


    switch( m->dir)
    {
      case RDWR_LOOP_READ:
      {
        while( len--)
        {
          tmp_rd = *(short *)addr;
        }
	break;
      }
      case RDWR_LOOP_WRITE:
      {
        if( get_user( tmp_wr, (short *)buf))
        {
          return( -EFAULT);
        }
        while( len--)
        {
          *(short *)addr = tmp_wr;
        }
	break;
      }
      case RDWR_LOOP_RDWR:
      {
        if( get_user( tmp_wr, (short *)buf))
        {
          return( -EFAULT);
        }
        while( len--)
        {
          tmp_rd = *(short *)addr;
          *(short *)addr = tmp_wr;
        }
	break;
      }
      case RDWR_LOOP_CHECK:
      {
        if( get_user( tmp_wr, (short *)buf))
        {
          return( -EFAULT);
        }
        while( len--)
        {
          tmp_rd = *(short *)addr;
	  if( tmp_rd != tmp_wr)
	  {
	    put_user( tmp_rd, (short *)buf);
	    return( -EFAULT);
	  }
        }
	break;
      }
    }
  }
  if( m->ds == RDWR_INT) /* 32 bit access */
  {
    volatile int tmp_rd;
    volatile int tmp_wr;

    switch( m->dir)
    {
      case RDWR_LOOP_READ:
      {
        while( len--)
        {
          tmp_rd = *(int *)addr;
        }
	break;
      }
      case RDWR_LOOP_WRITE:
      {
        if( get_user( tmp_wr, (int *)buf))
        {
          return( -EFAULT);
        }
        while( len--)
        {
          *(int *)addr = tmp_wr;
        }
	break;
      }
      case RDWR_LOOP_RDWR:
      {
        if( get_user( tmp_wr, (int *)buf))
        {
          return( -EFAULT);
        }
        while( len--)
        {
          tmp_rd = *(int *)addr;
          *(int *)addr = tmp_wr;
        }
	break;
      }
      case RDWR_LOOP_CHECK:
      {
        if( get_user( tmp_wr, (int *)buf))
        {
          return( -EFAULT);
        }
        while( len--)
        {
          tmp_rd = *(int *)addr;
	  if( tmp_rd != tmp_wr)
	  {
	    put_user( tmp_rd, (int *)buf);
	    return( -EFAULT);
	  }
        }
	break;
      }
    }
  }
  if( m->ds == RDWR_LONG) /* 64 bit access */
  {
    volatile long tmp_rd;
    volatile long tmp_wr;


    switch( m->dir)
    {
      case RDWR_LOOP_READ:
      {
        while( len--)
        {
          tmp_rd = *(long *)addr;
        }
	break;
      }
      case RDWR_LOOP_WRITE:
      {
        if( get_user( tmp_wr, (long *)buf))
        {
          return( -EFAULT);
        }
        while( len--)
        {
          *(long *)addr = tmp_wr;
        }
	break;
      }
      case RDWR_LOOP_RDWR:
      {
        if( get_user( tmp_wr, (long *)buf))
        {
          return( -EFAULT);
        }
        while( len--)
        {
          tmp_rd = *(long *)addr;
          *(long *)addr = tmp_wr;
        }
	break;
      }
      case RDWR_LOOP_CHECK:
      {
        if( get_user( tmp_wr, (long *)buf))
        {
          return( -EFAULT);
        }
        while( len--)
        {
          tmp_rd = *(long *)addr;
	  if( tmp_rd != tmp_wr)
	  {
	    put_user( tmp_rd, (long *)buf);
	    return( -EFAULT);
	  }
        }
	break;
      }
    }
  }

  return( retval);
}
