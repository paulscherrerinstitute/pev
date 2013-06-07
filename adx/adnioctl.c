/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : adnioctl.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the all ioctl commmands called from the ADN device 
 *    driver main file (adndrvr.c).
 *    Each function takes a pointer to the ADN1100 device data structure as first
 *    driver argument (struct adn_dev *).
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
 * $Log: adnioctl.c,v $
 * Revision 1.1  2013/06/07 15:03:25  zimoch
 * update to latest version
 *
 * Revision 1.3  2013/04/15 14:35:54  ioxos
 * read/write for SHM, CSR, HCR, SPI + ADN400x [JFG]
 *
 * Revision 1.2  2013/03/14 11:14:11  ioxos
 * rename spi to sflash [JFG]
 *
 * Revision 1.1  2013/03/08 16:04:16  ioxos
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
#include "adnklib.h"
#include "hrmslib.h"

#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif          

int
adn_ioctl_cam( struct adn_dev *adn,
	          unsigned int cmd, 
	          unsigned long arg)
{
  int retval = 0;  

  switch ( cmd)
  {
    case ADN_IOCTL_CAM_RDWR:
    {
      struct adn_ioctl_rdwr rw;
      if( copy_from_user(  (void *)&rw, (void *)arg, sizeof(rw)))
      {
	return( -EFAULT);
      }
      adn_cam_rdwr( adn, &rw);
      break;
    }
    default:
    {
      retval = -EINVAL;
    }
  }
  return( retval);
}

int
adn_ioctl_sflash( struct adn_dev *adn,
	          unsigned int cmd, 
	          unsigned long arg)
{
  int retval = 0;  

  switch ( cmd)
  {
    case ADN_IOCTL_SFLASH_RDWR:
    {
      struct adn_ioctl_rdwr rw;
      if( copy_from_user(  (void *)&rw, (void *)arg, sizeof(rw)))
      {
	return( -EFAULT);
      }
      adn_sflash_rdwr( adn, &rw);
      break;
    }
    default:
    {
      retval = -EINVAL;
    }
  }
  return( retval);
}

int
adn_ioctl_hrms( struct adn_dev *adn,
	           unsigned int cmd, 
	           unsigned long arg)
{
  int retval = 0;  

  switch ( cmd)
  {
    case ADN_IOCTL_HRMS_INIT:
    {
      adn_hrms_init( adn);
      break;
    }
    case ADN_IOCTL_HRMS_CMD:
    {
      struct adn_ioctl_hrms hrms;
      if( copy_from_user(  (void *)&hrms, (void *)arg, sizeof(hrms)))
      {
	return( -EFAULT);
      }
      adn_hrms_cmd( adn, &hrms);
      break;
    }
    case ADN_IOCTL_HRMS_RDWR:
    {
      struct adn_ioctl_rdwr rw;

      if( copy_from_user(  (void *)&rw, (void *)arg, sizeof(rw)))
      {
	return( -EFAULT);
      }
      if(  adn->board == PEV_BOARD_ADN4001)
      {
        adn_hrms_rdwr( adn, &rw);
      }
      else
      {
	adn_shm_rdwr( adn, &rw);
      }
      break;
    }
    case ADN_IOCTL_HRMS_LOAD:
    {
      struct adn_ioctl_hrms_code *hc;

      hc = (struct adn_ioctl_hrms_code *)kmalloc( sizeof( struct adn_ioctl_hrms_code), GFP_KERNEL);
      if( copy_from_user(  (void *)hc, (void *)arg, sizeof(  struct adn_ioctl_hrms_code)))
      {
	return( -EFAULT);
      }
      adn_hrms_load( adn, hc);
      kfree( hc);
      break;
    }
    default:
    {
      retval = -EINVAL;
    }
  }
  return( retval);
}

int
adn_ioctl_rw( struct adn_dev *adn,
	      unsigned int cmd, 
	      unsigned long arg)
{
  int retval = 0;  
  struct adn_ioctl_rw rw;
 
  if( copy_from_user(  (void *)&rw, (void *)arg, sizeof(rw)))
  {
	return( -EFAULT);
  }

  hrms_set_board_type( adn->board);
  switch ( cmd)
  {
    case ADN_IOCTL_RD_CBM:
    {
      if(  adn->board == PEV_BOARD_ADN4001)
      {
#ifdef OLD_ADP4003
        outl( rw.offset, adn->io_base + 0xcc0);
        rw.data = inl( adn->io_base + 0xcc4);
#else
        rw.data = hrms_spi_read( adn->io_base, rw.offset);
#endif
      }
      else
      {
	rw.data = hrms_spi_read( (uint)adn->shm_ptr, rw.offset);
      }
      break;
    }
    case ADN_IOCTL_WR_CBM:
    {
      if(  adn->board == PEV_BOARD_ADN4001)
      {
#ifdef OLD_ADP4003
      outl( rw.offset, adn->io_base + 0xcc0);
      outl( rw.data, adn->io_base + 0xcc4);
#else
      hrms_spi_write( adn->io_base, rw.offset, rw.data);
#endif
      }
      else
      {
	hrms_spi_write( (uint)adn->shm_ptr, rw.offset, rw.data);
      }
      break;
    }
    case ADN_IOCTL_RD_CSR:
    {
      if(  adn->board == PEV_BOARD_ADN4001)
      {
	rw.data = hrms_spi_read( adn->io_base, rw.offset | 0x4000000);
      }
      else
      {
	rw.data = hrms_spi_read( (uint)adn->adr_ptr, rw.offset);
      }
      break;
    }
    case ADN_IOCTL_WR_CSR:
    {
      if(  adn->board == PEV_BOARD_ADN4001)
      {
	hrms_spi_write( adn->io_base, rw.offset | 0x4000000, rw.data);
      }
      else
      {
	hrms_spi_write( (uint)adn->adr_ptr, rw.offset, rw.data);
      }
      break;
    }
    case ADN_IOCTL_RD_HCR:
    {
      if(  adn->board == PEV_BOARD_ADN4001)
      {
	rw.data = hrms_spi_read( adn->io_base, rw.offset | 0x2000000);
      }
      else
      {
	rw.data = hrms_spi_read( (uint)adn->hcr_ptr, rw.offset);
      }
      break;
    }
    case ADN_IOCTL_WR_HCR:
    {
      if(  adn->board == PEV_BOARD_ADN4001)
      {
	hrms_spi_write( adn->io_base, rw.offset | 0x2000000, rw.data);
      }
      else
      {
	hrms_spi_write( (uint)adn->hcr_ptr, rw.offset, rw.data);
      }
      break;
    }
    case ADN_IOCTL_RD_SPI:
    {
      if(  adn->board == PEV_BOARD_ADN4001)
      {
	rw.data = hrms_spi_read( adn->io_base, rw.offset | 0x2000000);
      }
      else
      {
	rw.data = hrms_spi_read( (uint)adn->shm_ptr, rw.offset);
      }
      break;
    }
    case ADN_IOCTL_WR_SPI:
    {
      if(  adn->board == PEV_BOARD_ADN4001)
      {
	hrms_spi_write( adn->io_base, rw.offset | 0x2000000, rw.data);
      }
      else
      {
	hrms_spi_write( (uint)adn->shm_ptr, rw.offset, rw.data);
      }
      break;
    }
    default:
    {
      return( -EINVAL);
    }
  }
  if( ( cmd & 0xf00) == 0x100)
  {
    if( copy_to_user( (void *)arg,  (void *)&rw, sizeof(struct adn_ioctl_rw)))
    {
      return( -EFAULT);
    }
  }
         
  return( retval);
}

