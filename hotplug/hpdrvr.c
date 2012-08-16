/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : hpdrvr.c
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
 *     -> hp_init()    :
 *     -> hp_exit()    :
 *     -> hp_open()    :
 *     -> hp_release() :
 *     -> hp_ioctl()   :
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
 * $Log: hpdrvr.c,v $
 * Revision 1.8  2012/08/16 09:11:38  kalantari
 * added version 4.16 of tosca driver
 *
 * Revision 1.5  2011/09/06 09:14:35  ioxos
 * use unlocked_ioctl instead of ioctl [JFG]
 *
 * Revision 1.4  2010/08/16 15:23:05  ioxos
 * save/restore 256 registers [JFG]
 *
 * Revision 1.3  2010/07/13 09:44:43  ioxos
 * change command number [JFG]
 *
 * Revision 1.2  2009/06/04 14:07:49  ioxos
 * don't refer dev->slot for compatibility with old linux kernel [jfg]
 *
 * Revision 1.1  2009/05/18 08:46:51  ioxos
 * first CVS checkin [JFG]
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

#define DBGno



#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

struct hp_drv
{
  struct cdev cdev;
  dev_t dev_id;
  struct pci_dev *pex;
  int ndev;
  char *save_cfg_p;
} hp_drv;




/*
  Ioctl callback
*/
long
hp_ioctl( struct file *filp, 
	  unsigned int cmd, 
	  unsigned long arg)
{
  struct pci_dev *ldev;
  int i;
  int retval = 0;              // Will contain the result

  switch ( cmd) 
  {
    case 10:
    {
      for( i = 0; i < hp_drv.ndev; i++)
      {
        char *p;

        p = hp_drv.save_cfg_p + i*0x1000;
	retval = copy_to_user( (void *)arg, p, 0x100);
	arg += 0x100;
      }
      break;
    }
    case 11:
    {
      ldev = hp_drv.pex;
      for( i = 0; i < hp_drv.ndev; i++)
      {
        char *p;
        int o;

        p = hp_drv.save_cfg_p + i*0x1000;
        o = 0;
        for( o = 0; o < 0x100; o++)
        {
          pci_read_config_byte( ldev, o, p++);
        }
        ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, ldev);
      }
      if( arg)
      {
        for( i = 0; i < hp_drv.ndev; i++)
        {
          char *p;

          p = hp_drv.save_cfg_p + i*0x1000;
	  retval = copy_to_user( (void *)arg, p, 0x100);
	  arg += 0x100;
        }
      }
      break;
    }
    case 12:
    {
      if( arg)
      {
        for( i = 0; i < hp_drv.ndev; i++)
        {
          char *p;

          p = hp_drv.save_cfg_p + i*0x1000;
	  retval = copy_from_user( p, (void *)arg, 0x100);
	  arg += 0x100;
        }
      }
      ldev = hp_drv.pex;
      for( i = 0; i < hp_drv.ndev; i++)
      {
        char *p;
        int o;

        p = hp_drv.save_cfg_p + i*0x1000;
        o = 0;
        for( o = 0; o < 0x100; o++)
        {
          pci_write_config_byte( ldev, o, *p++);
        }
        ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, ldev);
      }
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
hp_open( struct inode *inode, 
	  struct file *filp)
{
  int minor;

  minor = iminor(inode);
  return(0);
}


/*
Release callback
*/
int hp_release(struct inode *inode, struct file *filp)
{
  debugk(( KERN_ALERT "pev: entering hp_release\n"));
  return 0;
}


// File operations for pev device
struct file_operations hp_fops = 
{
  .owner =    THIS_MODULE,
  .unlocked_ioctl =    hp_ioctl,
  .open =     hp_open,
  .release =  hp_release,
};



/*----------------------------------------------------------------------------
 * Function name : hp_init
 * Prototype     : int
 * Parameters    : none
 * Return        : 0 if OK
 *----------------------------------------------------------------------------
 * Description
 *
 *----------------------------------------------------------------------------*/

static int hp_init( void)
{
  int retval;
  dev_t hp_dev_id;
  struct pci_dev *ldev, *pex;
  char s_bus, p_bus, subordinate;
  int ndev;

  debugk(( KERN_ALERT "pev: entering hp_init\n"));

  /*--------------------------------------------------------------------------
   * device number dynamic allocation 
   *--------------------------------------------------------------------------*/
  retval = alloc_chrdev_region( &hp_dev_id, 0, 1, "hppev");
  if( retval < 0)
  {
    debugk(( KERN_WARNING "pev: cannot allocate device number\n"));
    goto err_alloc_chrdev;
  }
  else
  {
    debugk((KERN_WARNING "pev: registered with major number:%i\n", MAJOR( hp_dev_id)));
  }

  hp_drv.dev_id = hp_dev_id;

  /*--------------------------------------------------------------------------
   * register device
   *--------------------------------------------------------------------------*/
  cdev_init( &hp_drv.cdev, &hp_fops);
  hp_drv.cdev.owner = THIS_MODULE;
  hp_drv.cdev.ops = &hp_fops;
  retval = cdev_add( &hp_drv.cdev, hp_drv.dev_id ,1);
  if(retval)
  {
    debugk((KERN_NOTICE "pev : Error %d adding device\n", retval));
    goto err_cdev_add;
  }
  else
  {
    debugk((KERN_NOTICE "pev : device added\n"));
  }


  /* check for PEX86XX switch (cannot be pci probed because already owned) */
  ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, 0);
  pex = (struct pci_dev *)NULL;
  while( ldev)
  {
    /* check for PEX8624 */
    if( ( ( ldev->vendor == 0x10b5) &&  ( ldev->device == 0x8624)) ||
        ( ( ldev->vendor == 0x10b5) &&  ( ldev->device == 0x8616))    )
    {
      /* check for upstream port */
      if( ldev->devfn == 0)
      {
	pex = ldev;
	break;
      }
    }
    ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, ldev);
  }

  /* if PEX8624 not found */
  if( !pex)
  {
    /* return no such device */
    debugk((KERN_NOTICE "pev : didn't find any PEX86XX switch\n"));
    goto err_no_pex86XX;
  }
  hp_drv.pex = pex;
  p_bus = pex->bus->primary;
  s_bus = pex->bus->secondary;
  subordinate = pex->bus->subordinate;
  debugk((KERN_NOTICE "crate 0 : bus number = %x:%x:%x\n", p_bus, s_bus, subordinate));
  p_bus = pex->subordinate->primary;
  s_bus = pex->subordinate->secondary;
  debugk((KERN_NOTICE "crate 0 : bus number = %x:%x\n", p_bus, s_bus));

  ldev = pex;
  ndev = 0;
  while( ldev)
  {
    debugk((KERN_NOTICE "device: %04x:%04x\n", ldev->device, ldev->devfn));
    if( ldev->bus)
    {
      debugk((KERN_NOTICE "  bus: %x:%x\n", ldev->bus->primary, ldev->bus->secondary));
    }
    if( ldev->bus->secondary > pex->bus->subordinate)
    {
      break;
    }
#ifdef jfg
    debugk((KERN_NOTICE "device: %x:%x:%x\n", ldev->slot->bus->primary, 
                                              ldev->slot->number, 
	                                      ldev->device));
#endif
    ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, ldev);
    ndev++;
  }
  hp_drv.ndev = ndev;
  debugk((KERN_NOTICE "number of devices in PPEV1100 tree = %d\n", ndev));
  hp_drv.save_cfg_p = kmalloc( 0x1000 * ndev, GFP_KERNEL);
  memset( hp_drv.save_cfg_p, 0, 0x1000 * ndev);

  /* if everything OK, install realtime driver */
  return( 0);

err_no_pex86XX:
  cdev_del( &hp_drv.cdev);
err_cdev_add:
  unregister_chrdev_region( hp_drv.dev_id, 1);
err_alloc_chrdev:

  return( retval);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Function name : hp_exit
 * Prototype     : int
 * Parameters    : none
 * Return        : 0 if OK
 *----------------------------------------------------------------------------
 * Description
 *
 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static void hp_exit(void)
{
  dev_t hp_dev_id;

  debugk(( KERN_ALERT "pev: entering hp_exit\n"));

  kfree( hp_drv.save_cfg_p);

  hp_dev_id = hp_drv.dev_id;
  cdev_del( &hp_drv.cdev);
  unregister_chrdev_region( hp_dev_id, 1);
}

module_init( hp_init);
module_exit( hp_exit);

/*================================< end file >================================*/

