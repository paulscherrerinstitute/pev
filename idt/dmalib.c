/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : dmalib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the low level functions to drive the DMA controllers
 *    implemented on the PEV1100.
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
 * $Log: dmalib.c,v $
 * Revision 1.1  2013/06/07 15:03:26  zimoch
 * update to latest version
 *
 * Revision 1.5  2013/02/21 11:07:00  ioxos
 * bug in calculating dc->buf_ptr [JFG]
 *
 * Revision 1.4  2013/02/21 10:58:04  ioxos
 * support for 2 dma channels [JFG]
 *
 * Revision 1.3  2013/02/19 10:52:21  ioxos
 * correct bug for ctlr #1 + support for pipe mode [JFG]
 *
 * Revision 1.2  2013/02/18 16:31:38  ioxos
 * kill and restart DMA if still running [JFG]
 *
 * Revision 1.1  2013/02/05 10:46:16  ioxos
 * first checkin [JFG]
 *
 * Revision 1.27  2012/12/13 15:21:40  ioxos
 * support for 2 DMA controllers [JFG]
 *
 * Revision 1.26  2012/11/14 08:56:52  ioxos
 * prepare for second DMA channel [JFG]
 *
 * Revision 1.25  2012/09/27 09:47:18  ioxos
 * generate interrupt in case of error when DMA are linked [JFG]
 *
 * Revision 1.24  2012/09/05 12:31:54  ioxos
 * support for transfer to/from FPGA user space [JFG]
 *
 * Revision 1.23  2012/08/13 08:51:19  ioxos
 * correct bug in setting block boundary for VME [JFG]
 *
 * Revision 1.22  2012/06/28 12:05:00  ioxos
 * cleanup [JFG]
 *
 * Revision 1.21  2012/04/17 11:57:44  ioxos
 * bug correction for VME list mode on PPC [JFG]
 *
 * Revision 1.20  2012/04/17 08:11:48  ioxos
 * support VME list mode on PPC [JFG]
 *
 * Revision 1.19  2012/04/17 07:47:03  ioxos
 * support pipe mode on PPC [JFG]
 *
 * Revision 1.18  2012/04/12 13:33:37  ioxos
 * support for dma swapping [JFG]
 *
 * Revision 1.17  2012/02/14 16:12:54  ioxos
 * support for byte swapping [JFG]
 *
 * Revision 1.16  2011/10/19 14:03:28  ioxos
 * support for powerpc [JFG]
 *
 * Revision 1.15  2011/01/25 13:40:43  ioxos
 * support for VME RMW [JFG]
 *
 * Revision 1.14  2010/09/14 07:32:05  ioxos
 * bug correction [JFG]
 *
 * Revision 1.13  2010/01/13 16:49:35  ioxos
 * xenomai support for DMA list [JFG]
 *
 * Revision 1.12  2010/01/12 15:41:36  ioxos
 * set max size for DMA list to 63 [JFG]
 *
 * Revision 1.11  2010/01/08 11:20:07  ioxos
 * add support to read DMA list from VME [JFG]
 *
 * Revision 1.10  2009/12/15 17:13:24  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.9  2009/11/10 09:06:15  ioxos
 * add support for DMA transfer mode [JFG]
 *
 * Revision 1.8  2009/07/17 13:28:27  ioxos
 * reset chain element count to 1 at init time [JFG]
 *
 * Revision 1.7  2009/05/20 08:21:39  ioxos
 * multicrate support + pipeline mode [JFG]
 *
 * Revision 1.6  2009/04/06 09:44:09  ioxos
 * remove pevdrvr.h
 *
 * Revision 1.5  2009/02/09 11:03:52  ioxos
 * set address boundary in DMA desc [JFG]
 *
 * Revision 1.4  2009/01/27 14:36:00  ioxos
 * remove ref to semaphore.h + synchronise desc write [JFG]
 *
 * Revision 1.3  2009/01/09 13:09:45  ioxos
 * add support for DMA status [JFG]
 *
 * Revision 1.2  2008/12/12 13:41:11  jfg
 * implement functions to prepare DMA chains in shared memory [JFG]
 *
 * Revision 1.1  2008/09/17 12:10:25  ioxos
 * file creation [JFG]
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

#include "../include/pevioctl.h"

#ifdef XENOMAI
#include <rtdm/rtdm_driver.h>
#endif

struct dma_desc
{
  uint ctl;
  uint wcnt;
  uint shm_addr;
  uint next;
  uint rem_addr_l;
  uint rem_addr_h;
  uint status;
  uint time_msec;
};

struct dma_chain
{
  struct dma_desc desc[64];
};

struct dma_ctl
{
  struct dma_chain *wr_chain_p;
  struct dma_chain *rd_chain_p;
  char *buf_ptr;
  uint wr_chain_offset;
  uint rd_chain_offset;
  uint buf_offset;
  uint shm_size;
  char *io_base_rd;
  char *io_base_wr;
  uint wr_chain_cnt;
  uint rd_chain_cnt;
  void *dma_buf_kaddr;
  ulong dma_buf_baddr;
  uint dma_mbx_addr;
  uint dma_mbx_data;
#ifdef XENOMAI
  rtdm_user_info_t *dma_user_info;
#endif
};

struct pev_ioctl_dma_list vme_list[63];
struct pev_ioctl_dma_list shm_list[63];
int idt_swap_32( int);
#define SWAP32(x) idt_swap_32(x)


#include "dmalib.h"

#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

void
dma_write_io( void *dma_ctl,
	      uint offset,
	      uint data,
	      uint ctlr)
{
  struct dma_ctl *dc;

  dc = (struct dma_ctl *)dma_ctl;
  if( ctlr & 1) dc++;
  if( ctlr & DMA_CTLR_WR)
  {
    //printk("dma_write_io: %p - %x - %x\n", dc->io_base_wr, offset, data);
    *(uint *)(dc->io_base_wr + offset) = SWAP32(data);
  }
  else
  {
    //printk("dma_write_io: %p - %x - %x\n", dc->io_base_rd, offset, data);
    *(uint *)(dc->io_base_rd + offset) = SWAP32(data);
  }
  return;
}

uint
dma_read_io( void *dma_ctl,
	     uint offset,
	     uint ctlr)
{
  struct dma_ctl *dc;
  uint data;

  dc = (struct dma_ctl *)dma_ctl;
  if( ctlr & 1) dc++;
  if( ctlr & DMA_CTLR_WR)
  {
    data = *(uint *)(dc->io_base_wr + offset);
  }
  else
  {
    data = *(uint *)(dc->io_base_rd + offset);
  }
  return( SWAP32(data));
}

void *
dma_init( void *shm_ptr,
	  ulong shm_base,
	  uint shm_size,
	  char * io_base_rd0,
	  char * io_base_wr0,
	  char * io_base_rd1,
	  char * io_base_wr1)
{
  struct dma_ctl *dc, *dma_ctl;

  /* allocate a buffer in SHM and map it */
  debugk(("in dma_init( %p, %lx, %x)...\n", shm_ptr, shm_base, shm_size));

  //dc = &dma_ctl[crate];
  dma_ctl = (struct dma_ctl *)kmalloc( 2*sizeof(struct dma_ctl), GFP_KERNEL);
  if( !dma_ctl)
  {
    return( (void *)dma_ctl);
  }

  dc = (struct dma_ctl *)dma_ctl;
  dc->shm_size = shm_size -  4*sizeof( struct dma_chain);
  dc->wr_chain_p = (struct dma_chain *)shm_ptr;
  dc->rd_chain_p = (struct dma_chain *)(shm_ptr + sizeof( struct dma_chain));
  dc->buf_ptr = (char *)(shm_ptr + 4*sizeof( struct dma_chain));
  dc->wr_chain_offset = (uint)shm_base;
  dc->rd_chain_offset = (uint)shm_base + sizeof( struct dma_chain);
  dc->buf_offset = (uint)shm_base + 4*sizeof( struct dma_chain);
  dc->io_base_rd = io_base_rd0;
  dc->io_base_wr = io_base_wr0;
  dc->wr_chain_cnt = 1;
  dc->rd_chain_cnt = 1;
  dc->dma_buf_kaddr = NULL;
  dc->dma_buf_baddr = 0;

  dc++;
  dc->shm_size = shm_size/2;
  dc->wr_chain_p = (struct dma_chain *)(shm_ptr + 2*sizeof( struct dma_chain));
  dc->rd_chain_p = (struct dma_chain *)(shm_ptr + 3*sizeof( struct dma_chain));
  dc->buf_ptr = (char *)(shm_ptr + shm_size/2);
  dc->wr_chain_offset = (uint)shm_base + 2*sizeof( struct dma_chain);
  dc->rd_chain_offset = (uint)shm_base + 3*sizeof( struct dma_chain);
  dc->buf_offset = (uint)shm_base + shm_size/2;
  dc->io_base_rd = io_base_rd1;
  dc->io_base_wr = io_base_wr1;
  dc->wr_chain_cnt = 1;
  dc->rd_chain_cnt = 1;
  dc->dma_buf_kaddr = NULL;
  dc->dma_buf_baddr = 0;
  dc->dma_mbx_addr = 0;
  dc->dma_mbx_data = 0;

  return( (void *)dma_ctl);
}
void
dma_exit( void *dma_ctl)
{
  kfree(dma_ctl);
}
#ifdef XENOMAI
int
dma_init_xeno( void *dma_ctl,
	       rtdm_user_info_t *user_info)
{
  struct dma_ctl *dc;

  dc = (struct dma_ctl *)dma_ctl;
  dc->dma_user_info = user_info;
  return( 0);
}
#endif

int
dma_alloc_kmem( void *dma_ctl,
	        void *kaddr,
		ulong baddr)
{
  struct dma_ctl *dc;

  /* allocate a buffer in SHM and map it */
  debugk(("in dma_alloc_kmem( %p, %lx)...\n", kaddr, baddr));

  dc = (struct dma_ctl *)dma_ctl;
  dc->dma_buf_kaddr = kaddr;
  dc->dma_buf_baddr = baddr;
  dc++;
  dc->dma_buf_kaddr = kaddr;
  dc->dma_buf_baddr = baddr;

  return( 0);
}

int
dma_alloc_mbx( void *dma_ctl,
	       uint mbx_addr,
	       uint mbx_data,
	       int ctlr)
{
  struct dma_ctl *dc;

  /* allocate a buffer in SHM and map it */
  debugk(("in dma_alloc_mbx( %x, %x)...\n", mbx_addr, mbx_data));

  dc = (struct dma_ctl *)dma_ctl;
  if( ctlr) dc++;
  dc->dma_mbx_addr = mbx_addr;
  dc->dma_mbx_data = mbx_data;

  return( 0);
}


void *
dma_get_buf_kaddr( void *dma_ctl)
{
  struct dma_ctl *dc;
  dc = (struct dma_ctl *)dma_ctl;
  return( dc->dma_buf_kaddr);
}
 
ulong
dma_get_buf_baddr( void *dma_ctl)
{
  struct dma_ctl *dc;
  dc = (struct dma_ctl *)dma_ctl;
  return( dc->dma_buf_baddr);
}

uint 
dma_set_ctl( uint space,
	     uint trig,
	     uint intr,
	     uint mode)
{
  uint ctl;

  ctl = 0x2000 | (trig << 10) | (intr << 14);
  ctl |= (mode&0x3ff) << 20;
  if( (space & DMA_SPACE_MASK) == DMA_SPACE_PCIE)
  { 
    ctl |= 0x80000;
  }
  if( ( (space & DMA_SPACE_MASK) == DMA_SPACE_USR1) ||
      ( (space & DMA_SPACE_MASK) == DMA_SPACE_USR2)     )
  {
    ctl |= (mode & 0xc000) << 4;
  }
  if( (space & DMA_SPACE_MASK) == DMA_SPACE_VME)
  { 
    ctl |= (mode & 0xc000) << 4;
    if (( space > 0x30) && (space < 0x50))
    {
      ctl |= 0x30000;
    }
    else if (space < 0x60)
    {
      ctl |= 0x20000;
    }
    else
    {
      ctl |= 0x10000;
    }
  }
  return( ctl);
}
 
int
dma_set_rd_desc( void *dma_ctl, 
		 ulong  shm_addr, 
		 ulong des_addr, 
		 uint size,
		 unsigned char space,
		 uint mode)
{
  struct dma_ctl *dc;
  struct dma_desc *dd;
  uint next;
  volatile uint sync_desc;

  debugk(("in dma_set_rd_desc( %lx, %lx, %x, %x)...\n", shm_addr, des_addr, size, space));
  dc = (struct dma_ctl *)dma_ctl;
  if( mode & 0x1000)
  {
    dc++;
    mode &= ~0x1000;
  }
  dd = (struct dma_desc *)dc->rd_chain_p;
  debugk(("chain pointer = %p\n", dd));
  next = dc->rd_chain_offset + sizeof(struct dma_desc);
  if( shm_addr == -1)
  {
    /* allocate temporary buffer */
    shm_addr = dc->buf_offset;
  }

  dd->ctl = SWAP32(dma_set_ctl( space, 0, 0, mode));
  dd->wcnt = SWAP32((uint)space << 24 | size);    /* 4kbyte, VME A32  */
  dd->shm_addr = SWAP32(shm_addr);                /* SHM local buffer */
  dd->next = SWAP32(next);                   
  dd->rem_addr_l = SWAP32((uint)des_addr);        /* vme address      */
  dd->rem_addr_h = 0;

  /* generate interrupt by writing in mailbox */
  dd++;
  dc->rd_chain_cnt = 2;
  next += sizeof(struct dma_desc);     /* offset of next location after end of list */
  next &= 0xffffffe0;

  dd->ctl = SWAP32(dma_set_ctl( 0, 0, 0, 0));
  dd->wcnt = SWAP32(4);                        /* 4 byte to mbx */ 
  dd->shm_addr = SWAP32(next);                 /* get dsata from next shm location after end of list  */
  dd->next = SWAP32( 0x10);                    /* last transfer */       
  dd->rem_addr_l = SWAP32(dc->dma_mbx_addr);   /* mail box  address      */
  dd->rem_addr_h = 0;

  dd++;
  *(uint *)dd = dc->dma_mbx_data;           /* message to be written in mailbox */
  sync_desc =SWAP32( dd->status);           /* synchronize descriptor writing */

  return(dc->rd_chain_offset);
}

int
dma_set_wr_desc( void *dma_ctl,
		 ulong  shm_addr,
		 ulong src_addr, 
		 uint size,
		 unsigned char space,
		 uint mode)
{
  struct dma_ctl *dc;
  struct dma_desc *dd;
  uint intr, trig, trig_wait;
  uint next;
  volatile uint sync_desc;
  int ctlr, tmp;

  debugk(("in dma_set_wr_desc( %lx, %lx, %x, %x)...\n", shm_addr, src_addr, size, space));
  dc = (struct dma_ctl *)dma_ctl;
  ctlr = (mode & 0x1000) >> 12;
  if( ctlr)
  {
    dc++;
    mode &= ~0x1000;
  }
  dd = (struct dma_desc *)dc->wr_chain_p;
  debugk(("chain pointer = %p\n", dd));
  next = dc->wr_chain_offset + sizeof(struct dma_desc);

  dd->ctl = SWAP32(0x2000);            /* get current time */
  dd->wcnt = 0;                /* don't perform any transfer */
  dd->shm_addr = 0;            /* SHM local buffer */
  dd->next = SWAP32( next);             /* chain to next descriptor */            
  dd->rem_addr_l = 0;          /* remote address      */
  dd->rem_addr_h = 0;


  dd++;
  dc->wr_chain_cnt = 2;
  /* don't generate interrupt */
  intr = 0;
  /* generate trig out at end of transfer to start read engine */
  trig = 3;
  if( shm_addr == -1) /* rd engine waits for wr engine */
  {
    /* allocate temporary buffer */
    shm_addr = dc->buf_offset;
  }
  else
  {
    struct dma_desc *dm;
    int rdo;

    /* generate mailbox interrupt at end of transfer */
    dm = (struct dma_desc *)dc->rd_chain_p;
    rdo = dc->rd_chain_offset + sizeof(struct dma_desc);

    dm->ctl = SWAP32(dma_set_ctl( 0, 0, 0, 0));
    dm->wcnt = SWAP32(4);                        /* 4 byte to mbx */ 
    dm->shm_addr = SWAP32( rdo);                 /* get data from next shm location after end of list  */
    dm->next = SWAP32( 0x10);                    /* last transfer */       
    dm->rem_addr_l = SWAP32(dc->dma_mbx_addr);   /* mail box  address      */
    dm->rem_addr_h = 0;
    sync_desc = SWAP32(dm->status);                 /* synchronize descriptor writing */

    dm++;
    *(uint *)dm = dc->dma_mbx_data;           /* message to be written in mailbox */
    //printk("start read engine for mbx: %x - %x\n", dma_ctl, dc->rd_chain_offset | 0x6);
    /* check if DMA read ended */
    tmp = dma_read_io( dma_ctl, 0x0c, DMA_CTLR_RD | ctlr);
    if( !(tmp & 0x80000000))
    {
      /* if not, kill current DMA transfer */
      dma_write_io( dma_ctl, 0x00, 0x40000000, DMA_CTLR_RD | ctlr);
      /* re-enable DMA engine */
      dma_write_io( dma_ctl, 0x00, 0x80000000, DMA_CTLR_RD | ctlr);
    }
    trig_wait = 0x6;
    if( ctlr) trig_wait = 0x7;
    dma_write_io( dma_ctl, 0x04, dc->rd_chain_offset | trig_wait, DMA_CTLR_RD | ctlr);
  }


  dd->ctl = SWAP32(dma_set_ctl( space, trig, intr, mode));
  dd->wcnt = SWAP32((uint)space << 24 | size);    /* 4kbyte, VME A32  */
  dd->shm_addr = SWAP32(shm_addr);                /* SHM local buffer */
  dd->next = SWAP32(0x10);                        /*last descriptor   */
  dd->rem_addr_l = SWAP32((uint)src_addr);        /* vme address      */
  dd->rem_addr_h = 0;
  sync_desc = SWAP32(dd->status);                 /* synchronize descriptor writing */

  return( dc->wr_chain_offset);
}

int
dma_get_desc( void *dma_ctl,
	      uint *d)
{
  struct dma_ctl *dc;
  int i;
  uint *s;

  dc = (struct dma_ctl *)dma_ctl;

  /* get time marker */
  s = (uint *)dc->wr_chain_p;
  for( i = 0; i < 8; i++)
  {
    *d++ =SWAP32( *s++);
  }

  /* get time of last wr chain element */
  s = (uint *)(&dc->wr_chain_p->desc[dc->wr_chain_cnt - 1]);
  for( i = 0; i < 8; i++)
  {
    *d++ = SWAP32(*s++);
  }

  /* get time of last rd chain element */
  s = (uint *)(&dc->rd_chain_p->desc[dc->rd_chain_cnt - 1]);
  for( i = 0; i < 8; i++)
  {
    *d++ = SWAP32(*s++);
  }
  return( 0);
}

int
dma_set_pipe_desc( void *dma_ctl, 
		   ulong des_addr, 
		   ulong  src_addr, 
		   uint size,
		   char des_space,
		   char src_space,
		   uint mode)
{
  struct dma_ctl *dc;
  struct dma_desc *dd_r, *dd_w;
  ulong  shm_addr;
  uint wdo, rdo;
  int n, nbuf;
  int buf_cnt, last_cnt;
  uint next_r, next_w;
  uint ctl_r, ctl_w;
  volatile uint sync_desc;
  uint mode_rd, mode_wr;
  int ctlr, trig;

  ctlr = 0;
  dc = (struct dma_ctl *)dma_ctl;
  if( mode & 0x10001000)
  {
    dc++;
    mode &= ~0x10001000;
    ctlr = 1;
  }
  mode_rd = mode >> 16;
  mode_wr = mode & 0xffff;
  if( size <= 0x1000)
  {
    wdo = dma_set_wr_desc( dma_ctl, -1, src_addr, size, src_space, mode_wr);
    rdo = dma_set_rd_desc( dma_ctl, -1, des_addr, size, des_space, mode_rd);
    trig = 0x6;
    if( ctlr) trig = 0x7;
    dma_write_io(  dma_ctl, 0x04, rdo | trig, DMA_CTLR_RD | ctlr);
    dma_write_io(  dma_ctl, 0x04, wdo, DMA_CTLR_WR | ctlr);
    return( 0);
  }

  debugk(("in dma_set_pipe_desc( %lx, %lx, %x, %x,%x)...\n", src_addr, des_addr, size, src_space,mode));
  dd_r = (struct dma_desc *)dc->rd_chain_p;
  dd_w = (struct dma_desc *)dc->wr_chain_p;
  wdo = dc->wr_chain_offset;
  rdo = dc->rd_chain_offset;

  if( size <= 0x20000)
  {
    buf_cnt = 0x1000;
  }
  else
  {
    buf_cnt = (size >> 5) & 0xfffff000;
  }
  nbuf = size/buf_cnt;
  last_cnt = size % buf_cnt;
  next_w = wdo + sizeof(struct dma_desc);
  next_r = rdo + sizeof(struct dma_desc);


  /* open write chain with time marker element                 */
  dd_w->ctl = SWAP32(0xb2000);           /* get current time           */
  dd_w->wcnt = 0;                /* don't perform any transfer */
  dd_w->shm_addr = 0;            /* SHM local buffer           */
  dd_w->next = SWAP32(next_w);           /* chain to next descriptor   */            
  dd_w->rem_addr_l = 0;          /* remote address             */
  dd_w->rem_addr_h = 0;
  dd_w++;
  next_w += sizeof(struct dma_desc);

  shm_addr = dc->buf_offset;
  dc->wr_chain_cnt = nbuf + 1;
  dc->rd_chain_cnt = nbuf;

  /* generate timing info +                */
  /* trig out at end of shm write transfer */
  ctl_w = 0x2c00;
  if( src_space == 0x00)
  {
    //ctl_w |= 0x2000000; /* PCIe outstanding read */
  }
  if( src_space < 0x30)
  {
    ctl_w |= 0x80000;
  }
  else if (src_space < 0x50)
  {
    ctl_w |= 0xb0000;
  }
  else if (src_space < 0x60)
  {
    ctl_w |= 0xa0000;
  }
  else
  {
    ctl_w |= 0x90000;
  }
  /* generate timing info  */
  ctl_r = 0x2000;
  if( des_space < 0x30)
  {
    ctl_r |= 0x80000;
  }
  else if (des_space < 0x50)
  {
    ctl_r |= 0xb0000;
  }
  else if (des_space < 0x60)
  {
    ctl_r |= 0xa0000;
  }
  else
  {
    ctl_r |= 0x90000;
  }

  n = 0;
  while( nbuf--)
  {
    /* prepare write element */
    dd_w->ctl = SWAP32(ctl_w | ((mode_wr&0x3ff) << 20));
    dd_w->wcnt = SWAP32((uint)src_space << 24 | buf_cnt);    /* 4kbyte, VME A32  */
    dd_w->shm_addr = SWAP32(shm_addr);                       /* SHM local buffer */
    dd_w->next = SWAP32(next_w);                             /*last descriptor   */
    dd_w->rem_addr_l = SWAP32((uint)src_addr);               /* source address      */
    dd_w->rem_addr_h = 0;
    dd_w++;
    next_w += sizeof(struct dma_desc);

    /* prepare read element */
    dd_r->ctl = SWAP32(ctl_r | ((mode_rd&0x3ff) << 20));
    dd_r->wcnt = SWAP32((uint)des_space << 24 | buf_cnt);    /* 4kbyte, VME A32  */
    dd_r->shm_addr = SWAP32(shm_addr);                       /* SHM local buffer */
    trig = 0x6;
    if( ctlr) trig = 0x7;
    dd_r->next = SWAP32(next_r | trig);                       /* wait for trig from write engine */   
    dd_r->rem_addr_l = SWAP32((uint)des_addr);               /* destination address      */
    dd_r->rem_addr_h = 0;
    dd_r++;
    next_r += sizeof(struct dma_desc);

    n++;
    shm_addr += buf_cnt;
    if( !(n%3))shm_addr = dc->buf_offset; /* use 4 buffers of size buf_cnt with rollover */
    src_addr += buf_cnt;
    des_addr += buf_cnt;
  }
  if( last_cnt) /* create last descriptor */
  {
    /* prepare last write element */
    dd_w->ctl = SWAP32(ctl_w | ((mode_wr&0x3ff) << 20));
    dd_w->wcnt = SWAP32((uint)src_space << 24 | last_cnt);    /* 4kbyte, VME A32  */
    dd_w->shm_addr = SWAP32(shm_addr);                    /* SHM local buffer */
    dd_w->next = SWAP32(0x10);                            /*last descriptor   */
    dd_w->rem_addr_l = SWAP32((uint)src_addr);            /* src address      */
    dd_w->rem_addr_h = 0;
    /* prepare last read element */
    dd_r->ctl = SWAP32(ctl_r | ((mode_rd&0x3ff) << 20));
    dd_r->wcnt = SWAP32((uint)des_space << 24 | last_cnt);    /* 4kbyte, VME A32  */
    dd_r->shm_addr = SWAP32(shm_addr);                    /* SHM local buffer */
    dd_r->next = SWAP32(next_r);                          /* don't wait for trig from write engine to write mailbox */    
    dd_r->rem_addr_l = SWAP32((uint)des_addr);            /* dest address      */
    dd_r->rem_addr_h = 0;
  }
  else /* update last descriptor */
  {
    /* close write chain */
    dd_w--;
    dd_w->next = SWAP32(0x10);                 /*last descriptor   */
    /* close read chain */
    dd_r--;
    next_r -= sizeof(struct dma_desc);
    dd_r->next = SWAP32(next_r);                 /* don't wait for trig from write engine to write mailbox */
  }
  /* generate interrupt by writing in mailbox */
  dc->rd_chain_cnt += 1;
  dd_r++;
  next_r += sizeof(struct dma_desc);     /* offset of next location after end of list */
  next_r &= 0xffffffe0;

  dd_r->ctl = SWAP32(ctl_r | ((mode_rd&0x3ff) << 20));        /* don't generate interrupt at end of chain */  
  dd_r->wcnt = SWAP32(4);                        /* 4 byte to mbx */ 
  dd_r->shm_addr = SWAP32(next_r);                 /* get dsata from next shm location after end of list  */
  dd_r->next = SWAP32( 0x10);                    /* last transfer */       
  dd_r->rem_addr_l = SWAP32(dc->dma_mbx_addr);   /* mail box  address      */
  dd_r->rem_addr_h = 0;

  dd_r++;
  *(uint *)dd_r = dc->dma_mbx_data;           /* message to be written in mailbox */

  sync_desc = dd_w->status;                 /* synchronize descriptor writing */
  sync_desc = dd_r->status;                 /* synchronize descriptor writing */

  //outl( 0, dc->io_base + 0x850);         /* reset read counter                               */
  //outl( 0, dc->io_base + 0x858);         /* reset write counter                              */
  //outl( 0x8200, dc->io_base_rd + 0x850);      /* enable read pipe                                 */
  //outl( 0x80c0, dc->io_base_wr + 0x858);      /* enable write pipe                                */
  //outl( rdo | 0x6, dc->io_base_rd + 0x04); /* start read engine waiting for trigger from write */
  trig = 0x6;
  if( ctlr) trig = 0x7;
  dma_write_io(  dma_ctl, 0x04, rdo | trig, DMA_CTLR_RD | ctlr);
  //outl( wdo, dc->io_base_wr + 0x04);       /* start write engine                               */
  dma_write_io(  dma_ctl, 0x04, wdo, DMA_CTLR_WR | ctlr);

  return( 0);
}

int
dma_set_list_desc( void *dma_ctl, 
		   struct pev_ioctl_dma_req *req)
{
  int i, tot_size;
  ulong  shm_addr;
  struct dma_ctl *dc;
  struct dma_desc *dd_r, *dd_w;
  uint wdo, rdo;
  uint next_r, next_w;
  uint ctl;
  volatile uint sync_desc;
  int retval;

  debugk(("in dma_set_list_desc()...%lx -%x\n", req->src_addr, req->size));

  if(req->size > 63)
  { 
    return( -1);
  }

  dc = (struct dma_ctl *)dma_ctl;
  dd_r = (struct dma_desc *)dc->rd_chain_p;
  dd_w = (struct dma_desc *)dc->wr_chain_p;
  wdo = dc->wr_chain_offset;
  rdo = dc->rd_chain_offset;
  next_w = wdo + sizeof(struct dma_desc);
  next_r = rdo + sizeof(struct dma_desc);

  /* get transfer list from application */
#ifdef XENOMAI
  rtdm_copy_from_user( dc->dma_user_info, &vme_list[0], (void *)req->src_addr, req->size*sizeof( struct pev_ioctl_dma_list));
#else
  retval = copy_from_user(&vme_list[0], (void *)req->src_addr, req->size*sizeof( struct pev_ioctl_dma_list));
#endif

  /* open write chain with time marker element                 */
  dd_w->ctl = SWAP32(0x2000);            /* get current time           */
  dd_w->wcnt = 0;                /* don't perform any transfer */
  dd_w->shm_addr = 0;            /* SHM local buffer           */
  dd_w->next = SWAP32(next_w);           /* chain to next descriptor   */            
  dd_w->rem_addr_l = 0;          /* remote address             */
  dd_w->rem_addr_h = 0;
  dd_w++;
  next_w += sizeof(struct dma_desc);
  dc->wr_chain_cnt = 1;

  tot_size = 0;
  ctl = 0;
  shm_addr = dc->buf_offset;
  for( i = 0; i < req->size; i++)
  {
    shm_list[i].addr = vme_list[i].addr & 7;
    shm_list[i].size = (shm_list[i].addr + vme_list[i].size + 7) & ~7;
    debugk(("transfer from vme addr : 0x%lx to shm offset : 0x%x [size = %d]\n", 
            vme_list[i].addr & ~7, tot_size, shm_list[i].size));
    /* build chain descriptor for VME -> shm transfer */
    ctl = dma_set_ctl( vme_list[i].mode, 0, 0, req->src_mode);
    dd_w->ctl = SWAP32(ctl);
    dd_w->wcnt = SWAP32((((vme_list[i].mode & 0xf0) | DMA_SPACE_VME) << 24) | shm_list[i].size);
    dd_w->shm_addr = SWAP32(shm_addr + tot_size);                   
    dd_w->next = SWAP32(next_w);                                    
    dd_w->rem_addr_l = SWAP32((uint)(vme_list[i].addr & ~7));    
    dd_w->rem_addr_h = 0;
    dd_w++;
    next_w += sizeof(struct dma_desc);
    dc->wr_chain_cnt++;

    tot_size += shm_list[i].size;
  }
  /* end chain on last element */
  (dd_w-1)->ctl = SWAP32(ctl | 0xc00); /* generate trig out at end of transfer */
  (dd_w-1)->next = SWAP32(0x10);       /* end of transfer                      */
  debugk(("total size = %4d\n", tot_size));

  /* Build descriptor for SHM -> KBUF transfer */
  dd_r->ctl =  SWAP32(dma_set_ctl( 0, 0, 2, (req->des_space & 0x30) << 4));         /* generate interrupt at end of transfer */                
  dd_r->wcnt = SWAP32(tot_size);   
  dd_r->shm_addr = SWAP32(shm_addr);                    /* SHM local buffer */
  dd_r->next = SWAP32(0x16);                            /* wait for trig from write engine and stop */   
  dd_r->rem_addr_l = SWAP32((uint)dc->dma_buf_baddr);   /* dest address      */
  dd_r->rem_addr_h = 0;
  dc->rd_chain_cnt = 1;

  sync_desc = dd_r->status;                      /* synchronize descriptor writing */

  return( tot_size);


}

int
dma_get_list( void *dma_ctl, 
	      struct pev_ioctl_dma_req *req)
{
  int i, shm_offset, mem_offset;
  void *kaddr, *uaddr;
  struct dma_ctl *dc;
  int retval;

  dc = (struct dma_ctl *)dma_ctl;
  shm_offset = 0;
  mem_offset = 0;
  uaddr = (void *)req->des_addr;
  kaddr = dc->dma_buf_kaddr;
  for( i = 0; i < req->size; i++)
  {
    debugk(("transfer from shm offset : 0x%lx to mem offset : 0x%x [size = %d]\n", 
	    shm_offset + (vme_list[i].addr & 7), mem_offset, vme_list[i].size));

#ifdef XENOMAI
    rtdm_copy_to_user( dc->dma_user_info, uaddr+mem_offset, kaddr+shm_offset + (vme_list[i].addr & 7), vme_list[i].size);
#else
    retval = copy_to_user( uaddr+mem_offset, kaddr+shm_offset + (vme_list[i].addr & 7), vme_list[i].size);
#endif
    shm_offset += shm_list[i].size;
    mem_offset += vme_list[i].size;
  }
      debugk(("total size = %d\n", mem_offset));

  return( mem_offset);
}
