/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pevklib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the all functions called from the PEV1100 device 
 *    driver main file (pevdrvr.c).
 *    Each function takes a pointer to the PEV1100 device data structure as first
 *    driver argument (struct pev_dev *).
 *    To perform the requested operation, these functions rey on a set of low
 *    level libraries:
 *      -> i2clib.c
 *      -> maplib.c
 *      -> pevlib.c
 *      -> rdwrlib.c
 *      -> sflashlib.c
 *      -> vmelib.c
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
 * $Log: pevklib.c,v $
 * Revision 1.6  2012/05/15 14:35:48  kalt_r
 * workaround with down_timeout 2
 *
 * Revision 1.5  2012/04/25 13:18:28  kalantari
 * added i2c epics driver and updated linux driver to v.4.10
 *
 * Revision 1.40  2012/04/18 07:42:07  ioxos
 * delay work around for eeprom_wr [JFG]
 *
 * Revision 1.39  2012/04/17 07:47:03  ioxos
 * support pipe mode on PPC [JFG]
 *
 * Revision 1.38  2012/04/12 13:33:13  ioxos
 * support for dma swapping + eeprom bug correction [JFG]
 *
 * Revision 1.37  2012/04/05 13:42:36  ioxos
 * dynamic io_remap + pev_fifo_wait_ef: reset sem & clear any pending IRQ [JFG]
 *
 * Revision 1.36  2012/03/29 09:07:15  ioxos
 * update fifo->cnt with wcnt [JFG]
 *
 * Revision 1.35  2012/03/27 09:17:40  ioxos
 * add support for FIFOs [JFG]
 *
 * Revision 1.34  2012/02/03 10:26:07  ioxos
 * dynamic use of elbc for i2c [JFG]
 *
 * Revision 1.33  2012/01/30 16:42:54  ioxos
 * pex access with new i2clib [JFG]
 *
 * Revision 1.32  2012/01/27 13:13:05  ioxos
 * prepare release 4.01 supporting x86 & ppc [JFG]
 *
 * Revision 1.31  2011/12/06 13:15:18  ioxos
 * support for multi task VME IRQ [JFG]
 *
 * Revision 1.30  2011/10/19 14:09:58  ioxos
 * support for vme irq handling + powerpc [JFG]
 *
 * Revision 1.29  2011/09/06 09:11:13  ioxos
 * use unlocked_ioctl instead of ioctl [JFG]
 *
 * Revision 1.28  2011/05/02 14:38:00  ioxos
 * bug: pg_size for mas64 and slave [JFG]
 *
 * Revision 1.27  2011/03/03 15:42:15  ioxos
 * support for 1MBytes VME slave granularity [JFG]
 *
 * Revision 1.26  2011/01/25 13:40:43  ioxos
 * support for VME RMW [JFG]
 *
 * Revision 1.24  2010/09/14 07:34:53  ioxos
 * keep PCI address in map structure [JFG]
 *
 * Revision 1.23  2010/08/31 13:34:30  ioxos
 * remove debug [JFG]
 *
 * Revision 1.22  2010/08/31 11:58:37  ioxos
 * bug in helios_sflash_write() [JFG]
 *
 * Revision 1.21  2010/08/02 10:26:08  ioxos
 * use get_free_pages() instead of kmalloc() [JFG]
 *
 * Revision 1.20  2010/01/13 16:49:35  ioxos
 * xenomai support for DMA list [JFG]
 *
 * Revision 1.19  2010/01/08 11:20:07  ioxos
 * add support to read DMA list from VME [JFG]
 *
 * Revision 1.18  2009/12/15 17:13:25  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.17  2009/11/10 09:06:15  ioxos
 * add support for DMA transfer mode [JFG]
 *
 * Revision 1.16  2009/10/02 07:56:45  ioxos
 * Correct bug for 16Mb sflash [CM]
 *
 * Revision 1.15  2009/09/29 12:43:38  ioxos
 * support to read/write sflash status [JFG]
 *
 * Revision 1.14  2009/07/17 13:21:01  ioxos
 * support to access SHM reserved for DMA + correct bug in DME status report [JFG]
 *
 * Revision 1.13  2009/05/20 08:18:26  ioxos
 * add XENOMAI synchronization for dma functions [JFG]
 *
 * Revision 1.12  2009/04/14 14:01:18  ioxos
 * base frequency for timer is now 100 MHz [JFG]
 *
 * Revision 1.11  2009/04/06 12:16:55  ioxos
 * add timer and histo + update sflash functions [JFG]
 *
 * Revision 1.10  2009/02/09 13:56:29  ioxos
 * enable DMA irq according to engine actually started [JFG]
 *
 * Revision 1.9  2009/01/09 13:10:10  ioxos
 * add support for DMA status [JFG]
 *
 * Revision 1.8  2008/12/12 13:35:14  jfg
 * allow to register irq handle + dma functions [JFG]
 *
 * Revision 1.7  2008/11/12 09:30:42  ioxos
 * improve sflash write, add map_find and timer functions [JFG]
 *
 * Revision 1.6  2008/09/17 11:36:15  ioxos
 * add support for VME configuration & read loop with data check [JFG]
 *
 * Revision 1.5  2008/08/08 08:55:47  ioxos
 * access to PEX86xx and PLX8112 + allocate buffers for DMA + bug in vme slave scstter/gather [JFG]
 *
 * Revision 1.4  2008/07/18 14:44:17  ioxos
 * correct map size calculation and implement map cleanup [JFG]
 *
 * Revision 1.3  2008/07/04 07:40:12  ioxos
 * update address mapping functions [JFG]
 *
 * Revision 1.2  2008/07/01 10:34:31  ioxos
 * add function pev_sflash_id() [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
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

#ifdef XENOMAI
#include <rtdm/rtdm_driver.h>
#endif

#include "../include/pevioctl.h"
#include "rdwrlib.h"
#include "sflashlib.h"
#include "maplib.h"
#include "i2clib.h"
#include "fpgalib.h"
#include "vmelib.h"
#include "dmalib.h"
#include "fifolib.h"
#include "pevdrvr.h"
#include "pevklib.h"
#include "histolib.h"

#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

uint vme_irq_vector[16];
uint vme_irq_idx[16];

void pev_irq_register( struct pev_dev *pev,
		       int src,
		       void (* func)( struct pev_dev*, int, void *),
		       void *arg)
{
  pev->irq_tbl[src].func = func;
  pev->irq_tbl[src].arg = arg;
}

int
pev_rdwr(struct pev_dev *pev,
         struct pev_ioctl_rdwr *rdwr_p)
{
  struct pev_rdwr_mode *m;
  void *pev_addr;
  int pev_remap;

  m = &rdwr_p->mode;
  if( ( m->space < 0x10) && !pev->dev)
  {
    return( -ENODEV);
  }

  pev_addr = NULL;
  pev_remap = 0;

  if( rdwr_p->len) /* block transfer */
  {
    if( m->space < RDWR_PMEM) /* block are not supported in cfg and io */
    {
      return( -EFAULT);
    }
    if( m->space == RDWR_PMEM)
    {
      if( rdwr_p->offset + rdwr_p->len >= pev->pmem_len) return( -EFAULT);
      //pev_addr = pev->pmem_ptr + rdwr_p->offset;
      pev_addr = ioremap( pev->pmem_base + rdwr_p->offset, rdwr_p->len);
      pev_remap = 1;
    }
    if( m->space == RDWR_MEM)
    {
      if( rdwr_p->offset + rdwr_p->len >= pev->mem_len) return( -EFAULT);
      //pev_addr = pev->mem_ptr + rdwr_p->offset;
      pev_addr = ioremap( pev->mem_base + rdwr_p->offset, rdwr_p->len);
      pev_remap = 1;
    }
    if( m->space == RDWR_CSR)
    {
      if( rdwr_p->offset + rdwr_p->len >= pev->csr_len) return( -EFAULT);
      pev_addr = pev->csr_ptr + rdwr_p->offset;
    }
    if( m->space == RDWR_ELB)
    {
      if( pev->board != PEV_BOARD_IFC1210) /* ELB access only on IFC1210 */
      {
	return( -EFAULT);
      }
      if( rdwr_p->offset + rdwr_p->len >= pev->elb_len) return( -EFAULT);
      pev_addr = pev->elb_ptr + rdwr_p->offset;
    }
    if( m->space == RDWR_DMA_SHM)
    {
      if( rdwr_p->offset + rdwr_p->len >= pev->dma_shm_len) return( -EFAULT);
      pev_addr = pev->dma_shm_ptr + rdwr_p->offset;
    }
    if( m->space == RDWR_PEX)
    {
      if( pev->board == PEV_BOARD_IFC1210) /* block are not supported on IDT CSR regs */
      {
	return( -EFAULT);
      }
      if( rdwr_p->offset + rdwr_p->len >= pev->pex_len) return( -EFAULT);
      if( rdwr_p->offset < 0x10000)
      {
	pev_addr = pev->pex_ptr + rdwr_p->offset;
      }
      else
      {
	if( !pev->plx_ptr)
        {
          return( -EFAULT);
        }
	pev_addr = pev->plx_ptr + rdwr_p->offset - 0x10000;
      }
    }
    if( m->space == RDWR_KMEM)
    {
      pev_addr = rdwr_p->k_addr  + rdwr_p->offset;
    }

    if( pev_addr)
    {
      int retval;

      retval = 0;
      switch( m->dir)
      {
        case RDWR_WRITE: /* write cycle */
        {
	  retval = rdwr_wr_blk( rdwr_p->buf, pev_addr, rdwr_p->len, m);
	  break;
        }
        case RDWR_READ: /* read cycle */
        {
	  retval = rdwr_rd_blk( rdwr_p->buf, pev_addr, rdwr_p->len, m);
	  break;
        }
        case RDWR_LOOP_READ:
        case RDWR_LOOP_WRITE:
        case RDWR_LOOP_RDWR:
        case RDWR_LOOP_CHECK:
        {
	  retval = rdwr_loop( rdwr_p->buf, pev_addr, rdwr_p->len, m);
        }
      }
      return( retval);
    }
  }
  /* single cycle */
  if( m->space == RDWR_CFG) /* PCI Config cycle */
  {
    if( rdwr_p->offset >= 0x1000) return( -EINVAL);
    if( m->dir) /* write cycle */
    {
      return( rdwr_cfg_wr( pev->dev, rdwr_p->buf, rdwr_p->offset, m));
    }
    else /* read cycle */
    {
      return( rdwr_cfg_rd( pev->dev, rdwr_p->buf, rdwr_p->offset, m));
    }
  }
  if( m->space == RDWR_IO) /* PCI IO cycle */
  {
    if( rdwr_p->offset >  pev->io_len - m->ds) return( -EINVAL);
    if( m->dir) /* write cycle */
    {
      return( rdwr_io_wr( pev->io_base, rdwr_p->buf, rdwr_p->offset, m));
    }
    else /* read cycle */
    {
      return( rdwr_io_rd( pev->io_base, rdwr_p->buf, rdwr_p->offset, m));
    }
  }
  if( m->space == RDWR_PMEM) /* PMEM cycle */
  {
    if( rdwr_p->offset >  pev->pmem_len - m->ds) return( -EINVAL);
    //pev_addr = pev->pmem_ptr + rdwr_p->offset;
    pev_addr = ioremap( pev->pmem_base + rdwr_p->offset, 8);
    pev_remap = 1;
  }
  if( m->space == RDWR_MEM) /* MEM cycle */
  {
    if( rdwr_p->offset >  pev->mem_len - m->ds) return( -EINVAL);
    //pev_addr = pev->mem_ptr + rdwr_p->offset;
    pev_addr = ioremap( pev->mem_base + rdwr_p->offset, 8);
    pev_remap = 1;
  }
  if( m->space == RDWR_CSR) /* CSR cycle */
  {
    if( rdwr_p->offset >  pev->csr_len - m->ds) return( -EINVAL);
    pev_addr = pev->csr_ptr + rdwr_p->offset;
  }
  if( m->space == RDWR_ELB) /* ELB cycle */
  {
    if( pev->board != PEV_BOARD_IFC1210) /* ELB access only on IFC1210 */
    {
      return( -EFAULT);
    }
    if( rdwr_p->offset >  pev->elb_len - m->ds) return( -EINVAL);
    pev_addr = pev->elb_ptr + rdwr_p->offset;
  }
  if( m->space == RDWR_DMA_SHM)
  {
    if( rdwr_p->offset >  pev->mem_len - m->ds) return( -EINVAL);
    if( rdwr_p->offset > pev->dma_shm_len - m->ds) return( -EFAULT);
    pev_addr = pev->dma_shm_ptr + rdwr_p->offset;
  }
  if( m->space == RDWR_PEX) /* PEX cycle */
  {
    if( pev->board == PEV_BOARD_IFC1210)
    {
      if( rdwr_p->offset >= 0x100000) return( -EINVAL);
      if( m->dir) /* write cycle */
      {
        return( rdwr_idt_wr( pev->pex, rdwr_p->buf, rdwr_p->offset, m));
      }
      else /* read cycle */
      {
        return( rdwr_idt_rd( pev->pex, rdwr_p->buf, rdwr_p->offset, m));
      }
    }
    if( rdwr_p->offset >  pev->pex_len - m->ds) return( -EINVAL);
    if( rdwr_p->offset < 0x10000)
    {
      pev_addr = pev->pex_ptr + rdwr_p->offset;
    }
    else
    {
      if( !pev->plx_ptr)
      {
        return( -EFAULT);
      }
      pev_addr = pev->plx_ptr + rdwr_p->offset - 0x10000;
    }
  }
  if( m->space == RDWR_KMEM)
  {
    pev_addr = rdwr_p->k_addr  + rdwr_p->offset;
  }

  if( pev_addr)
  {
    if( m->dir) /* write cycle */
    {
      return( rdwr_wr_sgl( rdwr_p->buf, pev_addr, m));
    }
    else /* read cycle */
    {
      return( rdwr_rd_sgl( rdwr_p->buf, pev_addr, m));
    }
    if( pev_remap)
    {
      iounmap( pev_addr);
    }
  }
  return( -EINVAL);
}

static unsigned long
pev_sflash_set_reg( struct pev_dev *pev,
		    uint dev)
{
  unsigned long reg_p;

  if( pev->board == PEV_BOARD_IFC1210)
  {
    if( !dev) return(-1);
    reg_p = (unsigned long)pev->elb_ptr + 0x28;
  }
  else
  {
    if( dev) return(-1);
    if( pev->io_remap.short_io)
    {
      outl( PEV_SCSR_SEL_ILOC, pev->io_base);
    }
    reg_p =(unsigned long)pev->io_base + pev->io_remap.iloc_spi;
  }
  sflash_set_dev(dev);

  return( reg_p);
}

void
pev_sflash_id( struct pev_dev *pev,
	       unsigned char *id,
	       uint dev)
{
  unsigned long reg_p;

  reg_p = pev_sflash_set_reg( pev, dev);
  if( reg_p == -1) return;
  sflash_read_ID( reg_p, id);
  return;
}

unsigned short
pev_sflash_rdsr( struct pev_dev *pev,
		 uint dev)
{
  unsigned long reg_p;

  reg_p = pev_sflash_set_reg( pev, dev);
  if( reg_p == -1) return(0);
  return( sflash_read_status( reg_p));
}

void
pev_sflash_wrsr( struct pev_dev *pev,
	         unsigned short sr,
		 uint dev)
{
  unsigned long reg_p;

  reg_p = pev_sflash_set_reg( pev, dev);
  if( reg_p == -1) return;
  sflash_write_status( reg_p, sr);
  return;
}

int
pev_sflash_read(struct pev_dev *pev,
		struct pev_ioctl_sflash_rw *rdwr_p)
{
  unsigned char *k_buf;
  unsigned long reg_p;
  int retval;
  int order;

  retval = 0;
  reg_p = pev_sflash_set_reg( pev, rdwr_p->dev);
  if( reg_p == -1)
  {
    return( -EIO);
  }
  order = get_order( rdwr_p->len); 
  k_buf = (unsigned char *)__get_free_pages( GFP_KERNEL, order);
  if( !k_buf)
  {
    return(-ENOMEM);
  }

  sflash_read_data( reg_p, rdwr_p->offset, k_buf, rdwr_p->len);

  if( copy_to_user( rdwr_p->buf, k_buf, rdwr_p->len))
  {
    retval = -EIO;
  }
  free_pages( (unsigned long)k_buf, order);

  return( retval);
}

int
pev_sflash_write(struct pev_dev *pev,
		 struct pev_ioctl_sflash_rw *rdwr_p)
{
  uint s_start;         /* start of first sector                    */
  uint s_end;           /* end of last sector                       */
  uint k_size;          /* total size to be erased and reprogrammed */
  uint first;           /* data offset in first sector              */
  uint last ;           /* data offset in last sector              */
  unsigned char *k_buf; /* temporary kernel buffer                  */
  int retval;           /* function return value                    */
  int order;
  unsigned long reg_p;
  int sect_size, sect_mask;

  retval = 0;

  if( pev->board == PEV_BOARD_IFC1210)
  {
    sect_size = 0x10000;
  }
  else
  {
    sect_size = 0x40000;
  }
  sect_mask = sect_size - 1;
  if (!rdwr_p->len)
  {
    return retval;
  }
  reg_p = pev_sflash_set_reg( pev, rdwr_p->dev);
  if( reg_p == -1)
  {
    return( -EIO);
  }
  s_start = rdwr_p->offset & ~sect_mask;
  first = rdwr_p->offset & sect_mask;
  s_end = (rdwr_p->offset + rdwr_p->len) & ~sect_mask;
  last =  (rdwr_p->offset + rdwr_p->len) & sect_mask;

  if( last)
  {
    s_end += sect_size;
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

  if( first)
  {
    sflash_read_data( reg_p, s_start, k_buf, first);
  }
  if( last)
  {
    sflash_read_data( reg_p, s_end - sect_size + last, k_buf + first + rdwr_p->len, sect_size - last);
  }

  if( copy_from_user( k_buf + first, rdwr_p->buf, rdwr_p->len))
  {
    free_pages( (unsigned long)k_buf, order);
    return( -EIO);
  }
  if( sflash_write_sector( reg_p, s_start, k_buf, k_size, sect_size) < 0)
  {
    retval = -EINVAL;
  }
  free_pages( (unsigned long)k_buf, order);
  return( retval);
}

int
pev_fpga_load(struct pev_dev *pev,
	      struct pev_ioctl_sflash_rw *rdwr_p)
{
  unsigned char *k_buf; /* temporary kernel buffer                  */
  int retval;           /* function return value                    */
  int order;
  unsigned long reg_p;
  int nblk, res, first, bsize;
  unsigned char *s;

  retval = 0;
  bsize = 0x100000;
  order = get_order( bsize); 
  k_buf = (unsigned char *)__get_free_pages( GFP_KERNEL, order);
  if( !k_buf)
  {
    return(-ENOMEM);
  }

  reg_p = (unsigned long)pev->elb_ptr + 0x20;

  nblk =  rdwr_p->len/bsize;
  res =   rdwr_p->len%bsize;
  s = rdwr_p->buf;
  first = 1;
  fpga_set_dev( rdwr_p->dev);
  while( nblk--)
  {
    if( copy_from_user( k_buf, s, bsize))
    {
      retval = -EIO;
      goto pev_fpga_load_exit;
    }
    retval = fpga_load( reg_p, k_buf, bsize, first);
    if( retval == -1)
    {
      retval = -EIO;
      goto pev_fpga_load_exit;
    }
    first = 0;
    s += bsize;

  }
  if( res)
  {
    if( copy_from_user( k_buf, s, res))
    {
      retval = -EIO;
      goto pev_fpga_load_exit;
    }
    retval = fpga_load( reg_p, k_buf, res+100, first);
    if( retval == -1)
    {
      retval = -EIO;
      goto pev_fpga_load_exit;
    }
  }

pev_fpga_load_exit:
  free_pages( (unsigned long)k_buf, order);
  return( retval);
}

void
pev_sg_master_32_set( struct pev_dev *pev,
                      uint off,
		      ulong rem_addr,
		      uint mode)
{
  outl( 0x10000 + (4*off), pev->io_base + pev->io_remap.pcie_mmu);
  outl( mode, pev->io_base + pev->io_remap.pcie_mmu + 0x4);
  outl( 0x10002 + (4*off), pev->io_base + pev->io_remap.pcie_mmu);
  outl( (uint)(rem_addr >> 18), pev->io_base + pev->io_remap.pcie_mmu + 0x4);

  return;
}

void
pev_sg_master_64_set( struct pev_dev *pev,
                      uint off,
		      ulong rem_addr,
		      uint mode)
{
  outl( 4*off, pev->io_base + pev->io_remap.pcie_mmu);
  outl( mode, pev->io_base + pev->io_remap.pcie_mmu + 0x4);
  outl( 2 + (4*off), pev->io_base + pev->io_remap.pcie_mmu);
  outl( (uint)(rem_addr >> 18), pev->io_base + pev->io_remap.pcie_mmu + 0x4);

  return;
}

void
pev_sg_slave_vme_set( struct pev_dev *pev,
                      uint off,
		      ulong rem_addr,
		      uint mode)
{
  outl( 8*off, pev->io_base + pev->io_remap.vme_base + 0x10);
  outl( mode, pev->io_base + pev->io_remap.vme_base + 0x14);
  outl( 2 + (8*off), pev->io_base + pev->io_remap.vme_base + 0x10);
  outl( (uint)(rem_addr >> 16), pev->io_base + pev->io_remap.vme_base + 0x14);
#ifdef PPC
  outl( 4 + (8*off), pev->io_base + pev->io_remap.vme_base + 0x10);
  outl( 0, pev->io_base + pev->io_remap.vme_base + 0x14);
  outl( 6 + (8*off), pev->io_base + pev->io_remap.vme_base + 0x10);
  outl( 0, pev->io_base + pev->io_remap.vme_base + 0x14);
#else
  outl( 4 + (8*off), pev->io_base + pev->io_remap.vme_base + 0x10);
  outl( (uint)(rem_addr >> 32), pev->io_base + pev->io_remap.vme_base + 0x14);
  outl( 6 + (8*off), pev->io_base + pev->io_remap.vme_base + 0x10);
  outl( (uint)(rem_addr >> 48), pev->io_base + pev->io_remap.vme_base + 0x14);
#endif
  return;
}


int
pev_map_set_sg( struct pev_dev *pev,
                struct pev_ioctl_map_ctl *mc,
	        uint off)
{
  uint i;
  ulong rem_addr;
  struct pev_map_blk *mp;
  uint mode;

  mp = mc->map_p;
  if( !( mp[off].flag & MAP_FLAG_BUSY))
  {
    return( -1);
  }
  rem_addr = mp[off].rem_addr;
  mode = mp[off].mode;
  switch( mc->sg_id)
  {
    case MAP_MASTER_32:
    {
      for( i = off; i < off + mp[off].npg; i++)
      {
	pev_sg_master_32_set( pev, i, rem_addr, mode);
        rem_addr += pev->map_mas32.pg_size;
      }
      break;
    }
    case MAP_MASTER_64:
    {
      for( i = off; i < off + mp[off].npg; i++)
      {
	pev_sg_master_64_set( pev, i, rem_addr, mode);
        rem_addr += pev->map_mas64.pg_size;
      }
      break;
    }
    case MAP_SLAVE_VME:
    {
      uint tmp;

      tmp = inl( pev->io_base + pev->io_remap.vme_base + 0x8) & 0xf;
      if( tmp & VME_SLV_1MB)
      {
        uint base, delta, size, pg;

        size = 0x100000 << (tmp&7);
        base = inl( pev->io_base + pev->io_remap.vme_ader) << 24;
        base += inl( pev->io_base + pev->io_remap.vme_ader + 4) << 16;
	delta = (base % size)>>20;
        for( i = off; i < off + mp[off].npg; i++)
        {
	  pg = (i+delta) & ((size>>20)-1);
 	  pev_sg_slave_vme_set( pev, pg, rem_addr, mode);
          rem_addr += pev->map_mas32.pg_size;
        }
      }
      else
      {
        for( i = off; i < off + mp[off].npg; i++)
        {
 	  pev_sg_slave_vme_set( pev, i, rem_addr, mode);
          rem_addr += pev->map_mas32.pg_size;
        }
      } 
      break;
    }
    default:
    {
      return( -1);
    }
  }
  return( 0);
}


int
pev_map_clear_sg( struct pev_dev *pev,
		  struct pev_ioctl_map_ctl *mc,
	          uint off)
{
  uint i;
  struct pev_map_blk *mp;

  mp = mc->map_p;
  if( !( mp[off].flag & MAP_FLAG_BUSY))
  {
    return( -1);
  }
  switch( mc->sg_id)
  {
    case MAP_MASTER_32:
    {
      for( i = off; i < off + mp[off].npg; i++)
      {
	pev_sg_master_32_set( pev, i, 0, 0);
      }
      break;
    }
    case MAP_MASTER_64:
    {
      for( i = off; i < off + mp[off].npg; i++)
      {
	pev_sg_master_64_set( pev, i, 0, 0);
      }
      break;
    }
    case MAP_SLAVE_VME:
    {
      uint tmp;

      tmp = inl( pev->io_base + pev->io_remap.vme_base + 0x8) & 0xf;
      if( tmp & VME_SLV_1MB)
      {
        uint base, delta, size, pg;

        size = 0x100000 << (tmp&7);
        base = inl( pev->io_base + pev->io_remap.vme_ader) << 24;
        base += inl( pev->io_base + pev->io_remap.vme_ader + 4) << 16;
	delta = (base % size)>>20;
        for( i = off; i < off + mp[off].npg; i++)
        {
	  pg = (i+delta) & ((size>>20)-1);
  	  pev_sg_slave_vme_set( pev, pg, 0, 0);
        } 
      }
      else
      {
        for( i = off; i < off + mp[off].npg; i++)
        {
  	  pev_sg_slave_vme_set( pev, i, 0, 0);
        } 
      } 
      break;
    }
    default:
    {
      return( -1);
    }
  }
  return( 0);
}
  
int
pev_map_init(struct pev_dev *pev,
             struct pev_ioctl_map_ctl *map_ctl_p)
{
  int map_size;
  map_size = sizeof( struct pev_map_blk) * map_ctl_p->pg_num;
  if( !map_ctl_p->map_p)
  {
    map_ctl_p->map_p = (struct pev_map_blk *)kmalloc( map_size, GFP_KERNEL);
  }
  memset( (void *)map_ctl_p->map_p, 0, map_size);
  map_ctl_p->map_p[0].npg = map_ctl_p->pg_num;
  return( 0);
}

struct pev_ioctl_map_ctl *
map_get_map( struct pev_dev *pev,
	     char sg_id)
{
  struct pev_ioctl_map_ctl *mc;

  switch( sg_id)
  {
    case MAP_MASTER_32:
    {
      mc = &pev->map_mas32;
      break;
    }
    case MAP_MASTER_64:
    {
      mc = &pev->map_mas64;
      break;
    }
    case MAP_SLAVE_VME:
    {
      mc = &pev->map_slave;
      break;
    }
    default:
    {
      return(0);
    }
  }
  if( mc->sg_id != sg_id)
  {
    return( 0);
  }
  return( mc);
}

int
pev_map_alloc(struct pev_dev *pev,
	      struct pev_ioctl_map_pg *map_pg_p)
{
  uint off;
  struct pev_ioctl_map_ctl *mc;

  mc = map_get_map( pev, map_pg_p->sg_id);
  if( !mc)
  {
    return(-1);
  }

  off = map_blk_alloc( mc, map_pg_p);
  if( off < 0)
  {
    return( -1);
  }
  else
  {
    map_pg_p->pci_base = mc->loc_base;
    return( pev_map_set_sg( pev, mc, off));
  }
}

int
pev_map_modify(struct pev_dev *pev,
	       struct pev_ioctl_map_pg *map_pg_p)
{
  struct pev_ioctl_map_ctl *mc;

  mc = map_get_map( pev, map_pg_p->sg_id);
  if( !mc)
  {
    return(-1);
  }
  if( map_blk_modify( mc, map_pg_p) < 0)
  {
    return( -1);
  }
  else
  {
    return( pev_map_set_sg( pev, mc, map_pg_p->offset));
  }
}

int
pev_map_find(struct pev_dev *pev,
	     struct pev_ioctl_map_pg *map_pg_p)
{
  struct pev_ioctl_map_ctl *mc;

  mc = map_get_map( pev, map_pg_p->sg_id);
  if( !mc)
  {
    return(-1);
  }
  if( map_blk_find( mc, map_pg_p) < 0)
  {
    return( -1);
  }
  return( 0);
}

int
pev_map_free(struct pev_dev *pev,
	     struct pev_ioctl_map_pg *map_pg_p)
{
  struct pev_ioctl_map_ctl *mc;

  mc = map_get_map( pev, map_pg_p->sg_id);
  if( !mc)
  {
    return(-1);
  }
  if(  pev_map_clear_sg( pev, mc, map_pg_p->offset) < 0)
  {
    return( -1);
  }
  else
  {
    return( map_blk_free( mc, map_pg_p->offset));
  }
}

int
pev_map_read(struct pev_dev *pev,
	     struct pev_ioctl_map_ctl *map_ctl_p)
{
  struct pev_ioctl_map_ctl *mc;

  mc = map_get_map( pev, map_ctl_p->sg_id);
  if( !mc)
  {
    map_ctl_p->sg_id = MAP_INVALID;
    return(-1);
  }
  map_ctl_p->pg_num = mc->pg_num;
  map_ctl_p->pg_size = mc->pg_size;
  map_ctl_p->loc_base = mc->loc_base;
  if( map_ctl_p->map_p)
  {
    if( copy_to_user( map_ctl_p->map_p, mc->map_p, (sizeof( struct pev_map_blk))*mc->pg_num))
    {
      return( -EIO);
    }
  }
  return(0);
}

int
pev_map_clear(struct pev_dev *pev,
	     struct pev_ioctl_map_ctl *map_ctl_p)
{
  struct pev_ioctl_map_ctl *mc;
  int map_size;


  mc = map_get_map( pev, map_ctl_p->sg_id);
  if( !mc)
  {
    map_ctl_p->sg_id = MAP_INVALID;
    return(-1);
  }
  map_size = sizeof( struct pev_map_blk) * mc->pg_num;
  memset( (void *)mc->map_p, 0, map_size);
  mc->map_p[0].npg = mc->pg_num;
  pev_map_clear_sg(pev, mc, 0);
  return(0);
}




static ulong
pev_i2c_set_reg( struct pev_dev *pev,
		 int elbc)
{
  ulong reg_p;

  if( (pev->board == PEV_BOARD_IFC1210) && elbc)
  {
    i2c_set_elb( 1);
    reg_p = (ulong)pev->elb_ptr + 0x30;
  }
  else
  {
    i2c_set_elb(0);
    if( pev->io_remap.short_io)
    {
      outl( PEV_SCSR_SEL_ILOC, pev->io_base);
    }
    reg_p = (ulong)pev->io_base + pev->io_remap.iloc_i2c;
  }
  return( reg_p);
}

void
pev_i2c_dev_cmd(struct pev_dev *pev,
	         struct pev_ioctl_i2c *i2c_para_p)
{
  ulong reg_p;
  int elbc;

  elbc = i2c_para_p->device & 0x80;
  i2c_para_p->device &= ~0x80;
  reg_p = pev_i2c_set_reg( pev, elbc);
  //printk("pev_i2c_dev_cmd(): %08lx - %08x -%08x\n", reg_p, i2c_para_p->device, i2c_para_p->cmd);
  i2c_cmd( reg_p, i2c_para_p->device, i2c_para_p->cmd);

  return;
}

void
pev_i2c_dev_read(struct pev_dev *pev,
	         struct pev_ioctl_i2c *i2c_para_p)
{
  ulong reg_p;
  int elbc;

  elbc = i2c_para_p->device & 0x80;
  i2c_para_p->device &= ~0x80;
  reg_p = pev_i2c_set_reg( pev, elbc);
  //printk("pev_i2c_dev_read(): %08lx - %08x -%08x\n", reg_p, i2c_para_p->device, i2c_para_p->cmd);
  i2c_cmd( reg_p, i2c_para_p->device, i2c_para_p->cmd);
  i2c_wait( reg_p, 100000);
  i2c_para_p->data = i2c_read( reg_p,  i2c_para_p->device);

  return;
}

void
pev_i2c_dev_write(struct pev_dev *pev,
	          struct pev_ioctl_i2c *i2c_para_p)
{
  ulong reg_p;
  int elbc;

  elbc = i2c_para_p->device & 0x80;
  i2c_para_p->device &= ~0x80;
  reg_p = pev_i2c_set_reg( pev, elbc);
  //printk("pev_i2c_dev_write(): %08lx - %08x -%08x\n", reg_p, i2c_para_p->device, i2c_para_p->cmd);
  i2c_write( reg_p,  i2c_para_p->device, i2c_para_p->cmd, i2c_para_p->data);
  i2c_wait( reg_p, 100000);

  return;
}

void
pev_i2c_pex_read(struct pev_dev *pev,
	         struct pev_ioctl_i2c *i2c_para_p)
{
  i2c_cmd( pev->io_base + pev->io_remap.iloc_i2c, 0x010f0069, i2c_para_p->cmd);
  i2c_wait( pev->io_base + pev->io_remap.iloc_i2c, 100000);
  i2c_para_p->data = i2c_read( pev->io_base + pev->io_remap.iloc_i2c, 0x010f0069);

  return;
}

void
pev_i2c_pex_write(struct pev_dev *pev,
	          struct pev_ioctl_i2c *i2c_para_p)
{
  i2c_write( pev->io_base + pev->io_remap.iloc_i2c, 0x010f0069, i2c_para_p->cmd, i2c_para_p->data);
  i2c_wait( pev->io_base + pev->io_remap.iloc_i2c, 100000);

  return;
}

int
pev_idt_eeprom_read(struct pev_dev *pev,
		    struct pev_ioctl_rdwr *rdwr_p)
{
  unsigned char *k_buf, *p;
  int order, data, tmo, i;
  int retval;

  if( (rdwr_p->offset + rdwr_p->len) > 0x10000)
  {
    return( -EIO);
  }

  order = get_order( rdwr_p->len); 
  k_buf = (unsigned char *)__get_free_pages( GFP_KERNEL, order);
  if( !k_buf)
  {
    return(-ENOMEM);
  }
  p = k_buf;

  if( pci_write_config_dword( pev->pex, 0xff8, 0x3f190))
  {
    return( -EIO);
  }
  for( i = 0; i < rdwr_p->len; i++)
  { 
    data = 0x4000000 + rdwr_p->offset + i;
    pci_write_config_dword( pev->pex, 0xffc, data);
    data = 0;
    tmo = 100000;
    do
    {
      pci_read_config_dword( pev->pex, 0xffc, &data);
    }
    while( ((data & 0x3000000) != 0x2000000) && --tmo);
    *p++ = (unsigned char)((data >> 16) & 0xff);
  }
  if( copy_to_user( rdwr_p->buf, k_buf, rdwr_p->len))
  {
    retval = -EIO;
  }

  free_pages( (unsigned long)k_buf, order);

  return( retval);
}

/* declare semaphore to perform delay */
struct semaphore pev_eeprom_sem;
int
pev_idt_eeprom_write(struct pev_dev *pev,
		     struct pev_ioctl_rdwr *rdwr_p)
{
  unsigned char *k_buf, *p;
  int order, i, data;
  volatile int tmo;
  int retval;

  if( (rdwr_p->offset + rdwr_p->len) > 0x10000)
  {
    return( -EIO);
  }

  order = get_order( rdwr_p->len); 
  k_buf = (unsigned char *)__get_free_pages( GFP_KERNEL, order);
  if( !k_buf)
  {
    return(-ENOMEM);
  }
  if( copy_from_user( k_buf, rdwr_p->buf, rdwr_p->len))
  {
    free_pages( (unsigned long)k_buf, order);
    return( -EIO);
  }

  if( pci_write_config_dword( pev->pex, 0xff8, 0x3f190))
  {
    return( -EIO);
  }
  p = k_buf;
  for( i = 0; i < rdwr_p->len; i++)
  { 
    /* init semaphore to perform delay */
    sema_init( &pev_eeprom_sem, 0);
    data = (*p++ << 16) + rdwr_p->offset + i;
    pci_write_config_dword( pev->pex, 0xffc, data);
    tmo = 100000;
    do
    {
      pci_read_config_dword( pev->pex, 0xffc, &data);
    }
    while( ((data & 0x3000000) != 0x2000000) && --tmo);
    /* add delay because hardware gives command complete too early !!!  */
    retval = down_timeout( &pev_eeprom_sem, 2);
  }

  free_pages( (unsigned long)k_buf, order);

  return( 0);
}

void
pev_vme_irq( struct pev_dev *pev,
	     int src,
	     void *arg)
{
  int idx;

  pev->vme_status |= VME_IRQ_RECEIVED | ( 0x10000 << (src&0xf));
  idx = vme_irq_idx[src&0xf];
  /* get interrupt vector */
  pev->vme_irq_ctl[idx].vector = inl(pev->io_base + pev->io_remap.vme_itc);
  /* mask interrupt belonging to the same set */
  outl( pev->vme_irq_ctl[idx].set, pev->io_base + pev->io_remap.vme_itc + 0x0c);
  /* raise semaphore */
  up( &pev->vme_irq_ctl[idx].sem);
}

uint
pev_vme_irq_alloc( struct pev_dev *pev,
		 struct pev_ioctl_vme_irq *irq)
{
  int i, idx;

  /* check if irq_set is free */
  if( ( pev->vme_irq_set & irq->irq) ||
      (irq->irq & 0xffff0000))
  {
    irq->irq |= 0x80000000;
    return( -1);
  }
  /* if free, allocate a control structure */
  for( i = 0; i < 16; i++)
  {
    if( !pev->vme_irq_ctl[i].set) break;
  }
  idx = i;
  if( idx == 16)
  {
    irq->irq |= 0x80000000;
    return( -1);
  }

  pev->vme_irq_set |= irq->irq;
  for( i = 0; i < 16; i++)
  {
    if( irq->irq & (1<<i)) vme_irq_idx[i] = idx;
  }
  pev->vme_irq_ctl[idx].set = irq->irq;
  irq->irq |= ( idx << 16);
  //printk("pev_vme_irq_alloc(): %08x - %08x - %08x\n", irq->irq, pev->vme_irq_set, pev->vme_irq_ctl[idx].set);

    /* mask VME  interrupts belonging to set */
  debugk(("VME IRQ unmasking -> %08x\ - %08n", pev->vme_irq_ctl[idx].set, pev->vme_irq_set));
  outl( pev->vme_irq_ctl[idx].set, pev->io_base + pev->io_remap.vme_itc + 0x0c);

  /* reset semahore */
  sema_init( &pev->vme_irq_ctl[idx].sem, 0);

  pev->vme_status = VME_IRQ_STARTED;
  return( 0);
}

uint
pev_vme_irq_arm( struct pev_dev *pev,
		 struct pev_ioctl_vme_irq *irq)
{
  int idx;

  idx = (irq->irq >> 16) & 0xf;
  //printk("pev_vme_irq_arm(): %08x - %08x - %08x\n", irq->irq, pev->vme_irq_set, pev->vme_irq_ctl[idx].set);
  if( (pev->vme_irq_ctl[idx].set & 0xffff) != (irq->irq & 0xffff))
  {
    return( -1);
  }
  /* reset semaphore */
  sema_init( &pev->vme_irq_ctl[idx].sem, 0);

    /* unmask VME  interrupts belonging to set */
  debugk(("VME IRQ unmasking -> %08x\ - %08n", pev->vme_irq_ctl[idx].set, pev->vme_irq_set));
  outl( pev->vme_irq_ctl[idx].set, pev->io_base + pev->io_remap.vme_itc + 0x08);

  pev->vme_status = VME_IRQ_STARTED;

  return( 0);
}

uint
pev_vme_irq_wait( struct pev_dev *pev,
		  struct pev_ioctl_vme_irq *irq)
{
  int jiffies;
  int retval;
  int idx;

  pev->vme_status |= VME_IRQ_WAITING;
  idx = (irq->irq >> 16) & 0xf;
  //printk("pev_vme_irq_wait(): %08x - %08x - %08x\n", irq->irq, pev->vme_irq_set, pev->vme_irq_ctl[idx].set);
  jiffies = msecs_to_jiffies( irq->tmo);
  if( irq->tmo)
  {
    retval = down_timeout( &pev->vme_irq_ctl[idx].sem, jiffies);
  }
  else
  {
    retval = down_interruptible( &pev->vme_irq_ctl[idx].sem);
  }

  if( retval)
  {
    pev->vme_status |= VME_IRQ_TMO;
    irq->vector = 0xffffffff;
    debugk(("VME IRQ aborted -> %x\n", pev->vme_status));
  }
  else
  {
    pev->vme_status |= VME_IRQ_ENDED;
    irq->vector = inl(pev->io_base + pev->io_remap.vme_itc);
    debugk(("VME IRQ received -> %x\n", pev->vme_status));
  }
  return( 0);
}


uint
pev_vme_irq_clear( struct pev_dev *pev,
	  	   struct pev_ioctl_vme_irq *irq)
{  
  int idx;

  idx = (irq->irq >> 16) & 0xf;
  //printk("pev_vme_irq_clear(): %08x - %08x - %08x\n", irq->irq, pev->vme_irq_set, pev->vme_irq_ctl[idx].set);
  if( (pev->vme_irq_ctl[idx].set & 0xffff) != (irq->irq & 0xffff))
  {
    return( -1);
  }
  pev->vme_irq_set &= ~pev->vme_irq_ctl[idx].set;
  pev->vme_irq_ctl[idx].set = 0;

    /* mask VME  interrupts belonging to set */
  debugk(("VME IRQ unmasking -> %08x\ - %08n", pev->vme_irq_ctl[idx].set, pev->vme_irq_set));
  outl( pev->vme_irq_ctl[idx].set, pev->io_base + pev->io_remap.vme_itc + 0x0c);

  return( 0);
}

void
pev_vme_irq_init(struct pev_dev *pev)
{
  int i;

  /* mask all VME interupt */
  pev->vme_status = 0;
  pev->vme_irq_set = 0;
  outl( 0xffff, pev->io_base + pev->io_remap.vme_itc + 0x0c);
  for( i = 0; i < 16; i++)
  {
    pev_irq_register( pev, 0x10+i, pev_vme_irq, &vme_irq_vector[i]);
    pev->vme_irq_ctl[i].set = 0;
    sema_init( &pev->vme_irq_ctl[i].sem, 0);
  }
  sema_init( &pev->vme_sem, 0);
  /* clear all pending interrupts and enable VME global interupt */
  outl( 0x7, pev->io_base + pev->io_remap.vme_itc + 0x04);
}


void
pev_vme_conf_read(struct pev_dev *pev,
	          struct pev_ioctl_vme_conf *conf)
{
  struct vme_ctl vme;
  struct vme_reg reg;

  if( pev->io_remap.short_io)
  {
    outl( PEV_SCSR_SEL_VME, pev->io_base);
  }
  reg.ctl = pev->io_base + pev->io_remap.vme_base;
  reg.ader = pev->io_base + pev->io_remap.vme_ader;
  reg.csr = pev->io_base + pev->io_remap.vme_csr;
  vme_conf_get( &reg, &vme);
  conf->a24_base  = (vme.arb & 0x1f0000)<<3;
  conf->a24_size  = 0x80000;
  conf->a32_base  = vme.ader & 0xfff00000;
  if( conf->slv_ena & VME_SLV_1MB)
  {
     conf->a32_size  = 0x100000 << ( vme.slave & 7) ;
  }
  else
  {
     conf->a32_size  = 0x1000000 << ( vme.slave & 7) ;
  }
  conf->x64       = (char)((vme.arb >> 24) & 1);
  conf->slot1     = (char)((vme.arb >> 25) & 1);
  conf->sysrst    = (char)((vme.arb >> 26) & 1);
  conf->rto       = (vme.arb & 0x10)?0:512;
  conf->arb       = (char)( vme.arb & 0x3);
  conf->bto       = 0x10 << (( vme.arb >> 8) & 3) ;
  conf->req       = (char)( vme.master & 0x3);
  conf->level     = (char)(( vme.master >>  2) & 3);
  conf->mas_ena   = (char)(( vme.master >> 31) & 1);
  conf->slv_ena   = (char)((( vme.slave  >> 31) & 1) | (vme.slave & VME_SLV_1MB));
  conf->slv_retry = (char)(( vme.slave >> 4) & 1);
  conf->burst     = (char)(( vme.slave >>  6) & 3);

  return;
}

void
pev_vme_conf_write(struct pev_dev *pev,
	           struct pev_ioctl_vme_conf *conf)
{
  struct vme_ctl vme;
  struct vme_reg reg;

  if( pev->io_remap.short_io)
  {
    outl( PEV_SCSR_SEL_VME, pev->io_base);
  }
  reg.ctl = pev->io_base + pev->io_remap.vme_base;
  reg.ader = pev->io_base + pev->io_remap.vme_ader;
  reg.csr = pev->io_base + pev->io_remap.vme_csr;

  vme.arb = ( conf->arb  & 3);
  if(  (conf->bto > 16) && (conf->bto <= 32)) vme.arb  |= 100;
  else if(  conf->bto <= 64) vme.arb  |= 200;
  else  vme.arb  |= 300;

  vme.master = ( conf->req  & 3) | (( conf->level  & 3) << 2);
  if( conf->mas_ena) vme.master |= 0x80000000;

  if( conf->slv_ena & VME_SLV_1MB)
  {
         if( conf->a32_size <=  0x100000) vme.slave = 8;
    else if( conf->a32_size <=  0x200000) vme.slave = 9;
    else if( conf->a32_size <=  0x400000) vme.slave = 0xa;
    else if( conf->a32_size <=  0x800000) vme.slave = 0xb;
    else if( conf->a32_size <= 0x1000000) vme.slave = 0xc;
    else if( conf->a32_size <= 0x2000000) vme.slave = 0xd;
    else if( conf->a32_size <= 0x4000000) vme.slave = 0xe;
    else vme.slave = 0xf;
  }
  else
  {
         if( conf->a32_size <=  0x1000000) vme.slave = 0;
    else if( conf->a32_size <=  0x2000000) vme.slave = 1;
    else if( conf->a32_size <=  0x4000000) vme.slave = 2;
    else if( conf->a32_size <=  0x8000000) vme.slave = 3;
    else if( conf->a32_size <= 0x10000000) vme.slave = 4;
    else if( conf->a32_size <= 0x20000000) vme.slave = 5;
    else if( conf->a32_size <= 0x40000000) vme.slave = 6;
    else vme.slave = 7;
  }
  if( conf->slv_retry) vme.slave |= 0x10;
  if( conf->slv_ena & VME_SLV_ENA) vme.slave |= 0x80000000;
  vme.ader = conf->a32_base;
  vme_conf_set( &reg, &vme);
  if( conf->slv_ena & VME_SLV_ENA) vme_crcsr_set( pev->io_base + pev->io_remap.vme_csr, 0x10);
  else vme_crcsr_clear( pev->io_base + pev->io_remap.vme_csr, 0x10);

  return;
}

uint
pev_vme_crcsr( struct pev_dev *pev,
	       struct pev_ioctl_vme_crcsr *crcsr)
{
  uint data = 0;

  if( crcsr->operation & VME_CRCSR_GET)
  {
    data = vme_crcsr_set( pev->io_base + pev->io_remap.vme_csr, 0);
    crcsr->get = data;
  }
  if( crcsr->operation & VME_CRCSR_CLEAR)
  {
    data = vme_crcsr_clear( pev->io_base + pev->io_remap.vme_csr, crcsr->clear);
  }
  if( crcsr->operation & VME_CRCSR_SET)
  {
    data = vme_crcsr_set( pev->io_base + pev->io_remap.vme_csr, crcsr->set);
  }
  return( data);
}

uint
pev_vme_rmw( struct pev_dev *pev,
	     struct pev_ioctl_vme_rmw *rmw_p)
{
  uint mode;

  mode = rmw_p->ds & 3;
  if( !mode) mode = 3;
  switch( rmw_p->am)
  {
    case 0x9:
    {
      mode |= 0x10;
      break;
    }
    case 0xd:
    {
      mode |= 0x14;
      break;
    }
    case 0x29:
    {
      break;
    }
    case 0x2d:
    {
      mode |= 0x4;
      break;
    }
    case 0x39:
    {
      mode |= 0x8;
      break;
    }
    case 0x3d:
    {
      mode |= 0xc;
      break;
    }
    default:
    {
      rmw_p->status = -1;
      return( -1);
    }
  }
  rmw_p->status = vme_cmp_swap( pev->io_base + pev->io_remap.vme_base, rmw_p->addr,  rmw_p->cmp,  rmw_p->up, mode);

  return(0);
}
uint
pev_vme_lock( struct pev_dev *pev,
	     struct pev_ioctl_vme_lock *lock_p)
{
  uint mode;

  mode = lock_p->mode;
  lock_p->status = vme_lock( pev->io_base + pev->io_remap.vme_base, lock_p->addr,  mode);
  return(0);
}

uint
pev_vme_unlock( struct pev_dev *pev)
{
  vme_unlock( pev->io_base);
  return(0);
}

uint
pev_vme_slv_init( struct pev_dev *pev)
{
  int i, tmp;
  uint size, base;

  tmp = inl( pev->io_base + pev->io_remap.vme_base + 0x8) & 0xf;
  pev->map_slave.pg_size = 0x100000;
  if( tmp & VME_SLV_1MB)
  {
    size = 0x100000 << (tmp&7);
  }
  else
  {
    size = 0x1000000 << (tmp&7);
    if( size == 0x40000000) pev->map_slave.pg_size = 0x200000;
    if( size == 0x80000000) pev->map_slave.pg_size = 0x400000;
  }

  base = inl( pev->io_base + pev->io_remap.vme_ader) << 24;
  base += inl( pev->io_base + pev->io_remap.vme_ader + 4) << 16;

  /* prepare scatter/gather for VME slave */
  pev->map_slave.pg_num = size/pev->map_slave.pg_size;
  pev->map_slave.sg_id = MAP_SLAVE_VME;
  pev_map_init( pev, &pev->map_slave);
  for( i = 0; i <  pev->map_slave.pg_num; i++)
  {
    pev_sg_slave_vme_set( pev, i, 0, 0); 
  }
  pev->map_slave.loc_base = (u64)base;

  return(0);
}

void
pev_timer_irq( struct pev_dev *pev,
	       int src,
	       void *arg)
{
  struct vme_time tm;
  int chan;
  vme_timer_read( pev->io_base + pev->io_remap.vme_timer, &tm);
  chan = (tm.utime & 0x1ffff)/10;
  histo_inc( pev->crate, chan, 1000);
  /* unmask TIMER  interrupts */
  outl( 0x400, pev->io_base + pev->io_remap.vme_itc + 0x08);
  return;
}

void
pev_timer_init( struct pev_dev *pev)
{
  pev_irq_register( pev, 0x1a, pev_timer_irq, NULL);
  histo_init();
  vme_timer_start( pev->io_base + pev->io_remap.vme_timer, 3, 0);
  return;
}

void
pev_timer_irq_ena( struct pev_dev *pev)
{
  /* unmask TIMER  interrupts */
  outl( 0x400, pev->io_base + pev->io_remap.vme_itc + 0x08);
  return;
}

void
pev_timer_irq_dis( struct pev_dev *pev)
{
  /* mask TIMER  interrupts */
  outl( 0x400, pev->io_base + pev->io_remap.vme_itc + 0x0c);
  return;
}

int
pev_timer_start( struct pev_dev *pev,
		 struct pev_ioctl_timer *tmr)
{
  struct vme_time *tm;

  tm = (struct vme_time *)&tmr->time;
  return( vme_timer_start( pev->io_base + pev->io_remap.vme_timer, tmr->mode, tm));
}
void
pev_timer_restart( struct pev_dev *pev)
{
  vme_timer_restart( pev->io_base + pev->io_remap.vme_timer);
  return;
}

void
pev_timer_stop( struct pev_dev *pev)
{
  vme_timer_stop( pev->io_base + pev->io_remap.vme_timer);
  return;
}

void
pev_timer_read( struct pev_dev *pev,
		struct pev_ioctl_timer *tmr)
{
  struct vme_time *tm;

  tm = (struct vme_time *)&tmr->time;
  vme_timer_read( pev->io_base + pev->io_remap.vme_timer, tm);
  return;
}

void 
pev_fifo_irq(struct pev_dev *pev, int src, void *arg)
{
//  printf("fifo interrupt; %d\n", idx);
//  NOTE: If the above print is present, it is necessary to
//  wait after a call to pevx_fifo_wait_XX before any printing
//  otherwise VxWorks seems to lock. (fg 10-01-2012)

    up( &pev->fifo_sem[src&7]);
    return;
}
void 
pev_fifo_init(struct pev_dev *pev)
{
  int i;
  u32 value;

  if( pev->io_remap.short_io)
  {
    outl( PEV_SCSR_SEL_USR, pev->io_base);
  }
  fifo_init( pev->io_base + pev->io_remap.usr_fifo);

  for (i = 0; i < 8; i++) 
  {
    pev_irq_register(pev, 0x30+i, pev_fifo_irq, NULL);
    sema_init( &pev->fifo_sem[i], 0);
  }

  /* enable USR global interupt */
  value = 0x3;
  outl( value, pev->io_base + pev->io_remap.usr_itc + 0x04);

  return;
}

void
pev_fifo_status( struct pev_dev *pev,
	         struct pev_ioctl_fifo *fifo)

{
  if( pev->io_remap.short_io)
  {
    outl( PEV_SCSR_SEL_USR, pev->io_base);
  }
  /* get FIFO status*/
  fifo_status( pev->io_base + pev->io_remap.usr_fifo, fifo->idx, &fifo->sts);

  return;
}

void
pev_fifo_clear( struct pev_dev *pev,
	        struct pev_ioctl_fifo *fifo)

{
  uint irq;

  if( pev->io_remap.short_io)
  {
    outl( PEV_SCSR_SEL_USR, pev->io_base);
  }
  /* mask FIFO  interrupts */
  irq = 1 << fifo->idx;
  outl( irq, pev->io_base + pev->io_remap.usr_itc + 0x0c);
  irq = 0x10 << fifo->idx;
  outl( irq, pev->io_base + pev->io_remap.usr_itc + 0x0c);
  /* clear FIFO */
  fifo_clear( pev->io_base + pev->io_remap.usr_fifo, fifo->idx, &fifo->sts);
  /* reset semaphores */
  sema_init( &pev->fifo_sem[fifo->idx], 0);
  sema_init( &pev->fifo_sem[fifo->idx+4], 0);
  return;
}

int
pev_fifo_wait_ef( struct pev_dev *pev,
	          struct pev_ioctl_fifo *fifo)

{
  uint irq;
  int retval;
  int jiffies;

  irq = 1 << fifo->idx;
  if( pev->io_remap.short_io)
  {
    outl( PEV_SCSR_SEL_USR, pev->io_base);
  }
  /* reset semaphores */
  sema_init( &pev->fifo_sem[fifo->idx], 0);

  /* clear FIFO  interrupts */
  outl( irq << 16, pev->io_base + pev->io_remap.usr_itc);

  /* unmask FIFO  interrupts */
  outl( irq, pev->io_base + pev->io_remap.usr_itc + 0x08);

  /* wait for semaphores */
  jiffies = msecs_to_jiffies( fifo->tmo);
  if( fifo->tmo)
  {
    retval = down_timeout( &pev->fifo_sem[fifo->idx], jiffies);
  }
  else
  {
    retval = down_interruptible( &pev->fifo_sem[fifo->idx]);
  }

  fifo_status( pev->io_base + pev->io_remap.usr_fifo, fifo->idx, &fifo->sts);

  return( retval);
}


int
pev_fifo_wait_ff( struct pev_dev *pev,
	          struct pev_ioctl_fifo *fifo)

{
  uint irq;
  int retval;
  int jiffies;

  irq = 0x10 << fifo->idx;
  if( pev->io_remap.short_io)
  {
    outl( PEV_SCSR_SEL_USR, pev->io_base);
  }
  /* reset semaphores */
  sema_init( &pev->fifo_sem[fifo->idx+4], 0);

  /* clear FIFO  interrupts */
  outl( irq << 16, pev->io_base + pev->io_remap.usr_itc);

  /* unmask FIFO  interrupts */
  outl( irq, pev->io_base + pev->io_remap.usr_itc + 0x08);

  /* wait for semaphores */
  jiffies = msecs_to_jiffies( fifo->tmo);
  if( fifo->tmo)
  {
    retval = down_timeout( &pev->fifo_sem[fifo->idx+4], jiffies);
  }
  else
  {
    retval = down_interruptible( &pev->fifo_sem[fifo->idx+4]);
  }

  fifo_status( pev->io_base + pev->io_remap.usr_fifo, fifo->idx, &fifo->sts);

  return( retval);
}

int 
pev_fifo_read( struct pev_dev *pev,
	       struct pev_ioctl_fifo *fifo)

{
  int *ptr, bcnt, wcnt;


  bcnt = sizeof(int) * fifo->cnt;
  ptr = (int *)kmalloc( bcnt, GFP_KERNEL);
  if( pev->io_remap.short_io)
  {
    outl( PEV_SCSR_SEL_USR, pev->io_base);
  }
  wcnt = fifo_read( pev->io_base + pev->io_remap.usr_fifo, fifo->idx, ptr, fifo->cnt, &fifo->sts);
  fifo->cnt = wcnt;
  bcnt = sizeof(int) * wcnt;
  if( copy_to_user( fifo->data, ptr, bcnt))
  {
    wcnt = -EFAULT;
  }
  kfree( ptr);
  return( wcnt);
}

int 
pev_fifo_write( struct pev_dev *pev,
	        struct pev_ioctl_fifo *fifo)

{
  int *ptr, wcnt, bcnt;

  bcnt = sizeof(int) * fifo->cnt;
  ptr = (int *)kmalloc( bcnt, GFP_KERNEL);
  if( copy_from_user(ptr, fifo->data, bcnt))
  {
    wcnt = -EFAULT;
    fifo->cnt = 0;
  }
  else
  {
    if( pev->io_remap.short_io)
    {
      outl( PEV_SCSR_SEL_USR, pev->io_base);
    }
    wcnt = fifo_write( pev->io_base + pev->io_remap.usr_fifo, fifo->idx, ptr, fifo->cnt, &fifo->sts);
    fifo->cnt = wcnt;
  }
  kfree( ptr);
  return( wcnt);
}


void
pev_dma_irq( struct pev_dev *pev,
	     int src,
	     void *arg)
{
  pev->dma_status |= DMA_DONE;
#ifdef XENOMAI
  rtdm_sem_up( &pev->dma_done);
#else
  up( &pev->dma_sem);
#endif
  return;
}


void
pev_dma_init( struct pev_dev *pev)
{
  int i;
  void *kaddr;
  ulong baddr;
  int order;

  dma_init( pev->crate, pev->dma_shm_ptr, pev->dma_shm_base, pev->dma_shm_len, pev->io_base + pev->io_remap.dma_rd, pev->io_base + pev->io_remap.dma_wr);
  //kaddr = kmalloc( 0x100000, GFP_KERNEL);
  //kaddr = kmalloc( 0x20000, GFP_KERNEL | __GFP_DMA);
  order = get_order( 0x100000); 
  kaddr = (void *)__get_free_pages( GFP_KERNEL | __GFP_DMA, order);
  baddr = 0;
  if( kaddr)
  {
    baddr = (ulong)dma_map_single( &pev->dev->dev, kaddr, 0x100000, DMA_BIDIRECTIONAL);
    dma_alloc_kmem( pev->crate, kaddr, baddr); 
  }

  for( i = 0; i < 8; i++)
  {
    pev_irq_register( pev, 0x20+i, pev_dma_irq, NULL);
  }

#ifdef XENOMAI
  dma_init_xeno( pev->crate, pev->user_info);
  rtdm_sem_init( &pev->dma_done, 0);
  rtdm_lock_init( &pev->dma_lock);
#else
  //init_MUTEX( &pev->dma_lock);
  sema_init( &pev->dma_lock, 1);
#endif

  /* enable DMA read engine */
  outl( 0x80000000, pev->io_base + pev->io_remap.dma_rd);
  /* enable DMA write engine */
  outl( 0x80000000, pev->io_base + pev->io_remap.dma_wr);
  /* enable DMA global interupt */
  outl( 0x3, pev->io_base + pev->io_remap.dma_itc + 0x04);

  return;
}
void
pev_dma_exit( struct pev_dev *pev)
{
  void *kaddr;
  ulong baddr;

  baddr = dma_get_buf_baddr( pev->crate);
  if( baddr)
  {
    dma_unmap_single( &pev->dev->dev, (dma_addr_t)baddr, 0x100000, DMA_BIDIRECTIONAL);
  }
  kaddr = dma_get_buf_kaddr( pev->crate);
  if( kaddr)
  {
    int order;
    //kfree( kaddr);
    order = get_order( 0x100000); 
    free_pages( (unsigned long)kaddr, order);
  }

  return;
}

int
pev_dma_move_blk( struct pev_dev *pev,
	          struct pev_ioctl_dma_req *dma,
		  int *irq_p)
{
  int retval;
  uint wdo;
  uint rdo;
  uint swap_rd;
  uint swap_wr;


  swap_rd = 0;
  swap_wr = 0;
  if( ( dma->src_space & DMA_SPACE_MASK) !=  DMA_SPACE_VME)
  {
    if( ( dma->src_space & DMA_SPACE_MASK) ==  DMA_SPACE_SHM)
    {
      swap_rd |= (dma->src_space & 0x30) << 4;
    }
    else
    {
      swap_wr |= (dma->src_space & 0x30) << 4;
    }
    dma->src_space &= DMA_SPACE_MASK;
  }
  if( ( dma->des_space & DMA_SPACE_MASK) !=  DMA_SPACE_VME)
  {
    if( ( dma->des_space & DMA_SPACE_MASK) ==  DMA_SPACE_SHM)
    {
      swap_wr |= (dma->des_space & 0x30) << 4;
    }
    else
    {
      swap_rd |= (dma->des_space & 0x30) << 4;
    }
    dma->des_space &= DMA_SPACE_MASK;
  }
  retval = 0;
  *irq_p = 0x00;

  if( ( dma->src_space & DMA_SPACE_MASK) ==  DMA_SPACE_SHM)
  {
    if( ( dma->des_space & DMA_SPACE_MASK) ==  DMA_SPACE_SHM)
    {
      retval = -EINVAL;
    }
    else
    {
      rdo = dma_set_rd_desc( pev->crate, dma->src_addr, dma->des_addr, dma->size, dma->des_space, dma->des_mode | swap_rd);
      outl( rdo, pev->io_base + pev->io_remap.dma_rd + 0x04); /* start read engine */
      pev->dma_status = DMA_RUN_RD0;
      *irq_p = 0x03;
    }
  }
  else
  {
    if( ( dma->des_space & DMA_SPACE_MASK) ==  DMA_SPACE_SHM)
    {
      wdo = dma_set_wr_desc( pev->crate, dma->des_addr, dma->src_addr, dma->size, dma->src_space, dma->src_mode | swap_wr);
      outl( wdo, pev->io_base + pev->io_remap.dma_wr + 0x04); /* start write engine */
      pev->dma_status = DMA_RUN_WR0;
      *irq_p = 0x30;
    }
    else
    {
      debugk(("%lx:%lx:%x:%x:%x\n", dma->src_addr,  dma->des_addr, dma->size, 
	      dma->src_space, dma->des_space));
      if( dma->start_mode & DMA_MODE_PIPE)
      {
	uint mode;

	mode = ((dma->des_mode | swap_rd) << 16) | ((dma->src_mode | swap_wr) & 0xffff);
        if( pev->io_remap.short_io)
        {
          outl( PEV_SCSR_SEL_DMA, pev->io_base);
	  outl( 0x8200, pev->io_base + 0xc8);      /* enable read pipe                                 */
	  outl( 0x80c0, pev->io_base + 0xcc);      /* enable write pipe                                */
        }
	else
	{
	  outl( 0x8200, pev->io_base + 0x850);      /* enable read pipe                                 */
	  outl( 0x80c0, pev->io_base + 0x858);      /* enable write pipe                                */
	}
        dma_set_pipe_desc( pev->crate, dma->des_addr, dma->src_addr, dma->size, dma->des_space, dma->src_space, mode);
      }
      else
      {
        wdo = dma_set_wr_desc( pev->crate, -1, dma->src_addr, dma->size, dma->src_space, dma->src_mode | swap_wr);
        rdo = dma_set_rd_desc( pev->crate, -1, dma->des_addr, dma->size, dma->des_space, dma->des_mode | swap_rd);
        outl( rdo | 0x6, pev->io_base + pev->io_remap.dma_rd + 0x04); /* start read engine waiting for trigger from write */
        outl( wdo, pev->io_base + pev->io_remap.dma_wr + 0x04); /* start write engine */
      }
      pev->dma_status = DMA_RUN_RD0 | DMA_RUN_WR0;
      *irq_p = 0x33;
    }
  }
  return( retval);
}

int
pev_dma_list_rd( struct pev_dev *pev,
	         struct pev_ioctl_dma_req *dma,
		 int *irq_p)
{
  int retval;

  debugk(("in pev_dma_list_rd()...%lx -%x\n", dma->src_addr, dma->size));
  retval = 0;
  *irq_p = 0x00;
  dma_set_list_desc( pev->crate, dma);
  outl( 0xff00800 | 0x6, pev->io_base + pev->io_remap.dma_rd + 0x04); /* start read engine waiting for trigger from write */
  outl( 0xff00000, pev->io_base + pev->io_remap.dma_wr + 0x04); /* start write engine */
  pev->dma_status = DMA_RUN_RD0 | DMA_RUN_WR0;
  *irq_p = 0x33;
  dma->intr_mode |= DMA_INTR_ENA;
  dma->wait_mode |= DMA_WAIT_INTR;

  return( retval);
}


int
pev_dma_move( struct pev_dev *pev,
	      struct pev_ioctl_dma_req *dma)
{
  //struct vme_time tm;
  int retval;
  int irq;

  debugk(("in pev_dma_move()...%x -%x\n", dma->src_mode, DMA_SRC_BLOCK));

  /* multi user protection */
#ifdef XENOMAI
  rtdm_lock_get( &pev->dma_lock);
#else
  retval = down_interruptible( &pev->dma_lock);
#endif

  retval = -EINVAL;
  if(( dma->start_mode == DMA_MODE_BLOCK) ||
     ( dma->start_mode == DMA_MODE_PIPE)     )
  {
    retval= pev_dma_move_blk( pev, dma, &irq);
  }
  if( dma->start_mode == DMA_MODE_LIST_RD)
  {
    retval= pev_dma_list_rd( pev, dma, &irq);
  }



#ifdef XENOMAI
  rtdm_lock_put( &pev->dma_lock);
#else
  up( &pev->dma_lock);
#endif
  if( retval)
  {
    return( retval);
  }

  /* check interrupt mode */
  if( dma->intr_mode & DMA_INTR_ENA)
  {
    /* reset semaphore */
#ifdef XENOMAI
    //rtdm_sem_init( &pev->dma_done, 0);
#else
    //init_MUTEX_LOCKED( &pev->dma_sem);
    sema_init( &pev->dma_sem, 0);
#endif
    /* unmask DMA  interrupts */
    outl( irq, pev->io_base + pev->io_remap.dma_itc + 0x08);
    /* check for wait mode */
    if( dma->wait_mode & DMA_WAIT_INTR)
    {
      /* wait for end of DMA */
      pev->dma_status |= DMA_WAITING;
#ifdef XENOMAI
      rtdm_sem_down( &pev->dma_done);
#else
      retval = down_interruptible( &pev->dma_sem);
#endif
      if( dma->start_mode == DMA_MODE_LIST_RD)
      {
        retval= dma_get_list( pev->crate, dma);
      }
      pev->dma_status |= DMA_ENDED;
      /*vme_timer_read( pev->io_base, &tm);
      debugk(("DMA ended: %08x - %08x\n", tm.time, tm.utime));*/
    }
  }
  return( retval);
}

int
pev_dma_status( struct pev_dev *pev,
	        struct pev_ioctl_dma_sts *dma_sts)
{
  dma_sts->rd_csr  = inl(pev->io_base + pev->io_remap.dma_rd + 0x00);
  dma_sts->rd_ndes = inl(pev->io_base + pev->io_remap.dma_rd + 0x04);
  dma_sts->rd_cdes = inl(pev->io_base + pev->io_remap.dma_rd + 0x08);
  dma_sts->rd_cnt  = inl(pev->io_base + pev->io_remap.dma_rd + 0x0c);
  dma_sts->wr_csr  = inl(pev->io_base + pev->io_remap.dma_wr + 0x00);
  dma_sts->wr_ndes = inl(pev->io_base + pev->io_remap.dma_wr + 0x04);
  dma_sts->wr_cdes = inl(pev->io_base + pev->io_remap.dma_wr + 0x08);
  dma_sts->wr_cnt  = inl(pev->io_base + pev->io_remap.dma_wr + 0x0c);
  dma_get_desc( pev->crate, (uint *)&dma_sts->start);
  return( 0);
}

void
pev_dma_wait( struct pev_dev *pev)
{
  int retval;

  pev->dma_status |= DMA_WAITING;
#ifdef XENOMAI
  rtdm_sem_down( &pev->dma_done);
#else
  retval = down_interruptible( &pev->dma_sem);
#endif
  pev->dma_status |= DMA_ENDED;
  return;
}

int
pev_histo_read( struct pev_dev *pev,
		struct pev_ioctl_histo *hp)
{
  void *histo_p;

  histo_p = histo_get( hp->idx, hp->size);
  if( !histo_p)
  {
    return( -EFAULT);
  }
  if( copy_to_user( (void *)hp->histo, histo_p, hp->size*sizeof(int)))
  {
    return( -EFAULT);
  }
  return(0);
}

int
pev_histo_clear( struct pev_dev *pev,
	 	 struct pev_ioctl_histo *hp)
{
  histo_clear( hp->idx);
  return(0);
}

