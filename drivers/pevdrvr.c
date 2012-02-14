/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pevdrvr.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *   This file is the main file of the device driver modules for the PEV1100
 *   It contain all entry points for the driver:
 *     -> pev_init()    :
 *     -> pev_exit()    :
 *     -> pev_probe()   :
 *     -> pev_remove()  :
 *     -> pev_open()    :
 *     -> pev_release() :
 *     -> pev_read()    :
 *     -> pev_write()   :
 *     -> pev_llseek()  :
 *     -> pev_ioctl()   :
 *     -> pev_mmap()    :
 *     -> pev_irq()     :
 *   It contain all entry points for the xenomai real time driver:
 *     -> pev_nrt_open()    :
 *     -> pev_nrt_close()   :
 *     -> pev_rt_ioctl()    :
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
 * $Log: pevdrvr.c,v $
 * Revision 1.1  2012/02/14 14:15:45  kalantari
 * added IoxoS driver and module version 3_13 under drivers and modules
 *
 * Revision 1.30  2012/01/06 14:40:15  ioxos
 * release 3.13 [JFG]
 *
 * Revision 1.29  2012/01/06 13:38:20  ioxos
 * support for X86 32 bit [JFG]
 *
 * Revision 1.28  2011/12/06 13:15:18  ioxos
 * support for multi task VME IRQ [JFG]
 *
 * Revision 1.27  2011/10/19 14:06:05  ioxos
 * support for powerpc + release 3.11 [JFG]
 *
 * Revision 1.26  2011/10/03 09:57:02  ioxos
 * release 1.10 [JFG]
 *
 * Revision 1.25  2011/09/06 09:11:13  ioxos
 * use unlocked_ioctl instead of ioctl [JFG]
 *
 * Revision 1.24  2011/03/03 15:42:15  ioxos
 * support for 1MBytes VME slave granularity [JFG]
 *
 * Revision 1.23  2010/11/29 15:16:32  ioxos
 * modif for Eurocopter/T&S (cosmetic) [JFG]
 *
 * Revision 1.22  2010/10/05 09:00:02  ioxos
 * bus number shall be unsigned char [JFG]
 *
 * Revision 1.21  2010/07/13 09:39:10  ioxos
 * page size for master windows are static [JFG]
 *
 * Revision 1.20  2010/06/11 11:31:55  ioxos
 * support mapping of kernel memory in mmap() [JFG]
 *
 * Revision 1.19  2010/01/08 11:20:41  ioxos
 * add exit function for dma [JFG]
 *
 * Revision 1.18  2009/12/15 17:13:24  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.17  2009/10/21 08:51:28  ioxos
 * Disable SERR generation for PEX8624 upstream port [cm]
 *
 * Revision 1.16  2009/10/08 13:34:12  ioxos
 * correct loc_base in map structure [JFG]
 *
 * Revision 1.15  2009/07/17 13:29:29  ioxos
 * init port 8 and 9 only on PEX8624 [JFG]
 *
 * Revision 1.14  2009/06/04 13:24:23  ioxos
 * use buf_alloc instead of dma_alloc [JFG]
 *
 * Revision 1.13  2009/06/02 11:45:11  ioxos
 * add driver identification + protect xeno/linux ioctl [jfg]
 *
 * Revision 1.12  2009/05/20 08:06:49  ioxos
 * add pev_rt_ioctl + cleanup [JFG]
 *
 * Revision 1.11  2009/04/14 14:00:06  ioxos
 * mmap for PMEM + disable interrupt when exit + VME base according to crate number [JFG]
 *
 * Revision 1.10  2009/04/06 12:13:33  ioxos
 * move ioctl code to pevioctl.c + support Xenomai + support multi crate[JFG]
 *
 * Revision 1.9  2009/01/27 14:37:30  ioxos
 * remove ref to semaphore.h [JFG]
 *
 * Revision 1.8  2009/01/09 13:13:06  ioxos
 * set no debug for official realease [JFG]
 *
 * Revision 1.7  2008/12/12 13:21:05  jfg
 * support forIRQ table and DMA + ISA mode [JFG]
 *
 * Revision 1.6  2008/11/12 08:56:32  ioxos
 *  mask VME BERR transmission to host, enable PCI master, add ioctl for timer, mapping and bit setting [JFG]
 *
 * Revision 1.5  2008/09/17 11:27:20  ioxos
 * add support for VME configuration [JFG]
 *
 * Revision 1.4  2008/08/08 08:54:18  ioxos
 * allow access to PEX86xx and PLX8112 and allocate buffers for DMA [JFG]
 *
 * Revision 1.3  2008/07/18 14:55:44  ioxos
 * initialze A32 slave + interrupt debugging + cleanup [JFG]
 *
 * Revision 1.2  2008/07/04 07:40:12  ioxos
 * update address mapping functions [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:06  ioxos
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

#define DBGno

#ifdef XENOMAI
#include <rtdm/rtdm_driver.h>
#endif

#include "../include/pevioctl.h"
#include "pevdrvr.h"
#include "pevklib.h"
#include "ioctllib.h"
#include "histolib.h"


#ifdef DBG
#define debugk(x) printk x
#define MARK( addr, data)  *(int *)(addr) = data

#else
#define debugk(x) 
#define MARK( addr, data)
#endif

#ifdef PPC
short rdwr_swap_16( short);
int rdwr_swap_32( int);
#define SWAP16(x) rdwr_swap_16(x)
#define SWAP32(x) rdwr_swap_32(x)
#else
#define SWAP16(x) x
#define SWAP32(x) x
#endif

struct pev_drv pev_drv;
char *pev_version="3.13";


struct pev_drv *
pev_register( void)
{
  return( &pev_drv);
}
EXPORT_SYMBOL( pev_register);



#ifdef XENOMAI
#define PEV_RT_NAME		"pev_rt"
#define SOME_SUB_CLASS		4711
char *pev_drv_id = "pev-xeno";
int pev_irq( rtdm_irq_t *);
#else
irqreturn_t pev_irq( int, void *);
char *pev_drv_id = "pev-linux";
#endif

void
pev_irq_spurious( struct pev_dev *p,
		  int src,
		  void *arg)
{
#ifdef XENOMAI
#else
  debugk(("PEV1100 spurious interrupt : %x\n", src));
#endif
  return;
}



/*
  interrupt handler
*/
#ifdef XENOMAI

int
pev_irq( rtdm_irq_t *rtdm_irq)
{
  struct pev_dev *p;
  register uint ip;
  register uint base;
  register uint src;

  p = rtdm_irq_get_arg( rtdm_irq, struct pev_dev);  

#else

irqreturn_t 
pev_irq( int irq, 
	 void *arg)
{
  struct pev_dev *p;
  register uint ip;
  register uint base;
  register uint src;

  p = (struct pev_dev *)arg;  

#endif

  /* generate IACK cycle */
  base =  p->io_base  + p->io_remap.iloc_itc;
  ip = inl(  base);
  MARK( p->mem_ptr+0x00, 0xa5a5a5a5);

  /* get interrupt source */
  src = (ip >> 8) & 0x3f;
  MARK( p->mem_ptr+0x0c, src);

  //base += (( ip >> 2) & 0xc00);
  if( p->io_remap.short_io)
  {
    base += (( ip >> PEV_SCSR_ITC_SHIFT) & PEV_SCSR_ITC_MASK);
  }
  else
  {
    base += (( ip >> PEV_CSR_ITC_SHIFT) & PEV_CSR_ITC_MASK);
  }
  ip = 1 << ((ip>>8)&0xf);
  p->irq_pending = ip;
  MARK( p->mem_ptr+0x04, ip);

  /* mask interrupt source */
  outl( ip, base + 0xc);

  /* increment interrupt count */
  p->irq_cnt += 1;
  MARK( p->mem_ptr+0x08, p->irq_cnt);

  /* activates tasket handling interrupts */
  p->irq_tbl[src].func( p, src, p->irq_tbl[src].arg);

  /* clear IP and restart interrupt scanning */
  outl( ip << 16, base);

#ifdef XENOMAI
  return RTDM_IRQ_HANDLED;
#else
  return( IRQ_HANDLED);
#endif
}

/*
  Mmap callback
*/
int 
pev_mmap( struct file *filp, 
	  struct vm_area_struct *vma)
{   
  int minor;
  ssize_t size;
  off_t off;
  struct pev_dev *pev;

  pev =  (struct pev_dev *)filp->private_data;;
  minor = iminor( filp->f_dentry->d_inode);
  debugk(( KERN_ALERT "pev: entering pev_mmap %lx %lx %lx\n",
	   vma->vm_start, vma->vm_end, vma->vm_pgoff));

  size = vma->vm_end - vma->vm_start;
  off = vma->vm_pgoff << PAGE_SHIFT;

#if defined(PPC) || defined(X86_32)
  if( off < 0x80000000)
  {
    off &= 0xffffffff;
    remap_pfn_range( vma, 
		     vma->vm_start,
		     off >> PAGE_SHIFT,
		     size,
		     vma->vm_page_prot); 
    return( 0);
  }

  if( (off & 0xc0000000) == 0x80000000)
  {
    off &= 0x3fffffff;
    if( off < (ulong)pev->mem_len)
    {
      remap_pfn_range( vma, 
		       vma->vm_start,
		       (((ulong)pev->mem_base + off) >> PAGE_SHIFT),
		       size,
		       vma->vm_page_prot); 
      return( 0);
    }
  }

  if( (off & 0xc0000000) == 0xc0000000)
  {
    off &= 0x3fffffff;
    if( off < (ulong)pev->pmem_len)
    {
      remap_pfn_range( vma, 
		       vma->vm_start,
		       (((ulong)pev->pmem_base + off) >> PAGE_SHIFT),
		       size,
		       vma->vm_page_prot); 
      return( 0);
    }
  }
#else
  if( off < (ulong)pev->mem_len)
  {
    remap_pfn_range( vma, 
		     vma->vm_start,
		     ((ulong)pev->mem_base >> PAGE_SHIFT) + vma->vm_pgoff,
		     size,
		     vma->vm_page_prot); 
    return( 0);
  }

  if( off & 0x100000000)
  {
    off &= 0xffffffff;
    if( off < (ulong)pev->pmem_len)
    {
      remap_pfn_range( vma, 
		       vma->vm_start,
		       ( pev->pmem_base + off) >> PAGE_SHIFT,
		       size,
		       vma->vm_page_prot); 
      return( 0);
    }
  }
  if( off & 0x200000000)
  {
    off &= 0xffffffff;
    remap_pfn_range( vma, 
		     vma->vm_start,
		     off >> PAGE_SHIFT,
		     size,
		     vma->vm_page_prot); 
    return( 0);
  }
#endif
  return( -1);
}

/*
  Seek callback
*/
loff_t 
pev_llseek( struct file *filp, 
	           loff_t off,
	           int whence)
{   
  int minor;
  struct pev_dev *pev;

  pev =  (struct pev_dev *)filp->private_data;;
  minor = iminor( filp->f_dentry->d_inode);
  debugk(( KERN_ALERT "pev: entering pev_llseek\n"));
  return( off);
}
/*
  Read callback
*/
ssize_t 
pev_read( struct file *filp, 
	  char __user *buf, 
	  size_t count,
	  loff_t *f_pos)
{
  struct pev_dev *pev;
  ssize_t retval = 0;              // Will contain the result
  int minor;

  pev =  (struct pev_dev *)filp->private_data;;
  minor = iminor( filp->f_dentry->d_inode);
  debugk(( KERN_ALERT "pev: entering pev_read [%d]\n", minor));
  return retval;
}

/*
  Write callback
*/
ssize_t 
pev_write( struct file *filp, 
	   const char __user *buf, 
	   size_t count, 
	   loff_t *f_pos)
{
  struct pev_dev *pev;
  ssize_t retval = 0;              // Will contain the result
  int minor;

  pev =  (struct pev_dev *)filp->private_data;;
  minor = iminor( filp->f_dentry->d_inode);
  debugk(( KERN_ALERT "pev: entering pev_write [%d]\n", minor));
  return retval;
}



/*
  Ioctl callback
*/
long 
pev_ioctl( struct file *filp, 
	   unsigned int cmd, 
	   unsigned long arg)
{
  struct pev_dev *pev;
  int retval = 0;              // Will contain the result

  pev =  (struct pev_dev *)filp->private_data;;
  switch ( cmd &  PEV_IOCTL_OP_MASK) 
  {
    case PEV_IOCTL_ID:
    {
      if( arg)
      {
	retval = copy_to_user( (void *)arg, pev_drv_id, strlen( pev_drv_id));
      }
      break;
    }
    case PEV_IOCTL_VERSION:
    {
      if( copy_to_user( (void *)arg, (void *)pev_version, 4))
      {
	return( -EFAULT);
      }
      break;
    }
    case PEV_IOCTL_IO_REMAP:
    {
      if( arg)
      {
	retval = copy_to_user( (void *)arg, &pev->io_remap, sizeof( struct pev_reg_remap));
      }
      break;
    }
    case PEV_IOCTL_TIMER:
    {
#ifdef XENOMAI
      retval = -EINVAL;
#else
      retval = pev_ioctl_timer( pev, cmd, arg);
#endif
      break;
    }
    case PEV_IOCTL_VME:
    {
      retval = pev_ioctl_vme( pev, cmd, arg);
      break;
    }
    case PEV_IOCTL_I2C:
    {
      retval = pev_ioctl_i2c( pev, cmd, arg);
      break;
    }
    case PEV_IOCTL_SFLASH:
    {
      retval = pev_ioctl_sflash( pev, cmd, arg);
      break;
    }
    case PEV_IOCTL_EVT:
    {
      retval = pev_ioctl_evt( pev, cmd, arg);
      break;
    }
    case PEV_IOCTL_BUF:
    {
      retval = pev_ioctl_buf( pev, cmd, arg);
      break;
    }
    case PEV_IOCTL_DMA:
    {
#ifdef XENOMAI
      retval = -EINVAL;
#else
      retval = pev_ioctl_dma( pev, cmd, arg);
#endif
      break;
    }
    case PEV_IOCTL_HISTO:
    {
      retval = pev_ioctl_histo( pev, cmd, arg);
      break;
    }
    case PEV_IOCTL_MAP:/* for writing data to arg */
    {
      retval = pev_ioctl_map( pev, cmd, arg);
      break;
    }

    case PEV_IOCTL_REG:/* for writing data to arg */
    {
      retval = pev_ioctl_reg( pev, cmd, arg);
      break;
    }

    case PEV_IOCTL_RW:/* for writing data to arg */
    {
      retval = pev_ioctl_rw( pev, cmd, arg);
      break;
    }
    case PEV_IOCTL_RDWR:/* for writing data to arg */
    {
      retval = pev_ioctl_rdwr( pev, cmd, arg);
      break;
    }
    default:
    {
      retval = -EINVAL;
    }
  }
  return retval;
}

#ifdef XENOMAI
int 
pev_ioctl_rt( struct rtdm_dev_context *ctx,
	      rtdm_user_info_t *user_info, 
	      unsigned int cmd,
	      void *arg)
{
  struct pev_dev *pev;
  int crate;
  int retval = 0;              // Will contain the result

  crate = (cmd >> 28)&0xf;
  pev = (void *)pev_drv.pev[crate];
  if( !pev)
  {
    return( -1);
  }
  debugk(("in pev_ioctl_nrt() crate %d - %x...\n", pev->crate, *addr));

  pev->user_info = user_info;
  cmd &= 0xfffffff;
  switch ( cmd &  PEV_IOCTL_OP_MASK) 
  {
    case PEV_IOCTL_DMA:
    {
      retval = pev_ioctl_dma( pev, cmd, (unsigned long)arg);
      break;
    }
    case PEV_IOCTL_TIMER:
    {
      retval = pev_ioctl_timer( pev, cmd, (unsigned long)arg);
      break;
    }
    default:
    {
      retval = -EINVAL;
    }
  }
  return retval;
}
#endif

/*
Open callback
*/
int 
pev_open( struct inode *inode, 
	  struct file *filp)
{
  int minor;
  struct pev_dev *pev;

  minor = iminor(inode);
  debugk(( KERN_ALERT "pev_open( %px, %px, %d)\n", inode, filp, minor));

  pev = (void *)pev_drv.pev[minor&0xf];
  if( pev)
  {
    filp->private_data = (void *)pev;
    return(0);
  }
  filp->private_data = (void *)0;
  return(-1);
}

#ifdef XENOMAI
static int 
pev_open_nrt( struct rtdm_dev_context *context,
	       rtdm_user_info_t * user_info, 
               int oflags)
{
  debugk(("in pev_open_nrt()...\n"));
  return 0;
}
#endif

/*
Release callback
*/
int pev_release(struct inode *inode, struct file *filp)
{
  debugk(( KERN_ALERT "pev: entering pev_release\n"));
  filp->private_data = (void *)0;
  return 0;
}

#ifdef XENOMAI
static int 
pev_close_nrt( struct rtdm_dev_context *context,
	       rtdm_user_info_t * user_info)
{
  debugk(("in pev_close_nrt()...\n"));
  return 0;
}
#endif

// File operations for pev device
struct file_operations pev_fops = 
{
  .owner =    THIS_MODULE,
  .mmap =     pev_mmap,
  .llseek =   pev_llseek,
  .read =     pev_read,
  .write =    pev_write,
  .unlocked_ioctl =    pev_ioctl,
  .open =     pev_open,
  .release =  pev_release,
};

#ifdef XENOMAI

static struct rtdm_device pev_rt = 
{
  .struct_version = RTDM_DEVICE_STRUCT_VER,
  .device_flags = RTDM_NAMED_DEVICE,
  .context_size = sizeof(struct pev_drv),
  .device_name = PEV_RT_NAME,
  .open_nrt = pev_open_nrt,
  .open_rt = pev_open_nrt,
  .ops = 
  {
    .close_nrt = pev_close_nrt,
    .close_rt = pev_close_nrt,
    .ioctl_rt = pev_ioctl_rt,
  },
  .device_class = RTDM_CLASS_EXPERIMENTAL,
  .device_sub_class = SOME_SUB_CLASS,
  .profile_version = 1,
  .driver_name = "pev",
  .driver_version = RTDM_DRIVER_VER(0, 1, 0),
  .peripheral_name = "PEV1100",
  .provider_name = "IOxOS",
  .proc_name = pev_rt.device_name,
};
#endif


// pev probe function - called when the pci_register_driver is called
static int __devinit
pev_probe( struct pev_dev *pev) 
{
  struct pci_dev *dev;
  int retval;
  int i;
  short tmp;

  dev = pev->dev;
  dev->sysdata = (void *)pev;
  debugk(("pev_probe initialize crate %d\n", pev->crate));

  retval = pci_enable_device( dev);

  retval = pci_enable_msi( dev);
  if( retval)
  {
    pev->msi = 0;
    debugk(("Cannot enable MSI\n"));
  }
  else
  {
    pev->msi = dev->irq;
    debugk(("MSI enabled : pev irq = %d\n", dev->irq));
  }

  /* install default interrupt handlers */
  pev->irq_tbl = (struct pev_irq_handler *)kmalloc( 64 * sizeof(struct pev_irq_handler), GFP_KERNEL);
  for( i = 0; i < 64; i++)
  {
    pev_irq_register( pev, i, pev_irq_spurious, NULL);
  }

  pev->pmem_base   = pci_resource_start( dev, 0);
  pev->pmem_len   = pci_resource_len( dev, 0);
  pev->mem_base = pci_resource_start( dev, 2);
  pev->mem_len = pci_resource_len( dev, 2);
  pev->io_base    = pci_resource_start( dev, 4);
  pev->io_len    = pci_resource_len( dev, 4);


  /* check for compressed IO window... */
  if( inl( pev->io_base) & 0x80000000)
  {
    pev->io_remap.short_io  = 1;
    pev->io_remap.iloc_base = PEV_SCSR_ILOC_BASE;
    pev->io_remap.iloc_spi  = PEV_SCSR_ILOC_SPI;
    pev->io_remap.iloc_sign = PEV_SCSR_ILOC_SIGN;
    pev->io_remap.iloc_ctl  = PEV_SCSR_ILOC_CTL;
    pev->io_remap.pcie_mmu  = PEV_SCSR_PCIE_MMU;
    pev->io_remap.iloc_smon = PEV_SCSR_ILOC_SMON;
    pev->io_remap.iloc_i2c  = PEV_SCSR_ILOC_I2C;
    pev->io_remap.vme_base  = PEV_SCSR_VME_BASE;
    pev->io_remap.vme_timer = PEV_SCSR_VME_TIMER;
    pev->io_remap.vme_ader  = PEV_SCSR_VME_ADER;
    pev->io_remap.vme_csr   = PEV_SCSR_VME_CSR;
    pev->io_remap.shm_base  = PEV_SCSR_SHM_BASE;
    pev->io_remap.dma_rd    = PEV_SCSR_DMA_RD;
    pev->io_remap.dma_wr    = PEV_SCSR_DMA_WR;
    pev->io_remap.iloc_itc  = PEV_SCSR_ITC_ILOC;
    pev->io_remap.vme_itc   = PEV_SCSR_ITC_VME;
    pev->io_remap.dma_itc   = PEV_SCSR_ITC_DMA;
    pev->io_remap.usr_itc   = PEV_SCSR_ITC_USR;
  }
  else
  {
    pev->io_remap.short_io = 0;
    pev->io_remap.iloc_base = PEV_CSR_ILOC_BASE;
    pev->io_remap.iloc_spi  = PEV_CSR_ILOC_SPI;
    pev->io_remap.iloc_sign = PEV_CSR_ILOC_SIGN;
    pev->io_remap.iloc_ctl  = PEV_CSR_ILOC_CTL;
    pev->io_remap.pcie_mmu  = PEV_CSR_PCIE_MMU;
    pev->io_remap.iloc_smon = PEV_CSR_ILOC_SMON;
    pev->io_remap.iloc_i2c  = PEV_CSR_ILOC_I2C;
    pev->io_remap.vme_base  = PEV_CSR_VME_BASE;
    pev->io_remap.vme_timer = PEV_CSR_VME_TIMER;
    pev->io_remap.vme_ader  = PEV_CSR_VME_ADER;
    pev->io_remap.vme_csr   = PEV_CSR_VME_CSR;
    pev->io_remap.shm_base  = PEV_CSR_SHM_BASE;
    pev->io_remap.dma_rd    = PEV_CSR_DMA_RD;
    pev->io_remap.dma_wr    = PEV_CSR_DMA_WR;
    pev->io_remap.iloc_itc  = PEV_CSR_ITC_ILOC;
    pev->io_remap.vme_itc   = PEV_CSR_ITC_VME;
    pev->io_remap.dma_itc   = PEV_CSR_ITC_DMA;
    pev->io_remap.usr_itc   = PEV_CSR_ITC_USR;
  }

  pev->shm_len = 0x8000000 << ((inl( pev->io_base + pev->io_remap.iloc_ctl) >> 8) & 3); /* get SHM size             */
  pev->dma_shm_ptr = NULL;


  /* check for 64bit address window (PMEM space)... */
  pev->map_mas64.sg_id = MAP_INVALID;
  if( pev->pmem_len)
  {
    pev->pmem_ptr = ioremap(pev->pmem_base, pev->pmem_len);
    if( pev->pmem_ptr == 0)
    {
      debugk(("Failed to remap PEV prefetchable memory space [size = %x]\n", pev->pmem_len));
    }
    else
    {
      debugk(("pev pmem space mapping = %px - %x\n", pev->pmem_ptr, pev->pmem_len));

      /* prepare scatter/gather for PCI PMEM space */
      //pev->map_mas64.pg_size = 0x100000 << ( ( inl( pev->io_base + pev->io_remap.pcie_mmu) >> 28) & 0x7);
      pev->map_mas64.pg_size = 0x400000;
      pev->map_mas64.pg_num = pev->pmem_len/pev->map_mas64.pg_size;
      pev->map_mas64.loc_base = (u64)pev->pmem_base;
      pev->map_mas64.sg_id = MAP_MASTER_64;
      pev->map_mas64.map_p = (struct pev_map_blk *)0;
      pev_map_init( pev, &pev->map_mas64);
      for( i = 0; i <  pev->map_mas64.pg_num; i++)
      {
	pev_sg_master_64_set( pev, i, 0, 0); 
      }
    }
  }
  else
  {
      debugk(("Didn't find PEV prefetchable memory space [BAR0]\n"));
  }

  /* check for 32bit address window (PMEM space)... */
  pev->map_mas32.sg_id = MAP_INVALID;
  if( pev->mem_len)
  {
    pev->mem_ptr = ioremap(pev->mem_base, pev->mem_len);
    if( pev->mem_ptr == 0)
    {
      debugk(("Failed to remap PEV non prefetchable memory space [size = %x]\n", pev->mem_len));
    }
    else
    {
      debugk(("pev mem space mapping = %px - %x\n", pev->mem_ptr, pev->mem_len));

      /* prepare scatter/gather for PCI MEM space */
      //pev->map_mas32.pg_size = 0x100000 << ( ( inl( pev->io_base + pev->io_remap.pcie_mmu) >> 24) & 0x7);
      pev->map_mas32.pg_size = 0x100000;
      pev->map_mas32.pg_num = pev->mem_len/pev->map_mas32.pg_size;
      pev->map_mas32.pg_num -= 1; /* reserve last page for DMA chains */
      pev->map_mas32.loc_base = (u64)pev->mem_base;
      pev->map_mas32.sg_id = MAP_MASTER_32;
      pev->map_mas32.map_p = (struct pev_map_blk *)0;
      pev_map_init( pev, &pev->map_mas32);
      debugk(("pev mem space scatter gather = %x - %x\n",
                                               pev->map_mas32.pg_size, 
                                               pev->map_mas32.pg_num));
      for( i = 0; i <  pev->map_mas32.pg_num; i++)
      {
	pev_sg_master_32_set( pev, i, 0, 0); 
      }
      pev->dma_shm_len = pev->map_mas32.pg_size;
      pev->dma_shm_base = pev->shm_len - pev->dma_shm_len;
      pev_sg_master_32_set( pev, i, pev->dma_shm_base, 0x2003); /* point to SHM last MByte */
      pev->dma_shm_ptr = pev->mem_ptr + (i*pev->map_mas32.pg_size);
      debugk(("pev mem space DMA = %x - %p - %llx\n",
                                   pev->dma_shm_len, 
                                   pev->dma_shm_ptr, 
                                   pev->dma_shm_base));
      pev_dma_init( pev);
    }
  }
  else
  {
    debugk(("Didn't find PEV non prefetchable memory space [BAR2]\n"));
  }


  /* initialize VME CSR */
  if( pev->io_remap.short_io)
  {
    outl( PEV_SCSR_SEL_VME, pev->io_base);
  }
  outl( 0x404, pev->io_base + pev->io_remap.vme_base);               /* clear error flags              */
  outl( 0x80000081, pev->io_base + pev->io_remap.vme_base + 0x4);    /* enable VME master + ROR        */
  outl( 0x80000004, pev->io_base + pev->io_remap.vme_base + 0x8);    /* enable VME slave + 256*1MBytes */
  outl( pev->crate<<4, pev->io_base + pev->io_remap.vme_ader);       /* set base address according to crate number */
  outl( 0, pev->io_base + pev->io_remap.vme_ader+ 4);                /* set base address according to crate number */
  pev_vme_slv_init( pev);
  outl( 0x08, pev->io_base + pev->io_remap.vme_csr + 0x4);           /* clear bus error                */
  outl( 0x10, pev->io_base + pev->io_remap.vme_csr + 0x8);           /* enable slave                   */
  outl( 0xffff, pev->io_base + pev->io_remap.vme_itc + 0xc);         /* mask all VME interrupt sources */
  outl( 0x7, pev->io_base + pev->io_remap.vme_itc + 0x4);            /* enable VME global interupt     */

  /* enable PCIe master access from FPGA */
  pci_read_config_word( pev->dev, 4, &tmp);
  tmp |= 4;
  pci_write_config_word( pev->dev, 4, tmp);

#ifdef XENOMAI
  retval = rtdm_irq_request( &pev->rtdm_irq, pev->dev->irq, pev_irq, RTDM_IRQTYPE_SHARED, pev_rt.device_name, pev);
  pev->affinity.bits[0] = 0x2;
  xnintr_affinity (  &pev->rtdm_irq, pev->affinity);
#else
  retval = request_irq( pev->dev->irq, pev_irq, IRQF_SHARED, PEV_NAME, pev);
#endif
  if(retval)
  {
    debugk((KERN_NOTICE "pev : Error %d requesting interrupt\n", retval));
  }
  debugk((KERN_NOTICE "pev : interrupt handler pev_irq() installed for vector %d\n", pev->dev->irq));

#ifndef CONFIG_PREEMPT_RT
  //init_MUTEX_LOCKED( &pev->sem);
  sema_init( &pev->sem, 0);
#endif
  
  pev_timer_init( pev);                        /* initialize VME timer           */
  pev_vme_irq_init( pev);
 
  return( 0);
}

static void 
pev_remove(struct pev_dev *pev) 
{
  debugk(("pev_remove\n"));
  /* disable all PEV1100 interrupts */
  outl( 0xffff, pev->io_base + pev->io_remap.iloc_itc + 0xc);         /* mask all local interrupt sources */
  outl( 0x0, pev->io_base + pev->io_remap.iloc_itc + 0x04);            /* disable local interrupts          */
  outl( 0xffff, pev->io_base + pev->io_remap.vme_itc + 0xc);         /* mask all VME interrupt sources */
  outl( 0x0, pev->io_base + pev->io_remap.vme_itc + 0x4);            /* disable VME interrupts     */
  outl( 0xffff, pev->io_base + pev->io_remap.dma_itc + 0xc);         /* mask all DMA/SMEM interrupt sources */
  outl( 0x0, pev->io_base + pev->io_remap.dma_itc + 0x4);            /* disable DMA/SMEM interrupts     */
  outl( 0xffff, pev->io_base + pev->io_remap.usr_itc + 0xc);         /* mask all user interrupt sources */
  outl( 0x0, pev->io_base + pev->io_remap.usr_itc + 0x4);            /* disable user interrupts     */
  /* return resources to OS */
  pev_dma_exit( pev);

  if( pev->pmem_ptr)
  {
    iounmap( pev->pmem_ptr);
  }
  if( pev->mem_ptr)
  {
    iounmap( pev->mem_ptr);
  }
#ifdef XENOMAI
  rtdm_irq_free(&pev->rtdm_irq);
#else
  free_irq( pev->dev->irq, (void *)pev);
#endif
  if( pev->msi)
  {
    pci_disable_msi( pev->dev);
  }

}

/*----------------------------------------------------------------------------
 * Function name : pev_init
 * Prototype     : int
 * Parameters    : none
 * Return        : 0 if OK
 *----------------------------------------------------------------------------
 * Description
 *
 *----------------------------------------------------------------------------*/

static int pev_init( void)
{
  int retval;
  dev_t pev_dev_id;
  struct pci_dev *ldev, *pex;
  struct pev_dev *pev;
  short bcr;
  int i;

  debugk(( KERN_ALERT "pev: entering pev_init\n"));

  /*--------------------------------------------------------------------------
   * device number dynamic allocation 
   *--------------------------------------------------------------------------*/
  retval = alloc_chrdev_region( &pev_dev_id, PEV_MINOR_START, PEV_COUNT, PEV_NAME);
  if( retval < 0)
  {
    debugk(( KERN_WARNING "pev: cannot allocate device number\n"));
    goto err_alloc_chrdev;
  }
  else
  {
    debugk((KERN_WARNING "pev: registered with major number:%i\n", MAJOR( pev_dev_id)));
  }

  pev_drv.dev_id = pev_dev_id;

  /*--------------------------------------------------------------------------
   * register device
   *--------------------------------------------------------------------------*/
  cdev_init( &pev_drv.cdev, &pev_fops);
  pev_drv.cdev.owner = THIS_MODULE;
  pev_drv.cdev.ops = &pev_fops;
  retval = cdev_add( &pev_drv.cdev, pev_drv.dev_id ,PEV_COUNT);
  if(retval)
  {
    debugk((KERN_NOTICE "pev : Error %d adding device\n", retval));
    goto err_cdev_add;
  }
  else
  {
    debugk((KERN_NOTICE "pev : device added\n"));
  }
#ifdef XENOMAI
   retval =  rtdm_dev_register(&pev_rt);
  if(retval)
  {
    debugk((KERN_NOTICE "pev : Error %d registering pev_rt device\n", retval));
    goto err_xeno_register;
  }
  else
  {
    debugk((KERN_NOTICE "pev : real time driver registered\n"));
  }
#endif

  for( i = 0; i < 16; i++)
  {
    pev_drv.pev[i] = (struct pev_dev *)NULL;
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
        /*--------------------------------------------------------------------------
        * allocate space for driver control structures 
        *--------------------------------------------------------------------------*/
        pev = (struct pev_dev *)kmalloc( sizeof(struct pev_dev), GFP_KERNEL);
        memset( pev, 0, sizeof(struct pev_dev));
        pev->pex = pex;

        /* else store BAR0 parameters */
        pev->pex_base   = pci_resource_start( pev->pex, 0);
        pev->pex_len   = pci_resource_len( pev->pex, 0);
        pev->pex_ptr = ioremap( pev->pex_base, pev->pex_len);
        debugk((KERN_NOTICE "pev : PEX%04x switch found at address %lx\n", ldev->device, (ulong)pev->pex_base));

        pev->crate = ~(SWAP32(*(int *)(pev->pex_ptr + 0x640)))&0xf;
        pev_drv.pev[pev->crate] = pev;
        printk("PEV1100 crate number = %d\n", pev->crate);
	
	/* Disable SERR# bit[8] generation */
	bcr = SWAP16(*(short *)(pev->pex_ptr + 0x04));
        *(short *)(pev->pex_ptr + 0x04) =  SWAP16(bcr & ~0x100);

        /* make sure ISA mode is not set in Bridge control register (offset 0x3e) */
        bcr =  SWAP16(*(short *)(pev->pex_ptr + 0x3e));
        *(short *)(pev->pex_ptr + 0x3e) =  SWAP16(bcr & ~4);
        bcr =  SWAP16(*(short *)(pev->pex_ptr + 0x103e));
        *(short *)(pev->pex_ptr + 0x103e) =  SWAP16(bcr & ~4);
        bcr =  SWAP16(*(short *)(pev->pex_ptr + 0x503e));
        *(short *)(pev->pex_ptr + 0x503e) =  SWAP16(bcr & ~4);
        bcr =  SWAP16(*(short *)(pev->pex_ptr + 0x603e));
        *(short *)(pev->pex_ptr + 0x603e) =  SWAP16(bcr & ~4);
	if( ldev->device == 0x8624)
	{
	  bcr =  SWAP16(*(short *)(pev->pex_ptr + 0x803e));
	  *(short *)(pev->pex_ptr + 0x803e) =  SWAP16(bcr & ~4);
	  bcr =  SWAP16(*(short *)(pev->pex_ptr + 0x903e));
	  *(short *)(pev->pex_ptr + 0x903e) =  SWAP16(bcr & ~4);
	}
      }
#ifdef PPC
      break;
#endif
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
  
  for( i = 0; i < 16; i++)
  {
    pev = pev_drv.pev[i];
    if( pev)
    {
      unsigned char s_bus, p_bus;

      p_bus = pev->pex->bus->primary;
      s_bus = pev->pex->bus->secondary;
      debugk((KERN_NOTICE "crate %d : bus number = %x:%x\n", i, p_bus, s_bus));
      p_bus = pev->pex->subordinate->primary;
      s_bus = pev->pex->subordinate->secondary;
      debugk((KERN_NOTICE "crate %d : bus number = %x:%x\n", i, p_bus, s_bus));
      ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, pev->pex);
      while( ldev)
      {
        /* check for PEX8624 */
        if( ( ( ldev->vendor == 0x10b5) &&  ( ldev->device == 0x8624)) ||
            ( ( ldev->vendor == 0x10b5) &&  ( ldev->device == 0x8616))    )
        {
          /* check for port #5*/
          if( ldev->devfn == PCI_DEVFN( 5, 0))
          {
	    p_bus = ldev->subordinate->primary;
	    s_bus = ldev->subordinate->secondary;;
	    debugk((KERN_NOTICE "pex8624 port#5: bus number = %x:%x\n", p_bus, s_bus));
	    pev->dev = ldev;
            while( pev->dev)
            {
              /* check for PEV1100 FPGA End Point */
              if( ( pev->dev->vendor == 0x7357) &&  ( pev->dev->device == 0x1100))
              {
		if( pev->dev->bus->number == s_bus)
		{ 
		  debugk((KERN_NOTICE "FPGA PCIe End Point found on bus 0x%x\n", pev->dev->bus->number));
		  pev_probe( pev);           
		  break;
		}
              }
              pev->dev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, pev->dev);
	    }
	  }
          /* check for port #6*/
          if( ldev->devfn == PCI_DEVFN( 6, 0))
          {
	    p_bus = ldev->subordinate->primary;
	    s_bus = ldev->subordinate->secondary;;
	    debugk((KERN_NOTICE "pex8624 port#6: bus number = %x:%x\n", p_bus, s_bus));
	    pev->plx = ldev;
	    pev->plx_ptr = 0;
            while( pev->plx)
            {
              /* check for PEX8112 */
              if( ( pev->plx->vendor == 0x10b5) &&  ( pev->plx->device == 0x8112))
              {
		if( pev->plx->bus->number == s_bus)
		{            
		  debugk((KERN_NOTICE "plx8212 found on bus 0x%x\n", pev->plx->bus->number));            
                  pev->plx_base   = pci_resource_start( pev->plx, 0);
                  pev->plx_len   = pci_resource_len( pev->plx, 0);
                  pev->plx_ptr = ioremap( pev->plx_base, pev->plx_len);
                  debugk((KERN_NOTICE "pev : PLX%04x bridge found at address %lx [%x]\n", 
	                               pev->plx->device, (ulong)pev->plx_base, pev->plx_len));
		  break;
		}
              }
              pev->plx =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, pev->plx);
	    }
	    break;
	  }
	}
	ldev =  pci_get_device( PCI_ANY_ID, PCI_ANY_ID, ldev);
      }
    }
  }

  /* if everything OK, install realtime driver */
  return( 0);

err_no_pex86XX:
#ifdef XENOMAI
err_xeno_register:
#endif
  cdev_del( &pev_drv.cdev);
err_cdev_add:
  unregister_chrdev_region( pev_drv.dev_id, PEV_COUNT);
err_alloc_chrdev:

  return( retval);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Function name : pev_exit
 * Prototype     : int
 * Parameters    : none
 * Return        : 0 if OK
 *----------------------------------------------------------------------------
 * Description
 *
 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static void pev_exit(void)
{
  dev_t pev_dev_id;
  struct pev_dev *pev;
  int i;

  debugk(( KERN_ALERT "pev: entering pev_exit\n"));

  for( i = 0; i < 16; i++)
  {
    pev = pev_drv.pev[i];
    if( pev)
    {
      pev_remove(pev);
      if( pev->plx_ptr)
      {
        iounmap( pev->plx_ptr);
      }
      iounmap( pev->pex_ptr);
      kfree( pev);
    }
  }
#ifdef XENOMAI
  rtdm_dev_unregister(&pev_rt, 1000);
#endif
  pev_dev_id = pev_drv.dev_id;
  cdev_del( &pev_drv.cdev);
  unregister_chrdev_region( pev_dev_id, PEV_COUNT);
}

module_init( pev_init);
module_exit( pev_exit);

/*================================< end file >================================*/

