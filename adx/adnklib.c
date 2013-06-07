/*=========================< begin file & file header >=======================
 *  References 
 *  
 *    filename : adnklib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the all functions called from the ADN device 
 *    driver main file (adndrvr.c).
 *    Each function takes a pointer to the ADN device data structure as first
 *    argument (struct adn_dev *).
 *    To perform the requested operation, these functions rely on a set of low
 *    level libraries:
 *      -> acqlib.c
 *      -> awglib.c
 *      -> dmalib.c
 *      -> i2clib.c
 *      -> sflashlib.c
 *      -> timerlib.c
 *
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
 * $Log: adnklib.c,v $
 * Revision 1.1  2013/06/07 15:03:25  zimoch
 * update to latest version
 *
 * Revision 1.3  2013/04/15 14:36:37  ioxos
 * add support multiple APX2300/ADN400x [JFG]
 *
 * Revision 1.2  2013/03/14 11:14:11  ioxos
 * rename spi to sflash [JFG]
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
#include "hrmslib.h"
#include "adndrvr.h"
#include "adnklib.h"

#define DBG

#ifdef DBGno
#define debugk(x) printk x
#else
#define debugk(x) 
#endif



int
adn_hrms_init( struct adn_dev *adn)
{
  hrms_set_board_type( adn->board);
  return( hrms_init( adn->io_base));
}

int
adn_hrms_cmd( struct adn_dev *adn,
		 struct adn_ioctl_hrms *hrms)
{
  int cmd;

  cmd = 0;
  hrms_set_board_type( adn->board);
  return( hrms_cmd( adn->io_base, cmd));
}


int
adn_hrms_load( struct adn_dev *adn,
		  struct adn_ioctl_hrms_code *hc)
{
  hrms_set_board_type( adn->board);
  return( hrms_load( adn->io_base, hc));
}

int
adn_hrms_rdwr( struct adn_dev *adn,
		  struct adn_ioctl_rdwr *rw)
{
  unsigned char *k_buf;
  int retval, order;

  if( rw->len <= 0)
  {
    return(-EINVAL);
  }
  retval = 0;
  hrms_set_board_type( adn->board);
  order = get_order( rw->len);
  k_buf = (unsigned char *)__get_free_pages( GFP_KERNEL, order);
  if( !k_buf)
  {
    return(-ENOMEM);
  }

  if( rw->mode.dir == RDWR_READ)
  {
#ifdef OLD_ADP4003
    hrms_read_cbm( adn->io_base, rw->offset, k_buf, rw->len, rw->mode.swap);
#else
    hrms_read_shm( adn->io_base, rw->offset, k_buf, rw->len, rw->mode.swap);
#endif

    if( copy_to_user( rw->buf, k_buf, rw->len))
    {
      retval = -EIO;
    }
  }
  else
  {
    if( copy_from_user( k_buf, rw->buf, rw->len))
    {
      free_pages( (unsigned long)k_buf, order);
      return( -EIO);
    }
#ifdef OLD_ADP4003
    hrms_write_cbm( adn->io_base, rw->offset, k_buf, rw->len, rw->mode.swap);
#else
    hrms_write_shm( adn->io_base, rw->offset, k_buf, rw->len, rw->mode.swap);
#endif
  }

  free_pages(  (unsigned long)k_buf, order);

  return( retval);
}

int
adn_shm_rdwr( struct adn_dev *adn,
	      struct adn_ioctl_rdwr *rw)
{
  unsigned char *k_buf;
  int *p, *q;
  int retval, order;
  int i, data, size;

  if( (rw->len <= 0) || ( rw->len > 0xf800) || ( rw->offset & 7))
  {
    return(-EINVAL);
  }
  retval = 0;
  hrms_set_board_type( adn->board);
  order = get_order( rw->len);
  k_buf = (unsigned char *)__get_free_pages( GFP_KERNEL, order);
  if( !k_buf)
  {
    return(-ENOMEM);
  }

  if( rw->mode.dir == RDWR_READ)
  {
    size = (rw->len + 7) & 0xfff8;
    p = (int *)adn->shm_ptr +  rw->offset;
    q = (uint *)k_buf;
    for( i = 0; i < size; i += 4)
    {
      *q = *p;
      if( rw->mode.swap)
      {
        *q = (k_buf[i+3] << 24) | (k_buf[i+2] << 16) | (k_buf[i+1] << 8) | k_buf[i];
      }
      p++; q++;
    }
    if( copy_to_user( rw->buf, k_buf, rw->len))
    {
      retval = -EIO;
    }
  }
  else
  {
    if( copy_from_user( k_buf, rw->buf, rw->len))
    {
      free_pages( (unsigned long)k_buf, order);
      return( -EIO);
    }
    size = (rw->len + 7) & 0xfff8;
    p = (int *)adn->shm_ptr +  rw->offset;
    for( i = 0; i < size; i += 4)
    {
      data = *(uint *)&k_buf[i];
      if(  rw->mode.swap)
      {
        data = (k_buf[i+3] << 24) | (k_buf[i+2] << 16) | (k_buf[i+1] << 8) | k_buf[i];
      }
      *p++ = data;
    }
  }

  free_pages(  (unsigned long)k_buf, order);

  return( retval);
}


int
adn_cam_rdwr( struct adn_dev *adn,
		 struct adn_ioctl_rdwr *rw)
{
  unsigned char *k_buf;
  int retval, order;

  retval = 0;
  order = get_order( rw->len);
  k_buf = (unsigned char *)__get_free_pages( GFP_KERNEL, order);
  if( !k_buf)
  {
    return(-ENOMEM);
  }

  hrms_set_board_type( adn->board);

  if( rw->mode.dir == RDWR_READ)
  {
    hrms_read_cam( adn->io_base, rw->offset, k_buf, rw->len);

    if( copy_to_user( rw->buf, k_buf, rw->len))
    {
      retval = -EIO;
    }
  }
  else
  {
    if( copy_from_user( k_buf, rw->buf, rw->len))
    {
      free_pages( (unsigned long)k_buf, order);
      return( -EIO);
    }
    hrms_write_cam( adn->io_base, rw->offset, k_buf, rw->len);
  }

  free_pages(  (unsigned long)k_buf, order);

  return( retval);
}

int
adn_sflash_rdwr( struct adn_dev *adn,
		 struct adn_ioctl_rdwr *rw)
{
  unsigned char *k_buf;
  int retval, order;
  uint s_start;         /* start of first sector                    */
  uint s_end;           /* end of last sector                       */
  uint k_size;          /* total size to be erased and reprogrammed */
  uint first;           /* data offset in first sector              */
  uint last ;           /* data offset in last sector              */
  
  retval = 0;
  order = get_order( rw->len);
  k_buf = (unsigned char *)__get_free_pages( GFP_KERNEL, order);
  if( !k_buf)
  {
    return(-ENOMEM);
  }

  hrms_set_board_type( adn->board);
  if( rw->mode.dir == RDWR_READ)
  {
    if (!rw->len)
    {
      return( -EINVAL);
    }
    s_start = rw->offset & 0xfffff00;
    first = rw->offset & 0xff;
    s_end = (rw->offset + rw->len) & 0xfffff00;
    last =  (rw->offset + rw->len) & 0xff;

    if( last)
    {
      s_end += 0x100;
    }

    if( s_end > 0x1000000)
    {
      return( -EFAULT);
    }
    k_size = s_end - s_start;
    order = get_order( k_size);
    k_buf = (unsigned char *)__get_free_pages( GFP_KERNEL, order);
    if( !k_buf)
    {
      return(-ENOMEM);
    }
    if( adn->adr_ptr)
    {
      hrms_read_sflash( (uint)adn->adr_ptr, s_start, k_buf, k_size);
    }
    else 
    {
      hrms_read_sflash( adn->io_base, s_start, k_buf, k_size);
    }

    if( copy_to_user( rw->buf, k_buf + first, rw->len))
    {
      retval = -EIO;
    }
  }
  if( rw->mode.dir == RDWR_WRITE)
  {
    int sect_size;

    if (!rw->len)
    {
      return( -EINVAL);
    }
    sect_size = 0xffff;
    if(  adn->board == PEV_BOARD_ADN4001) sect_size = 0x3ffff;
    s_start = rw->offset & (~sect_size);
    first = rw->offset & 0xffff;
    s_end = (rw->offset + rw->len) & (~sect_size);
    last =  (rw->offset + rw->len) & sect_size;

    if( last)
    {
      s_end += sect_size + 1;
    }

    if( s_end > 0x1000000)
    {
      return( -EFAULT);
    }
    k_size = s_end - s_start;
    order = get_order( k_size);
    k_buf = (unsigned char *)__get_free_pages( GFP_KERNEL, order);
    if( !k_buf)
    {
      return(-ENOMEM);
    }
    //printk("sflash_write(): %x %x %x %x %x\n", s_start, s_end, k_size, first, last);
    if( adn->adr_ptr)
    {
      hrms_read_sflash( (uint)adn->adr_ptr, s_start, k_buf, k_size);
    }
    else 
    {
      hrms_read_sflash( adn->io_base, s_start, k_buf, k_size);
    }

    if( copy_from_user( k_buf + first, rw->buf, rw->len))
    {
      free_pages( (unsigned long)k_buf, order);
      return( -EIO);
    }
    if( adn->adr_ptr)
    {
      hrms_erase_sflash( (uint)adn->adr_ptr, s_start, k_size);
      hrms_write_sflash( (uint)adn->adr_ptr, s_start, k_buf, k_size);
    }
    else
    {
      hrms_erase_sflash( adn->io_base, s_start, k_size);
      hrms_write_sflash( adn->io_base, s_start, k_buf, k_size);
    }
  }
  if( rw->mode.dir == RDWR_CLEAR)
  {
    if( adn->adr_ptr)
    {
      hrms_erase_sflash( (uint)adn->adr_ptr, rw->offset, rw->len);
    }
    else
    {
      hrms_erase_sflash( adn->io_base, rw->offset, rw->len);
    }
  }

  free_pages(  (unsigned long)k_buf, order);

  return( retval);
}

