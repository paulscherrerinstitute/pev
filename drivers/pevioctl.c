/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pevioctl.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : december 5,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the all ioctl commmands called from the PEV1100 device 
 *    driver main file (pevdrvr.c).
 *    Each function takes a pointer to the PEV1100 device data structure as first
 *    driver argument (struct pev_dev *).
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
 * $Log: pevioctl.c,v $
 * Revision 1.9  2012/07/10 10:21:48  kalantari
 * added tosca driver release 4.15 from ioxos
 *
 * Revision 1.18  2012/06/28 12:22:57  ioxos
 * support for register access through PCI MEM + IRQ from usr1 and usr2 [JFG]
 *
 * Revision 1.17  2012/05/23 08:14:39  ioxos
 * add support for event queues [JFG]
 *
 * Revision 1.16  2012/04/05 13:44:31  ioxos
 * dynamic io_remap [JFG]
 *
 * Revision 1.15  2012/03/27 09:17:40  ioxos
 * add support for FIFOs [JFG]
 *
 * Revision 1.14  2012/01/27 13:13:05  ioxos
 * prepare release 4.01 supporting x86 & ppc [JFG]
 *
 * Revision 1.13  2011/12/06 13:15:18  ioxos
 * support for multi task VME IRQ [JFG]
 *
 * Revision 1.12  2011/10/19 14:07:09  ioxos
 * support for vme irq handling [JFG]
 *
 * Revision 1.11  2011/03/03 15:42:15  ioxos
 * support for 1MBytes VME slave granularity [JFG]
 *
 * Revision 1.10  2011/01/25 13:40:43  ioxos
 * support for VME RMW [JFG]
 *
 * Revision 1.9  2010/08/05 08:23:12  ioxos
 * bug correction in buf_alloc [JFG]
 *
 * Revision 1.8  2010/08/02 10:26:08  ioxos
 * use get_free_pages() instead of kmalloc() [JFG]
 *
 * Revision 1.7  2009/09/29 12:43:38  ioxos
 * support to read/write sflash status [JFG]
 *
 * Revision 1.6  2009/06/03 13:36:16  ioxos
 * support for buf_alloc instead of dma_alloc [JFG]
 *
 * Revision 1.5  2009/05/20 08:31:34  ioxos
 * support for call from pev_rt_ioctl (xenomai) [JFG]
 *
 * Revision 1.4  2009/04/06 10:33:27  ioxos
 * move ioctl code from pevdrvr.c to here [JFG]
 *
 * Revision 1.3  2009/01/27 14:37:30  ioxos
 * remove ref to semaphore.h [JFG]
 *
 * Revision 1.2  2009/01/09 13:10:03  ioxos
 * add support for DMA status [JFG]
 *
 * Revision 1.1  2008/12/12 13:27:02  jfg
 * first cvs checkin [JFG]
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
#include "rdwrlib.h"
#include "sflashlib.h"
#include "maplib.h"
#include "i2clib.h"
#include "vmelib.h"
#include "dmalib.h"
#include "pevdrvr.h"
#include "pevklib.h"

#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif


int
pev_ioctl_buf( struct pev_dev *pev,
	       unsigned int cmd, 
	       unsigned long arg)
{
  int retval = 0;
  int order;           

  if( !pev->dev)
  {
    return( -ENODEV);
  }
  switch ( cmd)
  {
    case PEV_IOCTL_BUF_ALLOC:
    {
      int i;
      int *p;
      struct pev_ioctl_buf buf;

      if( copy_from_user(&buf, (void *)arg, sizeof(buf)))
      {
	return( -EFAULT);
      }
      order = get_order( buf.size); 
      buf.k_addr = (void *)__get_free_pages( GFP_KERNEL | __GFP_DMA, order);
      if( !buf.k_addr)
      {
        return( -EFAULT);
      }
      buf.b_addr = (void *)dma_map_single( &pev->dev->dev, buf.k_addr, buf.size, DMA_BIDIRECTIONAL);
      debugk(( KERN_ALERT "alloc buffer : %p - %p [%x] %lx\n", buf.k_addr, buf.b_addr, buf.size, virt_to_phys(buf.k_addr)));
      if( !buf.b_addr)
      {
	free_pages( (unsigned long)buf.k_addr, order);
	buf.k_addr = 0;
        return( -EFAULT);
      }
      p = (int *)buf.k_addr;
      for( i = 0; i < buf.size; i += 4)
      {
	*p++ = 0xdeadface;
      }
      if( copy_to_user( (void *)arg, &buf, sizeof( buf)))
      {
	return -EFAULT;
      }

      break;
    }
    case PEV_IOCTL_BUF_FREE:
    {
      struct pev_ioctl_buf buf;

      if( copy_from_user(&buf, (void *)arg, sizeof(buf)))
      {
        return( -EFAULT);
      }
      debugk(( KERN_ALERT "free buffer : %p - %p [%x]\n", buf.k_addr, buf.b_addr, buf.size));
      dma_unmap_single( &pev->dev->dev, (dma_addr_t)buf.b_addr, buf.size, DMA_BIDIRECTIONAL);
      order = get_order( buf.size); 
      free_pages( (unsigned long)buf.k_addr, order);
      break;
    }
    default:
    {
      return( -EINVAL);
    }
  }
     
  return( retval);
}

int
pev_ioctl_dma( struct pev_dev *pev,
	       unsigned int cmd, 
	       unsigned long arg)
{
  int retval = 0;           

  if( !pev->dev)
  {
    return( -ENODEV);
  }
  switch ( cmd)
  {
    case PEV_IOCTL_DMA_MOVE:
    {
      struct pev_ioctl_dma_req dma;

#ifdef XENOMAI
      if( rtdm_copy_from_user( pev->user_info, (void *)&dma, (void *)arg, sizeof(dma)))
#else
      if( copy_from_user(&dma, (void *)arg, sizeof(dma)))
#endif
      {
	return( -EFAULT);
      }
      retval = pev_dma_move( pev, &dma);
      break;
    }
    case PEV_IOCTL_DMA_STATUS:
    {
      struct pev_ioctl_dma_sts dma_sts;

      retval = pev_dma_status( pev, &dma_sts);
#ifdef XENOMAI
      if( rtdm_copy_to_user( pev->user_info, (void *)arg, &dma_sts, sizeof(dma_sts)))
#else
      if( copy_to_user( (void *)arg, &dma_sts, sizeof(dma_sts)))
#endif
      {
	return( -EFAULT);
      }
      break;
    }
    case PEV_IOCTL_DMA_WAIT:
    {
      break;
    }
    case PEV_IOCTL_DMA_KILL:
    {
      break;
    }
    default:
    {
      return( -EINVAL);
    }
  }
  return( retval);
}

int
pev_ioctl_evt( struct pev_dev *pev,
	       unsigned int cmd, 
	       unsigned long arg)
{
  struct pev_ioctl_evt evt;

  int retval = 0;  
  if( !pev->dev)
  {
    return( -ENODEV);
  }
  if( copy_from_user(&evt, (void *)arg, sizeof(evt)))
  {
    return( -EFAULT);
  }
  switch ( cmd)
  {
    case PEV_IOCTL_EVT_ALLOC:
    {
      pev_evt_alloc( pev, &evt);
      break;
    }
    case PEV_IOCTL_EVT_REGISTER:
    {
      pev_evt_register( pev, &evt);
      break;
    }
    case PEV_IOCTL_EVT_ENABLE:
    {
      pev_evt_enable( pev, &evt);
      break;
    }
    case PEV_IOCTL_EVT_UNMASK:
    {
      pev_evt_unmask( pev, &evt);
      break;
    }
    case PEV_IOCTL_EVT_MASK:
    {
      pev_evt_mask( pev, &evt);
      break;
    }
    case PEV_IOCTL_EVT_DISABLE:
    {
      pev_evt_disable( pev, &evt);
      break;
    }
    case PEV_IOCTL_EVT_UNREGISTER:
    {
      pev_evt_unregister( pev, &evt);
      break;
    }
    case PEV_IOCTL_EVT_FREE:
    {
      pev_evt_free( pev, &evt);
      break;
    }
    case PEV_IOCTL_EVT_WAIT:
    {
      debugk(( KERN_ALERT "wait for semaphore\n"));
      //retval = down_interruptible( &pev->sem);
      debugk(( KERN_ALERT "semaphore unlocked\n"));
      break;
    }
    case PEV_IOCTL_EVT_RESET:
    {
      debugk(( KERN_ALERT "set semaphore\n"));
      //up( &pev->sem);
      break;
    }
    case PEV_IOCTL_EVT_READ:
    {
      pev_evt_read( pev, &evt);
      break;
    }
    default:
    {
      return( -EINVAL);
    }
  }
  if( copy_to_user((void *)arg, &evt, sizeof(evt)))
  {
    return( -EFAULT);
  }
     
  return( retval);
}

int
pev_ioctl_histo( struct pev_dev *pev,
	         unsigned int cmd, 
	         unsigned long arg)
{
  int retval = 0;  
  struct pev_ioctl_histo hst;

  if( !pev->dev)
  {
    return( -ENODEV);
  }
  if( copy_from_user(&hst, (void *)arg, sizeof(hst)))
  {
    return( -EFAULT);
  }
  switch ( cmd)
  {
    case PEV_IOCTL_HISTO_READ:
    {
      retval = pev_histo_read( pev, &hst);
      break;
    }
    case PEV_IOCTL_HISTO_CLEAR:
    {
      retval = pev_histo_clear( pev, &hst);
      break;
    }
    default:
    {
      return( -EINVAL);
    }
  }
     
  return( retval);
}

int
pev_ioctl_fifo( struct pev_dev *pev,
	        unsigned int cmd, 
	        unsigned long arg)
{
  int retval = 0;  
  struct pev_ioctl_fifo fifo;

  if( !pev->dev)
  {
    return( -ENODEV);
  }
  if( cmd == PEV_IOCTL_FIFO_INIT)
  {
    pev_fifo_init(pev);
    return( 0);
  }
  if( copy_from_user(&fifo, (void *)arg, sizeof(fifo)))
  {
    return( -EFAULT);
  }
  switch ( cmd)
  {
    case PEV_IOCTL_FIFO_RD:
    {
      retval = pev_fifo_read( pev, &fifo);
      break;
    }
    case PEV_IOCTL_FIFO_WR:
    {
      retval = pev_fifo_write( pev, &fifo);
      break;
    }
    case PEV_IOCTL_FIFO_STATUS:
    {
      pev_fifo_status(pev, &fifo);
      break;
    }
    case PEV_IOCTL_FIFO_CLEAR:
    {
      pev_fifo_clear(pev, &fifo);
      break;
    }
    case PEV_IOCTL_FIFO_WAIT_EF:
    {
      retval = pev_fifo_wait_ef(pev, &fifo);
      break;
    }
    case PEV_IOCTL_FIFO_WAIT_FF:
    {
      retval = pev_fifo_wait_ff(pev, &fifo);
      break;
    }
    default:
    {
      return( -EINVAL);
    }
  }
  if( copy_to_user( (void *)arg, &fifo, sizeof( fifo)))
  {
    return -EFAULT;
  }
     
  return( retval);
}


int
pev_ioctl_i2c( struct pev_dev *pev,
	       unsigned int cmd, 
	       unsigned long arg)
{
  int retval = 0;  
  struct pev_ioctl_i2c i2c;

  if( !pev->dev)
  {
    return( -ENODEV);
  }
  if( copy_from_user(&i2c, (void *)arg, sizeof(i2c)))
  {
    return( -EFAULT);
  }
  switch ( cmd)
  {
    case PEV_IOCTL_I2C_DEV_CMD:
    {
      pev_i2c_dev_cmd( pev, &i2c);
      break;
    }
    case PEV_IOCTL_I2C_DEV_RD:
    {
      pev_i2c_dev_read( pev, &i2c);
      if( copy_to_user( (void *)arg, &i2c, sizeof( i2c)))
      {
        return -EFAULT;
      }
      break;
    }
    case PEV_IOCTL_I2C_DEV_WR:
    {
      pev_i2c_dev_write( pev, &i2c);
      break;
    }
    case PEV_IOCTL_I2C_PEX_RD:
    {
      pev_i2c_pex_read( pev, &i2c);
      if( copy_to_user( (void *)arg, &i2c, sizeof( i2c)))
      {
        return -EFAULT;
      }
      break;
    }
    case PEV_IOCTL_I2C_PEX_WR:
    {
      pev_i2c_pex_write( pev, &i2c);
      break;
    }
    default:
    {
      return( -EINVAL);
    }
  }
     
  return( retval);
}

int
pev_ioctl_map( struct pev_dev *pev,
	       unsigned int cmd, 
	       unsigned long arg)
{
  int retval = 0;  
  struct pev_ioctl_map_pg pg;
  struct pev_ioctl_map_ctl ctl;

  if( !pev->dev)
  {
    return( -ENODEV);
  }

  if(( cmd & PEV_IOCTL_MAP_MASK) == PEV_IOCTL_MAP_PG)
  {
    if( copy_from_user(&pg, (void *)arg, sizeof(pg)))
    {
      return( -EFAULT);
    }
  }
  if(( cmd & PEV_IOCTL_MAP_MASK) == PEV_IOCTL_MAP_CTL)
  {
    if( copy_from_user(&ctl, (void *)arg, sizeof(ctl)))
    {
      return( -EFAULT);
    }
  }
  switch ( cmd)
  {
    case PEV_IOCTL_MAP_ALLOC:
    {
      pev_map_alloc( pev, &pg);
      break;
    }
    case PEV_IOCTL_MAP_FREE:
    {
      pev_map_free( pev, &pg);
      break;
    }
    case PEV_IOCTL_MAP_MODIFY:
    {
      pev_map_modify( pev, &pg);
      break;
    }
    case PEV_IOCTL_MAP_FIND:
    {
      pev_map_find( pev, &pg);
      break;
    }
    case PEV_IOCTL_MAP_READ:
    {
      pev_map_read( pev, &ctl);
      break;
    }
    case PEV_IOCTL_MAP_CLEAR:
    {
      pev_map_clear( pev, &ctl);
      break;
    }
    default:
    {
      return( -EINVAL);
    }
  }
  if(( cmd & PEV_IOCTL_MAP_MASK) == PEV_IOCTL_MAP_PG)
  {
    if( copy_to_user( (void *)arg, &pg, sizeof(pg)))
    {
      return( -EFAULT);
    }
  }
  if(( cmd & PEV_IOCTL_MAP_MASK) == PEV_IOCTL_MAP_CTL)
  {
    if( copy_to_user( (void *)arg, &ctl, sizeof(ctl)))
    {
      return( -EFAULT);
    }
  }
     
  return( retval);
}

int
pev_ioctl_reg( struct pev_dev *pev,
	       unsigned int cmd, 
	       unsigned long arg)
{
  int retval = 0;  
  struct pev_ioctl_rw_reg rw;

  if( !pev->dev)
  {
    return( -ENODEV);
  }

  if( copy_from_user(&rw, (void *)arg, sizeof(rw)))
  {
    return( -EFAULT);
  }
  switch ( cmd)
  {
    case PEV_IOCTL_RD_REG_8:
    {
      if( ( rw.addr_off > pev->io_len - 1) || ( rw.data_off > pev->io_len - 1)) return( -EFAULT);
      outb( (u8)rw.reg_idx, pev->io_base + rw.addr_off);
      rw.reg_data = (u32)inb( pev->io_base + rw.data_off);
      break;
    }
    case PEV_IOCTL_RD_REG_16:
    {
      if( ( rw.addr_off > pev->io_len - 2) || ( rw.data_off > pev->io_len - 2)) return( -EFAULT);
      outw( (u16)rw.reg_idx, pev->io_base + rw.addr_off);
      rw.reg_data = (u32)inw( pev->io_base + rw.data_off);
      break;
    }
    case PEV_IOCTL_RD_REG_32:
    {
      if( ( rw.addr_off > pev->io_len - 4) || ( rw.data_off > pev->io_len - 4)) return( -EFAULT);
      outl( rw.reg_idx, pev->io_base + rw.addr_off);
      rw.reg_data = inl( pev->io_base + rw.data_off);
      break;
    }
    case PEV_IOCTL_WR_REG_8:
    {
      if( ( rw.addr_off > pev->io_len - 1) || ( rw.data_off > pev->io_len - 1)) return( -EFAULT);
      outb( (u8)rw.reg_idx, pev->io_base + rw.addr_off);
      outb( (u8)rw.reg_data, pev->io_base + rw.data_off);
      break;
    }
    case PEV_IOCTL_WR_REG_16:
    {
      if( ( rw.addr_off > pev->io_len - 2) || ( rw.data_off > pev->io_len - 2)) return( -EFAULT);
      outw( (u16)rw.reg_idx, pev->io_base + rw.addr_off);
      outw( (u16)rw.reg_data, pev->io_base + rw.data_off);
      break;
    }
    case PEV_IOCTL_WR_REG_32:
    {
      if( ( rw.addr_off > pev->io_len - 4) || ( rw.data_off > pev->io_len - 4)) return( -EFAULT);
      outl( rw.reg_idx, pev->io_base + rw.addr_off);
      outl( rw.reg_data, pev->io_base + rw.data_off);
      break;
    }
    default:
    {
      retval = -EINVAL;
    }
  }
  if( ( cmd & 0xf00) == 0x100)
  {
    if( copy_to_user( (void *)arg, &rw, sizeof( rw)))
    {
      return -EFAULT;
    }
  }  
  return( retval);
}

int
pev_ioctl_rdwr( struct pev_dev *pev,
	       unsigned int cmd, 
	       unsigned long arg)
{
  struct pev_ioctl_rdwr rdwr;

  if( copy_from_user(&rdwr, (void *)arg, sizeof(rdwr)))
  {
    return( -EFAULT);
  }
  return( pev_rdwr( pev, &rdwr));
}

int
pev_ioctl_rw( struct pev_dev *pev,
	      unsigned int cmd, 
	      unsigned long arg)
{
  int retval = 0;  
  struct pev_ioctl_rw rw;
  char *ptr;
 
  if( !pev->dev)
  {
    return( -ENODEV);
  }

  if( copy_from_user(&rw, (void *)arg, sizeof(rw)))
  {
	return( -EFAULT);
  }
  switch ( cmd)
  {
    case PEV_IOCTL_RD_IO_8:
    {
      if( rw.offset > pev->io_len - 1) return( -EFAULT);
      rw.data = (u32)inb( pev->io_base + rw.offset);
      break;
    }
    case PEV_IOCTL_RD_IO_16:
    {
      if( rw.offset > pev->io_len - 2) return( -EFAULT);
      rw.data = (u32)inw( pev->io_base + rw.offset);
      break;
    }
    case PEV_IOCTL_RD_IO_32:
    {
      if( rw.offset > pev->io_len - 4) return( -EFAULT);
      rw.data = inl( pev->io_base + rw.offset);
      break;
    }
    case PEV_IOCTL_RD_PMEM_8:
    {
      if( rw.offset > pev->pmem_len - 1) return( -EFAULT);
      ptr = (char *)ioremap( pev->pmem_base + rw.offset, 8);
      //rw.data = (u32)( *(u8 *)(pev->pmem_ptr + rw.offset));
      rw.data = (u32)( *(u8 *)ptr);
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_RD_PMEM_16:
    {
      if( rw.offset > pev->pmem_len - 2) return( -EFAULT);
      ptr = (char *)ioremap( pev->pmem_base + rw.offset, 8);
      //rw.data = (u32)( *(u16 *)(pev->pmem_ptr + rw.offset));
      rw.data = (u32)( *(u16 *)ptr);
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_RD_PMEM_32:
    {
      if( rw.offset > pev->pmem_len - 4) return( -EFAULT);
      ptr = (char *)ioremap( pev->pmem_base + rw.offset, 8);
      //rw.data = *(u32 *)(pev->pmem_ptr + rw.offset);
      rw.data = (u32)( *(u32 *)ptr);
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_RD_MEM_8:
    {
      if( rw.offset > pev->mem_len - 1) return( -EFAULT);
      ptr = (char *)ioremap( pev->mem_base + rw.offset, 8);
      //rw.data = (u32)( *(u8 *)(pev->mem_ptr + rw.offset));
      rw.data = (u32)( *(u8 *)ptr);
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_RD_MEM_16:
    {
      if( rw.offset > pev->mem_len - 2) return( -EFAULT);
      ptr = (char *)ioremap( pev->mem_base + rw.offset, 8);
      //rw.data = (u32)( *(u16 *)(pev->mem_ptr + rw.offset));
      rw.data = (u32)( *(u16 *)ptr);
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_RD_MEM_32:
    {
      if( rw.offset > pev->mem_len - 4) return( -EFAULT);
      ptr = (char *)ioremap( pev->mem_base + rw.offset, 8);
      //rw.data = *(u32 *)(pev->mem_ptr + rw.offset);
      rw.data = (u32)( *(u32 *)ptr);
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_RD_CSR_8:
    {
      if( rw.offset > pev->csr_len - 1) return( -EFAULT);
      rw.data = (u32)( *(u8 *)(pev->csr_ptr + rw.offset));
      break;
    }
    case PEV_IOCTL_RD_CSR_16:
    {
      if( rw.offset > pev->csr_len - 2) return( -EFAULT);
      rw.data = (u32)( *(u16 *)(pev->csr_ptr + rw.offset));
      break;
    }
    case PEV_IOCTL_RD_CSR_32:
    {
      if( rw.offset > pev->csr_len - 4) return( -EFAULT);
      rw.data = *(u32 *)(pev->csr_ptr + rw.offset);
      break;
    }
    case PEV_IOCTL_RD_CFG_8:
    {
      u8 tmp;
      if( rw.offset >= 0x100) return( -EFAULT);
      pci_read_config_byte( pev->dev, rw.offset, &tmp);
      rw.data = (u32)tmp;
      break;
    }
    case PEV_IOCTL_RD_CFG_16:
    {
      u16 tmp;
      if( rw.offset >= 0x100) return( -EFAULT);
      pci_read_config_word( pev->dev, rw.offset, &tmp);
      rw.data = (u32)tmp;
      break;
    }
    case PEV_IOCTL_RD_CFG_32:
    {
      if( rw.offset >= 0x100) return( -EFAULT);
      pci_read_config_dword( pev->dev, rw.offset, &rw.data);
      break;
    }
    case PEV_IOCTL_WR_IO_8:
    {
      if( rw.offset > pev->io_len - 1) return( -EFAULT);
      outb( (u8)rw.data, pev->io_base + rw.offset);
      break;
    }
    case PEV_IOCTL_WR_IO_16:
    {
      if( rw.offset > pev->io_len - 2) return( -EFAULT);
      outw( (u16)rw.data, pev->io_base + rw.offset);
      break;
    }
    case PEV_IOCTL_WR_IO_32:
    {
      if( rw.offset > pev->io_len - 4) return( -EFAULT);
      outl( rw.data, pev->io_base + rw.offset);
      break;
    }
    case PEV_IOCTL_WR_PMEM_8:
    {
      if( rw.offset > pev->pmem_len - 1) return( -EFAULT);
      ptr = (char *)ioremap( pev->pmem_base + rw.offset, 8);
      //*(u8 *)(pev->pmem_ptr + rw.offset) = (u8)rw.data;
      *(u8 *)ptr = (u8)rw.data;
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_WR_PMEM_16:
    {
      if( rw.offset > pev->pmem_len - 2) return( -EFAULT);
      ptr = (char *)ioremap( pev->pmem_base + rw.offset, 8);
      //*(u16 *)(pev->pmem_ptr + rw.offset) = (u16)rw.data;
      *(u16 *)ptr = (u16)rw.data;
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_WR_PMEM_32:
    {
      if( rw.offset > pev->pmem_len - 4) return( -EFAULT);
      ptr = (char *)ioremap( pev->pmem_base + rw.offset, 8);
      //*(u32 *)(pev->pmem_ptr + rw.offset) = rw.data;
      *(u32 *)ptr = rw.data;
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_WR_MEM_8:
    {
      if( rw.offset > pev->mem_len - 1) return( -EFAULT);
      ptr = (char *)ioremap( pev->mem_base + rw.offset, 8);
      //*(u8 *)(pev->mem_ptr + rw.offset) = (u8)rw.data;
      *(u8 *)ptr = (u8)rw.data;
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_WR_MEM_16:
    {
      if( rw.offset > pev->mem_len - 2) return( -EFAULT);
      ptr = (char *)ioremap( pev->mem_base + rw.offset, 8);
      //*(u16 *)(pev->mem_ptr + rw.offset) = (u16)rw.data;
      *(u16 *)ptr = (u16)rw.data;
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_WR_MEM_32:
    {
      if( rw.offset > pev->mem_len - 4) return( -EFAULT);
      ptr = (char *)ioremap( pev->mem_base + rw.offset, 8);
      //*(u32 *)(pev->mem_ptr + rw.offset) = rw.data;
      *(u32 *)ptr = rw.data;
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_WR_CSR_8:
    {
      if( rw.offset > pev->csr_len - 1) return( -EFAULT);
      *(u8 *)(pev->csr_ptr + rw.offset) = (u8)rw.data;
      break;
    }
    case PEV_IOCTL_WR_CSR_16:
    {
      if( rw.offset > pev->csr_len - 2) return( -EFAULT);
      *(u16 *)(pev->csr_ptr + rw.offset) = (u16)rw.data;
      break;
    }
    case PEV_IOCTL_WR_CSR_32:
    {
      if( rw.offset > pev->csr_len - 4) return( -EFAULT);
      *(u32 *)(pev->csr_ptr + rw.offset) = rw.data;
      break;
    }
    case PEV_IOCTL_WR_CFG_8:
    {
      u8 tmp;
      if( rw.offset >= 0x100) return( -EFAULT);
      tmp = (u8)rw.data;
      pci_write_config_byte( pev->dev, rw.offset, tmp);
      break;
    }
    case PEV_IOCTL_WR_CFG_16:
    {
      u16 tmp;
      if( rw.offset >= 0x100) return( -EFAULT);
      tmp = (u16)rw.data;
      pci_write_config_word( pev->dev, rw.offset, tmp);
      break;
    }
    case PEV_IOCTL_WR_CFG_32:
    {
      if( rw.offset >= 0x100) return( -EFAULT);
      pci_write_config_dword( pev->dev, rw.offset, rw.data);
      break;
    }
    case PEV_IOCTL_SET_IO_8:
    {
      u32 tmp;
      if( rw.offset > pev->io_len - 1) return( -EFAULT);
      tmp = (u32)inb( pev->io_base + rw.offset);
      rw.data |= tmp;
      outb( (u8)rw.data, pev->io_base + rw.offset);
      break;
    }
    case PEV_IOCTL_SET_IO_16:
    {
      u32 tmp;
      if( rw.offset > pev->io_len - 2) return( -EFAULT);
      tmp = (u32)inw( pev->io_base + rw.offset);
      rw.data |= tmp;
      outw( (u16)rw.data, pev->io_base + rw.offset);
      break;
    }
    case PEV_IOCTL_SET_IO_32:
    {
      u32 tmp;
      if( rw.offset > pev->io_len - 4) return( -EFAULT);
      tmp = (u32)inl( pev->io_base + rw.offset);
      rw.data |= tmp;
      outl( rw.data, pev->io_base + rw.offset);
      break;
    }
    case PEV_IOCTL_SET_PMEM_8:
    {
      u32 tmp;
      if( rw.offset > pev->pmem_len - 1) return( -EFAULT);
      ptr = (char *)ioremap( pev->pmem_base + rw.offset, 8);
      //tmp = (u32)( *(u8 *)(pev->pmem_ptr + rw.offset));
      tmp = (u32)( *(u8 *)ptr);
      rw.data |= tmp;
      //*(u8 *)(pev->pmem_ptr + rw.offset) = (u8)rw.data;
      *(u8 *)ptr = (u8)rw.data;
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_SET_PMEM_16:
    {
      u32 tmp;
      if( rw.offset > pev->pmem_len - 2) return( -EFAULT);
      ptr = (char *)ioremap( pev->pmem_base + rw.offset, 8);
      //tmp = (u32)( *(u16 *)(pev->pmem_ptr + rw.offset));
      tmp = (u32)( *(u16 *)ptr);
      rw.data |= tmp;
      //*(u16 *)(pev->pmem_ptr + rw.offset) = (u16)rw.data;
      *(u16 *)ptr = (u16)rw.data;
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_SET_PMEM_32:
    {
      u32 tmp;
      if( rw.offset > pev->pmem_len - 4) return( -EFAULT);
      ptr = (char *)ioremap( pev->pmem_base + rw.offset, 8);
      //tmp = (u32)( *(u32 *)(pev->pmem_ptr + rw.offset));
      tmp = *(u32 *)ptr;
      rw.data |= tmp;
      //*(u32 *)(pev->pmem_ptr + rw.offset) = rw.data;
      *(u32 *)ptr = rw.data;
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_SET_MEM_8:
    {
     u32 tmp;
      if( rw.offset > pev->mem_len - 1) return( -EFAULT);
      ptr = (char *)ioremap( pev->mem_base + rw.offset, 8);
      //tmp = (u32)( *(u8 *)(pev->mem_ptr + rw.offset));
      tmp = (u32)( *(u8 *)ptr);
      rw.data |= tmp;
      //*(u8 *)(pev->mem_ptr + rw.offset) = (u8)rw.data;
      *(u8 *)ptr = (u8)rw.data;
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_SET_MEM_16:
    {
      u32 tmp;
      if( rw.offset > pev->mem_len - 2) return( -EFAULT);
      ptr = (char *)ioremap( pev->mem_base + rw.offset, 8);
      //tmp = (u32)( *(u16 *)(pev->mem_ptr + rw.offset));
      tmp = (u32)( *(u16 *)ptr);
      rw.data |= tmp;
      //*(u16 *)(pev->mem_ptr + rw.offset) = (u16)rw.data;
      *(u16 *)ptr = (u16)rw.data;
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_SET_MEM_32:
    {
      u32 tmp;
      if( rw.offset > pev->mem_len - 4) return( -EFAULT);
      ptr = (char *)ioremap( pev->mem_base + rw.offset, 8);
      //tmp = (u32)( *(u32 *)(pev->mem_ptr + rw.offset));
      tmp = *(u32 *)ptr;
      rw.data |= tmp;
      //*(u32 *)(pev->mem_ptr + rw.offset) = rw.data;
      *(u32 *)ptr = rw.data;
      iounmap( ptr);
      break;
    }
    case PEV_IOCTL_SET_CSR_8:
    {
     u32 tmp;
      if( rw.offset > pev->csr_len - 1) return( -EFAULT);
      tmp = (u32)( *(u8 *)(pev->csr_ptr + rw.offset));
      rw.data |= tmp;
      *(u8 *)(pev->csr_ptr + rw.offset) = (u8)rw.data;
      break;
    }
    case PEV_IOCTL_SET_CSR_16:
    {
      u32 tmp;
      if( rw.offset > pev->csr_len - 2) return( -EFAULT);
      tmp = (u32)( *(u16 *)(pev->csr_ptr + rw.offset));
      rw.data |= tmp;
      *(u16 *)(pev->csr_ptr + rw.offset) = (u16)rw.data;
      break;
    }
    case PEV_IOCTL_SET_CSR_32:
    {
      u32 tmp;
      if( rw.offset > pev->csr_len - 4) return( -EFAULT);
      tmp = (u32)( *(u32 *)(pev->csr_ptr + rw.offset));
      rw.data |= tmp;
      *(u32 *)(pev->csr_ptr + rw.offset) = rw.data;
      break;
    }
    case PEV_IOCTL_SET_CFG_8:
    {
      u8 tmp;
      if( rw.offset >= 0x100) return( -EFAULT);
      pci_read_config_byte( pev->dev, rw.offset, &tmp);
      tmp |= (u8)rw.data;
      pci_write_config_byte( pev->dev, rw.offset, tmp);
      break;
    }
    case PEV_IOCTL_SET_CFG_16:
    {
      u16 tmp;
      if( rw.offset >= 0x100) return( -EFAULT);
      pci_read_config_word( pev->dev, rw.offset, &tmp);
      tmp |= (u16)rw.data;
      pci_write_config_word( pev->dev, rw.offset, tmp);
      break;
    }
    case PEV_IOCTL_SET_CFG_32:
    {
      u32 tmp;
      if( rw.offset >= 0x100) return( -EFAULT);
      pci_read_config_dword( pev->dev, rw.offset, &tmp);
      tmp |= rw.data;
      pci_write_config_dword( pev->dev, rw.offset, tmp);
      break;
    }
    default:
    {
      retval = -EINVAL;
    }
  }
  if( ( cmd & 0xf00) == 0x100)
  {
    if( copy_to_user( (void *)arg, &rw, sizeof(struct pev_ioctl_rw)))
    {
      return -EFAULT;
    }
  }
         
  return( retval);
}

int
pev_ioctl_sflash( struct pev_dev *pev,
	       unsigned int cmd, 
	       unsigned long arg)
{
  int retval = 0;  
  int data, *ptr;
  struct pev_ioctl_sflash_rw rdwr;

  if( pev->board != PEV_BOARD_IFC1210) /* IFC1210 has a direct path to SFLASH via ELBC */
  {
    if( !pev->dev)
    {
      return( -ENODEV);
    }
  }

  switch ( cmd & 0xfffffffc)
  {
    case PEV_IOCTL_SFLASH_ID:
    {
      ptr = (int *)arg;
      data = 0;
      pev_sflash_id( pev, (unsigned char *)&data, cmd&3);
      put_user( data, ptr);
      break;
    }
    case PEV_IOCTL_SFLASH_RDSR:
    {
      ptr = (int *)arg;
      data = (int)pev_sflash_rdsr( pev, cmd&3);
      put_user( data, ptr);
      break;
    }
    case PEV_IOCTL_SFLASH_WRSR:
    {
      unsigned short sr;

      ptr = (int *)arg;
      get_user( data, ptr);
      sr = (unsigned short)data;
      pev_sflash_wrsr( pev, sr, cmd&3);
      break;
    }
    case PEV_IOCTL_SFLASH_RD:
    {
      debugk(( KERN_ALERT "SFLASH read\n"));
      if( copy_from_user(&rdwr, (void *)arg, sizeof(rdwr)))
      {
        return( -EFAULT);
      }
      retval = pev_sflash_read( pev, &rdwr);
      break;
    }
    case PEV_IOCTL_SFLASH_WR:
    {
      debugk(( KERN_ALERT "SFLASH write\n"));
      if( copy_from_user(&rdwr, (void *)arg, sizeof(rdwr)))
      {
	return( -EFAULT);
      }
      retval = pev_sflash_write( pev, &rdwr);
      break;
    }
    default:
    {
      return( -EINVAL);
    }
  }   
  return( retval);
}

int
pev_ioctl_fpga( struct pev_dev *pev,
	        unsigned int cmd, 
	        unsigned long arg)
{
  int retval = 0;  
  struct pev_ioctl_sflash_rw rdwr;

  if( pev->board != PEV_BOARD_IFC1210)
  {
    return( -EPERM);
  }
  switch ( cmd)
  {
    case PEV_IOCTL_FPGA_LOAD:
    {
      debugk(( KERN_ALERT "FPGA load\n"));
      if( copy_from_user(&rdwr, (void *)arg, sizeof(rdwr)))
      {
	return( -EFAULT);
      }
      retval = pev_fpga_load( pev, &rdwr);
      break;
    }
    default:
    {
      return( -EINVAL);
    }
  }   
  return( retval);
}

int
pev_ioctl_eeprom( struct pev_dev *pev,
	          unsigned int cmd, 
	          unsigned long arg)
{
  int retval = 0;  
  struct pev_ioctl_rdwr rdwr;

  if( !pev->dev)
  {
    return( -ENODEV);
  }
  switch ( cmd)
  {
    case PEV_IOCTL_EEPROM_RD:
    {
      debugk(( KERN_ALERT "EEPROM read\n"));
      if( copy_from_user(&rdwr, (void *)arg, sizeof(rdwr)))
      {
        return( -EFAULT);
      }
      retval = -ENODEV;
      if( pev->board == PEV_BOARD_IFC1210)
      {
	retval = pev_idt_eeprom_read( pev, &rdwr);
      }
      break;
    }
    case PEV_IOCTL_EEPROM_WR:
    {
      debugk(( KERN_ALERT "EEPROM write\n"));
      if( copy_from_user(&rdwr, (void *)arg, sizeof(rdwr)))
      {
	return( -EFAULT);
      }
      retval = -ENODEV;
      if( pev->board == PEV_BOARD_IFC1210)
      {
	retval = pev_idt_eeprom_write( pev, &rdwr);
      }
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
pev_ioctl_timer( struct pev_dev *pev,
	       unsigned int cmd, 
	       unsigned long arg)
{
  int retval = 0;  
  struct pev_ioctl_timer tmr;

  if( !pev->dev)
  {
    return( -ENODEV);
  }
  switch ( cmd)
  {
    case PEV_IOCTL_TIMER_READ:
    {
      pev_timer_read( pev, &tmr);
#ifdef XENOMAI
      if( rtdm_copy_to_user( pev->user_info, (void *)arg, (void *)&tmr, sizeof(tmr)))
#else
      if( copy_to_user( (void *)arg, &tmr, sizeof( tmr)))
#endif
      {
	return -EFAULT;
      }
      break;
    }
    case PEV_IOCTL_TIMER_START:
    {
#ifdef XENOMAI
      if( rtdm_copy_from_user( pev->user_info, (void *)&tmr, (void *)arg, sizeof(tmr)))
#else
      if( copy_from_user(&tmr, (void *)arg, sizeof(tmr)))
#endif
      {
	return( -EFAULT);
      }
      pev_timer_start( pev, &tmr);
      break;
    }
    case PEV_IOCTL_TIMER_RESTART:
    {
      pev_timer_restart( pev);
      break;
    }
    case PEV_IOCTL_TIMER_STOP:
    {
      pev_timer_stop( pev);
      break;
    }
    case PEV_IOCTL_TIMER_IRQ_ENA:
    {
      pev_timer_irq_ena( pev);
      break;
    }
    case PEV_IOCTL_TIMER_IRQ_DIS:
    {
      pev_timer_irq_dis( pev);
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
pev_ioctl_vme( struct pev_dev *pev,
	       unsigned int cmd, 
	       unsigned long arg)
{
  int retval = 0; 
 
  if( !pev->dev)
  {
    return( -ENODEV);
  }
  if( !(cmd & 0xf))
  {
    struct pev_ioctl_vme_irq irq;
    int op;

    if( copy_from_user(&irq, (void *)arg, sizeof(irq)))
    {
      return( -EFAULT);
    }
    op = cmd & 0xf0;
    if( !op)
    {
      retval = -EINVAL;
    }

    if( op & PEV_IOCTL_VME_IRQ_ALLOC)
    {
      if( pev_vme_irq_alloc( pev, &irq) < 0)
      {
	retval = -EINVAL;
      }
    }
    if( op & PEV_IOCTL_VME_IRQ_ARM)
    {
      if( pev_vme_irq_arm( pev, &irq) < 0)
      {
	retval = -EINVAL;
      }
    }
    if( op & PEV_IOCTL_VME_IRQ_WAIT)
    {
      if( pev_vme_irq_wait( pev, &irq) < 0)
      {
	retval = -EINVAL;
      }
    }
    if( op & PEV_IOCTL_VME_IRQ_CLEAR)
    {
      if( pev_vme_irq_clear( pev, &irq) < 0)
      {
	retval = -EINVAL;
      }
    }
    if( copy_to_user( (void *)arg, &irq, sizeof( irq)))
    {
      return -EFAULT;
    }
    return( 0);
  }
  switch ( cmd)
  {
    case PEV_IOCTL_VME_CONF_RD:
    {
      struct pev_ioctl_vme_conf conf;

      pev_vme_conf_read( pev, &conf);
      if( copy_to_user( (void *)arg, &conf, sizeof( conf)))
      {
	return -EFAULT;
      }
      break;
    }
    case PEV_IOCTL_VME_CONF_WR:
    {
      struct pev_ioctl_vme_conf conf;

      if( copy_from_user(&conf, (void *)arg, sizeof(conf)))
      {
	return( -EFAULT);
      }
      pev_vme_conf_write( pev, &conf);
      break;
    }
    case PEV_IOCTL_VME_CRCSR:
    {
      struct pev_ioctl_vme_crcsr crcsr;

      if( copy_from_user(&crcsr, (void *)arg, sizeof(crcsr)))
      {
	return( -EFAULT);
      }
      pev_vme_crcsr( pev, &crcsr);
      if( copy_to_user( (void *)arg, &crcsr, sizeof( crcsr)))
      {
	return -EFAULT;
      }
      break;
    }
    case PEV_IOCTL_VME_RMW:
    {
      struct pev_ioctl_vme_rmw rmw;

      if( copy_from_user(&rmw, (void *)arg, sizeof(rmw)))
      {
	return( -EFAULT);
      }
      pev_vme_rmw( pev, &rmw);
      if( copy_to_user( (void *)arg, &rmw, sizeof( rmw)))
      {
	return -EFAULT;
      }
      break;
    }
    case PEV_IOCTL_VME_LOCK:
    {
      struct pev_ioctl_vme_lock lock;

      if( copy_from_user(&lock, (void *)arg, sizeof(lock)))
      {
	return( -EFAULT);
      }
      pev_vme_lock( pev, &lock);
      if( copy_to_user( (void *)arg, &lock, sizeof( lock)))
      {
	return -EFAULT;
      }
      break;
    }
    case PEV_IOCTL_VME_UNLOCK:
    {
      pev_vme_unlock( pev);
      break;
    }
    case PEV_IOCTL_VME_SLV_INIT:
    {
      pev_vme_slv_init( pev);
      break;
    }
    case PEV_IOCTL_VME_IRQ_INIT:
    {
      pev_vme_irq_init( pev);
      break;
    }
#ifdef JFG
    case PEV_IOCTL_VME_IRQ_ARM:
    {
      uint irq;

      if( copy_from_user(&irq, (void *)arg, sizeof(irq)))
      {
	return( -EFAULT);
      }
      pev_vme_irq_arm( pev, irq);
      break;
    }
    case PEV_IOCTL_VME_IRQ_WAIT:
    {
      struct pev_ioctl_vme_irq irq;

      if( copy_from_user(&irq, (void *)arg, sizeof(irq)))
      {
	return( -EFAULT);
      }
      pev_vme_irq_wait( pev, &irq);
      if( copy_to_user( (void *)arg, &irq, sizeof( irq)))
      {
	return -EFAULT;
      }
      break;
    }
#endif
    default:
    {
      retval = -EINVAL;
    }
  }
         
  return( retval);
}

