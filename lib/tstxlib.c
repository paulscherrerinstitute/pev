/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : tstlib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : october 10,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
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
 * $Log: tstxlib.c,v $
 * Revision 1.3  2012/04/25 13:18:28  kalantari
 * added i2c epics driver and updated linux driver to v.4.10
 *
 * Revision 1.3  2010/06/11 11:37:10  ioxos
 * use pev mmap() for kernel memory mapping [JFG]
 *
 * Revision 1.2  2009/12/15 17:15:28  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.1  2009/12/08 13:50:39  ioxos
 * first checkin [JFG]
 *
 * Revision 1.2  2009/11/10 09:12:02  ioxos
 * set explicitely number of oustanding request [JFG]
 *
 * Revision 1.1  2009/08/25 13:16:00  ioxos
 * move from XprsTst to here [JFG]
 *
 * Revision 1.1  2009/08/18 13:21:49  ioxos
 * first cvs checkin [JFG]
 *
 *
 *=============================< end file header >============================*/
#include <stdlib.h>
#include <stdio.h>
#include <pty.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>

typedef unsigned int u32;
#include <pevioctl.h>
#include <pevxulib.h>
#include "xprstst.h"



void * 
tstx_cpu_map_shm( int crate,
                  struct pev_ioctl_map_pg *m,
		  ulong offset,
		  uint size)
{
  void *usr_addr;

  if( size)
  {
    /* create an address translation window in the PCIe End Point */
    /* pointing at offset in the PEV1100 Shared Memory            */
    m->rem_addr = offset;
    m->mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_SHM;
    m->flag = 0x0;
    m->sg_id = MAP_MASTER_32;
    m->size = size;
    if( pevx_map_alloc( crate, m) < 0)
    {
      return( MAP_FAILED);
    }
  }
  usr_addr = pevx_mmap( crate, m);
  if( usr_addr == MAP_FAILED)
  {
    if( size)
    {
      pevx_map_free( crate, m);
    }
  }
  return( usr_addr);
}

void 
tstx_cpu_unmap_shm( int crate,
                    struct pev_ioctl_map_pg *m)
{
  if( m->usr_addr != MAP_FAILED)
  { 
    pevx_munmap( crate, m);
    pevx_map_free( crate, m);
  }
}

void * 
tstx_cpu_map_vme( int crate,
                  struct pev_ioctl_map_pg *m,
		  ulong addr,
		  uint size,
		  uint mode)
{
  void *usr_addr;

  if( size)
  {
    /* create an address translation window in the PCIe End Point */
    /* pointing at VME address addr                               */
    m->rem_addr = addr;
    m->mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|mode;
    m->flag = 0x0;
    m->sg_id = MAP_MASTER_32;
    m->size = size;
    if( pevx_map_alloc( crate, m) < 0)
    {
      return( MAP_FAILED);
    }
  }
  usr_addr = pevx_mmap( crate, m);
  if( usr_addr == MAP_FAILED)
  {
    if( size)
    {
      pevx_map_free( crate, m);
    }
  }
  return( usr_addr);
}

void 
tstx_cpu_unmap_vme( int crate,
                    struct pev_ioctl_map_pg *m)
{
  if( m->usr_addr != MAP_FAILED)
  { 
    pevx_munmap( crate, m);
    pevx_map_free( crate, m);
  }
}

void *
tstx_cpu_map_kbuf( int crate,
                   struct pev_ioctl_buf *b,
		   uint size)
{
  b->u_addr = MAP_FAILED;
  if( size)
  {
    b->size = size;
    if( !pevx_buf_alloc( crate, b))
    {
      return( MAP_FAILED);
    }
  }
  else
  {
    b->u_addr = mmap( NULL, b->size, PROT_READ|PROT_WRITE, MAP_SHARED, b->kmem_fd, (off_t)(0x200000000 | (long)b->b_addr));
  }
  return( b->u_addr);
}

void *
tstx_cpu_unmap_kbuf( int crate,
                     struct pev_ioctl_buf *b)
{
  if( b->u_addr != MAP_FAILED)
  {
    munmap( b->u_addr, b->size);
    pevx_buf_free( crate, b);
  }
}

ulong
tstx_vme_map_shm( int crate,
                  struct pev_ioctl_map_pg *m,
		  ulong offset,
		  uint size)
{
  void *usr_addr;

  if( size)
  {
    /* create an address translation window in the PCIe End Point */
    /* pointing at offset in the PEV1100 Shared Memory            */
    m->rem_addr = offset;
    m->mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_SHM;
    m->flag = 0x0;
    m->sg_id = MAP_SLAVE_VME;
    m->size = size;
    if( pevx_map_alloc( crate, m) < 0)
    {
      return( (ulong)-1);
    }
  }
  m->usr_addr = pevx_vmap( crate, m);

  return( (ulong)m->usr_addr);
}

void 
tstx_vme_unmap_shm( int crate,
                    struct pev_ioctl_map_pg *m)
{
  if( m->loc_addr != -1)
  { 
    pevx_map_free( crate, m);
  }
}

ulong 
tstx_vme_map_pci( int crate,
                  struct pev_ioctl_map_pg *m,
		  ulong addr,
		  uint size)
{
  void *usr_addr;

  if( size)
  {
    /* create an address translation window in the PCIe End Point */
    /* pointing at offset in the PEV1100 Shared Memory            */
    m->rem_addr = addr;
    m->mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_PCIE;
    m->flag = 0x0;
    m->sg_id = MAP_SLAVE_VME;
    m->size = size;
    if( pevx_map_alloc( crate, m) < 0)
    {
      return( (ulong)-1);
    }
  }
  m->usr_addr = pevx_vmap( crate, m);

  return( (ulong)m->usr_addr);
}

void 
tstx_vme_unmap_pci( int crate,
                    struct pev_ioctl_map_pg *m)
{
  if( m->loc_addr != -1)
  { 
    pevx_map_free( crate, m);
  }
}

uint 
tstx_vme_conf_read( int crate,
                    struct pev_ioctl_vme_conf *v)
{
  pevx_vme_conf_read( crate, v);
  return(v->a32_base); 
}

int tstx_cpu_wr_shm( int crate,
                     uint offset,
		     uint data)
{
  struct pev_ioctl_rdwr rdwr;

  rdwr.buf = &data;
  rdwr.offset = offset;
  rdwr.len = 0;
  rdwr.mode.ds = RDWR_INT;
  rdwr.mode.dir = RDWR_WRITE;
  rdwr.mode.space = RDWR_DMA_SHM;
  rdwr.mode.swap = RDWR_NOSWAP;

  return( pevx_rdwr( crate, &rdwr));
}

int tstx_cpu_rd_shm( int crate,
                     uint offset)
{
  struct pev_ioctl_rdwr rdwr;
  int data;

  rdwr.buf = &data;
  rdwr.offset = offset;
  rdwr.len = 0;
  rdwr.mode.ds = RDWR_INT;
  rdwr.mode.dir = RDWR_READ;
  rdwr.mode.space = RDWR_DMA_SHM;
  rdwr.mode.swap = RDWR_NOSWAP;
  pevx_rdwr( crate, &rdwr);

  return( data);
}

int 
tstx_dma_rd_wait( int crate,
                  int tmo)
{
  struct pev_reg_remap *r;

  r = pevx_io_remap();
  while( ( !(pevx_csr_rd( crate, r->dma_rd + 0x0c) & 0x80000000)) && --tmo);
  if( !tmo)
  {
    return(-1);
  }
  return(0);
}

int 
tstx_dma_wr_wait( int crate,
                  int tmo)
{
  struct pev_reg_remap *r;

  r = pevx_io_remap();
  while( ( !(pevx_csr_rd( crate, r->dma_wr + 0x0c) & 0x80000000)) && --tmo);
  if( !tmo)
  {
    return(-1);
  }
  return(0);
}

int 
tstx_dma_move_pci_shm( int crate,
                        ulong pci_addr,
		        ulong shm_offset, 
		        int size,
		        int mode)
{
  void *d, *s;
  int i, retval;
  struct pev_ioctl_dma_req dma_req;

  dma_req.src_addr = shm_offset;        /* source is VME address of SHM */
  dma_req.des_addr = pci_addr;       /* destination is kernel buffer    */
  dma_req.size = size;                  
  dma_req.src_space = DMA_SPACE_SHM;
  dma_req.des_space = DMA_SPACE_PCIE;
  dma_req.src_mode = 0;
  dma_req.des_mode = 0;
  dma_req.start_mode = DMA_MODE_BLOCK;
  dma_req.end_mode = 0;
  if( mode == TST_DMA_INTR)
  {
    dma_req.intr_mode = DMA_INTR_ENA;
    dma_req.wait_mode = DMA_WAIT_INTR;
  }
  else
  {
    dma_req.intr_mode = 0;
    dma_req.wait_mode = 0;
  }

  retval = pevx_dma_move( crate, &dma_req);
  if( retval < 0)
  {
    return(-1);
  }
  /* Wait until DMA ended */
  if( mode == TST_DMA_POLL)
  {
    return( tstx_dma_rd_wait( crate, size));
  }
  return(0);
}

int 
tstx_dma_move_shm_pci( int crate,
                       ulong shm_offset,
		       ulong pci_addr,    
		       int size,
		       int mode)
{
  void *d, *s;
  int i, retval;
  struct pev_ioctl_dma_req dma_req;

  dma_req.src_addr = pci_addr;      /* source is kernel buffer */
  dma_req.des_addr = shm_offset;       /* destination is SHM    */
  dma_req.size = size;                  
  dma_req.src_space = DMA_SPACE_PCIE;
  dma_req.des_space = DMA_SPACE_SHM;
  dma_req.src_mode = DMA_PCIE_RR1;
  dma_req.des_mode = 0;
  dma_req.start_mode = DMA_MODE_BLOCK;
  dma_req.end_mode = 0;
  if( mode == TST_DMA_INTR)
  {
    dma_req.intr_mode = DMA_INTR_ENA;
    dma_req.wait_mode = DMA_WAIT_INTR;
  }
  else
  {
    dma_req.intr_mode = 0;
    dma_req.wait_mode = 0;
  }

  retval = pevx_dma_move( crate, &dma_req);
  if( retval < 0)
  {
    return(-1);
  }
  /* Wait until DMA ended */
  if( mode == TST_DMA_POLL)
  {
    return( tstx_dma_wr_wait( crate, size));
  }
  return(0);
}


int 
tstx_dma_move_vme_shm( int crate,
                       ulong vme_addr,
		       ulong shm_offset, 
                       int am,   
		       int size,
		       int mode)
{
  void *d, *s;
  int i, retval;
  struct pev_ioctl_dma_req dma_req;

  dma_req.src_addr = shm_offset;        /* source is VME address of SHM */
  dma_req.des_addr = vme_addr;          /* destination is VME bus    */
  dma_req.size = size;                  
  dma_req.src_space = DMA_SPACE_SHM;
  dma_req.des_space = DMA_SPACE_VME|am;
  dma_req.src_mode = 0;
  dma_req.des_mode = 0;
  dma_req.start_mode = DMA_MODE_BLOCK;
  dma_req.end_mode = 0;
  if( mode == TST_DMA_INTR)
  {
    dma_req.intr_mode = DMA_INTR_ENA;
    dma_req.wait_mode = DMA_WAIT_INTR;
  }
  else
  {
    dma_req.intr_mode = 0;
    dma_req.wait_mode = 0;
  }

  retval = pevx_dma_move( crate, &dma_req);
  if( retval < 0)
  {
    return(-1);
  }
  /* Wait until DMA ended */
  if( mode == TST_DMA_POLL)
  {
    return( tstx_dma_rd_wait( crate, size));
  }
  return(0);
}

int 
tstx_dma_move_shm_vme( int crate,
		       ulong shm_offset, 
		       ulong vme_addr, 
                       int am,   
		       int size,
		       int mode)
{
  void *d, *s;
  int i, retval;
  struct pev_ioctl_dma_req dma_req;

  dma_req.src_addr = vme_addr;         /* source is VME bus */
  dma_req.des_addr = shm_offset;       /* destination is SHM    */
  dma_req.size = size;                  
  dma_req.src_space = DMA_SPACE_VME|am;
  dma_req.des_space = DMA_SPACE_SHM;
  dma_req.src_mode = 0;
  dma_req.des_mode = 0;
  dma_req.start_mode = DMA_MODE_BLOCK;
  dma_req.end_mode = 0;
  if( mode == TST_DMA_INTR)
  {
    dma_req.intr_mode = DMA_INTR_ENA;
    dma_req.wait_mode = DMA_WAIT_INTR;
  }
  else
  {
    dma_req.intr_mode = 0;
    dma_req.wait_mode = 0;
  }

  retval = pevx_dma_move( crate, &dma_req);
  if( retval < 0)
  {
    return(-1);
  }
  /* Wait until DMA ended */
  if( mode == TST_DMA_POLL)
  {
    return( tstx_dma_wr_wait( crate, size));
  }
  return(0);
}


