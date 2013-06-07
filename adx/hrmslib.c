/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : hrmslib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the low level functions to drive the analog waveform
 *    generator implemented on the ADN box.
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
 * $Log: hrmslib.c,v $
 * Revision 1.1  2013/06/07 15:03:25  zimoch
 * update to latest version
 *
 * Revision 1.4  2013/04/15 14:36:55  ioxos
 * add support multiple APX2300/ADN400x [JFG]
 *
 * Revision 1.3  2013/03/14 11:14:11  ioxos
 * rename spi to sflash [JFG]
 *
 * Revision 1.2  2013/03/13 15:37:47  ioxos
 * support for x86 swapping [JFG]
 *
 * Revision 1.1  2013/03/08 16:04:17  ioxos
 * first checkin [JFG]
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

#include "../include/adnioctl.h"
#include "../include/pevioctl.h"
#include "adndrvr.h"

typedef unsigned char uchar;
uint hrms_board_type = 0;

#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

#define CBM_ADD_OFF  0xcc0
#define CBM_DATA_OFF 0xcc4

#define SPIMAS_CSR    0xcc4
#define SPIMAS_CTL    0xcc8
#define SPIMAS_WRBUF  0xccc
#define SPIMAS_RDBUF  0xcd0
#define SPIMAS_STATUS 0xcd4
#define SPIMAS_USER   0xcd8
#define SPIMAS_CRCS   0xcdc

#define SPACE_SHM    0x0000000
#define SPACE_HCR    0x2000000
#define SPACE_CSR    0x4000000
#define SPACE_SPI    0x6000000
#define SPACE_MASK   0x6000000
#define OFFSET_MASK  0xffff

void
hrms_set_board_type( uint board_type)
{ 
  hrms_board_type = board_type;
  return; 
}

int
hrms_spi_read( uint io_base,
	       uint offset)
{
  uint data;
  uint tmo;

  //printk("in spi_read( %x, %x) -> ", io_base, offset);
  if(  hrms_board_type == PEV_BOARD_ADN4001)
  {
    data = inl( io_base + SPIMAS_CTL);
    tmo = 1000;
    while( data & 0x60000000)
    {
      if( ! --tmo) return( -1);
      data = inl( io_base + SPIMAS_CTL);
    }
    outl( 0x21,  io_base + SPIMAS_CSR);      /* clear read buffer          */
    data = 0x80020000 | (offset & ( SPACE_MASK | OFFSET_MASK));
    outl( data,  io_base + SPIMAS_CTL);      /* start SPI read transaction */
    data = inl( io_base + SPIMAS_CTL);
    tmo = 1000;
    while( data & 0x60000000)
    {
      if( ! --tmo) return( -1);
      data = inl( io_base + SPIMAS_CTL);
    }
    data = inl( io_base + SPIMAS_RDBUF);     /* get data from read buffer */
  }
  else
  {
    data = *(int *)(io_base + (offset&0x7fc));
    data = (( data&0xff)<<24) | (( data&0xff00)<<8) | (( data&0xff0000)>>8) | (( data&0xff000000)>>24);
  }
  //printk("data = %08x\n", data);
  return(data);
}

int
hrms_spi_write( uint io_base,
	        uint offset,
	        uint data)
{
  uint tmo;
  uint tmp;

  //printk("in spi_write( %x, %x, %x)\n", io_base, offset, data);
  if(  hrms_board_type == PEV_BOARD_ADN4001)
  {
    tmp = inl( io_base + SPIMAS_CTL);
    tmo = 1000;
    while( tmp & 0x60000000)
    {
      if( ! --tmo) return( -1);
      tmp = inl( io_base + SPIMAS_CTL);
    }
    outl( 0x11,  io_base + SPIMAS_CSR);      /* clear write buffer          */
    outl( data,  io_base + SPIMAS_WRBUF);    /* load write buffer with data */
    tmp = 0x81020000 | (offset & ( SPACE_MASK | OFFSET_MASK));
    outl( tmp,  io_base + SPIMAS_CTL);      /* start SPI write transaction */
  }
  else
  {
    data = (( data&0xff)<<24) | (( data&0xff00)<<8) | (( data&0xff0000)>>8) | (( data&0xff000000)>>24);
    *(int *)(io_base + (offset&0x7fc)) = data;
  }

  return(0);
}

int
hrms_init( uint io_base)
{

  return( 0);
}

int
hrms_cmd( uint io_base,
	  uint cmd)
{
  printk("in hrms_cmd( %x, %x)\n", io_base, cmd);
  return( 0);
}


int hrms_load_wait( uint io_base)
{
  int tmo;

  tmo = 100;
  while( ( inl( io_base + 0xC28) & 0x8000) && (--tmo > 0));
  if( !tmo) return( -1);
  return( 0);
}
int
hrms_load( uint io_base,
	   struct adn_ioctl_hrms_code *hc)
{
  uint i, *p;
  uint ctl;

  ctl = inl( io_base + 0xC28) & 0x700000;
  ctl |= (hc->dev << 16);                    /* select HRMS */
  outl( ctl, io_base + 0xC28);
  p = &hc->ins[0];
  ctl = ctl | 0x8000 | (hc->offset/8);
  for( i = 0; i < (hc->size/8)-1; i++)
  {
    outl( *p++, io_base + 0xC20);
    outl( *p++, io_base + 0xC24);
    outl( ctl+i, io_base + 0xC28);
    if( hrms_load_wait( io_base)) return( -1);
  }
  outl( *p++, io_base + 0xC20);
  outl( *p++, io_base + 0xC24);
  ctl |= 0x4000;                              /* LastMCI */
  outl( ctl+i, io_base + 0xC28);
  if( hrms_load_wait( io_base)) return( -1);
  return( 0);
}

void
hrms_read_cbm( uint io_base,
               uint offset,
	       char *buf,
	       uint size,
	       char swap)
{
  uint *p, i;

  p = (uint *)buf;
  for( i = 0; i < size; i += 4)
  {
    outl( offset+i, io_base + CBM_ADD_OFF);
    if(swap)
    {
#ifdef PPC
      *p++ =*(uint*)( io_base + CBM_DATA_OFF);
#else
    *p = inl( io_base + CBM_DATA_OFF);
    *p++ = (buf[i+3] << 24) | (buf[i+2] << 16) | (buf[i+1] << 8) | buf[i];
#endif
    }
    else
    {
      *p++ = inl( io_base + CBM_DATA_OFF);
    }
    //p++;
  }
  return;
}

void
hrms_write_cbm( uint io_base,
                uint offset,
	        char *buf,
	        uint size,
		char swap)
{
  uint i;
  uint data;

  for( i = 0; i < size; i += 4)
  {
    outl( offset+i, io_base + CBM_ADD_OFF);
    data = *(uint *)&buf[i];
    if(swap)
    {
#ifdef PPC
      *(uint*)(io_base + CBM_DATA_OFF) = data;
#else
      data = (buf[i+3] << 24) | (buf[i+2] << 16) | (buf[i+1] << 8) | buf[i];
      outl( data, io_base + CBM_DATA_OFF);
#endif
    }
    else
    {
      outl( data, io_base + CBM_DATA_OFF);
    }
  }
  return;
}

void
hrms_read_shm( uint io_base,
               uint offset,
	       char *buf,
	       uint size,
	       char swap)
{
  uint *p, i;

  p = (uint *)buf;
  for( i = 0; i < size; i += 4)
  {
    *p = hrms_spi_read( io_base, SPACE_SHM | (offset+i));
    if(swap)
    {
      *p = (buf[i+3] << 24) | (buf[i+2] << 16) | (buf[i+1] << 8) | buf[i];
    }
    p++;
  }
  return;
}

void
hrms_write_shm( uint io_base,
                uint offset,
	        char *buf,
	        uint size,
		char swap)
{
  uint i;
  uint data;

  for( i = 0; i < size; i += 4)
  {
    data = *(uint *)&buf[i];
    if(swap)
    {
      data = (buf[i+3] << 24) | (buf[i+2] << 16) | (buf[i+1] << 8) | buf[i];
    }
    hrms_spi_write( io_base, SPACE_SHM | (offset+i), data);
  }
  return;
}

void
hrms_read_cam( uint io_base,
               uint offset,
	       unsigned char *buf,
	       uint size)
{
  uint *p, i;

  p = (uint *)buf;
  for( i = 0; i < size; i += 12)
  {
#ifdef XILINX
    *p++ = inl( io_base + 0xc54);
    outl( offset, io_base + 0xc50);
    *p++ = inl( io_base + 0xc54);
    outl( offset+1, io_base + 0xc50);
    *p++ = inl( io_base + 0xc54);
    outl( offset+2, io_base + 0xc50);
#endif
    outl( 0x5040, io_base + CBM_ADD_OFF);
    outl( offset, io_base + CBM_DATA_OFF);
    outl( 0x5048, io_base + CBM_ADD_OFF);
    *p++ = inl( io_base + CBM_DATA_OFF);
    outl( 0x5040, io_base + CBM_ADD_OFF);
    outl( offset+1, io_base + CBM_DATA_OFF);
    outl( 0x5048, io_base + CBM_ADD_OFF);
    *p++ = inl( io_base + CBM_DATA_OFF);
    outl( 0x5040, io_base + CBM_ADD_OFF);
    outl( offset+2, io_base + CBM_DATA_OFF);
    outl( 0x5048, io_base + CBM_ADD_OFF);
    *p++ = inl( io_base + CBM_DATA_OFF);
    offset += 4;
  }
  return;
}

void
hrms_write_cam( uint io_base,
                uint offset,
	        unsigned char *buf,
	        uint size)
{
  uint *p, i;

  p = (uint *)buf;
  printk("hrms_write_cam(%x,%x,%p,%x)\n", io_base, offset, buf, size); 
  printk("cam_buf = %08x - %08x - %08x\n", *p, *(p+1), *(p+2)); 
  for( i = 0; i < size; i += 12)
  {
#ifdef XILINX
    outl( *p++, io_base + 0xc54);
    outl( offset, io_base + 0xc50);
    outl( *p++, io_base + 0xc54);
    outl( offset+1, io_base + 0xc50);
    outl( *p++, io_base + 0xc54);
    outl( offset+2, io_base + 0xc50);
#endif
    outl( 0x5048, io_base + CBM_ADD_OFF);
    outl( *p++, io_base + CBM_DATA_OFF);
    outl( 0x5040, io_base + CBM_ADD_OFF);
    outl( offset, io_base + CBM_DATA_OFF);
    outl( 0x5048, io_base + CBM_ADD_OFF);
    outl( *p++, io_base + CBM_DATA_OFF);
    outl( 0x5040, io_base + CBM_ADD_OFF);
    outl( offset+1, io_base + CBM_DATA_OFF);
    outl( 0x5048, io_base + CBM_ADD_OFF);
    outl( *p++, io_base + CBM_DATA_OFF);
    outl( 0x5040, io_base + CBM_ADD_OFF);
    outl( offset+2, io_base + CBM_DATA_OFF);
    offset += 4;
  }
  return;
}

#define SFLASH_DATA 0x40
#define SFLASH_CMD  0x44
#define SFLASH_LB   0x48
#define SFLASH_MB   0x4c
#define SFLASH_HB   0x50

int
hrms_sts_sflash( uint io_base)
{
  uchar sts;

#ifdef OLD_ADP4003
  /* read status */
  outl( 0x7108, io_base + CBM_ADD_OFF);
  outl( 0x05, io_base + CBM_DATA_OFF);

  /* get data */
  outl( 0x7108, io_base + CBM_ADD_OFF);
  sts = (uchar)inl( io_base + CBM_DATA_OFF);
#else
  /* read status */
  hrms_spi_read( io_base, SPACE_CSR | SFLASH_CMD);
  hrms_spi_write( io_base, SPACE_CSR | SFLASH_CMD, 0x05);
  /* get data */
  sts = (uchar)hrms_spi_read( io_base, SPACE_CSR | SFLASH_CMD);
#endif

  return( (int)sts);
}
int
hrms_read_sflash( uint io_base,
               uint offset,
	       unsigned char *buf,
	       uint size)
{
  uint i, j;
  uint npg, pg_size;
  uchar *p;
  int flush, sts, tmo;

  flush = 1;
  if(  hrms_board_type == PEV_BOARD_ADN4001)
  {
    flush = 0;
  }
  if( (offset & 0xff) || (size & 0xff) || !size)
  {
    return( -1);
  }
  p = (uchar *)buf;
  npg = size >> 8;
  pg_size = 256;
#ifdef OLD_ADP4003
  /* NOP -> make sure last command ended */
  outl( 0x7108, io_base + CBM_ADD_OFF);
  outl( 0xff, io_base + CBM_DATA_OFF);

  for( i = 0; i < npg; i++)
  {
    /* load offset */
    outl( 0x7110, io_base + CBM_ADD_OFF);
    outl( 0, io_base + CBM_DATA_OFF);
    outl( 0x7118, io_base + CBM_ADD_OFF);
    outl( (offset >> 8) & 0xff, io_base + CBM_DATA_OFF);
    outl( 0x7120, io_base + CBM_ADD_OFF);
    outl( (offset >> 16) & 0xff, io_base + CBM_DATA_OFF);
    /* READ */
    outl( 0x7108, io_base + CBM_ADD_OFF);
    outl( 0x3, io_base + CBM_DATA_OFF);
    for( j = 0; j < pg_size; j++)
    {
      /* transfer data */
      outl( 0x7100, io_base + CBM_ADD_OFF);
      *p++ = (uchar)inl( io_base + CBM_DATA_OFF);
    }
    outl( 0x7108, io_base + CBM_ADD_OFF);
    outl( 0xff, io_base + CBM_DATA_OFF);
    offset += pg_size;
  }
#else
  /* NOP -> make sure last command ended */
  hrms_spi_write( io_base, SPACE_CSR | SFLASH_CMD, 0xff);
  tmo = 100000;
  do
  {
    sts = hrms_sts_sflash( io_base);
  }
  while( (sts & 1) && --tmo);
  //printk("sflash status aftert NOP = %x\n", sts);
  //if( flush) hrms_spi_read( io_base, SPACE_CSR | SFLASH_CMD);
  for( i = 0; i < npg; i++)
  {
    /* load page offset */
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_LB, 0x0);
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_MB, (offset >> 8) & 0xff);
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_HB, (offset >> 16) & 0xff);
    if( flush) hrms_spi_read( io_base, SPACE_CSR | SFLASH_HB);
    /* READ one page */
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_CMD, 0x3);
    if( flush) hrms_spi_read( io_base, SPACE_CSR | SFLASH_CMD);
    for( j = 0; j < pg_size; j++)
    {
      /* transfer data */
      *p++ = hrms_spi_read( io_base, SPACE_CSR | SFLASH_DATA);
    }
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_CMD, 0xff);
    tmo = 100000;
    do
    {
      sts = hrms_sts_sflash( io_base);
    }
    while( (sts & 1) && --tmo);
    //printk("sflash status aftert NOP = %x\n", sts);
    //if( flush) hrms_spi_read( io_base, SPACE_CSR | SFLASH_CMD);
    offset += pg_size;
  }
#endif
  return(size);
}

int
hrms_write_sflash( uint io_base,
                uint offset,
	        unsigned char *buf,
	        uint size)
{
  uchar *p;
  int sts;
  uint i, j, tmo;
  uint npg, pg_size;
  int flush;

  if( (offset & 0xff) || (size & 0xff) || !size)
  {
    return( -1);
  }
  p = (uchar *)buf;
  npg = size >> 8;
  pg_size = 256;
  flush = 1;
  if(  hrms_board_type == PEV_BOARD_ADN4001)
  {
    flush = 0;
  }

  //npg = size >> 9;
  //pg_size = 512;
#ifdef OLD_ADP4003
  /* NOP -> make sure last command ended */
  outl( 0x7108, io_base + CBM_ADD_OFF);
  outl( 0xff, io_base + CBM_DATA_OFF);
  /* Write enable */
  outl( 0x7108, io_base + CBM_ADD_OFF);
  outl( 0x06, io_base + CBM_DATA_OFF);
  tmo = 1000;
  do
  {
    sts = hrms_sts_sflash( io_base);
  }
  while( ((sts & 3) != 2) && --tmo);
  if( !tmo)
  {
    printk("ADN ERROR: hrms_write_sflash() -> write enable status = %x\n", sts);
    return( -1);
  }
  for( i = 0; i < npg; i++)
  {
    outl( 0x7108, io_base + CBM_ADD_OFF);
    outl( 0x06, io_base + CBM_DATA_OFF);
    /* load offset */
    outl( 0x7110, io_base + CBM_ADD_OFF);
    outl( 0, io_base + CBM_DATA_OFF);
    outl( 0x7118, io_base + CBM_ADD_OFF);
    outl( (offset >> 8) & 0xff, io_base + CBM_DATA_OFF);
    outl( 0x7120, io_base + CBM_ADD_OFF);
    outl( (offset >> 16) & 0xff, io_base + CBM_DATA_OFF);
    /* PROGRAM PAGE */
    outl( 0x7108, io_base + CBM_ADD_OFF);
    outl( 0x02, io_base + CBM_DATA_OFF);
    for( j = 0; j < pg_size; j++)
    {
      /* transfer data */
      outl( 0x7100, io_base + CBM_ADD_OFF);
      outl( *p++, io_base + CBM_DATA_OFF);
    }
    offset += pg_size;
    outl( 0x7108, io_base + CBM_ADD_OFF);
    outl( 0xff, io_base + CBM_DATA_OFF);
    tmo = 10000;
    do
    {
      sts = hrms_sts_sflash( io_base);
    }
    while( (sts & 1) && --tmo);
  }
#else
  //sts = hrms_sts_sflash( io_base);
  //printk("sflash status before NOP  = %x\n", sts);
  /* NOP -> make sure last command ended */
  hrms_spi_write( io_base, SPACE_CSR | SFLASH_CMD, 0xff);
  tmo = 100000;
  do
  {
    sts = hrms_sts_sflash( io_base);
  }
  while( (sts & 1) && --tmo);
  //printk("sflash status aftert NOP = %x\n", sts);
  /* Write enable */
  hrms_spi_write( io_base, SPACE_CSR | SFLASH_CMD, 0x06);
  sts = hrms_sts_sflash( io_base);
  //printk("sflash status = %x\n", sts);
  tmo = 100000;
  do
  {
    sts = hrms_sts_sflash( io_base);
  }
  while( ((sts & 3) != 2) && --tmo);
  if( !tmo)
  {
    printk("ADN ERROR: hrms_write_sflash() -> write enable status = %x\n", sts);
    return( -1);
  }
  //printk("sflash status after wait WR enable = %x\n", sts);
  for( i = 0; i < npg; i++)
  {
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_CMD, 0x06);
    if( flush) hrms_spi_read( io_base, SPACE_CSR | SFLASH_CMD);
    /* load page offset */
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_LB, 0x0);
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_MB, (offset >> 8) & 0xff);
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_HB, (offset >> 16) & 0xff);
    if( flush) hrms_spi_read( io_base, SPACE_CSR | SFLASH_HB);
    /* PROGRAM PAGE */
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_CMD, 0x02);
    if( flush) hrms_spi_read( io_base, SPACE_CSR | SFLASH_CMD);
    for( j = 0; j < pg_size; j++)
    {
      /* transfer data */
      hrms_spi_write( io_base, SPACE_CSR | SFLASH_DATA, *p++);
      if( flush) hrms_spi_read( io_base, SPACE_CSR | SFLASH_DATA);
    }
    tmo = 1000000;
    do
    {
      sts = hrms_sts_sflash( io_base);
    }
    while( (sts & 1) && --tmo);
    if( !tmo)
    {
      printk("write status = %x\n", sts);
    }
    offset += pg_size;
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_CMD, 0xff);
    tmo = 10000;
    do
    {
      sts = hrms_sts_sflash( io_base);
    }
    while( (sts & 1) && --tmo);
  }
#endif
  return(size);
}

int
hrms_erase_sflash( uint io_base,
                   uint offset,
	           uint size)
{
  int sts;
  uint i, tmo;
  uint npg;
  int flush;
  int sect_size;

  flush = 1;
  sect_size = 0xffff;
  //printk("entering hrms_erase_sflash\n");
  if( (offset & 0xffff) || (size & 0xffff) || !size)
  {
    return( -1);
  }
  if(  hrms_board_type == PEV_BOARD_ADN4001)
  {
    flush = 0;
    sect_size = 0x3ffff;
  }
  //npg = size >> 16;
  npg = size/(sect_size+1);
#ifdef OLD_ADP4003
  /* NOP -> make sure last command ended */
  outl( 0x7108, io_base + CBM_ADD_OFF);
  outl( 0xff, io_base + CBM_DATA_OFF);
  /* Write enable */
  outl( 0x7108, io_base + CBM_ADD_OFF);
  outl( 0x06, io_base + CBM_DATA_OFF);
  tmo = 1000;
  do
  {
    sts = hrms_sts_sflash( io_base);
  }
  while( ((sts & 3) != 2) && --tmo);
  if( !tmo)
  {
    printk("ADN ERROR: hrms_erase_sflash() -> write enable status = %x\n", sts);
  }
  for( i = 0; i < npg; i++)
  {
    /* load offset */
    outl( 0x7110, io_base + CBM_ADD_OFF);
    outl( 0, io_base + CBM_DATA_OFF);
    outl( 0x7118, io_base + CBM_ADD_OFF);
    outl( (offset >> 8) & 0xff, io_base + CBM_DATA_OFF);
    outl( 0x7120, io_base + CBM_ADD_OFF);
    outl( (offset >> 16) & 0xff, io_base + CBM_DATA_OFF);
    /* ERASE */
    outl( 0x7108, io_base + CBM_ADD_OFF);
    outl( 0xd8, io_base + CBM_DATA_OFF);
    tmo = 1000000;
    do
    {
      sts = hrms_sts_sflash( io_base);
    }
    while( (sts & 1) && --tmo);
    if( !tmo)
    {
      printk("erase status = %x\n", sts);
    }
    offset += sect_size + 1;
  }
#else
  /* NOP -> make sure last command ended */
  hrms_spi_write( io_base, SPACE_CSR | SFLASH_CMD, 0xff);
  if( flush) sts = hrms_spi_read( io_base, SPACE_CSR | SFLASH_CMD);
  /* Write enable */
  hrms_spi_write( io_base, SPACE_CSR | SFLASH_CMD, 0x06);
  tmo = 100000;
  do
  {
    sts = hrms_sts_sflash( io_base);
  }
  while( ((sts & 3) != 2) && --tmo);
  if( !tmo)
  {
    printk("ADN ERROR: sflash_erase -> write enable status = %x\n", sts);
  }
  for( i = 0; i < npg; i++)
  {
    /* load page offset */
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_LB, 0x0);
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_MB, (offset >> 8) & 0xff);
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_HB, (offset >> 16) & 0xff);
    /* ERASE */
    hrms_spi_write( io_base, SPACE_CSR | SFLASH_CMD, 0xd8);
    sts = hrms_sts_sflash( io_base);
    //printk("erase status before wait = %x\n", sts);
    tmo = 1000000;
    do
    {
      sts = hrms_sts_sflash( io_base);
    }
    while( (sts & 1) && --tmo);
    if( !tmo)
    {
      printk("erase status = %x\n", sts);
    }
    //printk("erase status after wait = %x\n", sts);
    offset += sect_size + 1;
  }
#endif
  return(size);
}

