/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : idtklib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the all functions called from the IDT device 
 *    driver main file (idtdrvr.c).
 *    Each function takes a pointer to the IDT switch device data structure as 
 *    first driver argument (struct idt_dev *).
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
 * $Log: idtklib.c,v $
 * Revision 1.1  2013/06/07 15:03:26  zimoch
 * update to latest version
 *
 * Revision 1.3  2013/02/21 10:58:04  ioxos
 * support for 2 dma channels [JFG]
 *
 * Revision 1.2  2013/02/18 16:31:38  ioxos
 * kill and restart DMA if still running [JFG]
 *
 * Revision 1.1  2013/02/05 11:09:03  ioxos
 * first checkin [JFG]
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


#include "../include/pevioctl.h"
#include "../include/idtioctl.h"
#include "idtdrvr.h"
#include "idtklib.h"
#include "dmalib.h"

#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

int
idt_swap_32( int data)
{
  int tmp;

  tmp = ((data&0xff)<< 24) | ((data&0xff00)<<8) | ((data&0xff0000)>>8) | ((data&0xff000000)>>24);

  return(tmp);
}
int
idt_csr_rd( struct pci_dev *dev,
	    int offset,
	    int *data_p)
{
  if( pci_write_config_dword( dev, 0xff8,offset))
  {
    return( -EIO);
  }
  pci_read_config_dword( dev, 0xffc, data_p);
  return(0);
}


int
idt_csr_wr( struct pci_dev *dev,
	    int offset,
	    int data)
{
  if( pci_write_config_dword( dev, 0xff8,offset))
  {
    return( -EIO);
  }
  pci_write_config_dword( dev, 0xffc, data);
  return(0);
}

int
idt_mbx_wait( struct idt_dev *idt,
	      struct idt_ioctl_mbx *mbx)
{
  int retval = 0;              // Will contain the result
  int jiffies;
  int sts;
  int idx;

  idx = mbx->idx & 3;

  mbx->sts = 0;
  if( mbx->tmo)
  {
    jiffies = msecs_to_jiffies( mbx->tmo) + 1;
    retval = down_timeout( &idt->sem_msi[idx], jiffies);
    if( retval)
    {
      printk("IDT wait timeout..\n"),
      mbx->sts = -1;
    }
  }
  else
  {
    retval = down_interruptible( &idt->sem_msi[idx]);
    if( retval)
    {
      printk("IDT wait cancelled..\n"),
      mbx->sts = -1;
    }
  }
  if( !mbx->sts)
  {
    sts =  *(int *)(idt->nt_csr_ptr + 0x404);
    sts =  idt_swap_32( sts);
    /* if message interrupt pending */
    if( sts & 1)
    {
      /* get mailboxes status */
      mbx->sts = *(int *)(idt->nt_csr_ptr + 0x460);
      mbx->sts =  idt_swap_32( mbx->sts);
      /* read INMSG0 */
      mbx->msg = *(int *)(idt->nt_csr_ptr + 0x440 + ( idx << 2));
      /* clear INMSG0 status bit */
      *(int *)(idt->nt_csr_ptr + 0x460) = idt_swap_32( 0x10000 << idx);
    }
    else
    {
      mbx->sts = -2;
    }
  }
  return( mbx->sts);
}


int
idt_dma_init( struct idt_dev *idt)
{
  debugk(("in idt_dma_init()...\n"));
  idt->dma_ctl = dma_init( idt->dma_shm_ptr, idt->dma_shm_offset,  idt->dma_shm_len,
                          (idt->dma_csr_ptr + PEV_CSR_DMA_RD0), (idt->dma_csr_ptr + PEV_CSR_DMA_WR0), 
                          (idt->dma_csr_ptr + PEV_CSR_DMA_RD1), (idt->dma_csr_ptr + PEV_CSR_DMA_WR1));


  dma_alloc_mbx( idt->dma_ctl, idt->dma_mbx_base + 0x430, 0x11223344, 0);
  dma_alloc_mbx( idt->dma_ctl, idt->dma_mbx_base + 0x434, 0xaabbccdd, 1);
  sema_init( &idt->dma_lock[0], 1);
  sema_init( &idt->dma_lock[1], 1);

  return(0);
}

int
idt_dma_move_blk( struct idt_dev *idt,
	          struct idt_ioctl_dma_req *dma)
{
  int retval;
  uint wdo;
  uint rdo;
  uint swap_rd;
  uint swap_wr;
  uint blk;
  int ctlr, tmp;
  int start_mode;


  swap_rd = 0;
  swap_wr = 0;
  blk = (dma->size >> 16) & 0xf000;
  dma->size &= 0xffffff;
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
  start_mode = dma->start_mode & 0xf;
  ctlr = 0;
  if( dma->start_mode & DMA_START_CTLR_1)
  {
    ctlr = 1;
  }
  retval = 0;

  if( ( dma->src_space & DMA_SPACE_MASK) ==  DMA_SPACE_SHM)
  {
    if( ( dma->des_space & DMA_SPACE_MASK) ==  DMA_SPACE_SHM)
    {
      retval = -EINVAL;
    }
    else
    {
      ulong dma_des_addr;

      dma_des_addr = dma->des_addr;
      debugk(("dma_des_addr = %lx - dma->des_space = %x - idx = %d\n",  dma_des_addr, dma->des_space, idt->idx));
      /* if DMA is located in slave CPU (idx != 0) and destination is PCIe tree in master CPU */
      if( ( (dma->des_space & DMA_SPACE_MASK) ==  DMA_SPACE_PCIE) && idt->idx)
      {
	ulong rem_base, rem_offset;

	rem_base = dma_des_addr & 0xfffff000;
	rem_offset = dma_des_addr & 0xfff;
	dma_des_addr = idt->dma_des_base[1] + rem_offset;
	/* set remote address in BARLTBASE4 of NT port #0 in CPU slave */
	idt_csr_wr( idt->dev, 0x14b8, rem_base);
      }
      rdo = dma_set_rd_desc( idt->dma_ctl, dma->src_addr, dma_des_addr, dma->size, dma->des_space, dma->des_mode | swap_rd | blk | (ctlr << 12));
      /* check if DMA ended */
      tmp = dma_read_io( idt->dma_ctl, 0x0c, DMA_CTLR_RD | ctlr);
      if( !(tmp & 0x80000000))
      {
	/* if not, kill current DMA transfer */
	dma_write_io( idt->dma_ctl, 0x00, 0x40000000, DMA_CTLR_RD | ctlr);
	/* re-enable DMA engine */
	dma_write_io( idt->dma_ctl, 0x00, 0x80000000, DMA_CTLR_RD | ctlr);
      }
      dma_write_io( idt->dma_ctl, 0x04, rdo, DMA_CTLR_RD | ctlr);
      if( !ctlr)
      {
        idt->dma_status[0] = DMA_STATUS_RUN_RD0;
      }
      else
      {
        idt->dma_status[1] = DMA_STATUS_RUN_RD1;
      }
    }
  }
  else
  {
    if( ( dma->des_space & DMA_SPACE_MASK) ==  DMA_SPACE_SHM)
    {
      ulong dma_src_addr;

      dma_src_addr = dma->src_addr;
      debugk(("dma_des_addr = %lx - dma->des_space = %x - idx = %d\n",  dma_src_addr, dma->src_space, idt->idx));
      /* if DMA is located in slave CPU (idx != 0) and destination is PCIe tree in master CPU */
      if( ( (dma->src_space & DMA_SPACE_MASK) ==  DMA_SPACE_PCIE) && idt->idx)
      {
	ulong rem_base, rem_offset;

	rem_base = dma_src_addr & 0xfffff000;
	rem_offset = dma_src_addr & 0xfff;
	dma_src_addr = idt->dma_des_base[0] + rem_offset;
	/* set remote address in BARLTBASE2 of NT port #0 in CPU slave */
	idt_csr_wr( idt->dev, 0x1498, rem_base);
      }
      wdo = dma_set_wr_desc( idt->dma_ctl, dma->des_addr, dma_src_addr, dma->size, dma->src_space, dma->src_mode | swap_wr | blk | (ctlr << 12));
      /* check if DMA ended */
      tmp = dma_read_io( idt->dma_ctl, 0x0c, DMA_CTLR_WR | ctlr);
      if( !(tmp & 0x80000000))
      {
	/* if not, kill current DMA transfer */
	dma_write_io( idt->dma_ctl, 0x00, 0x40000000, DMA_CTLR_WR | ctlr);
	/* re-enable DMA engine */
	dma_write_io( idt->dma_ctl, 0x00, 0x80000000, DMA_CTLR_WR | ctlr);
      }
      dma_write_io( idt->dma_ctl, 0x04, wdo, DMA_CTLR_WR | ctlr);
      if( !ctlr)
      {
        idt->dma_status[0] = DMA_STATUS_RUN_WR0;
      }
      else
      {
        idt->dma_status[1] = DMA_STATUS_RUN_WR1;
      }
    }
    else
    {
      ulong dma_src_addr;
      ulong dma_des_addr;
      ulong rem_base, rem_offset;

      debugk(("%lx:%lx:%x:%x:%x\n", dma->src_addr,  dma->des_addr, dma->size, 
	      dma->src_space, dma->des_space));
      dma_src_addr = dma->src_addr;
      dma_des_addr = dma->des_addr;
      if( ( (dma->src_space & DMA_SPACE_MASK) ==  DMA_SPACE_PCIE) && idt->idx)
      {
	rem_base = dma_src_addr & 0xfffff000;
	rem_offset = dma_src_addr & 0xfff;
	dma_src_addr = idt->dma_des_base[0] + rem_offset;
	/* set remote address in BARLTBASE2 of NT port #0 in CPU slave */
	idt_csr_wr( idt->dev, 0x1498, rem_base);
      }
      if( ( (dma->des_space & DMA_SPACE_MASK) ==  DMA_SPACE_PCIE) && idt->idx)
      {
	rem_base = dma_des_addr & 0xfffff000;
	rem_offset = dma_des_addr & 0xfff;
	dma_des_addr = idt->dma_des_base[1] + rem_offset;
	/* set remote address in BARLTBASE4 of NT port #0 in CPU slave */
	idt_csr_wr( idt->dev, 0x14b8, rem_base);
      }
      if( start_mode & DMA_MODE_PIPE)
      {
	uint mode;

	mode = ((dma->des_mode | swap_rd | blk) << 16) | ((dma->src_mode | swap_wr | blk) & 0xffff);
	dma_write_io(  idt->dma_ctl, 0x50 - 0x100, 0x8200, DMA_CTLR_RD);     /* enable read pipe                                 */
	dma_write_io(  idt->dma_ctl, 0x58 - 0x100, 0x80c0, DMA_CTLR_RD);     /* enable write pipe                                */
        dma_set_pipe_desc( idt->dma_ctl, dma_des_addr, dma_src_addr, dma->size, dma->des_space, dma->src_space, mode | (ctlr << 28) | (ctlr << 12) );
      }
      else
      {
	int trig;
        wdo = dma_set_wr_desc( idt->dma_ctl, -1, dma_src_addr, dma->size, dma->src_space, dma->src_mode | swap_wr | blk | (ctlr << 12));
        rdo = dma_set_rd_desc( idt->dma_ctl, -1, dma_des_addr, dma->size, dma->des_space, dma->des_mode | swap_rd | blk | (ctlr << 12));
        /* check if DMA read ended */
        tmp = dma_read_io( idt->dma_ctl, 0x0c, DMA_CTLR_RD | ctlr);
        if( !(tmp & 0x80000000))
        {
 	  /* if not, kill current DMA transfer */
	  dma_write_io( idt->dma_ctl, 0x00, 0x40000000, DMA_CTLR_RD | ctlr);
	  /* re-enable DMA engine */
	  dma_write_io( idt->dma_ctl, 0x00, 0x80000000, DMA_CTLR_RD | ctlr);
        }
	trig = 6;
	if( ctlr) trig = 7;
	dma_write_io(  idt->dma_ctl,0x04,  rdo | trig, DMA_CTLR_RD | ctlr);
        /* check if DMA write ended */
        tmp = dma_read_io( idt->dma_ctl, 0x0c, DMA_CTLR_WR | ctlr);
        if( !(tmp & 0x80000000))
        {
 	  /* if not, kill current DMA transfer */
	  dma_write_io( idt->dma_ctl, 0x00, 0x40000000, DMA_CTLR_WR | ctlr);
	  /* re-enable DMA engine */
	  dma_write_io( idt->dma_ctl, 0x00, 0x80000000, DMA_CTLR_WR | ctlr);
        }
	dma_write_io(  idt->dma_ctl, 0x04, wdo, DMA_CTLR_WR | ctlr);
      }
      if( !ctlr)
      {
	idt->dma_status[0] = DMA_STATUS_RUN_RD0 | DMA_STATUS_RUN_WR0;
      }
      else
      {
	idt->dma_status[1] = DMA_STATUS_RUN_RD1 | DMA_STATUS_RUN_WR1;
      }
    }
  }
  return( retval);
}
int
idt_dma_move( struct idt_dev *idt,
	      struct idt_ioctl_dma_req *dma)
{
  struct idt_ioctl_mbx mbx;
  int retval;
  int start_mode;
  int ctlr;

  ctlr = 0;
  if( dma->start_mode & DMA_START_CTLR_1)
  {
    ctlr = 1;
  }
  debugk(("in idt_dma_move()...\n"));
  /* set multi user protection */
#ifdef XENOMAI
  rtdm_lock_get( &idt->dma_lock[ctlr]);
#else
  retval = down_interruptible( &idt->dma_lock[ctlr]);
#endif

  retval = -EINVAL;
  if( dma->intr_mode & DMA_INTR_ENA)
  {
    mbx.idx = ctlr;
    if( dma->wait_mode & DMA_WAIT_INTR)
    {
      /* prepare mailbox #0 to wait for end of tranfer interrupt */
      mbx.tmo = ( dma->wait_mode & 0xf0) >> 4;
      if( mbx.tmo)
      {
        int i, scale;
  
        i = ( dma->wait_mode & 0x0e) >> 1;
        scale = 1;
        if( i)
        {
          i -= 1;
          while(i--)
          {
            scale = scale*10;
	  }
        }
	mbx.tmo = mbx.tmo*scale;
      }
    }

    /* allocate mailbox #0 */
    dma_alloc_mbx( idt->dma_ctl, idt->dma_mbx_base + 0x430 + (4*mbx.idx), dma->mbx_data, ctlr);
    /* reset semaphore */
    sema_init( &idt->sem_msi[mbx.idx], 0);

    /* clear INMSG0 status */
    *(int *)(idt->nt_csr_ptr + 0x460) = idt_swap_32( 0x10000 << mbx.idx);
    /* unmask mailbox #0 */
    *(int *)(idt->nt_csr_ptr + 0x464) = idt_swap_32( ~(0x10000 << mbx.idx));
    /* unmask MSI */
    *(int *)(idt->nt_csr_ptr + 0x408) = idt_swap_32( 0xfe);
  }
  /* perform DMA transfer */
  start_mode = dma->start_mode & 0xf;
  if(( start_mode == DMA_MODE_BLOCK) ||
     ( start_mode == DMA_MODE_PIPE)     )
  {
    retval= idt_dma_move_blk( idt, dma);
  }
  if( retval)
  {
    dma->dma_status = idt->dma_status[ctlr];
    goto idt_dma_move_exit;
  }
  /* check interrupt mode */
  if( dma->intr_mode & DMA_INTR_ENA)
  {
    dma->mbx_data = -1;
    if( dma->wait_mode & DMA_WAIT_INTR)
    {
      int sts;

      idt->dma_status[ctlr] |= DMA_STATUS_WAITING;
      sts = idt_mbx_wait( idt, &mbx);
      if( sts < 0)
      {
	idt->dma_status[ctlr] |= DMA_STATUS_TMO;
      }
      else
      {
	dma->mbx_data = mbx.msg;
      }
      idt->dma_status[ctlr] |= DMA_STATUS_ENDED;
    }
  }
  dma->dma_status = idt->dma_status[ctlr];

idt_dma_move_exit:
#ifdef XENOMAI
  rtdm_lock_put( &idt->dma_lock[ctlr]);
#else
  up( &idt->dma_lock[ctlr]);
#endif
  return( retval);
}

void
idt_dma_exit( struct idt_dev *idt)
{
  dma_exit( idt->dma_ctl);
  return;
}
