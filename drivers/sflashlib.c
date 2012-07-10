/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : sflashlib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the low level functions to drive the SFLAH device
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
 *  $Log: sflashlib.c,v $
 *  Revision 1.9  2012/07/10 10:21:48  kalantari
 *  added tosca driver release 4.15 from ioxos
 *
 *  Revision 1.8  2012/03/14 14:03:19  ioxos
 *  read status should return short [JFG]
 *
 *  Revision 1.7  2012/01/27 13:13:05  ioxos
 *  prepare release 4.01 supporting x86 & ppc [JFG]
 *
 *  Revision 1.6  2009/12/15 17:13:25  ioxos
 *  modification for short io window [JFG]
 *
 *  Revision 1.5  2009/09/29 12:43:38  ioxos
 *  support to read/write sflash status [JFG]
 *
 *  Revision 1.4  2009/04/06 09:53:29  ioxos
 *  remove pevdrvr.h + take register address as first parameter in each function [JFG]
 *
 *  Revision 1.3  2009/01/27 14:37:30  ioxos
 *  remove ref to semaphore.h [JFG]
 *
 *  Revision 1.2  2008/07/04 07:40:13  ioxos
 *  update address mapping functions [JFG]
 *
 *  Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
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

#define SFLASH_CMD_DUMMY 0xA5
#define SFLASH_CMD_WREN  0x06
#define SFLASH_CMD_WRDI  0x04
#define SFLASH_CMD_RDID  0x9F
#define SFLASH_CMD_RDSR  0x05
#define SFLASH_CMD_RDSR2 0x35
#define SFLASH_CMD_WRSR  0x01
#define SFLASH_CMD_READ  0x03
#define SFLASH_CMD_FREAD 0x0B
#define SFLASH_CMD_PP    0x02
#define SFLASH_CMD_SE    0xD8
#define SFLASH_CMD_BE    0xC7

#define SPI_OFFSET    0x00
#define SPI_CLK       0x01
#define SPI_DO        0x02
#define SPI_DI        0x04
#define SPI_CS        0x08
#define SPI_DEV0      0x00
#define SPI_DEV1      0x40
#define SPI_DEV2      0x80
#define SPI_DEV3      0xc0

int sflash_dev = 0;

void
sflash_set_dev( uint dev)
{
  sflash_dev = (dev & 3) << 6;
}

static void
sflash_write_io( uint data,
                 volatile ulong reg_p)
{
  volatile uint tmp;
  if( sflash_dev)
  {
    tmp = *(uint *)reg_p;
    *(uint *)reg_p = sflash_dev | data;
    tmp = *(uint *)reg_p;
  }
  else
  {
    outl( 0x40 | data, reg_p);
  }
  return;
}

static uint
sflash_read_io(  ulong reg_p)
{
  volatile uint data;

  if( sflash_dev)
  {
    data = *(uint *)reg_p;
  }
  else
  {
    data = inl( reg_p);
  }

  return( data);
}


void
sflash_load_cmd( uint io_reg,
                 uint cmd,
		 uint para)
{
  int size;
  uint data;

  switch( cmd)
  {
    case SFLASH_CMD_READ:
    case SFLASH_CMD_FREAD:
    case SFLASH_CMD_PP:
    case SFLASH_CMD_SE:
    {
      cmd = ( cmd << 24) | (para & 0xffffff);
      size = 32;
      break;
    }
    case SFLASH_CMD_WREN:
    case SFLASH_CMD_WRDI:
    case SFLASH_CMD_RDID:
    case SFLASH_CMD_RDSR:
    case SFLASH_CMD_RDSR2:
    case SFLASH_CMD_BE:
    case SFLASH_CMD_DUMMY:
    {
      size = 8;
      break;
    }
    case SFLASH_CMD_WRSR:
    {
      if( sflash_dev)
      {
	cmd = ( cmd << 16) | ((para & 0xff)<<8) | ((para & 0xff00)>>8);
	size = 24;
      }
      else
      {
	cmd = ( cmd << 8) | (para & 0xff);
	size = 16;
      }
      break;
    }
    default:
    {
      return;
    }
  }

  while( size--)
  {
    data = 0;
    if( cmd & ( 1 <<  size)) data = SPI_DO;
    data |= SPI_CLK | SPI_CS;
    sflash_write_io( data, io_reg);
  }

  return;
}

void
sflash_start_cmd( uint io_reg)
{
  sflash_write_io( SPI_CS, io_reg);
}

void
sflash_end_cmd( uint io_reg)
{
  sflash_write_io( 0x0, io_reg);
}

void
sflash_write_byte( uint io_reg,
                   unsigned char b)
{
  uint i, data;

  i = 8;
  while( i--)
  {
    data = 0;
    if( b & ( 1 <<  i)) data = SPI_DO;
    data |= SPI_CLK | SPI_CS;
    sflash_write_io( data, io_reg);
  }

  return;
}

unsigned char
sflash_read_byte( uint io_reg)
{
  unsigned char b;
  uint i, data;

  i = 8;
  b = 0;
  while( i--)
  {
    data = sflash_read_io( io_reg);
    if( data & SPI_DI)
    {
      b |= 1 <<  i;
    }
    sflash_write_io( SPI_CLK | SPI_CS, io_reg);
  }

  return( b);
}

void
sflash_read_ID( uint io_base,
                unsigned char *data_p)
{
  uint io_reg; 
  io_reg = io_base + SPI_OFFSET;

  sflash_start_cmd( io_reg);
  sflash_load_cmd( io_reg, SFLASH_CMD_RDID, 0);
  data_p[0] = sflash_read_byte(io_reg);
  data_p[1] = sflash_read_byte(io_reg);
  data_p[2] = sflash_read_byte(io_reg);
  sflash_end_cmd(io_reg);
  return;
}


unsigned short
sflash_read_status( uint io_base)
{
  unsigned short status;
  uint io_reg;
 
  io_reg = io_base + SPI_OFFSET;

  sflash_start_cmd( io_reg);
  sflash_load_cmd( io_reg, SFLASH_CMD_RDSR2, 0);
  status = (sflash_read_byte( io_reg) << 8) & 0xff00;
  sflash_end_cmd( io_reg);
  sflash_start_cmd( io_reg);
  sflash_load_cmd( io_reg, SFLASH_CMD_RDSR, 0);
  status |= sflash_read_byte( io_reg) & 0xff;
  sflash_end_cmd( io_reg);
  return( status);
}

unsigned char
sflash_wait_busy( uint io_reg,
                  uint tmo)
{
  unsigned char status;

  sflash_start_cmd( io_reg);
  sflash_load_cmd( io_reg, SFLASH_CMD_RDSR, 0);
  do
  {
    status = sflash_read_byte( io_reg);
    if( !tmo--) break; 
  } while( status&1);
  sflash_end_cmd( io_reg);
  return( status);
}


unsigned char
sflash_write_enable( uint io_reg,
                     uint tmo)
{
  unsigned char status;

  sflash_start_cmd( io_reg);
  sflash_load_cmd( io_reg, SFLASH_CMD_WREN, 0);
  sflash_end_cmd( io_reg);
  sflash_start_cmd( io_reg);
  sflash_load_cmd( io_reg, SFLASH_CMD_RDSR, 0);
  do
  {
    status = sflash_read_byte( io_reg);
    if( !tmo--) break; 
  } while( !(status&2));
  sflash_end_cmd( io_reg);
  return( status);
}

void
sflash_write_status( uint io_base,
		     unsigned short status)
{
  uint io_reg;
 
  io_reg = io_base + SPI_OFFSET;

  sflash_write_enable(  io_reg, 0x100);
  sflash_start_cmd( io_reg);
  sflash_load_cmd( io_reg, SFLASH_CMD_WRSR, (uint)status);
  sflash_end_cmd( io_reg);
  return;
}

int
sflash_sector_erase( uint io_base,
                     uint offset)
{
  uint io_reg;
 
  io_reg = io_base + SPI_OFFSET;

  sflash_start_cmd( io_reg);
  sflash_load_cmd(  io_reg, SFLASH_CMD_SE, offset);
  sflash_end_cmd( io_reg);
  if( sflash_wait_busy(  io_reg, 1000000) & 1) return( -1);
  return( 0);
}

int
sflash_page_program( uint io_base,
                     uint offset,
		     unsigned char *p,
		     uint size)
{
  int i;
  uint io_reg;
 
  io_reg = io_base + SPI_OFFSET;

  if( size > 0x100) return( -1);
  sflash_start_cmd( io_reg);
  sflash_load_cmd(  io_reg, SFLASH_CMD_PP, offset);
  for( i = 0; i < size; i++)
  {
    sflash_write_byte(  io_reg, *p++);
  }
  sflash_end_cmd( io_reg);
  if( sflash_wait_busy(  io_reg, 1000000) & 1) return( -1);
  return( 0);
}

void
sflash_read_data( uint io_base,
                  uint offset,
	          unsigned char *buf,
	          uint size)
{
  uint io_reg;
 
  io_reg = io_base + SPI_OFFSET;

  sflash_start_cmd( io_reg);
  sflash_load_cmd( io_reg, SFLASH_CMD_READ, offset);
  while( size--)
  {
    *buf++ = sflash_read_byte( io_reg);
  }
  sflash_end_cmd( io_reg);
  return;
}

int
sflash_write_sector( uint io_base,
                     uint start,
	             unsigned char *buf,
	             uint size,
		     uint sect_size)
{
  int i, retval;
  uint offset;
  unsigned char status;
  uint io_reg;
 
  io_reg = io_base + SPI_OFFSET;

  retval = 0;

  offset = start;
  while( offset < start+size)
  {
    status = sflash_write_enable( io_reg, 0x100);
    if( !(status & 2))
    {
      return( -1);
    }
    if( sflash_sector_erase( io_base, offset) < 0)
    {
      retval = -1;
      goto sflash_write_data_exit;
    }
    for( i = 0; i < (sect_size >> 8); i++)
    {
      sflash_write_enable(  io_reg, 0x100);
      if( sflash_page_program( io_base, offset, buf, 0x100) < 0)
      {
	retval = -1;
        goto sflash_write_data_exit;
      }
      buf += 0x100;
      offset += 0x100;
    }
  }

sflash_write_data_exit:
  /* write disable */
  sflash_start_cmd( io_reg);
  sflash_load_cmd(  io_reg, SFLASH_CMD_WRDI, 0);
  sflash_end_cmd( io_reg);

  return( retval);
}

