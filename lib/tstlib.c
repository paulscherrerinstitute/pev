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
 * $Log: tstlib.c,v $
 * Revision 1.8  2012/08/16 09:11:38  kalantari
 * added version 4.16 of tosca driver
 *
 * Revision 1.8  2012/06/07 09:07:36  ioxos
 * bug in mapping kbuf for 32 bit systems [JFG]
 *
 * Revision 1.7  2012/06/06 12:17:14  ioxos
 * add rcsid [JFG]
 *
 * Revision 1.6  2012/06/01 13:59:22  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.5  2010/06/11 11:37:10  ioxos
 * use pev mmap() for kernel memory mapping [JFG]
 *
 * Revision 1.4  2009/12/15 17:15:28  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.3  2009/12/08 13:54:41  ioxos
 *  add tst_cpu_check[JFG]
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
#ifndef lint
static char rcsid[] = "$Id: tstlib.c,v 1.8 2012/08/16 09:11:38 kalantari Exp $";
#endif

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
#include <pevulib.h>
#include "xprstst.h"

char *
tst_rcsid()
{
  return( rcsid);
}


int
tst_get_range( char *para,
	       int *first,
	       int *last)
{
  char *p;

  *first = strtoul( para, &p, 16);
  *last = *first;
  p = strpbrk( para,".");
  if( p)
  {
    para = p + strspn(p,".");
    *last =  strtoul( para, &p, 16);
  }
  return( *first);
}

void * 
tst_cpu_map_shm( struct pev_ioctl_map_pg *m,
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
    if( pev_map_alloc( m) < 0)
    {
      return( MAP_FAILED);
    }
  }
  usr_addr = pev_mmap( m);
  if( usr_addr == MAP_FAILED)
  {
    if( size)
    {
      pev_map_free( m);
    }
  }
  return( usr_addr);
}

void 
tst_cpu_unmap_shm( struct pev_ioctl_map_pg *m)
{
  if( m->usr_addr != MAP_FAILED)
  { 
    pev_munmap( m);
    pev_map_free( m);
  }
}

void * 
tst_cpu_map_vme( struct pev_ioctl_map_pg *m,
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
    if( pev_map_alloc( m) < 0)
    {
      return( MAP_FAILED);
    }
  }
  usr_addr = pev_mmap( m);
  if( usr_addr == MAP_FAILED)
  {
    if( size)
    {
      pev_map_free( m);
    }
  }
  return( usr_addr);
}

void 
tst_cpu_unmap_vme( struct pev_ioctl_map_pg *m)
{
  if( m->usr_addr != MAP_FAILED)
  { 
    pev_munmap( m);
    pev_map_free( m);
  }
}

void *
tst_cpu_map_kbuf( struct pev_ioctl_buf *b,
		  uint size)
{
  b->u_addr = MAP_FAILED;
  if( size)
  {
    b->size = size;
    if( !pev_buf_alloc( b))
    {
      return( MAP_FAILED);
    }
  }
  else
  {
#if defined(PPC) || defined(X86_32)
    b->u_addr = mmap( NULL, b->size, PROT_READ|PROT_WRITE, MAP_SHARED, b->kmem_fd, (off_t)b->b_addr);
#else
    b->u_addr = mmap( NULL, b->size, PROT_READ|PROT_WRITE, MAP_SHARED, b->kmem_fd, (off_t)(0x200000000 | (long)b->b_addr));
#endif
  }
  return( b->u_addr);
}
void
tst_cpu_unmap_kbuf( struct pev_ioctl_buf *b)
{
  if( b->u_addr != MAP_FAILED)
  {
    munmap( b->u_addr, b->size);
    pev_buf_free( b);
  }
  return;
}

ulong
tst_vme_map_shm( struct pev_ioctl_map_pg *m,
		 ulong offset,
		 uint size)
{
  if( size)
  {
    /* create an address translation window in the PCIe End Point */
    /* pointing at offset in the PEV1100 Shared Memory            */
    m->rem_addr = offset;
    m->mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_SHM;
    m->flag = 0x0;
    m->sg_id = MAP_SLAVE_VME;
    m->size = size;
    if( pev_map_alloc( m) < 0)
    {
      return( (ulong)-1);
    }
  }
  return( m->loc_addr);
}

void 
tst_vme_unmap_shm( struct pev_ioctl_map_pg *m)
{
  if( m->loc_addr != -1)
  { 
    pev_map_free( m);
  }
}

ulong 
tst_vme_map_kbuf( struct pev_ioctl_map_pg *m,
		  ulong addr,
		  uint size)
{
  if( size)
  {
    /* create an address translation window in the PCIe End Point */
    /* pointing at offset in the PEV1100 Shared Memory            */
    m->rem_addr = addr;
    m->mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_PCIE;
    m->flag = 0x0;
    m->sg_id = MAP_SLAVE_VME;
    m->size = size;
    if( pev_map_alloc( m) < 0)
    {
      return( (ulong)-1);
    }
  }
  return( m->loc_addr);
}

void 
tst_vme_unmap_kbuf( struct pev_ioctl_map_pg *m)
{
  if( m->loc_addr != -1)
  { 
    pev_map_free( m);
  }
}

uint 
tst_vme_conf_read( struct pev_ioctl_vme_conf *v)
{
  pev_vme_conf_read( v);
  return(v->a32_base); 
}

int tst_cpu_wr_shm( uint offset,
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

  return( pev_rdwr( &rdwr));
}

int tst_cpu_rd_shm( uint offset)
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
  pev_rdwr( &rdwr);

  return( data);
}

int 
tst_dma_rd_wait( int tmo)
{
  struct pev_reg_remap *r;

  r = pev_io_remap();
  while( ( !(pev_csr_rd( r->dma_rd + 0x0c) & 0x80000000)) && --tmo);
  if( !tmo)
  {
    return(-1);
  }
  return(0);
}

int 
tst_dma_wr_wait( int tmo)
{
  struct pev_reg_remap *r;

  r = pev_io_remap();
  while( ( !(pev_csr_rd( r->dma_wr + 0x0c) & 0x80000000)) && --tmo);
  if( !tmo)
  {
    return(-1);
  }
  return(0);
}

int 
tst_dma_move_kbuf_shm( ulong kbuf_b_addr,
		       ulong shm_offset, 
		       int size,
		       int mode)
{
  int retval;
  struct pev_ioctl_dma_req dma_req;

  dma_req.src_addr = shm_offset;        /* source is VME address of SHM */
  dma_req.des_addr = kbuf_b_addr;       /* destination is kernel buffer    */
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

  retval = pev_dma_move(&dma_req);
  if( retval < 0)
  {
    return(-1);
  }
  /* Wait until DMA ended */
  if( mode == TST_DMA_POLL)
  {
    return( tst_dma_rd_wait( size));
  }
  return(0);
}

int 
tst_dma_move_shm_kbuf( ulong shm_offset,
		       ulong kbuf_b_addr,    
		       int size,
		       int mode)
{
  int retval;
  struct pev_ioctl_dma_req dma_req;

  dma_req.src_addr = kbuf_b_addr;      /* source is kernel buffer */
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

  retval = pev_dma_move(&dma_req);
  if( retval < 0)
  {
    return(-1);
  }
  /* Wait until DMA ended */
  if( mode == TST_DMA_POLL)
  {
    return( tst_dma_wr_wait( size));
  }
  return(0);
}


int
tst_cpu_fill( void *a,
	      int size,
	      int mode,
	      int p1,
	      int p2)
{
  int i;

  if( mode == 0)
  {
    for( i = 0; i < size; i += 4)
    {
      *(int *)(a+i) = p1;
    }
  }
  if( mode == 1)
  {
    for( i = 0; i < size; i += 4)
    {
      *(int *)(a+i) = p1;
      p1 += p2;
    }
  }
  return( 0);
}

void *
tst_cpu_check( void *a,
	       int size,
	       int mode,
	       int p1,
	       int p2)
{
  int i;

  if( mode == 0)
  {
    for( i = 0; i < size; i += 4)
    {
      if( *(int *)(a+i) != p1)
      {
	return( a+i);
      }
    }
  }
  if( mode == 1)
  {
    for( i = 0; i < size; i += 4)
    {
      if( *(int *)(a+i) != p1)
      {
	return( a+i);
      }
      p1 += p2;
    }
  }
  return( NULL);
}

int
tst_cpu_copy( void *a1,
	      void *a2,
	      int size,
	      int ds)
{
  int i;

  if( ds == 1)
  {
    for( i = 0; i < size; i += ds)
    {
      *(char *)(a1+i) = *(char *)(a2+i);
    }
    return( i);
  }
  if( ds == 2)
  {
    for( i = 0; i < size; i += ds)
    {
      *(short *)(a1+i) = *(short *)(a2+i);
    }
    return( i);
  }
  if( ds == 4)
  {
    for( i = 0; i < size; i += ds)
    {
      *(int *)(a1+i) = *(int *)(a2+i);
    }
    return( i);
  }
  if( ds == 8)
  {
    for( i = 0; i < size; i += ds)
    {
      *(long *)(a1+i) = *(long *)(a2+i);
    }
    return( i);
  }
  return( -1);
}


static char cmp_err_db1;
static char cmp_err_db2;
static short cmp_err_ds1;
static short cmp_err_ds2;
static int cmp_err_di1;
static int cmp_err_di2;
static long cmp_err_dl1;
static long cmp_err_dl2;

int
tst_cpu_cmp( void *a1,
	     void *a2,
	     int size,
	     int ds)
{
  int i;

  if( ds == 1)
  {
    for( i = 0; i < size; i += ds)
    {
      cmp_err_db1 = *(char *)(a1+i);
      cmp_err_db2 = *(char *)(a2+i);
      if( cmp_err_db1 != cmp_err_db2)
      {
	break;
      }
    }
    return( i);
  }
  if( ds == 2)
  {
    for( i = 0; i < size; i += ds)
    {
      cmp_err_ds1 = *(short *)(a1+i);
      cmp_err_ds2 = *(short *)(a2+i);
      if( cmp_err_ds1 != cmp_err_ds2)
      {
	break;
      }
    }
    return( i);
  }
  if( ds == 4)
  {
    for( i = 0; i < size; i += ds)
    {
      cmp_err_di1 = *(int *)(a1+i);
      cmp_err_di2 = *(int *)(a2+i);
      if( cmp_err_di1 != cmp_err_di2)
      {
	break;
      }
    }
    return( i);
  }
  if( ds == 8)
  {
    for( i = 0; i < size; i += ds)
    {
      cmp_err_dl1 = *(long *)(a1+i);
      cmp_err_dl2 = *(long *)(a2+i);
      if( cmp_err_dl1 != cmp_err_dl2)
      {
	break;
      }
    }
    return( i);
  }
  return( -1);
}

int
tst_get_cmp_err( void *a1,
	         void *a2,
	         int ds)
{
  if( ds == 1)
  {
    *(char *)a1 = cmp_err_db1;
    *(char *)a2 = cmp_err_db2;
    return(0);
  }
  if( ds == 2)
  {
    *(short *)a1 = cmp_err_ds1;
    *(short *)a2 = cmp_err_ds2;
    return(0);
  }
  if( ds == 4)
  {
    *(int *)a1 = cmp_err_di1;
    *(int *)a2 = cmp_err_di2;
    return(0);
  }
  if( ds == 8)
  {
    *(long *)a1 = cmp_err_dl1;
    *(long *)a2 = cmp_err_dl2;
    return(0);
  }
  return(-1);
}
