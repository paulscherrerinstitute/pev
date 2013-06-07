/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : vmedrvr.c
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
 *     -> vme_init()    :
 *     -> vme_exit()    :
 *     -> vme_open()    :
 *     -> vme_release() :
 *     -> vme_ioctl()   :
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
 * $Log: vmedrvr.c,v $
 * Revision 1.2  2013/06/07 15:03:26  zimoch
 * update to latest version
 *
 * Revision 1.4  2012/12/13 15:46:54  ioxos
 * add irq_wait + buf in ELB window size [JFG]
 *
 * Revision 1.3  2012/11/14 15:21:23  ioxos
 * release 4.23 [JFG]
 *
 * Revision 1.2  2012/10/15 13:40:12  ioxos
 * cleanup code [JFG]
 *
 * Revision 1.1  2012/10/12 12:45:53  ioxos
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
#include "../include/pevioctl.h"
#include "../include/vmeioctl.h"
#include "../drivers/pevdrvr.h"
#include "../drivers/pevklib.h"

#define DBG



#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

struct vme_drv
{
  struct cdev cdev;
  dev_t dev_id;
} vme_drv;

struct pev_drv *pev_drv_p;
struct pev_dev *pev;
uint vme_itc_reg = 0;
char *vme_cpu_addr = NULL;
struct vme_board vme_board;
struct semaphore vme_sem;

void
vme_irq( struct pev_dev *pev,
	 int src,
	 void *arg)
{
  int vec;
  uint data;

  vec = src & 0xff;
  src = src >> 8;

  debugk(("VME interrupt : %x - %x", src, vec));
  if( vme_cpu_addr)
  {
    /* read board base address */
    data = *(uint *)vme_cpu_addr;
    debugk((" - %x\n", data));
  }
  else
  {
    debugk((" - No mapping\n"));
  }
  up( &vme_sem);
  outl( 0xfe, vme_itc_reg+0x8);
}

void
vme_board_register( struct vme_board *v,
		    void (* func)( struct pev_dev*, int, void *),
		    void *arg)

{
  uint elb_off;
  uint vme_am;
  uint vme_size;
  int i, data;

  for( i = 1; i < 8; i++)
  {
    pev_irq_register( pev, EVT_SRC_VME + i, func, arg);
  }
  vme_itc_reg = pev->io_base + pev->io_remap[0].vme_itc;

  if( v->am == 0x0d)      vme_am = MAP_VME_ELB_256M | MAP_VME_ELB_D32 | MAP_VME_ELB_A32 | MAP_VME_ELB_SP;
  else if( v->am == 0x39) vme_am = MAP_VME_ELB_256M | MAP_VME_ELB_D32 | MAP_VME_ELB_A24;
  else if( v->am == 0x3d) vme_am = MAP_VME_ELB_256M | MAP_VME_ELB_D32 | MAP_VME_ELB_A24 | MAP_VME_ELB_SP;
  else if( v->am == 0x29) vme_am = MAP_VME_ELB_256M | MAP_VME_ELB_D32 | MAP_VME_ELB_A16;
  else if( v->am == 0x2d) vme_am = MAP_VME_ELB_256M | MAP_VME_ELB_D32 | MAP_VME_ELB_A16 | MAP_VME_ELB_SP;
  else if( v->am == 0x00) vme_am = MAP_VME_ELB_256M | MAP_VME_ELB_D32 | MAP_VME_ELB_IACK;
  else                    vme_am = MAP_VME_ELB_256M | MAP_VME_ELB_D32 | MAP_VME_ELB_A32;

  vme_size =  (v->size + 0xfff) & 0x0ffff000; /* align to multiple of 4k */
  elb_off = v->base & 0x0ffff000;             /* align to multiple of 4k */
  vme_cpu_addr = ioremap( pev->elb_base + elb_off, vme_size); /* map vme base address through ELB bus */
  vme_cpu_addr +=  (v->base & 0x0fffffff) - elb_off;
  if( pev->csr_ptr)
  {
    data = (vme_am << 24) | ((v->base&0xf0000000) >> 24);
    *(uint *)(pev->csr_ptr + PEV_CSR_VME_ELB) = data;
  }
  else
  {
    data = vme_am | (v->base&0xf0000000);
    if( pev->io_remap[0].short_io)
    {
      outl( data, pev->io_base +  PEV_SCSR_VME_ELB);
    }
    else
    {
      outl( data, pev->io_base +  PEV_CSR_VME_ELB);
    }
  }
  sema_init( &vme_sem, 0);

  return;
}

void
vme_irq_mask( int irq_level)
{
  //outl( 1<<irq_level, vme_itc_reg+0xc);
  outl( 0xfe, vme_itc_reg+0xc);
}

void
vme_irq_unmask( int irq_level)
{
  //outl( 1<<irq_level, vme_itc_reg+0x8);
  outl( 0xfe, vme_itc_reg+0x8);
}

/*
  Ioctl callback
*/
long
vme_ioctl( struct file *filp, 
	  unsigned int cmd, 
	  unsigned long arg)
{
  int retval = 0;              // Will contain the result

  debugk(("vme_ioctl: %x - %lx\n", cmd, arg));
  switch ( cmd) 
  {
    case VME_BOARD_REGISTER:
    {
      if( copy_from_user(&vme_board, (void *)arg, sizeof(struct vme_board)))
      {
	return( -EFAULT);
      }

      vme_board_register( &vme_board, vme_irq, NULL);

      debugk(("VME_IRQ_REGISTER: %p - %x\n", vme_cpu_addr,  *(uint *)(pev->csr_ptr + PEV_CSR_VME_ELB)));
      break;
    }
    case VME_IRQ_MASK:
    {
      debugk(("masking IRQ#%d\n", vme_board.irq));
      if( vme_board.irq == -1)
      {
        debugk(("ERROR -> VME board not registered\n"));
	return( -EINVAL);
      }
      vme_irq_mask( vme_board.irq);
      break;
    }
    case VME_IRQ_UNMASK:
    {
      debugk(("unmasking IRQ#%d\n", vme_board.irq));
      if( vme_board.irq == -1)
      {
        debugk(("ERROR -> VME board not registered\n"));
	return( -EINVAL);
      }
      vme_irq_unmask( vme_board.irq);
      break;
    }
    case VME_IRQ_WAIT:
    {
      retval = down_interruptible( &vme_sem);
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
vme_open( struct inode *inode, 
	  struct file *filp)
{
  int minor;

  debugk(( KERN_ALERT "vme: entering vme_open\n"));
  minor = iminor(inode);
  return(0);
}


/*
Release callback
*/
int vme_release(struct inode *inode, struct file *filp)
{
  debugk(( KERN_ALERT "vme: entering vme_release\n"));
  return 0;
}


// File operations for vme device
struct file_operations vme_fops = 
{
  .owner =    THIS_MODULE,
  .unlocked_ioctl =    vme_ioctl,
  .open =     vme_open,
  .release =  vme_release,
};



/*----------------------------------------------------------------------------
 * Function name : vme_init
 * Prototype     : int
 * Parameters    : none
 * Return        : 0 if OK
 *----------------------------------------------------------------------------
 * Description
 *
 *----------------------------------------------------------------------------*/

static int vme_init( void)
{
  int retval;
  dev_t vme_dev_id;

  debugk(( KERN_ALERT "vme: entering vme_init\n"));

  /*--------------------------------------------------------------------------
   * device number dynamic allocation 
   *--------------------------------------------------------------------------*/
  retval = alloc_chrdev_region( &vme_dev_id, 0, 1, "vme");
  if( retval < 0)
  {
    debugk(( KERN_WARNING "vme: cannot allocate device number\n"));
    goto err_alloc_chrdev;
  }
  else
  {
    debugk((KERN_WARNING "vme: registered with major number:%i\n", MAJOR( vme_dev_id)));
  }

  vme_drv.dev_id = vme_dev_id;

  /*--------------------------------------------------------------------------
   * register device
   *--------------------------------------------------------------------------*/
  cdev_init( &vme_drv.cdev, &vme_fops);
  vme_drv.cdev.owner = THIS_MODULE;
  vme_drv.cdev.ops = &vme_fops;
  retval = cdev_add( &vme_drv.cdev, vme_drv.dev_id ,1);
  if(retval)
  {
    debugk((KERN_NOTICE "vme : Error %d adding device\n", retval));
    goto err_cdev_add;
  }
  else
  {
    debugk((KERN_NOTICE "vme : device added\n"));
  }


  debugk(("vme: attaching to PEV driver..."));
  pev_drv_p = pev_register();
  debugk(( "%p - %d\n", pev_drv_p, pev_drv_p->pev_local_crate));
  pev =  pev_drv_p->pev[pev_drv_p->pev_local_crate];

  memset( (char *)&vme_board, 0, sizeof( vme_board));
  vme_board.irq = -1;

  /* if everything OK, install realtime driver */
  return( 0);

err_cdev_add:
  unregister_chrdev_region( vme_drv.dev_id, 1);
err_alloc_chrdev:

  return( retval);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Function name : vme_exit
 * Prototype     : int
 * Parameters    : none
 * Return        : 0 if OK
 *----------------------------------------------------------------------------
 * Description
 *
 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static void vme_exit(void)
{
  dev_t vme_dev_id;

  debugk(( KERN_ALERT "vme: entering vme_exit\n"));
  if( vme_cpu_addr)
  {
    iounmap(vme_cpu_addr);
  }
  vme_dev_id = vme_drv.dev_id;
  cdev_del( &vme_drv.cdev);
  unregister_chrdev_region( vme_dev_id, 1);
}

module_init( vme_init);
module_exit( vme_exit);

/*================================< end file >================================*/

