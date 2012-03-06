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
 * Revision 1.2  2012/03/06 10:31:34  kalantari
 * patch for pevdrvr.c to solve VME hang-up problem due to caching
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
  void *shm_ptr;
  struct dma_chain *wr_chain_p;
  struct dma_chain *rd_chain_p;
  char *buf_ptr;
  ulong shm_base;
  uint wr_chain_offset;
  uint rd_chain_offset;
  uint buf_offset;
  uint shm_size;
  uint io_base;
  uint wr_chain_cnt;
  uint rd_chain_cnt;
  void *dma_buf_kaddr;
  ulong dma_buf_baddr;
#ifdef XENOMAI
  rtdm_user_info_t *dma_user_info;
#endif
} dma_ctl[16];

struct pev_ioctl_dma_list vme_list[63];
struct pev_ioctl_dma_list shm_list[63];
#ifdef PPC
short rdwr_swap_16( short);
int rdwr_swap_32( int);
#define SWAP16(x) rdwr_swap_16(x)
#define SWAP32(x) rdwr_swap_32(x)
#else
#define SWAP16(x) x
#define SWAP32(x) x
#endif


#include "dmalib.h"

#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif


int
dma_init( int crate,
	  void *shm_ptr,
	  ulong shm_base,
	  uint shm_size,
	  uint io_base)
{
  struct dma_ctl *dc;

  /* allocate a buffer in SHM and map it */
  debugk(("in dma_init( %p, %lx, %x)...\n", shm_ptr, shm_base, shm_size));

  dc = &dma_ctl[crate];
  dc->shm_ptr = shm_ptr;
  dc->shm_base = shm_base;
  dc->shm_size = shm_size;
  dc->wr_chain_p = (struct dma_chain *)shm_ptr;
  dc->rd_chain_p = (struct dma_chain *)(shm_ptr + sizeof( struct dma_chain));
  dc->buf_ptr = (char *)(shm_ptr + 2*sizeof( struct dma_chain));
  dc->wr_chain_offset = (uint)shm_base;
  dc->rd_chain_offset = (uint)shm_base + sizeof( struct dma_chain);
  dc->buf_offset = (uint)shm_base + 2*sizeof( struct dma_chain);
  dc->io_base = io_base;
  dc->wr_chain_cnt = 1;
  dc->rd_chain_cnt = 1;
  dc->dma_buf_kaddr = NULL;
  dc->dma_buf_baddr = 0;

  return( 0);
}

#ifdef XENOMAI
int
dma_init_xeno( int crate,
	       rtdm_user_info_t *user_info)
{
  struct dma_ctl *dc;

  dc = &dma_ctl[crate];
  dc->dma_user_info = user_info;
  return( 0);
}
#endif

int
dma_alloc_kmem( int crate,
	        void *kaddr,
		ulong baddr)
{
  struct dma_ctl *dc;

  /* allocate a buffer in SHM and map it */
  debugk(("in dma_alloc_kmem( %p, %lx)...\n", kaddr, baddr));

  dc = &dma_ctl[crate];
  dc->dma_buf_kaddr = kaddr;
  dc->dma_buf_baddr = baddr;

  return( 0);
}


void *
dma_get_buf_kaddr( int crate)
{
  struct dma_ctl *dc;
  dc = &dma_ctl[crate];
  return( dc->dma_buf_kaddr);
}
 
ulong
dma_get_buf_baddr( int crate)
{
  struct dma_ctl *dc;
  dc = &dma_ctl[crate];
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
  ctl |= 0x80000 | ((mode&0xff) << 20);
  if( (space & DMA_SPACE_MASK) == DMA_SPACE_VME)
  { 
    if( space < 0x30)
    {
    ctl |= 0x80000;
    }
    else if (space < 0x50)
    {
      ctl |= 0xb0000;
    }
    else if (space < 0x60)
    {
      ctl |= 0xa0000;
    }
    else
    {
      ctl |= 0x90000;
    }
  }
  return( ctl);
}
 
int
dma_set_rd_desc( int crate, 
		 ulong  shm_addr, 
		 ulong des_addr, 
		 uint size,
		 unsigned char space,
		 char mode)
{
  struct dma_ctl *dc;
  struct dma_desc *dd;
  uint next;
  volatile uint sync_desc;

  debugk(("in dma_set_rd_desc( %lx, %lx, %x, %x)...\n", shm_addr, des_addr, size, space));
  dc = &dma_ctl[crate];
  dd = (struct dma_desc *)dc->rd_chain_p;
  debugk(("chain pointer = %p\n", dd));
  next = 0x10;                    /*last descriptor   */
  if( shm_addr == -1)
  {
    /* allocate temporary buffer */
    shm_addr = dc->buf_offset;
    /* wait for trigger from write engine */
    next |= 0x6;
  }
  dc->rd_chain_cnt = 1;


  dd->ctl = SWAP32(dma_set_ctl( space, 0, 2, mode));
  dd->wcnt = SWAP32((uint)space << 24 | size);    /* 4kbyte, VME A32  */
  dd->shm_addr = SWAP32(shm_addr);                /* SHM local buffer */
  dd->next = SWAP32(next);                   
  dd->rem_addr_l = SWAP32((uint)des_addr);        /* vme address      */
  dd->rem_addr_h = 0;
  sync_desc =SWAP32( dd->status);                 /* synchronize descriptor writing */

  return(dc->rd_chain_offset);
}

int
dma_set_wr_desc( int crate,
		 ulong  shm_addr,
		 ulong src_addr, 
		 uint size,
		 unsigned char space,
		 char mode)
{
  struct dma_ctl *dc;
  struct dma_desc *dd;
  uint intr, trig;
  uint next;
  volatile uint sync_desc;

  debugk(("in dma_set_wr_desc( %lx, %lx, %x, %x)...\n", shm_addr, src_addr, size, space));
  dc = &dma_ctl[crate];
  dd = (struct dma_desc *)dc->wr_chain_p;
  debugk(("chain pointer = %p\n", dd));
  next = dc->wr_chain_offset + sizeof(struct dma_desc);

  dd->ctl = SWAP32(0x2000);            /* get current time */
  dd->wcnt = 0;                /* don't perform any transfer */
  dd->shm_addr = 0;            /* SHM local buffer */
  dd->next = SWAP32(next);             /* chain to next descriptor */            
  dd->rem_addr_l = 0;          /* remote address      */
  dd->rem_addr_h = 0;


  dd++;
  dc->wr_chain_cnt = 2;

  trig = 0;
  intr = 0;
  if( shm_addr == -1)
  {
    /* allocate temporary buffer */
    shm_addr = dc->buf_offset;
    /* generate trig out at end of transfer */
    trig = 3;
  }
  else
  {
    /* generate interrupt at end of transfer */
    intr = 2;
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
dma_get_desc( int crate,
	      uint *d)
{
  struct dma_ctl *dc;
  int i;
  uint *s;

  dc = &dma_ctl[crate];

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
dma_set_pipe_desc( int crate, 
		   ulong des_addr, 
		   ulong  src_addr, 
		   uint size,
		   char des_space,
		   char src_space)
{
  struct dma_ctl *dc;
  struct dma_desc *dd_r, *dd_w;
  ulong  shm_addr;
  uint wdo, rdo;
  int nbuf;
  int buf_cnt, last_cnt;
  uint next_r, next_w;
  uint ctl_r, ctl_w;
  volatile uint sync_desc;

  dc = &dma_ctl[crate];
  if( size <= 0x1000)
  {
    wdo = dma_set_wr_desc( crate, -1, src_addr, size, src_space, 0);
    rdo = dma_set_rd_desc( crate, -1, des_addr, size, des_space, 0);
    outl( rdo | 0x6, dc->io_base + 0x904); /* start read engine waiting for trigger from write */
    outl( wdo, dc->io_base + 0xa04); /* start write engine */
    return( 0);
  }

  debugk(("in dma_set_pipe_desc( %lx, %lx, %x, %x)...\n", src_addr, des_addr, size, src_space));
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
  dd_w->ctl = 0xb2000;           /* get current time           */
  dd_w->wcnt = 0;                /* don't perform any transfer */
  dd_w->shm_addr = 0;            /* SHM local buffer           */
  dd_w->next = next_w;           /* chain to next descriptor   */            
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
  ctl_r = 0x2c00;
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

  while( nbuf--)
  {
    /* prepare write element */
    dd_w->ctl = ctl_w;
    dd_w->wcnt = (uint)src_space << 24 | buf_cnt;    /* 4kbyte, VME A32  */
    dd_w->shm_addr = shm_addr;                       /* SHM local buffer */
    dd_w->next = next_w;                             /*last descriptor   */
    dd_w->rem_addr_l = (uint)src_addr;               /* source address      */
    dd_w->rem_addr_h = 0;
    dd_w++;
    next_w += sizeof(struct dma_desc);

    /* prepare read element */
    dd_r->ctl = ctl_r;
    dd_r->wcnt = (uint)des_space << 24 | buf_cnt;    /* 4kbyte, VME A32  */
    dd_r->shm_addr = shm_addr;                       /* SHM local buffer */
    dd_r->next = next_r | 0x4;                       /* wait for trig from write engine */   
    dd_r->rem_addr_l = (uint)des_addr;               /* destination address      */
    dd_r->rem_addr_h = 0;
    dd_r++;
    next_r += sizeof(struct dma_desc);

    shm_addr += buf_cnt;
    src_addr += buf_cnt;
    des_addr += buf_cnt;
  }
  if( last_cnt) /* create last descriptor */
  {
    /* prepare last write element */
    dd_w->ctl = ctl_w;
    dd_w->wcnt = (uint)src_space << 24 | last_cnt;    /* 4kbyte, VME A32  */
    dd_w->shm_addr = shm_addr;                    /* SHM local buffer */
    dd_w->next = 0x10;                            /*last descriptor   */
    dd_w->rem_addr_l = (uint)src_addr;            /* src address      */
    dd_w->rem_addr_h = 0;
    /* prepare last read element */
    dd_r->ctl = ctl_r | 0x8000;                   /* generate interrupt at end of chain */
    dd_r->wcnt = (uint)des_space << 24 | last_cnt;    /* 4kbyte, VME A32  */
    dd_r->shm_addr = shm_addr;                    /* SHM local buffer */
    dd_r->next = 0x14;                            /* wait for trig from write engine and stop */   
    dd_r->rem_addr_l = (uint)des_addr;            /* dest address      */
    dd_r->rem_addr_h = 0;
  }
  else /* update last descriptor */
  {
    /* close write chain */
    dd_w--;
    dd_w->next = 0x10;                 /*last descriptor   */

    /* close read chain */
    dd_r--;
    dd_r->ctl = ctl_r | 0x8000;        /* generate interrupt at end of chain */  
    dd_r->next = 0x14;                 /* wait for trig from write engine and stop */
  }
  sync_desc = dd_w->status;                 /* synchronize descriptor writing */
  sync_desc = dd_r->status;                 /* synchronize descriptor writing */

  //outl( 0, dc->io_base + 0x850);         /* reset read counter                               */
  //outl( 0, dc->io_base + 0x858);         /* reset write counter                              */
  outl( 0x8200, dc->io_base + 0x850);      /* enable read pipe                                 */
  outl( 0x80c0, dc->io_base + 0x858);      /* enable write pipe                                */
  outl( rdo | 0x4, dc->io_base + 0x904); /* start read engine waiting for trigger from write */
  outl( wdo, dc->io_base + 0xa04);       /* start write engine                               */

  return( 0);
}

int
dma_set_list_desc( int crate, 
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

  dc = &dma_ctl[crate];
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
  dd_w->ctl = 0x2000;            /* get current time           */
  dd_w->wcnt = 0;                /* don't perform any transfer */
  dd_w->shm_addr = 0;            /* SHM local buffer           */
  dd_w->next = next_w;           /* chain to next descriptor   */            
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
    ctl = dma_set_ctl( vme_list[i].mode, 0, 0, 0);
    dd_w->ctl = ctl;
    dd_w->wcnt = (((vme_list[i].mode & 0xf0) | DMA_SPACE_VME) << 24) | shm_list[i].size;
    dd_w->shm_addr = shm_addr + tot_size;                   
    dd_w->next = next_w;                                    
    dd_w->rem_addr_l = (uint)(vme_list[i].addr & ~7);    
    dd_w->rem_addr_h = 0;
    dd_w++;
    next_w += sizeof(struct dma_desc);
    dc->wr_chain_cnt++;

    tot_size += shm_list[i].size;
  }
  /* end chain on last element */
  (dd_w-1)->ctl = ctl | 0xc00; /* generate trig out at end of transfer */
  (dd_w-1)->next = 0x10;       /* end of transfer                      */
  debugk(("total size = %4d\n", tot_size));

  /* Build descriptor for SHM -> KBUF transfer */
  dd_r->ctl = dma_set_ctl( 0, 0, 2, 0);         /* generate interrupt at end of transfer */                
  dd_r->wcnt = tot_size;   
  dd_r->shm_addr = shm_addr;                    /* SHM local buffer */
  dd_r->next = 0x14;                            /* wait for trig from write engine and stop */   
  dd_r->rem_addr_l = (uint)dc->dma_buf_baddr;   /* dest address      */
  dd_r->rem_addr_h = 0;
  dc->rd_chain_cnt = 1;

  sync_desc = dd_r->status;                      /* synchronize descriptor writing */

  return( tot_size);


}

int
dma_get_list( int crate, 
	      struct pev_ioctl_dma_req *req)
{
  int i, shm_offset, mem_offset;
  void *kaddr, *uaddr;
  struct dma_ctl *dc;
  int retval;

  dc = &dma_ctl[crate];
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
