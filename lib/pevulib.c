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
 *  Revision 1.3  2012/04/25 13:18:28  kalantari
 *  added i2c epics driver and updated linux driver to v.4.10
 *
 *  Revision 1.51  2012/04/19 08:40:39  ioxos
 *  tagging rel-4-10 [JFG]
 *
 *  Revision 1.50  2012/04/18 07:51:29  ioxos
 *  release 4.09 [JFG]
 *
 *  Revision 1.49  2012/04/12 13:41:02  ioxos
 *  support for eeprom access [JFG]
 *
 *  Revision 1.48  2012/04/10 08:32:03  ioxos
 *  version 4.08 [JFG]
 *
 *  Revision 1.47  2012/03/27 11:47:47  ioxos
 *  set version to 4.07 [JFG]
 *
 *  Revision 1.46  2012/03/27 09:17:40  ioxos
 *  add support for FIFOs [JFG]
 *
 *  Revision 1.45  2012/03/21 14:43:20  ioxos
 *  set software revision to 4.06 [JFG]
 *
 *  Revision 1.44  2012/03/21 11:23:25  ioxos
 *  support to read CSR from PCI MEM window [JFG]
 *
 *  Revision 1.43  2012/03/15 15:23:34  ioxos
 *  release 4.05 [JFG]
 *
 *  Revision 1.42  2012/02/28 16:08:40  ioxos
 *  set release to 4.04 [JFG]
 *
 *  Revision 1.41  2012/02/14 16:18:44  ioxos
 *  release 4.03 [JFG]
 *
 *  Revision 1.40  2012/02/14 16:09:16  ioxos
 *  remove debug print our [JFG]
 *
 *  Revision 1.39  2012/02/03 16:30:18  ioxos
 *  release 4.02 [JFG]
 *
 *  Revision 1.38  2012/02/03 13:01:00  ioxos
 *  ioxos_boards should be static [JFG]
 *
 *  Revision 1.37  2012/02/03 11:29:19  ioxos
 *  compilation warnings [JFG]
 *
 *  Revision 1.36  2012/02/03 11:02:29  ioxos
 *  use i2c lib for pex access [JFG]
 *
 *  Revision 1.35  2012/01/30 11:17:20  ioxos
 *  add support for board name [JFG]
 *
 *  Revision 1.34  2012/01/26 15:56:51  ioxos
 *  prepare for IFC1210 support [JFG]
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
static const char *rcsid = "$Id: pevulib.c,v 1.3 2012/04/25 13:18:28 kalantari Exp $";
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
char pev_lib_version[] = "4.10";
uint pev_board_id = 0;
static char ioxos_board_name[16];
static struct ioxos_boards
{
  int idx;
  int id;
  const char *name;
}
ioxos_boards[] =
{
  {0, PEV_BOARD_PEV1100, "PEV1100"},
  {1, PEV_BOARD_IPV1102, "IPV1102"},
  {2, PEV_BOARD_VCC1104, "VCC1104"},
  {3, PEV_BOARD_VCC1105, "VCC1105"},
  {4, PEV_BOARD_IFC1210, "IFC1210"},
  {5, PEV_BOARD_MPC1200, "MPC1200"},
  {-1, 0, NULL}
};


struct pev_node
*pev_init( uint crate)
{
  char dev_name[16];
  struct ioxos_boards *ib;
  int i;

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
    ioctl( pev->fd, PEV_IOCTL_BOARD, &pev_board_id);
    ioctl( pev->fd, PEV_IOCTL_IO_REMAP, &io_remap);
    ib = &ioxos_boards[0];
    while( ib->idx != -1)
    {
      if( ib->id == pev_board_id)
      {
	strcpy( ioxos_board_name,ib->name);
	break;
      }
      ib++;
    }
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

uint
pev_board()
{
  return( pev_board_id);
}

char *
pev_board_name()
{
  return( ioxos_board_name);
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
  int mode;

  mode = 0;
  if( idx & 0x80000000) mode = 1;
  rd.offset = idx & 0x7fffffff;;
  rd.data = 0x0;

  if( mode)
  {
    ioctl( pev->fd, PEV_IOCTL_RD_CSR_32, &rd);
    rd.data = pev_swap_32( rd.data);
  }
  else
  {
    ioctl( pev->fd, PEV_IOCTL_RD_IO_32, &rd);
  }


  return( rd.data);
} 

void
pev_csr_wr( int idx, int data)
{
  struct pev_ioctl_rw wr;
  int mode;

  mode = 0;
  if( idx & 0x80000000) mode = 1;
  wr.offset = idx & 0x7fffffff;;

  if( mode)
  {
    wr.data = pev_swap_32( data);
    ioctl( pev->fd, PEV_IOCTL_WR_CSR_32, &wr);
  }
  else
  {
    wr.data = data;
    ioctl( pev->fd, PEV_IOCTL_WR_IO_32, &wr);
  }

  return;
}
 
void
pev_csr_set( int idx, int data)
{
  struct pev_ioctl_rw wr;
  int mode;

  mode = 0;
  if( idx & 0x80000000) mode = 1;
  wr.offset = idx & 0x7fffffff;;

  if( mode)
  {
    wr.data = pev_swap_32( data);
    ioctl( pev->fd, PEV_IOCTL_SET_CSR_32, &wr);
  }
  else
  {
    wr.data = data;
    ioctl( pev->fd, PEV_IOCTL_SET_IO_32, &wr);
  }

  return;
}

int
pev_elb_rd( int reg)
{
  struct pev_ioctl_rdwr rdwr;
  uint data;

  rdwr.buf = &data;
  rdwr.offset = reg;
  rdwr.len = 0;
  rdwr.mode.dir = RDWR_READ;
  rdwr.mode.space = RDWR_ELB;
  rdwr.mode.ds = RDWR_INT;
  rdwr.mode.swap = RDWR_NOSWAP;
  ioctl( pev->fd, PEV_IOCTL_RDWR, &rdwr);
  return( data);
}
 
int
pev_elb_wr( int reg, int data)
{
  struct pev_ioctl_rdwr rdwr;

  rdwr.buf = &data;
  rdwr.offset = reg;
  rdwr.len = 0;
  rdwr.mode.dir = RDWR_WRITE;
  rdwr.mode.space = RDWR_ELB;
  rdwr.mode.ds = RDWR_INT;
  rdwr.mode.swap = RDWR_NOSWAP;
  return( ioctl( pev->fd, PEV_IOCTL_RDWR, &rdwr));
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
pev_i2c_read( uint dev,
	      uint reg)
{
  struct pev_ioctl_i2c i2c;

  i2c.device = dev;
  i2c.cmd = reg;
  i2c.data = 0;
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_RD, &i2c);

  return( i2c.data);
}

int
pev_i2c_cmd( uint dev,
	      uint cmd)
{
  struct pev_ioctl_i2c i2c;

  i2c.device = dev;
  i2c.cmd = cmd;
  i2c.data = 0;
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_CMD, &i2c);

  return( i2c.data);
}

int
pev_i2c_write( uint dev,
	       uint reg,
	       uint data)
{
  struct pev_ioctl_i2c i2c;

  i2c.device = dev;
  i2c.cmd = reg;
  i2c.data = data;
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_WR, &i2c);

  return( 0);
}

int
pev_pex_read( uint reg)
{
  struct pev_ioctl_i2c i2c;

  i2c.device = 0x010f0069;
  i2c.cmd = 0x4003c00 | (( reg >> 2) & 0x3ff);
  i2c.cmd |= (reg << 3) &0x78000;
  i2c.cmd = pev_swap_32( i2c.cmd);
  i2c.data = 0;
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_RD, &i2c);
  /* i2c.data = pev_swap_32( i2c.data); */

  return( i2c.data);
}

int
pev_pex_write( uint reg,
	       uint data)
{
  struct pev_ioctl_i2c i2c;

  i2c.device = 0x010f0069;
  i2c.cmd = 0x3003c00 | (( reg >> 2) & 0x3ff);
  i2c.cmd |= (reg << 3) &0x78000;
  i2c.cmd = pev_swap_32( i2c.cmd);
  i2c.data = data;
  /* i2c.data = pev_swap_32( data); */
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_WR, &i2c);

  return( 0);
}

int
pev_sflash_id( char *id,
	       uint dev)
{
  uint cmd;

  cmd =  PEV_IOCTL_SFLASH_ID | (dev&3);
  return( ioctl( pev->fd, cmd, id));
}

int
pev_sflash_rdsr( uint dev)
{
  int sr;
  uint cmd;

  cmd =  PEV_IOCTL_SFLASH_RDSR | (dev&3);
  ioctl( pev->fd, cmd, &sr);
  return( sr);
}

int
pev_sflash_wrsr( int sr,
		 uint dev)
{
  uint cmd;

  cmd =  PEV_IOCTL_SFLASH_WRSR | (dev&3);
  return( ioctl( pev->fd, cmd, &sr));
}

int
pev_sflash_read( uint offset,
		 void *addr,
		 uint len)
{
  struct pev_ioctl_sflash_rw rdwr;

  rdwr.buf = addr;
  rdwr.offset = offset & 0xfffffff;
  rdwr.len = len;
  rdwr.dev = offset >> 28;

  return( ioctl( pev->fd, PEV_IOCTL_SFLASH_RD, &rdwr));
}

int
pev_sflash_write( uint offset,
		  void *addr,
		  uint len)
{
  struct pev_ioctl_sflash_rw rdwr;

  rdwr.buf = addr;
  rdwr.offset = offset & 0xfffffff;
  rdwr.len = len;
  rdwr.dev = offset >> 28;

  return( ioctl( pev->fd, PEV_IOCTL_SFLASH_WR, &rdwr));
}

int
pev_fpga_load( uint fpga,
	       void *addr,
	       uint len)
{
  struct pev_ioctl_sflash_rw rdwr;

  rdwr.buf = addr;
  rdwr.offset = 0;
  rdwr.len = len;
  rdwr.dev = fpga;
  return( ioctl( pev->fd, PEV_IOCTL_FPGA_LOAD, &rdwr));
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

int
pev_fifo_init( void)
{
  int retval;
  
  retval = ioctl( pev->fd, PEV_IOCTL_FIFO_INIT, NULL);
  return( retval);
}

int
pev_fifo_status( uint idx,
		 uint *sts)
{
  struct pev_ioctl_fifo fifo;
  int retval;
  
  fifo.idx = idx;  
  fifo.sts = 0;  
  retval = ioctl( pev->fd, PEV_IOCTL_FIFO_STATUS, &fifo);
  if( sts)
  {
    *sts = fifo.sts;
  }
  if( ! retval)
  {
    retval = fifo.sts;
  }
  return( retval);
}

int
pev_fifo_clear( uint idx,
		uint *sts)
{
  struct pev_ioctl_fifo fifo;
  int retval;
  
  fifo.idx = idx;  
  fifo.sts = 0;  
  retval = ioctl( pev->fd, PEV_IOCTL_FIFO_CLEAR, &fifo);
  if( sts)
  {
    *sts = fifo.sts;
  }
  return( retval);
}

int
pev_fifo_wait_ef( uint idx,
		  uint *sts,
		  uint tmo)
{
  struct pev_ioctl_fifo fifo;
  int retval;
  
  fifo.idx = idx;  
  fifo.tmo = tmo;  
  fifo.sts = 0;  
  retval = ioctl( pev->fd, PEV_IOCTL_FIFO_WAIT_EF, &fifo);
  if( sts)
  {
    *sts = fifo.sts;
  }
  return( retval);
}

int
pev_fifo_wait_ff( uint idx,
		  uint *sts,
		  uint tmo)
{
  struct pev_ioctl_fifo fifo;
  int retval;
  
  fifo.idx = idx;  
  fifo.tmo = tmo;  
  fifo.sts = 0;  
  retval = ioctl( pev->fd, PEV_IOCTL_FIFO_WAIT_FF, &fifo);
  if( sts)
  {
    *sts = fifo.sts;
  }
  return( retval);
}

int
pev_fifo_read( uint idx,
	       uint *data,
	       uint wcnt,
	       uint *sts)
{
  struct pev_ioctl_fifo fifo;
  int retval;
  
  fifo.idx = idx;  
  fifo.data = data;  
  fifo.cnt = wcnt;  
  fifo.sts = 0;  
  retval = ioctl( pev->fd, PEV_IOCTL_FIFO_RD, &fifo);
  if( sts)
  {
    *sts = fifo.sts;
  }
  return( retval);
}

int
pev_fifo_write( uint idx,
	        uint *data,
	        uint wcnt,
	        uint *sts)
{
  struct pev_ioctl_fifo fifo;
  int retval;
  
  fifo.idx = idx;  
  fifo.data = data;  
  fifo.cnt = wcnt;  
  fifo.sts = 0;  
  retval = ioctl( pev->fd, PEV_IOCTL_FIFO_WR, &fifo);
  if( sts)
  {
    *sts = fifo.sts;
  }
  return( retval);
}

int
pev_eeprom_rd( uint offset,
	       char *data,
	       uint cnt)
{
  struct pev_ioctl_rdwr rdwr;

  rdwr.buf = data;
  rdwr.offset = offset;
  rdwr.len = cnt;
  return( ioctl( pev->fd, PEV_IOCTL_EEPROM_RD, &rdwr));
}

int
pev_eeprom_wr( uint offset,
	       char *data,
	       uint cnt)
{
  struct pev_ioctl_rdwr rdwr;

  rdwr.buf = data;
  rdwr.offset = offset;
  rdwr.len = cnt;
  return( ioctl( pev->fd, PEV_IOCTL_EEPROM_WR, &rdwr));
}
