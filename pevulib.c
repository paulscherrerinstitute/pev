/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pevulib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : 
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That library contains a set of function to access the PEV1000 interface
 *     through the /dev/pev device driver.
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
 *  $Log: pevulib.c,v $
 *  Revision 1.1  2012/02/06 14:14:34  kalantari
 *  added required IoxoS version 3.13 sources and headers
 *
 *  Revision 1.33  2012/01/06 14:39:58  ioxos
 *  release 3.13 [JFG]
 *
 *  Revision 1.32  2012/01/06 13:24:35  ioxos
 *  support for X86 32 bit [JFG]
 *
 *  Revision 1.31  2011/12/06 13:16:42  ioxos
 *  support for multi task VME IRQ [JFG]
 *
 *  Revision 1.30  2011/10/19 14:11:33  ioxos
 *  release 3.11 [JFG]
 *
 *  Revision 1.29  2011/10/19 14:02:22  ioxos
 *   32bit mmap + vme irq handling [JFG]
 *
 *  Revision 1.28  2011/10/03 09:57:16  ioxos
 *  release 1.10 [JFG]
 *
 *  Revision 1.27  2011/03/03 15:42:22  ioxos
 *  support for 1MBytes VME slave granularity [JFG]
 *
 *  Revision 1.26  2011/01/25 13:42:05  ioxos
 *  support for VME RMW [JFG]
 *
 *  Revision 1.25  2010/08/26 15:08:11  ioxos
 *  rcsid static const [JFG]
 *
 *  Revision 1.24  2010/08/26 14:25:32  ioxos
 *  cleanup void pointers [JFG]
 *
 *  Revision 1.23  2010/06/11 11:37:10  ioxos
 *  use pev mmap() for kernel memory mapping [JFG]
 *
 *  Revision 1.22  2010/05/18 07:32:28  ioxos
 *  add VME SYSRESET [JFG]
 *
 *  Revision 1.21  2010/01/12 15:43:11  ioxos
 *  add function to read vme list [JFG]
 *
 *  Revision 1.20  2010/01/08 15:39:01  ioxos
 *  add fuction to transfer dma list [JFG]
 *
 *  Revision 1.19  2009/12/15 17:15:28  ioxos
 *  modification for short io window [JFG]
 *
 *  Revision 1.18  2009/12/08 13:55:33  ioxos
 *   add vme map function [JFG]
 *
 *  Revision 1.17  2009/10/21 07:20:04  ioxos
 *  correct bug in pev_init() [JFG]
 *
 *  Revision 1.16  2009/09/29 12:44:24  ioxos
 *  support to read/write sflash status [JFG]
 *
 *  Revision 1.15  2009/08/25 13:18:59  ioxos
 *  store crate number in node structure + get_crate function [JFG]
 *
 *  Revision 1.14  2009/06/03 12:36:59  ioxos
 *  use buf_alloc instead of dma_alloc [JFG]
 *
 *  Revision 1.13  2009/06/02 11:48:12  ioxos
 *  add driver identification [jfg]
 *
 *  Revision 1.12  2009/04/15 12:59:01  ioxos
 *  support user mapping in PMEM space [JFG]
 *
 *  Revision 1.11  2009/04/06 13:52:33  ioxos
 *  correct device name in pev_init() [JFG]
 *
 *  Revision 1.10  2009/04/06 12:54:30  ioxos
 *  add set_crate() [JFG]
 *
 *  Revision 1.9  2009/04/06 12:36:59  ioxos
 *  add support for multicrate [JFG]
 *
 *  Revision 1.8  2009/01/09 13:12:05  ioxos
 *  add support for DMA status [JFG]
 *
 *  Revision 1.7  2009/01/06 14:51:27  ioxos
 *  remove struct pev_ioctl_mv [JFG]
 *
 *  Revision 1.6  2008/12/12 13:53:52  jfg
 *  add memory mapping and dma functions [JFG]
 *
 *  Revision 1.5  2008/11/12 12:15:19  ioxos
 *  add  pev_csr_set(), pev_map_find(), pev_timer_xxx() functions [JFG]
 *
 *  Revision 1.4  2008/09/17 12:41:04  ioxos
 *  add functions to get/set VME config and CRSCR [JFG]
 *
 *  Revision 1.3  2008/08/08 10:03:56  ioxos
 *  functions to alloc/free dma buffer and get FPGA signature [JFG]
 *
 *  Revision 1.2  2008/07/04 07:41:26  ioxos
 *  update address mapping functions [JFG]
 *
 *  Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 *  Import sources for PEV1100 project [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static const char *rcsid = "$Id: pevulib.c,v 1.1 2012/02/06 14:14:34 kalantari Exp $";
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

typedef unsigned int u32;
#include <pevioctl.h>
#include <pevulib.h>

static struct pev_node *pev = (struct pev_node *)NULL;
static struct pev_node *pevx[16]={ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static char pev_drv_id[16] = {0,};
static struct pev_reg_remap io_remap;
char pev_driver_version[16];
char pev_lib_version[] = "3.13";

struct pev_node
*pev_init( uint crate)
{
  char dev_name[16];
  if( (crate < 0) || (crate > 15))
  {
    return( (struct pev_node *)0);
  }
  if( !pevx[crate])
  {
    pev = (struct pev_node *)malloc( sizeof( struct pev_node));
    if( !pev)
    {
      return( pev);
    }
    pevx[crate] = pev;
    pev->crate = crate;
    pev->fd = -1;
  }
  else
  {
    pev = pevx[crate];
  }
  if( pev->fd < 0)
  {
    sprintf( dev_name, "/dev/pev%d", crate);
    pev->fd = open(dev_name, O_RDWR);
    ioctl( pev->fd, PEV_IOCTL_ID, pev_drv_id);
    ioctl( pev->fd, PEV_IOCTL_IO_REMAP, &io_remap);
  }
  return(pev);
}

char *
pev_get_driver_version()
{
  ioctl( pev->fd, PEV_IOCTL_VERSION, pev_driver_version);
  return( pev_driver_version);
}

char *
pev_get_lib_version()
{
  return( pev_lib_version);
}

char
*pev_id()
{
  return( pev_drv_id);
}

struct pev_reg_remap
*pev_io_remap()
{
  return( &io_remap);
}

struct pev_node
*pev_set_crate( uint crate)
{
  if( pevx[crate])
  {
    pev = pevx[crate];
    return( pev);
  }
  return( (struct pev_node *)0);
}

int
pev_get_crate()
{
  if( pev)
  {
    return( pev->crate);
  }
  return( -1);
}

int
pev_exit( struct pev_node *pev)
{
  int ret;

  ret = 0;
  if( pev->fd > 0)
  {
    ret = close( pev->fd);
    pev->fd = -1;
  }
  return(ret);
}

long
pev_swap_64( long data)
{
  char ci[8];
  char co[8];

  *(long *)ci = data;
  co[0] = ci[7];
  co[1] = ci[6];
  co[2] = ci[5];
  co[3] = ci[4];
  co[4] = ci[3];
  co[5] = ci[2];
  co[6] = ci[1];
  co[7] = ci[0];

  return( *(long *)co);
}

int
pev_swap_32( int data)
{
  char ci[4];
  char co[4];

  *(int *)ci = data;
  co[0] = ci[3];
  co[1] = ci[2];
  co[2] = ci[1];
  co[3] = ci[0];

  return( *(int *)co);
}

short
pev_swap_16( short data)
{
  char ci[2];
  char co[2];

  *(short *)ci = data;
  co[0] = ci[1];
  co[1] = ci[0];

  return( *(short *)co);
}

int
pev_rdwr( struct pev_ioctl_rdwr *rdwr_p)
{
  return( ioctl( pev->fd, PEV_IOCTL_RDWR, rdwr_p));
}
 
int
pev_smon_rd( int idx)
{
  struct pev_ioctl_rw_reg rd;

  rd.addr_off = io_remap.iloc_smon;
  rd.data_off = io_remap.iloc_smon + 0x4;
  rd.reg_idx = idx;
  rd.reg_data = 0x0;

  ioctl( pev->fd, PEV_IOCTL_RD_REG_32, &rd);

  return( rd.reg_data);
} 

void
pev_smon_wr( int idx, int data)
{
  struct pev_ioctl_rw_reg wr;

  wr.addr_off = io_remap.iloc_smon;
  wr.data_off = io_remap.iloc_smon + 0x4;
  wr.reg_idx = idx;
  wr.reg_data = data;

  ioctl( pev->fd, PEV_IOCTL_WR_REG_32, &wr);

  return;
}
 
int
pev_csr_rd( int idx)
{
  struct pev_ioctl_rw rd;

  rd.offset = idx;
  rd.data = 0x0;

  ioctl( pev->fd, PEV_IOCTL_RD_IO_32, &rd);

  return( rd.data);
} 

void
pev_csr_wr( int idx, int data)
{
  struct pev_ioctl_rw wr;

  wr.offset = idx;
  wr.data = data;

  ioctl( pev->fd, PEV_IOCTL_WR_IO_32, &wr);

  return;
}
 
void
pev_csr_set( int idx, int data)
{
  struct pev_ioctl_rw wr;

  wr.offset = idx;
  wr.data = data;

  ioctl( pev->fd, PEV_IOCTL_SET_IO_32, &wr);

  return;
}
void *
pev_mmap( struct pev_ioctl_map_pg *map)
{
  map->usr_addr = MAP_FAILED;
  if( map->sg_id == MAP_PCIE_MEM)
  {
#if defined(PPC) || defined(X86_32)
    map->usr_addr = mmap( NULL, map->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, map->loc_addr | 0x80000000);
#else
    map->usr_addr = mmap( NULL, map->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, map->loc_addr);
#endif
  }
  if( map->sg_id == MAP_PCIE_PMEM)
  {
#if defined(PPC) || defined(X86_32)
    map->usr_addr = mmap( NULL, map->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, map->loc_addr | 0xc0000000);
#else
    map->usr_addr = mmap( NULL, map->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, 0x100000000 | map->loc_addr);
#endif
  }

  return( map->usr_addr);
}
 
int
pev_munmap( struct pev_ioctl_map_pg *map)
{
  return( munmap( map->usr_addr, map->size));
}
 
void *
pev_vmap( struct pev_ioctl_map_pg *map)
{
  struct pev_ioctl_vme_conf conf;

  map->usr_addr = (void *)map->loc_addr;
  ioctl( pev->fd, PEV_IOCTL_VME_CONF_RD, &conf);
  map->usr_addr = (void *)((char *)map->usr_addr + conf.a32_base);

  return( map->usr_addr);
}

int
pev_map_alloc( struct pev_ioctl_map_pg *map)
{
  return( ioctl( pev->fd, PEV_IOCTL_MAP_ALLOC, map));
}
 
int
pev_map_free( struct pev_ioctl_map_pg *map)
{
  return( ioctl( pev->fd, PEV_IOCTL_MAP_FREE, map));
}
 
int
pev_map_modify( struct pev_ioctl_map_pg *map)
{
  return( ioctl( pev->fd, PEV_IOCTL_MAP_MODIFY, map));
}

int
pev_map_find( struct pev_ioctl_map_pg *map)
{
  return( ioctl( pev->fd, PEV_IOCTL_MAP_FIND, map));
}

int
pev_map_read( struct pev_ioctl_map_ctl *ctl)
{
  return( ioctl( pev->fd, PEV_IOCTL_MAP_READ, ctl));
}

int
pev_map_clear( struct pev_ioctl_map_ctl *ctl)
{
  return( ioctl( pev->fd, PEV_IOCTL_MAP_CLEAR, ctl));
}

void *
pev_buf_alloc( struct pev_ioctl_buf *db_p)
{
  db_p->kmem_fd = -1;
  ioctl( pev->fd, PEV_IOCTL_BUF_ALLOC, db_p);
  db_p->u_addr = NULL;
  if( db_p->k_addr)
  {
    db_p->kmem_fd = pev->fd;
#if defined(PPC) || defined(X86_32)
  printf("pev_map_clear: X86_32\n");
    db_p->u_addr = mmap( NULL, db_p->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, (off_t)db_p->b_addr);
#else
    db_p->u_addr = mmap( NULL, db_p->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, (off_t)(0x200000000 | (long)db_p->b_addr));
#endif
  }
  return( db_p->u_addr);
}
 
int
pev_buf_free( struct pev_ioctl_buf *db_p)
{
  if( db_p->u_addr)
  {
    munmap( db_p->u_addr, db_p->size);
    db_p->u_addr = NULL;
  }
  db_p->kmem_fd = -1;
  return( ioctl( pev->fd, PEV_IOCTL_BUF_FREE, db_p));
}

int
pev_dma_move( struct pev_ioctl_dma_req *dr_p)
{
  return( ioctl( pev->fd, PEV_IOCTL_DMA_MOVE, dr_p));
}

int
pev_dma_vme_list_rd( void *uaddr, 
		     struct pev_ioctl_dma_list *list_p,
		     int list_size)
{
  struct pev_ioctl_dma_req req;

  req.src_addr = (ulong)list_p;      /* source is VME list           */
  req.des_addr = (ulong)uaddr;       /* destination is DMA buffer    */
  req.size = list_size;                  
  req.src_space = DMA_SPACE_VME;
  req.des_space = DMA_SPACE_PCIE|DMA_PCIE_UADDR;
  req.src_mode = 0;
  req.des_mode = 0;
  req.start_mode = DMA_MODE_LIST_RD;
  req.end_mode = 0;
  req.intr_mode = 0;
  req.wait_mode = 0;

  return( ioctl( pev->fd, PEV_IOCTL_DMA_MOVE, &req));
}

int
pev_dma_status( struct pev_ioctl_dma_sts *ds_p)
{
  return( ioctl( pev->fd, PEV_IOCTL_DMA_STATUS, ds_p));
}

int
pev_pex_read( uint reg)
{
  struct pev_ioctl_i2c i2c;

  i2c.cmd = 0x4003c00 | (( reg >> 2) & 0x3ff);
  i2c.cmd |= (reg << 3) &0x78000;
  i2c.data = 0;
  ioctl( pev->fd, PEV_IOCTL_I2C_PEX_RD, &i2c);

  return( i2c.data);
}

int
pev_pex_write( uint reg,
	       uint data)
{
  struct pev_ioctl_i2c i2c;

  i2c.cmd = 0x3003c00 | (( reg >> 2) & 0x3ff);
  i2c.cmd |= (reg << 3) &0x78000;
  i2c.data = data;
  ioctl( pev->fd, PEV_IOCTL_I2C_PEX_WR, &i2c);

  return( 0);
}

int
pev_sflash_id( char *id)
{
  return( ioctl( pev->fd, PEV_IOCTL_SFLASH_ID, id));
}

int
pev_sflash_rdsr()
{
  int sr;
  ioctl( pev->fd, PEV_IOCTL_SFLASH_RDSR, &sr);
  printf("pevulib: SFLASH status %x\n", sr);
  return( sr);
}

int
pev_sflash_wrsr( int sr)
{
  return( ioctl( pev->fd, PEV_IOCTL_SFLASH_WRSR, &sr));
}

int
pev_sflash_read( uint offset,
		 void *addr,
		 uint len)
{
  struct pev_ioctl_rdwr rdwr;

  rdwr.buf = addr;
  rdwr.offset = offset;
  rdwr.len = len;

  return( ioctl( pev->fd, PEV_IOCTL_SFLASH_RD, &rdwr));
}

int
pev_sflash_write( uint offset,
		  void *addr,
		  uint len)
{
  struct pev_ioctl_rdwr rdwr;

  rdwr.buf = addr;
  rdwr.offset = offset;
  rdwr.len = len;

  return( ioctl( pev->fd, PEV_IOCTL_SFLASH_WR, &rdwr));
}

int
pev_fpga_sign( uint fpga,
	       void *addr,
	       uint len)
{
  struct pev_ioctl_rdwr rdwr;

  if( (fpga < 0) || (fpga > 3))
  {
    return( -1);
  }
  rdwr.buf = addr;
  rdwr.offset = ( fpga << 21) | 0x1f0000;
  rdwr.len = len;
  if( rdwr.len > 0x10000) rdwr.len = 0x10000; 

  return( ioctl( pev->fd, PEV_IOCTL_SFLASH_RD, &rdwr));
}

void
pev_vme_irq_init()
{
  ioctl( pev->fd, PEV_IOCTL_VME_IRQ_INIT,0);
}

void
pev_vme_irq_mask( uint im)
{
  pev_csr_wr( io_remap.vme_itc + 0xc, im);
}

void
pev_vme_irq_unmask( uint im)
{
  pev_csr_wr( io_remap.vme_itc + 0x8, im);
}

void
pev_vme_irq_enable()
{
  pev_csr_wr( io_remap.vme_itc + 0x4, 7);
}

struct pev_ioctl_vme_irq 
*pev_vme_irq_alloc( uint is)
{
  struct pev_ioctl_vme_irq *irq;
  int retval;

  irq = (struct pev_ioctl_vme_irq *)malloc( sizeof(struct pev_ioctl_vme_irq));
  if( !irq)
  {
    return( NULL);
  }

  irq->irq = is;
  retval = ioctl( pev->fd, PEV_IOCTL_VME_IRQ_ALLOC, irq);
  if( retval < 0)
  {
    return( NULL);
  }
  return( irq);
}

int
pev_vme_irq_free( struct pev_ioctl_vme_irq *irq)
{
  int retval;

  retval = ioctl( pev->fd, PEV_IOCTL_VME_IRQ_CLEAR, irq);
  free( irq);

  return( retval);
}

int
pev_vme_irq_arm( struct pev_ioctl_vme_irq *irq)
{
  int retval;

  retval = ioctl( pev->fd, PEV_IOCTL_VME_IRQ_ARM, irq);
  return( retval);
} 

int
pev_vme_irq_wait( struct pev_ioctl_vme_irq *irq,
		  uint tmo,
		  uint *vector)
{
  int retval;

  irq->tmo = tmo;
  retval = ioctl( pev->fd, PEV_IOCTL_VME_IRQ_WAIT, irq);
  *vector = irq->vector;
  return( retval);
} 

int
pev_vme_irq_clear( struct pev_ioctl_vme_irq *irq)
{
  int retval;

  retval = ioctl( pev->fd, PEV_IOCTL_VME_IRQ_CLEAR, irq);
  return( retval);
} 

int
pev_vme_irq_armwait( struct pev_ioctl_vme_irq *irq, uint tmo, uint *vector)
{
  int retval;
  int op;

  irq->tmo = tmo;
  op = PEV_IOCTL_VME_IRQ_ARM  | PEV_IOCTL_VME_IRQ_WAIT;
  retval = ioctl( pev->fd, op, irq);
  if( retval < 0)
  {
    return( retval);
  }
  *vector = irq->vector;
  return( 0);
} 
 
int
pev_evt_wait()
{
  return( ioctl( pev->fd, PEV_IOCTL_EVT_WAIT, 0));
} 

int
pev_evt_set()
{
  return( ioctl( pev->fd, PEV_IOCTL_EVT_SET, 0));
} 


int
pev_vme_conf_read( struct pev_ioctl_vme_conf *conf)
{
  return( ioctl( pev->fd, PEV_IOCTL_VME_CONF_RD, conf));
}
 
int
pev_vme_conf_write( struct pev_ioctl_vme_conf *conf)
{
  return( ioctl( pev->fd, PEV_IOCTL_VME_CONF_WR, conf));
} 

int
pev_vme_crcsr( struct pev_ioctl_vme_crcsr *crcsr)
{
  return( ioctl( pev->fd, PEV_IOCTL_VME_CRCSR, crcsr));
} 

int
pev_vme_rmw( struct pev_ioctl_vme_rmw *rmw_p)
{
  return( ioctl( pev->fd, PEV_IOCTL_VME_RMW, rmw_p));
}

int
pev_vme_lock( struct pev_ioctl_vme_lock *lock_p)
{
  return( ioctl( pev->fd, PEV_IOCTL_VME_LOCK, lock_p));
}

int
pev_vme_unlock( void)
{
  return( ioctl( pev->fd, PEV_IOCTL_VME_UNLOCK, 0));
}

int
pev_vme_init( void)
{
  return( ioctl( pev->fd, PEV_IOCTL_VME_SLV_INIT, 0));
}

void
pev_vme_sysreset( int usec)
{
  struct pev_ioctl_rw wr;

  wr.offset = 0x10;
  wr.data = 0x1f;

  ioctl( pev->fd, PEV_IOCTL_WR_IO_32, &wr);
  usleep( usec);
  wr.data = 0x0;
  ioctl( pev->fd, PEV_IOCTL_WR_IO_32, &wr);

  return;
}

int
pev_timer_start( uint mode,
		 uint msec)
{
  struct pev_ioctl_timer tmr;

  tmr.mode = mode;
  tmr.time = msec;
  return( ioctl( pev->fd, PEV_IOCTL_TIMER_START, &tmr));
} 

void
pev_timer_restart( void)
{
  ioctl( pev->fd, PEV_IOCTL_TIMER_RESTART, 0);
  return;
}

void
pev_timer_stop( void)
{
  ioctl( pev->fd, PEV_IOCTL_TIMER_STOP, 0);
  return;
}

uint
pev_timer_read( struct pev_time *tm)
{
  struct pev_ioctl_timer tmr;

  ioctl( pev->fd, PEV_IOCTL_TIMER_READ, &tmr);
  if( tm)
  {
    tm->time = tmr.time;
    tm->utime = tmr.utime;
  }
  return( tmr.time); 
} 
