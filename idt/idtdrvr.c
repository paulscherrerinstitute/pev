/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : idtdrvr.c
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
 *     -> idt_init()    :
 *     -> idt_exit()    :
 *     -> idt_open()    :
 *     -> idt_release() :
 *     -> idt_ioctl()   :
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
 * $Log: idtdrvr.c,v $
 * Revision 1.1  2013/06/07 15:03:26  zimoch
 * update to latest version
 *
 * Revision 1.7  2013/03/08 09:52:21  ioxos
 * bug in freeing idt data struct [JFG]
 *
 * Revision 1.6  2013/02/21 15:27:59  ioxos
 * cosmetics & optimizations -> release 4.28 [JFG]
 *
 * Revision 1.5  2013/02/21 10:58:04  ioxos
 * support for 2 dma channels [JFG]
 *
 * Revision 1.4  2013/02/15 11:30:55  ioxos
 * GPL licence + release 4.27 [JFG]
 *
 * Revision 1.3  2013/02/14 15:16:11  ioxos
 * tagging 4.26 [JFG]
 *
 * Revision 1.2  2013/02/14 10:03:06  ioxos
 * add support for XMC extention + set port 12-15 to 2.5 GT/s [JFG]
 *
 * Revision 1.1  2013/02/05 10:46:16  ioxos
 * first checkin [JFG]
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

#define DBGno

#include "../include/idtioctl.h"
#include "../include/pevioctl.h"
#include "idtdrvr.h"
#include "idtklib.h"
#include "../drivers/pevdrvr.h"


#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

#define DRIVER_VERSION "4.28"
char *idt_version=DRIVER_VERSION;

struct idt_drv idt_drv;
struct pev_drv *pev_drv_p;
struct pev_dev *pev;


irqreturn_t
idt_irq( int irq, 
	 void *arg)
{
  struct idt_dev *p;
  int i, sts, msk;

  p = (struct idt_dev *)arg;  
  debugk(("IDT32NT24 interrupt %d...\n", p->idx));
  sts = *(int *)(p->nt_csr_ptr + 0x460);
  sts = idt_swap_32( sts);
  msk = *(int *)(p->nt_csr_ptr + 0x464);
  msk = idt_swap_32( msk);
  sts = sts & ~msk;
  for( i = 0; i < 4; i++)
  {
    if( sts & (0x10000 << i))
    {
      p->dma_status[i] |= DMA_STATUS_DONE;
      up( &p->sem_msi[i]);
    }
  }
  //*(int *)(p->nt_csr_ptr + 0x464) = idt_swap_32( msk | sts);
  return( IRQ_HANDLED);
}


/*
  Ioctl callback
*/
long
idt_ioctl( struct file *filp, 
	  unsigned int cmd, 
	  unsigned long arg)
{
  int retval = 0;              // Will contain the result
  struct idt_dev *idt;

  idt =  (struct idt_dev *)filp->private_data;
  debugk(("idt_ioctl: %x - %lx - %x\n", cmd, arg, idt->nt_csr_base));

  switch ( cmd) 
  {
    case IDT_IOCTL_VERSION:
    {
      if( copy_to_user( (void *)arg, (void *)idt_version, 4))
      {
	return( -EFAULT);
      }
      return( 0);
    }
    case IDT_IOCTL_MBX_READ:
    {
      struct idt_ioctl_mbx mbx;
      int idx;

      if( copy_from_user(  (void *)&mbx, (void *)arg, sizeof( mbx)))
      {
	return( -EFAULT);
      }

      idx = mbx.idx & 3;

      /* get mailboxes status */
      mbx.sts = *(int *)(idt->nt_csr_ptr + 0x460);
      mbx.sts =  idt_swap_32( mbx.sts);
      /* read INMSG0 */
      mbx.msg = *(int *)(idt->nt_csr_ptr + 0x440 + (idx << 2));
      /* clear INMSG0 status bit */
      *(int *)(idt->nt_csr_ptr + 0x460) = idt_swap_32( 0x10000 << idx);
      if( copy_to_user(  (void *)arg, (void *)&mbx, sizeof( mbx)))
      {
	return( -EFAULT);
      }
      return( 0);
    }
    case IDT_IOCTL_MBX_WRITE:
    {
      struct idt_ioctl_mbx mbx;
      int idx;

      if( copy_from_user(  (void *)&mbx, (void *)arg, sizeof( mbx)))
      {
	return( -EFAULT);
      }

      idx = mbx.idx & 3;

      /* clear mailboxe  status */
      *(int *)(idt->nt_csr_ptr + 0x460) = idt_swap_32( 0x1 << idx);
      /* write OUTMSG */
      *(int *)(idt->nt_csr_ptr + 0x430 + (idx << 2)) = mbx.msg;
      /* get mailboxes status */
      mbx.sts = *(int *)(idt->nt_csr_ptr + 0x460);
      mbx.sts =  idt_swap_32( mbx.sts);
      if( copy_to_user(  (void *)arg, (void *)&mbx, sizeof( mbx)))
      {
	return( -EFAULT);
      }
      return( 0);
    }
    case IDT_IOCTL_MBX_MASK:
    {
      int mask;

      if( copy_from_user(&mask, (void *)arg, sizeof(mask)))
      {
        return( -EFAULT);
      }
      if( mask & 0x80000000)
      {
      /* if bit 31 set -> mask MSI */
	*(int *)(idt->nt_csr_ptr + 0x408) = idt_swap_32( 0xff);
      }
      mask &= 0xf;
      /* mask selected mailboxes */
      *(int *)(idt->nt_csr_ptr + 0x464) = idt_swap_32( mask << 16);

      return( 0);
    }
    case IDT_IOCTL_MBX_UNMASK:
    {
      int mask;

      if( copy_from_user(&mask, (void *)arg, sizeof(mask)))
      {
        return( -EFAULT);
      }
      mask &= 0xf;
      /* clear INMSGx status */
      *(int *)(idt->nt_csr_ptr + 0x460) = idt_swap_32( mask << 16);
      /* unmask selected mailbox */
      *(int *)(idt->nt_csr_ptr + 0x464) = idt_swap_32( ~mask << 16);
      /* unmask MSI */
      *(int *)(idt->nt_csr_ptr + 0x408) = idt_swap_32( 0xfe);

      return( 0);
    }
    case IDT_IOCTL_MBX_WAIT:
    { 
      struct idt_ioctl_mbx mbx;

      if( copy_from_user(  (void *)&mbx, (void *)arg, sizeof( mbx)))
      {
	return( -EFAULT);
      }

      idt_mbx_wait( idt, &mbx);

      if( copy_to_user(  (void *)arg, (void *)&mbx, sizeof( mbx)))
      {
	return( -EFAULT);
      }
      return( 0);
    }
    case IDT_IOCTL_DMA_MOVE:
    { 
      struct idt_ioctl_dma_req req;
      if( copy_from_user(  (void *)&req, (void *)arg, sizeof( req)))
      {
	return( -EFAULT);
      }
      idt_dma_move( idt, &req);
      if( copy_to_user(  (void *)arg, (void *)&req, sizeof( req)))
      {
	return( -EFAULT);
      }

      return( 0);
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
idt_open( struct inode *inode, 
	  struct file *filp)
{
  int minor;
  struct idt_dev *idt;

  minor = iminor(inode);
  debugk(( KERN_ALERT "idt: entering idt_open : %d\n", minor));
  idt = (void *)idt_drv.idt[minor&0x7];
  if( idt)
  {
    filp->private_data = (void *)idt;
    return(0);
  }
  return(-1);
}


/*
Release callback
*/
int idt_release(struct inode *inode, struct file *filp)
{
  debugk(( KERN_ALERT "idt: entering idt_release\n"));
  filp->private_data = (void *)0;
  return 0;
}


// File operations for idt device
struct file_operations idt_fops = 
{
  .owner =    THIS_MODULE,
  .unlocked_ioctl =    idt_ioctl,
  .open =     idt_open,
  .release =  idt_release,
};



/*----------------------------------------------------------------------------
 * Function name : idt_init
 * Prototype     : int
 * Parameters    : none
 * Return        : 0 if OK
 *----------------------------------------------------------------------------
 * Description
 *
 *----------------------------------------------------------------------------*/

static int idt_init( void)
{
  int retval;
  dev_t idt_dev_id;
  struct pci_dev *ldev;
  struct idt_dev *idt;
  int idt_idx;

  debugk(( KERN_ALERT "idt: entering idt_init\n"));

  /*--------------------------------------------------------------------------
   * device number dynamic allocation 
   *--------------------------------------------------------------------------*/
  retval = alloc_chrdev_region( &idt_dev_id, IDT_MINOR_START, IDT_COUNT, "idt");
  if( retval < 0)
  {
    debugk(( KERN_WARNING "idt: cannot allocate device number\n"));
    goto err_alloc_chrdev;
  }
  else
  {
    debugk((KERN_WARNING "idt: registered with major number:%i\n", MAJOR( idt_dev_id)));
  }

  idt_drv.dev_id = idt_dev_id;

  /*--------------------------------------------------------------------------
   * register device
   *--------------------------------------------------------------------------*/
  cdev_init( &idt_drv.cdev, &idt_fops);
  idt_drv.cdev.owner = THIS_MODULE;
  idt_drv.cdev.ops = &idt_fops;
  retval = cdev_add( &idt_drv.cdev, idt_drv.dev_id ,IDT_COUNT);
  if(retval)
  {
    debugk((KERN_NOTICE "idt : Error %d adding device\n", retval));
    goto err_cdev_add;
  }
  else
  {
    debugk((KERN_NOTICE "idt : device added\n"));
  }

  debugk(("idt: attaching to PEV driver..."));
  pev_drv_p = pev_register();
  debugk(( "%p - %d\n", pev_drv_p, pev_drv_p->pev_local_crate));
  pev =  pev_drv_p->pev[0];
  debugk(("pev CSR : %08x - %p -%08x [%x]\n", pev->csr_base,  pev->csr_ptr, *(int *)pev->csr_ptr, pev->csr_len));
  /* point to first PCI device in list */
  ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, 0);
  for( idt_idx = 0; idt_idx < 5; idt_idx++)
  {
    idt_drv.idt[idt_idx] = NULL;;
  }
  idt_idx = 0;
  /* scan device table for PES32NT24 NT port switch  */
  while( ldev)
  {
    /* check for IDT32NT24 */
    if( ( ldev->vendor == 0x111d) &&  
        (( ldev->device == 0x808c) || ( ldev->device == 0x8097)))
    {
      /* check for upstream port */
      if( ldev->bus->number == 1)
      {
	int id;
	int part1;
	int port0, port_nt;
	int nt_port;

	printk("IDT32NT24 Upstream port found on bus %d...", ldev->bus->number);
	idt_drv.dev = ldev;
	idt_csr_rd( ldev, 0, &id);
	printk("id = %08x\n", id);
	/* check if partition 1 enabled */
	idt_csr_rd( ldev, 0x3e124, &part1);
	if((part1 & 0x60) == 0x20)
	{
	  idt_csr_rd( ldev, 0x3e204, &port0);

	  nt_port = -1;
	  /* check for PSI rear extension (NT -> port 12) */
	  idt_csr_rd( ldev, 0x3e384, &port_nt);
	  if( (port_nt & 0x3c0) == 0x0c0)
	  {
	    nt_port = 12;
	  }
	  /* check for XMC extension (NT -> port 16) */
	  else
	  {
	    idt_csr_rd( ldev, 0x3e404, &port_nt);
	    if( (port_nt & 0x3c0) == 0x0c0)
	    {
	      nt_port = 16;
	    }
	  }

	  printk("Slave IFC: %08x - %08x - %08x\n", part1, port0, port_nt);
	  ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, ldev);
	  /* continue PCI device list scan for NT port */
          while( ldev)
          {
	    /* check for NT port #0 giving access to IFC master for DMA transfer */
            if( ( ldev->vendor == 0x111d) &&  (( ldev->device == 0x808c) ||( ldev->device == 0x8097))&&
                ( ldev->subsystem_vendor == 0x7357) &&  ( ldev->subsystem_device == 0x1210) && ( ldev->devfn == 1))
            {
  	      printk("IDT32NT24 NT port #0 found [%d]...\n", ldev->devfn);
              idt = (struct idt_dev *)kmalloc( sizeof(struct idt_dev), GFP_KERNEL);
              memset( idt, 0, sizeof(struct idt_dev));
              idt_drv.idt[0]= idt;
  	      idt->idx = 0;
  	      idt->dev = ldev;
	      /* Get NT Port 0 BAR0 to access idt CSR registers */
	      idt->nt_csr_base =  pci_resource_start( idt->dev, 0);
	      idt->nt_csr_len =  pci_resource_len( idt->dev, 0);
              idt->nt_csr_ptr = NULL;
	      if( idt->nt_csr_len)
	      {
	        idt->nt_csr_ptr = (char *)ioremap(idt->nt_csr_base, idt->nt_csr_len);
	      }
	      idt->dma_csr_base =  pev->csr_base;
	      idt->dma_csr_len =  pev->csr_len;
              idt->dma_csr_ptr = pev->csr_ptr;
	      idt->dma_shm_base =  pev->dma_shm_base;         /* PCI address */
	      idt->dma_shm_offset =  pev->dma_shm_offset;     /* offset in SHM  */
	      idt->dma_shm_len =  pev->dma_shm_len;
              idt->dma_shm_ptr = pev->dma_shm_ptr;
	      printk("idt CSR : %08x - %p - %08x [%x]\n", idt->nt_csr_base,  idt->nt_csr_ptr, *(int *)idt->nt_csr_ptr, idt->nt_csr_len);
	      printk("dma CSR : %08x - %p [%x]\n", idt->dma_csr_base,  idt->dma_csr_ptr, idt->dma_csr_len);
	      printk("dma SHM : %08x - %p - %08x [%x]\n", idt->dma_shm_base,  idt->dma_shm_ptr, idt->dma_shm_offset, idt->dma_shm_len);
	      /* slave IFC -> check NT Port 0 and 12 giving access to master IFC */
	      /* MAP NT Port 12 BAR2 to point on TOSCA CSR registers */
	      if( nt_port == 12) idt_csr_wr( ldev, 0x19498, idt->dma_csr_base); /* if PSI rear extension */
	      if( nt_port == 16) idt_csr_wr( ldev, 0x21498, idt->dma_csr_base); /* if XMC use port 16    */
	      /* MAP Port 12 BAR4 to point on TOSCA DMA shared memory */
	      if( nt_port == 12) idt_csr_wr( ldev, 0x194b8, idt->dma_shm_base); /* if PSI rear extension */
	      if( nt_port == 16) idt_csr_wr( ldev, 0x214b8, idt->dma_shm_base); /* if XMC use port 16    */
	      /* Initialize Requester IDs Mapping Table */
	      *(int *)(idt->nt_csr_ptr + 0x4d0) = idt_swap_32( 0);
	      *(int *)(idt->nt_csr_ptr + 0x4d8) = idt_swap_32( 0x1);
	      *(int *)(idt->nt_csr_ptr + 0x4d0) = idt_swap_32( 0x1);
	      *(int *)(idt->nt_csr_ptr + 0x4d8) = idt_swap_32( 0x20001);
	      *(int *)(idt->nt_csr_ptr + 0x4d0) = idt_swap_32( 0x2);
	      *(int *)(idt->nt_csr_ptr + 0x4d8) = idt_swap_32( 0x601);
	      *(int *)(idt->nt_csr_ptr + 0x4d0) = idt_swap_32( 0x3);
	      *(int *)(idt->nt_csr_ptr + 0x4d8) = idt_swap_32( 0x20601);
	       /* store DMA SHM offset in NTSDATA register for master CPU */
	      idt_csr_wr( ldev, 0x140c, idt->dma_shm_offset);
               /* Enable Requester IDs Translation for NT ports */
	      idt_csr_wr( ldev, 0x1400, 2);
	      if( nt_port == 12) idt_csr_wr( ldev, 0x19400, 2); /* if PSI rear extension */
	      if( nt_port == 16) idt_csr_wr( ldev, 0x21400, 2); /* if XMC use port 16    */
	      idt_idx++;
	      break;
	    }
	    ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, ldev);
	  }
	  break;
	}
	else
	{
	  printk("Master IFC: %08x\n", part1);
	  /* Master IFC -> scan for NT port giving access to slaves IFC */
	  /* MAP Port 0 BAR2 of slave IFC to system memory for DMA transfer */
  	  /* point to next PCI device in list */
	  ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, ldev);
	  /* continue PCI device list scan for NT port */
          while( ldev)
          {
	    /* check for NT ports giving access to IFC slave */
            if( ( ldev->vendor == 0x111d) &&  (( ldev->device == 0x808c) ||( ldev->device == 0x8097))&&
                ( ldev->subsystem_vendor == 0x7357) &&  ( ldev->subsystem_device == 0x1210))
            {
  	      printk("IDT32NT24 NT port found [%d]...\n", ldev->devfn);
              idt = (struct idt_dev *)kmalloc( sizeof(struct idt_dev), GFP_KERNEL);
              memset( idt, 0, sizeof(struct idt_dev));
              idt_drv.idt[idt_idx]= idt;
  	      idt->idx = idt_idx;
  	      idt->dev = ldev;
              retval = pci_enable_device( idt->dev);
              retval = pci_enable_msi( idt->dev);
              if( retval)
              {
                idt->msi = 0;
                debugk(("Cannot enable MSI\n"));
              }
              else
              {
                idt->msi = idt->dev->irq;
                debugk(("MSI enabled : idt irq = %d\n", idt->msi));
              }
              retval = request_irq( idt->dev->irq, idt_irq, IRQF_SHARED, IDT_NAME, idt);
	      sema_init( &idt->sem_msi[0], 0);
	      sema_init( &idt->sem_msi[1], 0);
	      sema_init( &idt->sem_msi[2], 0);
	      sema_init( &idt->sem_msi[3], 0);
	      idt->nt_csr_base =  pci_resource_start( idt->dev, 0);
	      idt->nt_csr_len =  pci_resource_len( idt->dev, 0);
              idt->nt_csr_ptr = NULL;
	      if( idt->nt_csr_len)
	      {
	        idt->nt_csr_ptr = (char *)ioremap(idt->nt_csr_base, idt->nt_csr_len);
	      }
	      printk("idt CSR : %08x - %p - %08x [%x]\n", idt->nt_csr_base,  idt->nt_csr_ptr, *(int *)idt->nt_csr_ptr, idt->nt_csr_len);
	      /* check if local or remote port */
	      if( ldev->devfn == 1)
	      {
		/* if NT port 0 -> DMA resources are local -> get addresses from pev driver */
	        idt->dma_csr_base =  pev->csr_base;
	        idt->dma_csr_len =  pev->csr_len;
                idt->dma_csr_ptr = pev->csr_ptr;
	        idt->dma_shm_base =  pev->dma_shm_base;         /* PCI address */
	        idt->dma_shm_offset =  pev->dma_shm_offset;     /* offset in SHM  */
	        idt->dma_shm_len =  pev->dma_shm_len;
                idt->dma_shm_ptr = pev->dma_shm_ptr;
		/* force dowstream port 12 - 15 to work at 2.5GT/s */
		idt_csr_wr( ldev, 0x18070, 1);
		idt_csr_wr( ldev, 0x18050, 0x20);
		idt_csr_wr( ldev, 0x1a070, 1);
		idt_csr_wr( ldev, 0x1a050, 0x20);
		idt_csr_wr( ldev, 0x1c070, 1);
		idt_csr_wr( ldev, 0x1c050, 0x20);
		idt_csr_wr( ldev, 0x1e070, 1);
		idt_csr_wr( ldev, 0x1e050, 0x20);
	      }
	      else
	      {
		/* if NT port 12 -> DMA resources are remote */
	        idt->dma_csr_base =  pci_resource_start( idt->dev, 2);
	        idt->dma_csr_len =  pci_resource_len( idt->dev, 2);
                idt->dma_csr_ptr = NULL;
	        if( idt->dma_csr_len)
	        {
	          idt->dma_csr_ptr = (char *)ioremap(idt->dma_csr_base, idt->dma_csr_len);
	        }
	        idt->dma_shm_base =  pci_resource_start( idt->dev, 4);
	        idt->dma_shm_len =  pci_resource_len( idt->dev, 4);
                idt->dma_shm_ptr = NULL;
	        if( idt->dma_shm_len)
	        {
	          idt->dma_shm_ptr = (char *)ioremap(idt->dma_shm_base, idt->dma_shm_len);
	        }
	        /* get DMA SHM offset from NTSDATA register(set by slave CPU) */
		idt_csr_rd( ldev, 0x140c, &idt->dma_shm_offset);
	      }
	      printk("dma CSR : %08x - %p [%x]\n", idt->dma_csr_base,  idt->dma_csr_ptr, idt->dma_csr_len);
	      printk("dma SHM : %08x - %p - %08x [%x]\n", idt->dma_shm_base,  idt->dma_shm_ptr, idt->dma_shm_offset, idt->dma_shm_len);
	      idt_idx++;
	      /* get NT port #0 BAR2 and BAR4 of slave IFC needed by DMA engine to target PCI space of master IFC */
	      idt_csr_rd( ldev, 0x1010, &idt->dma_mbx_base);
	      idt_csr_rd( ldev, 0x1018, &idt->dma_des_base[0]);
	      idt_csr_rd( ldev, 0x1020, &idt->dma_des_base[1]);
	      idt->dma_des_base[0] &= 0xfffff000;
	      idt->dma_des_base[1] &= 0xfffff000;
	      printk("dma DES : %08x - %08x : %08x\n", idt->dma_mbx_base,  idt->dma_des_base[0], idt->dma_des_base[1]);
	      idt_dma_init( idt);
	    }
	    ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, ldev);
    	  }
	  break;
	}
      }
    }
    ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, ldev);
  }
  idt_drv.idx_max = idt_idx;
  /* if PEX8624 not found */
  if( !idt_drv.idx_max)
  {
    /* return no such device */
    debugk((KERN_NOTICE "idt : didn't find any IDT32NT24 NT port\n"));
    goto err_no_idt32nt24;
  }
  /* if everything OK, install realtime driver */
  return( 0);

err_no_idt32nt24:
  cdev_del( &idt_drv.cdev);
err_cdev_add:
  unregister_chrdev_region( idt_drv.dev_id, IDT_COUNT);
err_alloc_chrdev:

  return( retval);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Function name : idt_exit
 * Prototype     : int
 * Parameters    : none
 * Return        : 0 if OK
 *----------------------------------------------------------------------------
 * Description
 *
 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static void idt_exit(void)
{
  dev_t idt_dev_id;
  struct idt_dev *idt;
  int i;

  debugk(( KERN_ALERT "idt: entering idt_exit\n"));
  for( i = 0; i < 5; i++)
  {
    idt = idt_drv.idt[i];
    if( idt)
    {
      if( idt->msi)
      {
	free_irq( idt->dev->irq, (void *)idt);
        pci_disable_msi( idt->dev);
      }
      if( idt->dma_ctl)
      {
	idt_dma_exit( idt);
      }
      if( idt->nt_csr_ptr)
      {
	iounmap(idt->nt_csr_ptr);
      }
      kfree( idt);
    }
  }
  idt_dev_id = idt_drv.dev_id;
  cdev_del( &idt_drv.cdev);
  unregister_chrdev_region( idt_dev_id, IDT_COUNT);
}

module_init( idt_init);
module_exit( idt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("IOxOS Technologies [JFG]");
MODULE_VERSION(DRIVER_VERSION);
MODULE_DESCRIPTION("driver for IOxOS Technologies PCI Express Interconnect");
/*================================< end file >================================*/

