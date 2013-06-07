/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : adndrvr.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : mai 15,2009
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *   This file is the main file of the device driver modules for the PEV1100
 *   hotplug
 *   It contain all entry points for the driver:
 *     -> adn_init()    :
 *     -> adn_exit()    :
 *     -> adn_open()    :
 *     -> adn_release() :
 *     -> adn_ioctl()   :
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
 * $Log: adndrvr.c,v $
 * Revision 1.1  2013/06/07 15:03:25  zimoch
 * update to latest version
 *
 * Revision 1.5  2013/04/15 14:34:20  ioxos
 * add support multiple APX2300/ADN400x [JFG]
 *
 * Revision 1.4  2013/03/14 11:14:11  ioxos
 * rename spi to sflash [JFG]
 *
 * Revision 1.3  2013/03/13 15:37:05  ioxos
 * correct bug in steeing release number [JFG]
 *
 * Revision 1.2  2013/03/13 08:06:06  ioxos
 * set version to ADN_RELEASE [JFG]
 *
 * Revision 1.1  2013/03/08 16:04:36  ioxos
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

#define DBGno

#include "../include/adnioctl.h"
#include "../include/pevioctl.h"
#include "adndrvr.h"
#include "ioctllib.h"
#include "../drivers/pevdrvr.h"


#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

char *adn_version=ADN_RELEASE;

struct adn_drv adn_drv;
struct pev_drv *pev_drv_p;
struct pev_dev *pev;


irqreturn_t
adn_irq( int irq, 
	 void *arg)
{
  struct adn_dev *p;

  p = (struct adn_dev *)arg;  
  return( IRQ_HANDLED);
}


/*
  Ioctl callback
*/
long
adn_ioctl( struct file *filp, 
	  unsigned int cmd, 
	  unsigned long arg)
{
  int retval = 0;              // Will contain the result
  struct adn_dev *adn;

  adn =  (struct adn_dev *)filp->private_data;
  debugk(("adn_ioctl: %x - %lx - %x\n", cmd, arg, adn->io_base));

  switch ( cmd &  ADN_IOCTL_OP_MASK) 
  {
    case ADN_IOCTL_VERSION:
    {
      if( copy_to_user( (void *)arg, (void *)adn_version, 4))
      {
	return( -EFAULT);
      }
      return( 0);
    }
    case ADN_IOCTL_HRMS:
    {
      retval = adn_ioctl_hrms( adn, cmd, arg);
      break;
    }
    case ADN_IOCTL_CAM:
    {
      retval = adn_ioctl_cam( adn, cmd, arg);
      break;
    }
    case ADN_IOCTL_SFLASH:
    {
      retval = adn_ioctl_sflash( adn, cmd, arg);
      break;
    }
    case ADN_IOCTL_RW:/* for writing data to arg */
    {
      retval = adn_ioctl_rw( adn, cmd, arg);
      break;
    }
    default:
    {
      retval = -EINVAL;
    }
  }
  return retval;
}


/*
Open callback
*/
int 
adn_open( struct inode *inode, 
	  struct file *filp)
{
  int minor;
  struct adn_dev *adn;

  minor = iminor(inode);
  debugk(( KERN_ALERT "adn: entering adn_open : %d\n", minor));
  adn = (void *)adn_drv.adn[minor&0x3];
  if( adn)
  {
    filp->private_data = (void *)adn;
    debugk(( KERN_ALERT "board type : %08x\n", adn->board));
    return(0);
  }
  return(-1);
}


/*
Release callback
*/
int adn_release(struct inode *inode, struct file *filp)
{
  debugk(( KERN_ALERT "adn: entering adn_release\n"));
  filp->private_data = (void *)0;
  return 0;
}


// File operations for adn device
struct file_operations adn_fops = 
{
  .owner =    THIS_MODULE,
  .unlocked_ioctl =    adn_ioctl,
  .open =     adn_open,
  .release =  adn_release,
};



/*----------------------------------------------------------------------------
 * Function name : adn_init
 * Prototype     : int
 * Parameters    : none
 * Return        : 0 if OK
 *----------------------------------------------------------------------------
 * Description
 *
 *----------------------------------------------------------------------------*/

static int adn_init( void)
{
  int retval;
  dev_t adn_dev_id;
  //struct pci_dev *ldev;
  struct adn_dev *adn;
  int i, adn_idx;;

  debugk(( KERN_ALERT "adn: entering adn_init\n"));

  /*--------------------------------------------------------------------------
   * device number dynamic allocation 
   *--------------------------------------------------------------------------*/
  retval = alloc_chrdev_region( &adn_dev_id, ADN_MINOR_START, ADN_COUNT, "adn");
  if( retval < 0)
  {
    debugk(( KERN_WARNING "adn: cannot allocate device number\n"));
    goto err_alloc_chrdev;
  }
  else
  {
    debugk((KERN_WARNING "adn: registered with major number:%i\n", MAJOR( adn_dev_id)));
  }

  adn_drv.dev_id = adn_dev_id;

  /*--------------------------------------------------------------------------
   * register device
   *--------------------------------------------------------------------------*/
  cdev_init( &adn_drv.cdev, &adn_fops);
  adn_drv.cdev.owner = THIS_MODULE;
  adn_drv.cdev.ops = &adn_fops;
  retval = cdev_add( &adn_drv.cdev, adn_drv.dev_id ,ADN_COUNT);
  if(retval)
  {
    debugk((KERN_NOTICE "adn : Error %d adding device\n", retval));
    goto err_cdev_add;
  }
  else
  {
    debugk((KERN_NOTICE "adn : device added\n"));
  }

  debugk(("adn: attaching to PEV driver..."));
  pev_drv_p = pev_register();
  debugk(( "%p - %d\n", pev_drv_p, pev_drv_p->pev_local_crate));

  adn_idx = 0;
  for( i = 0; i < 16; i++)
  {
    pev =  pev_drv_p->pev[i];
    if( pev)
    {
      if( pev->board == PEV_BOARD_ADN4001)
      {
        debugk(("ADN4001 found : %08x -%08x [%x]\n", pev->io_base, *(int *)pev->io_base, pev->io_len));
	adn = (struct adn_dev *)kmalloc( sizeof(struct adn_dev), GFP_KERNEL);
	adn_drv.adn[adn_idx++] = adn;
	adn->board = pev->board;
	adn->io_base = pev->io_base;
	adn->csr_ptr = NULL;
	adn->shm_ptr = NULL;
	adn->hcr_ptr = NULL;
	adn->adr_ptr = NULL;
      }
    }
  }
  for( i = 0; i < 16; i++)
  {
    pev =  pev_drv_p->xmc0[i];
    if( pev)
    {
      if( pev->board == PEV_BOARD_APX2300)
      {
        debugk(("APX2300 found on XMC0 : %08x -%08x [%x]\n", pev->io_base, *(int *)pev->io_base, pev->io_len));
	adn = (struct adn_dev *)kmalloc( sizeof(struct adn_dev), GFP_KERNEL);
	adn_drv.adn[adn_idx++] = adn;
	adn->board = pev->board;
	adn->io_base = pev->io_base;
	adn->csr_ptr = pev->csr_ptr;
	adn->shm_ptr = pev->usr_ptr;
	adn->hcr_ptr = pev->usr_ptr + 0xf800;
	adn->adr_ptr = pev->usr_ptr + 0xfc00;
      }
    }
  }
  for( i = 0; i < 16; i++)
  {
    pev =  pev_drv_p->xmc1[i];
    if( pev)
    {
      if( pev->board == PEV_BOARD_APX2300)
      {
        debugk(("APX2300 found on XMC1 : %08x -%08x [%x]\n", pev->io_base, *(int *)pev->io_base, pev->io_len));
	adn = (struct adn_dev *)kmalloc( sizeof(struct adn_dev), GFP_KERNEL);
	adn_drv.adn[adn_idx++] = adn;
	adn->board = pev->board;
	adn->io_base = pev->io_base;
	adn->csr_ptr = pev->csr_ptr;
	adn->shm_ptr = pev->usr_ptr;
	adn->hcr_ptr = pev->usr_ptr + 0xf800;
	adn->adr_ptr = pev->usr_ptr + 0xfc00;
      }
    }
  }
  for( i = 0; i < 16; i++)
  {
    pev =  pev_drv_p->xmc2[i];
    if( pev)
    {
      if( pev->board == PEV_BOARD_APX2300)
      {
        debugk(("APX2300 found on XMC2: %08x -%08x [%x]\n", pev->io_base, *(int *)pev->io_base, pev->io_len));
	adn = (struct adn_dev *)kmalloc( sizeof(struct adn_dev), GFP_KERNEL);
	adn_drv.adn[adn_idx++] = adn;
	adn->board = pev->board;
	adn->io_base = pev->io_base;
	adn->csr_ptr = pev->csr_ptr;
	adn->shm_ptr = pev->usr_ptr;
	adn->hcr_ptr = pev->usr_ptr + 0xf800;
	adn->adr_ptr = pev->usr_ptr + 0xfc00;
      }
    }
  }

  if( !adn_idx) goto err_no_adn;

  printk("adn registers: %p - %p\n", adn->csr_ptr, adn->shm_ptr);
  *(int *)(adn->csr_ptr + 0x1004) = 0x08008000; /* enable APX2300 (swap_32(0x00800008)*/
  //printk("adn identifier: %x\n", *(uint *)adn->adr_ptr);

  /* if everything OK, install realtime driver */
  return( 0);

err_no_adn:
  cdev_del( &adn_drv.cdev);
err_cdev_add:
  unregister_chrdev_region( adn_drv.dev_id, ADN_COUNT);
err_alloc_chrdev:

  return( retval);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Function name : adn_exit
 * Prototype     : int
 * Parameters    : none
 * Return        : 0 if OK
 *----------------------------------------------------------------------------
 * Description
 *
 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static void adn_exit(void)
{
  dev_t adn_dev_id;
  struct adn_dev *adn;
  int i;

  debugk(( KERN_ALERT "adn: entering adn_exit\n"));
  for( i = 0; i < 4; i++)
  {
    adn = adn_drv.adn[i];
    if( adn)
    {
      kfree( adn);
    }
  }
  adn_dev_id = adn_drv.dev_id;
  cdev_del( &adn_drv.cdev);
  unregister_chrdev_region( adn_dev_id, ADN_COUNT);
}

module_init( adn_init);
module_exit( adn_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("IOxOS Technologies [JFG]");
MODULE_VERSION(ADN_RELEASE);
MODULE_DESCRIPTION("driver for IOxOS Technologies A664 interface");
/*================================< end file >================================*/

