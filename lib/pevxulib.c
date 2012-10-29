/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pevxulib.c
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
 *  $Log: pevxulib.c,v $
 *  Revision 1.11  2012/10/29 10:06:56  kalantari
 *  added the tosca driver version 4.22 from IoxoS
 *
 *  Revision 1.54  2012/10/25 12:31:46  ioxos
 *  add evt_clear() + version 4.22 [JFG]
 *
 *  Revision 1.53  2012/10/12 14:25:06  ioxos
 *  tagging release 4.21 [JFG]
 *
 *  Revision 1.52  2012/09/27 11:49:36  ioxos
 *  tagging 4.20 [JFG]
 *
 *  Revision 1.51  2012/09/04 13:33:15  ioxos
 *  release 4.19 [JFG]
 *
 *  Revision 1.50  2012/09/04 13:18:45  ioxos
 *  new function to map system memory statically allocated [JFG]
 *
 *  Revision 1.49  2012/09/03 13:54:32  ioxos
 *  tagging release 4.18 [JFG]
 *
 *  Revision 1.48  2012/09/03 13:10:15  ioxos
 *  pointer to data as arg of read function and return i2c cycle status [JFG]
 *
 *  Revision 1.47  2012/08/28 13:59:45  ioxos
 *  release 4.17 [JFG]
 *
 *  Revision 1.46  2012/08/28 13:52:54  ioxos
 *  cleanup i2c + reset [JFG]
 *
 *  Revision 1.45  2012/08/27 08:47:23  ioxos
 *  support for VME fast single cycles through ELB bus [JFG]
 *
 *  Revision 1.44  2012/08/07 09:21:04  ioxos
 *  support for BMR DC-DC converter [JFG]
 *
 *  Revision 1.43  2012/07/10 09:47:12  ioxos
 *  rel 4.15 [JFG]
 *
 *  Revision 1.42  2012/06/28 14:01:11  ioxos
 *  set release 4.14 [JFG]
 *
 *  Revision 1.41  2012/06/06 15:26:25  ioxos
 *  release 4.13 [JFG]
 *
 *  Revision 1.40  2012/06/06 12:17:56  ioxos
 *  use pevx_node [JFG]
 *
 *  Revision 1.39  2012/06/01 13:59:22  ioxos
 *  -Wall cleanup [JFG]
 *
 *  Revision 1.38  2012/05/23 08:14:39  ioxos
 *  add support for event queues [JFG]
 *
 *  Revision 1.37  2012/04/19 08:40:39  ioxos
 *  tagging rel-4-10 [JFG]
 *
 *  Revision 1.36  2012/04/18 07:51:29  ioxos
 *  release 4.09 [JFG]
 *
 *  Revision 1.35  2012/04/12 13:41:02  ioxos
 *  support for eeprom access [JFG]
 *
 *  Revision 1.34  2012/04/10 08:32:03  ioxos
 *  version 4.08 [JFG]
 *
 *  Revision 1.33  2012/03/27 11:58:30  ioxos
 *  mistyping [JFG]
 *
 *  Revision 1.32  2012/03/27 11:47:47  ioxos
 *  set version to 4.07 [JFG]
 *
 *  Revision 1.31  2012/03/27 09:17:40  ioxos
 *  add support for FIFOs [JFG]
 *
 *  Revision 1.30  2012/03/21 14:43:20  ioxos
 *  set software revision to 4.06 [JFG]
 *
 *  Revision 1.29  2012/03/21 11:23:25  ioxos
 *  support to read CSR from PCI MEM window [JFG]
 *
 *  Revision 1.28  2012/03/15 15:21:53  ioxos
 *  add pevx_board_name() + release 4.05 [JFG]
 *
 *  Revision 1.27  2012/02/28 16:08:40  ioxos
 *  set release to 4.04 [JFG]
 *
 *  Revision 1.26  2012/02/14 16:18:44  ioxos
 *  release 4.03 [JFG]
 *
 *  Revision 1.25  2012/02/14 16:09:16  ioxos
 *  remove debug print our [JFG]
 *
 *  Revision 1.24  2012/02/03 16:30:18  ioxos
 *  release 4.02 [JFG]
 *
 *  Revision 1.23  2012/02/03 13:01:00  ioxos
 *  ioxos_boards should be static [JFG]
 *
 *  Revision 1.22  2012/02/03 11:28:44  ioxos
 *  support for board_name [JFG]
 *
 *  Revision 1.21  2012/02/03 11:02:29  ioxos
 *  use i2c lib for pex access [JFG]
 *
 *  Revision 1.20  2012/01/26 15:56:51  ioxos
 *  prepare for IFC1210 support [JFG]
 *
 *  Revision 1.19  2012/01/06 14:39:59  ioxos
 *  release 3.13 [JFG]
 *
 *  Revision 1.18  2012/01/06 13:24:35  ioxos
 *  support for X86 32 bit [JFG]
 *
 *  Revision 1.17  2011/12/06 14:40:42  ioxos
 *
 *  pev_ should be pevx_ [JFG]
 *
 *  Revision 1.15  2011/10/19 14:11:33  ioxos
 *  release 3.11 [JFG]
 *
 *  Revision 1.14  2011/10/19 14:02:22  ioxos
 *   32bit mmap + vme irq handling [JFG]
 *
 *  Revision 1.13  2011/10/03 09:57:16  ioxos
 *  release 1.10 [JFG]
 *
 *  Revision 1.12  2011/03/03 15:42:22  ioxos
 *  support for 1MBytes VME slave granularity [JFG]
 *
 *  Revision 1.11  2011/01/25 13:42:06  ioxos
 *  support for VME RMW [JFG]
 *
 *  Revision 1.10  2010/08/26 15:08:11  ioxos
 *  rcsid static const [JFG]
 *
 *  Revision 1.9  2010/08/26 14:25:33  ioxos
 *  cleanup void pointers [JFG]
 *
 *  Revision 1.8  2010/06/11 11:37:10  ioxos
 *  use pev mmap() for kernel memory mapping [JFG]
 *
 *  Revision 1.7  2010/05/18 07:32:28  ioxos
 *  add VME SYSRESET [JFG]
 *
 *  Revision 1.6  2010/01/12 15:43:11  ioxos
 *  add function to read vme list [JFG]
 *
 *  Revision 1.5  2010/01/08 15:39:01  ioxos
 *  add fuction to transfer dma list [JFG]
 *
 *  Revision 1.4  2009/12/15 17:15:28  ioxos
 *  modification for short io window [JFG]
 *
 *  Revision 1.3  2009/12/08 13:55:33  ioxos
 *   add vme map function [JFG]
 *
 *  Revision 1.2  2009/11/10 09:09:42  ioxos
 *  remove swap functions [JFG]
 *
 *  Revision 1.1  2009/10/23 09:08:59  ioxos
 *  first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char rcsid[] = "$Id: pevxulib.c,v 1.11 2012/10/29 10:06:56 kalantari Exp $";
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
#include <pevxulib.h>

static struct pevx_node *pev = (struct pevx_node *)NULL;
static struct pevx_node *pevx[16]={ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static char pevx_drv_id[16] = {0,};
static struct pev_reg_remap io_remap;
char pevx_driver_version[16];
char pevx_lib_version[] = "4.22";
int pevx_board_id = 0;
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

char *
pevx_rcsid()
{
  return( rcsid);
}

int
pevx_swap_32( int data)
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

struct pevx_node
*pevx_init( uint crate)
{
  struct ioxos_boards *ib;
  char dev_name[16];
  if( (crate < 0) || (crate > 15))
  {
    return( (struct pevx_node *)0);
  }
  if( !pevx[crate])
  {
    pev = (struct pevx_node *)malloc( sizeof( struct pevx_node));
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
    ioctl( pev->fd, PEV_IOCTL_ID, pevx_drv_id);
    ioctl( pev->fd, PEV_IOCTL_BOARD, &pevx_board_id);
    ioctl( pev->fd, PEV_IOCTL_IO_REMAP, &io_remap);
    ib = &ioxos_boards[0];
    while( ib->idx != -1)
    {
      if( ib->id == pevx_board_id)
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
pevx_get_driver_version()
{
  ioctl( pev->fd, PEV_IOCTL_VERSION, pevx_driver_version);
  return( pevx_driver_version);
}

char *
pevx_get_lib_version()
{
  return( pevx_lib_version);
}

char
*pevx_id()
{
  return( pevx_drv_id);
}

uint
pevx_board()
{
  return( pevx_board_id);
}

char *
pevx_board_name()
{
  return( ioxos_board_name);
}

struct pev_reg_remap
*pevx_io_remap()
{
  return( &io_remap);
}

struct pevx_node
*pevx_set_crate( uint crate)
{
  if( pevx[crate])
  {
    pev = pevx[crate];
    return( pev);
  }
  return( (struct pevx_node *)0);
}

int
pevx_get_crate()
{
  if( pev)
  {
    return( pev->crate);
  }
  return( -1);
}

int
pevx_exit( uint crate)
{
  int ret;

  ret = -1;
  if( pevx[crate])
  {
    pev = pevx[crate];
    if( pev->fd > 0)
    {
      ret = close( pev->fd);
      pev->fd = -1;
      ret = 0;
    }
  }
  return(ret);
}


int
pevx_rdwr( uint crate, struct pev_ioctl_rdwr *rdwr_p)
{
  if( pevx[crate])
  {
    pev = pevx[crate];
    if( pev->fd > 0)
    {
      return( ioctl( pev->fd, PEV_IOCTL_RDWR, rdwr_p));
    }
  }
  return(-1);
}
 
int
pevx_smon_rd( uint crate, int idx)
{
  struct pev_ioctl_rw_reg rd;

  if( pevx[crate])
  {
    pev = pevx[crate];
    if( pev->fd > 0)
    {
      rd.addr_off = 0x40;
      rd.data_off = 0x44;
      rd.reg_idx = idx;
      rd.reg_data = 0x0;

      ioctl( pev->fd, PEV_IOCTL_RD_REG_32, &rd);

      return( rd.reg_data);
    }
  }
  return(-1);
} 

int
pevx_smon_wr( uint crate, int idx, int data)
{
  struct pev_ioctl_rw_reg wr;

  if( pevx[crate])
  {
    pev = pevx[crate];
    if( pev->fd > 0)
    {
      wr.addr_off = 0x40;
      wr.data_off = 0x44;
      wr.reg_idx = idx;
      wr.reg_data = data;

      return( ioctl( pev->fd, PEV_IOCTL_WR_REG_32, &wr));
    }
  }
  return(-1);
}
 
int
pevx_csr_rd( uint crate, int idx)
{
  struct pev_ioctl_rw rd;
  int mode;

  if( pevx[crate])
  {
    pev = pevx[crate];
    if( pev->fd > 0)
    {
      mode = 0;
      if( idx & 0x80000000) mode = 1;
      rd.offset = idx & 0x7fffffff;
      rd.data = 0x0;

      if( mode)
      {
        ioctl( pev->fd, PEV_IOCTL_RD_CSR_32, &rd);
	rd.data = pevx_swap_32( rd.data);
      }
      else
      {
        ioctl( pev->fd, PEV_IOCTL_RD_IO_32, &rd);
      }
      return( rd.data);
    }
  }
  return(-1);
} 

int
pevx_csr_wr( uint crate, int idx, int data)
{
  struct pev_ioctl_rw wr;
  int mode;

  if( pevx[crate])
  {
    pev = pevx[crate];
    if( pev->fd > 0)
    {
      mode = 0;
      if( idx & 0x80000000) mode = 1;
      wr.offset = idx & 0x7fffffff;;

      if( mode)
      {
	wr.data = pevx_swap_32( data);
	return( ioctl( pev->fd, PEV_IOCTL_WR_CSR_32, &wr));
      }
      else
      {
	wr.data = data;
	return( ioctl( pev->fd, PEV_IOCTL_WR_IO_32, &wr));
      }
    }
  }
  return(-1);
}
 
int
pevx_csr_set( uint crate, int idx, int data)
{
  struct pev_ioctl_rw wr;
  int mode;

  if( pevx[crate])
  {
    pev = pevx[crate];
    if( pev->fd > 0)
    {
      mode = 0;
      if( idx & 0x80000000) mode = 1;
      wr.offset = idx & 0x7fffffff;;

      if( mode)
      {
	wr.data = pevx_swap_32( data);
        return( ioctl( pev->fd, PEV_IOCTL_SET_CSR_32, &wr));
      }
      else
      {
	wr.data = data;
        return( ioctl( pev->fd, PEV_IOCTL_SET_IO_32, &wr));
      }
    }
  }
  return(-1);
}
 
int
pevx_elb_rd( uint crate, int reg)
{
  struct pev_ioctl_rdwr rdwr;
  int data;

  if( !pevx[crate]) return(0);
  pev = pevx[crate];
  if( pev->fd < 0) return(0);

  rdwr.buf = (char *)&data;
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
pevx_elb_wr( uint crate, int reg, int data)
{
  struct pev_ioctl_rdwr rdwr;

  if( !pevx[crate]) return(0);
  pev = pevx[crate];
  if( pev->fd < 0) return(0);

  rdwr.buf = (char *)&data;
  rdwr.offset = reg;
  rdwr.len = 0;
  rdwr.mode.dir = RDWR_WRITE;
  rdwr.mode.space = RDWR_ELB;
  rdwr.mode.ds = RDWR_INT;
  rdwr.mode.swap = RDWR_NOSWAP;
  return( ioctl( pev->fd, PEV_IOCTL_RDWR, &rdwr));
}
 
void *
pevx_mmap( uint crate, struct pev_ioctl_map_pg *map)
{
  if( !pevx[crate]) return(NULL);
  pev = pevx[crate];
  if( pev->fd < 0) return(NULL);
  map->usr_addr = MAP_FAILED;
  if( map->sg_id == MAP_PCIE_MEM)
  {
#if defined(PPC) || defined(X86_32)
    map->usr_addr = mmap( NULL, map->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, map->loc_addr | 0x80000000);
#else
    map->usr_addr = mmap( NULL, map->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, map->loc_addr | 0x200000000 );
#endif
  }
  if( map->sg_id == MAP_PCIE_PMEM)
  {
#if defined(PPC) || defined(X86_32)
    map->usr_addr = mmap( NULL, map->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, map->loc_addr | 0xc0000000);
#else
    map->usr_addr = mmap( NULL, map->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, map->loc_addr | 0x100000000);
#endif
  }
  if( map->sg_id == MAP_VME_ELB)
  {
#if defined(PPC)
    map->usr_addr = mmap( NULL, map->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, map->loc_addr | 0xa0000000);
#endif
  }

  return( map->usr_addr);
}
 
int
pevx_munmap( uint crate, struct pev_ioctl_map_pg *map)
{
  return( munmap( map->usr_addr, map->size));
}
 
void *
pevx_vmap( uint crate, struct pev_ioctl_map_pg *map)
{
  struct pev_ioctl_vme_conf conf;

  if( !pevx[crate]) return(NULL);
  pev = pevx[crate];
  if( pev->fd < 0) return(NULL);

  map->usr_addr = (void *)map->loc_addr;
  ioctl( pev->fd, PEV_IOCTL_VME_CONF_RD, &conf);
  map->usr_addr = (void *)((char *)map->usr_addr + conf.a32_base);

  return( map->usr_addr);
}
 
int
pevx_map_alloc( uint crate, struct pev_ioctl_map_pg *map)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_MAP_ALLOC, map));
}
 
int
pevx_map_free( uint crate, struct pev_ioctl_map_pg *map)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_MAP_FREE, map));
}
 
int
pevx_map_modify( uint crate, struct pev_ioctl_map_pg *map)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_MAP_MODIFY, map));
}

int
pevx_map_find( uint crate, struct pev_ioctl_map_pg *map)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_MAP_FIND, map));
}

int
pevx_map_read( uint crate, struct pev_ioctl_map_ctl *ctl)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_MAP_READ, ctl));
}

int
pevx_map_clear( uint crate, struct pev_ioctl_map_ctl *ctl)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_MAP_CLEAR, ctl));
}

void *
pevx_buf_alloc( uint crate, struct pev_ioctl_buf *db_p)
{
  db_p->kmem_fd = -1;
  if( !pevx[crate]) return(NULL);
  pev = pevx[crate];
  if( pev->fd < 0) return(NULL);
  ioctl( pev->fd, PEV_IOCTL_BUF_ALLOC, db_p);
  db_p->u_addr = NULL;
  if( db_p->k_addr)
  {
    db_p->kmem_fd = pev->fd;
    db_p->u_addr = mmap( NULL, db_p->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, (off_t)db_p->b_addr);
  }
  return( db_p->u_addr);
}
 
int
pevx_buf_free( uint crate, struct pev_ioctl_buf *db_p)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  if( db_p->u_addr)
  {
    munmap( db_p->u_addr, db_p->size);
    db_p->u_addr = NULL;
  }
  return( ioctl( pev->fd, PEV_IOCTL_BUF_FREE, db_p));
}

void *
pevx_buf_map( uint crate, struct pev_ioctl_buf *db_p)
{
  db_p->kmem_fd = -1;
  if( !pevx[crate]) return(NULL);
  pev = pevx[crate];
  if( pev->fd < 0) return(NULL);
  ioctl( pev->fd, PEV_IOCTL_BUF_MAP, db_p);
  db_p->u_addr = NULL;
  if( db_p->k_addr)
  {
    db_p->kmem_fd = pev->fd;
    db_p->u_addr = mmap( NULL, db_p->size, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, (off_t)db_p->b_addr);
  }
  return( db_p->u_addr);
}
 
int
pevx_buf_unmap( uint crate, struct pev_ioctl_buf *db_p)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  if( db_p->u_addr)
  {
    munmap( db_p->u_addr, db_p->size);
    db_p->u_addr = NULL;
  }
  return( ioctl( pev->fd, PEV_IOCTL_BUF_UNMAP, db_p));
}

int
pevx_dma_move( uint crate, struct pev_ioctl_dma_req *dr_p)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_DMA_MOVE, dr_p));
}

int
pevx_dma_vme_list_rd( uint crate, void *uaddr, 
		      struct pev_ioctl_dma_list *list_p, 
		      int list_size)
{
  struct pev_ioctl_dma_req req;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

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
pevx_dma_status( uint crate, struct pev_ioctl_dma_sts *ds_p)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_DMA_STATUS, ds_p));
}

int
pevx_i2c_read( uint crate,
	       uint dev,
	       uint reg,
	       uint *data)
{
  struct pev_ioctl_i2c i2c;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  i2c.device = dev;
  i2c.cmd = reg;
  i2c.data = 0;
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_RD, &i2c);
  *data = i2c.data;
  return( i2c.status);
}

int
pevx_i2c_cmd( uint crate,
	      uint dev,
	      uint cmd)
{
  struct pev_ioctl_i2c i2c;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  i2c.device = dev;
  i2c.cmd = cmd;
  i2c.data = 0;
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_CMD, &i2c);

  return( i2c.status);
}

int
pevx_i2c_write( uint crate,
	        uint dev,
	        uint reg,
	        uint data)
{
  struct pev_ioctl_i2c i2c;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  i2c.device = dev;
  i2c.cmd = reg;
  i2c.data = data;
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_WR, &i2c);

  return( i2c.status);
}

int
pevx_i2c_reset( uint crate, uint dev)
{
  struct pev_ioctl_i2c i2c;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  i2c.device = dev;
  i2c.cmd = 0;
  i2c.data = 0;
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_RST, &i2c);

  return( i2c.status);
}

int
pevx_pex_read( uint crate,
	       uint reg,
	       uint *data)
{
  struct pev_ioctl_i2c i2c;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  i2c.device = 0x010f0069;
  i2c.cmd = 0x4003c00 | (( reg >> 2) & 0x3ff);
  i2c.cmd |= (reg << 3) &0x78000;
  i2c.cmd = pevx_swap_32( i2c.cmd);
  i2c.data = 0;
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_RD, &i2c);
  //i2c.data = pevx_swap_32( i2c.data);
  *data = i2c.data;
  return( i2c.status);
}

int
pevx_pex_write( uint crate, uint reg,
	       uint data)
{
  struct pev_ioctl_i2c i2c;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  i2c.device = 0x010f0069;
  i2c.cmd = 0x3003c00 | (( reg >> 2) & 0x3ff);
  i2c.cmd |= (reg << 3) &0x78000;
  i2c.cmd = pevx_swap_32( i2c.cmd);
  i2c.data = data;
  //i2c.data = pevx_swap_32( data);
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_WR, &i2c);

  return( i2c.status);
}

int
pevx_bmr_read( uint crate, 
               uint bmr,
	       uint reg,
	       uint *data,
	       uint cnt)
{
  struct pev_ioctl_i2c i2c;
  int device;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  device = 0;
  if( cnt > 3) return( -1);
  //device =  0x40000080;
  device =  0x41000000;
  switch( bmr)
  {
    case 0:
    {
      device |= 0x53;
      break;
    }
    case 1:
    {
      device |= 0x5b;
      break;
    }
    case 2:
    {
      device |= 0x63;
      break;
    }
    case 3:
    {
      device |= 0x24;
      break;
    }
    default:
    {
      return(-1);
    }
  }
  i2c.cmd = reg;
  i2c.device = device | ((cnt -1) << 18) | 0x8000;
  i2c.data = 0;
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_RD, &i2c);
  *data = i2c.data;
  return( i2c.status);
}

int
pevx_bmr_write( uint crate, 
                uint bmr,
	        uint reg,
	        uint data,
	        uint cnt)
{
  struct pev_ioctl_i2c i2c;
  int device;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  device = 0;
  if( cnt > 3) return( -1);
  if( cnt > 3) return( -1);
  //device =  0x40000080;
  device =  0x41000000;
  switch( bmr)
  {
    case 0:
    {
      device |= 0x53;
      break;
    }
    case 1:
    {
      device |= 0x5b;
      break;
    }
    case 2:
    {
      device |= 0x63;
      break;
    }
    case 3:
    {
      device |= 0x24;
      break;
    }
    default:
    {
      return(-1);
    }
  }
  i2c.device = device | 0x8000;
  i2c.cmd = reg;
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_CMD, &i2c);
  i2c.device = device | ((cnt -1) << 18);
  i2c.data = data;
  ioctl( pev->fd, PEV_IOCTL_I2C_DEV_WR, &i2c);

  return( i2c.status);
}

int
pevx_sflash_id( uint crate,
                char *id,
	        uint dev)
{
  uint cmd;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  cmd =  PEV_IOCTL_SFLASH_ID | (dev&3);
  return( ioctl( pev->fd, cmd, id));
}

int
pevx_sflash_rdsr( uint crate,
                  uint dev)
{
  int sr;
  uint cmd;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  cmd =  PEV_IOCTL_SFLASH_RDSR | (dev&3);
  ioctl( pev->fd, cmd, &sr);
  return( sr);
}

int
pevx_sflash_wrsr( uint crate,
                  int sr,
	 	  uint dev)
{
  uint cmd;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  cmd =  PEV_IOCTL_SFLASH_WRSR | (dev&3);
  return( ioctl( pev->fd, cmd, &sr));
}

int
pevx_sflash_read( uint crate,
                  uint offset,
	 	  void *addr,
		  uint len)
{
  struct pev_ioctl_sflash_rw rdwr;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  rdwr.buf = addr;
  rdwr.offset = offset & 0xfffffff;
  rdwr.len = len;
  rdwr.dev = offset >> 28;

  return( ioctl( pev->fd, PEV_IOCTL_SFLASH_RD, &rdwr));
}

int
pevx_sflash_write( uint crate,
                   uint offset,
		   void *addr,
		   uint len)
{
  struct pev_ioctl_sflash_rw rdwr;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  rdwr.buf = addr;
  rdwr.offset = offset & 0xfffffff;
  rdwr.len = len;
  rdwr.dev = offset >> 28;

  return( ioctl( pev->fd, PEV_IOCTL_SFLASH_WR, &rdwr));
}

int
pevx_fpga_load( uint crate,
                uint fpga,
	        void *addr,
	        uint len)
{
  struct pev_ioctl_sflash_rw rdwr;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  rdwr.buf = addr;
  rdwr.offset = 0;
  rdwr.len = len;
  rdwr.dev = fpga;
  return( ioctl( pev->fd, PEV_IOCTL_FPGA_LOAD, &rdwr));
}

int
pevx_fpga_sign(  uint crate,
		 uint fpga,
	         void *addr,
	         uint len)
{
  struct pev_ioctl_rdwr rdwr;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
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

int
pevx_vme_irq_init( uint crate)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  return( ioctl( pev->fd, PEV_IOCTL_VME_IRQ_INIT,0));
}

int
pevx_vme_irq_mask( uint crate, uint im)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  return( pevx_csr_wr( crate, io_remap.vme_itc + 0xc, im));
}

int
pevx_vme_irq_unmask( uint crate, uint im)
{
  return( pevx_csr_wr( crate, io_remap.vme_itc + 0x8, im));
}

int
pevx_vme_irq_enable( uint crate)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  return( pevx_csr_wr( crate, io_remap.vme_itc + 0x4, 7));
}

#ifdef JFG
int
pevx_vme_irq_arm( uint crate, uint im)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_VME_IRQ_ARM, &im));
} 

int
pevx_vme_irq_wait( uint crate, uint tmo, uint *vector)
{
  struct pev_ioctl_vme_irq irq;
  int retval;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  irq.tmo = tmo;
  retval = ioctl( pev->fd, PEV_IOCTL_VME_IRQ_WAIT, &irq);
  if( retval < 0)
  {
    return( retval);
  }
  *vector = irq.vector;
  return( irq.status);
} 

int
pevx_vme_irq_armwait( uint crate, uint tmo, uint *vector)
{
  struct pev_ioctl_vme_irq irq;
  int retval;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);

  irq.tmo = tmo;
  retval = ioctl( pev->fd, PEV_IOCTL_VME_IRQ_ARM, &irq.irq);
  if( retval < 0)
  {
    return( retval);
  }
  retval = ioctl( pev->fd, PEV_IOCTL_VME_IRQ_WAIT, &irq);
  if( retval < 0)
  {
    return( retval);
  }
  *vector = irq.vector;
  return( irq.status);
} 
#endif
struct pev_ioctl_vme_irq 
*pevx_vme_irq_alloc( uint crate, uint is)
{
  struct pev_ioctl_vme_irq *irq;
  int retval;

  if( !pevx[crate]) return(NULL);
  pev = pevx[crate];
  if( pev->fd < 0) return(NULL);
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
pevx_vme_irq_free( uint crate, struct pev_ioctl_vme_irq *irq)
{
  int retval;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  retval = ioctl( pev->fd, PEV_IOCTL_VME_IRQ_CLEAR, irq);
  free( irq);

  return( retval);
}

int
pevx_vme_irq_arm( uint crate, struct pev_ioctl_vme_irq *irq)
{
  int retval;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  retval = ioctl( pev->fd, PEV_IOCTL_VME_IRQ_ARM, irq);
  return( retval);
} 

int
pevx_vme_irq_wait( uint crate, 
                  struct pev_ioctl_vme_irq *irq,
		  uint tmo,
		  uint *vector)
{
  int retval;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  irq->tmo = tmo;
  retval = ioctl( pev->fd, PEV_IOCTL_VME_IRQ_WAIT, irq);
  *vector = irq->vector;
  return( retval);
} 

int
pevx_vme_irq_clear( uint crate, struct pev_ioctl_vme_irq *irq)
{
  int retval;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  retval = ioctl( pev->fd, PEV_IOCTL_VME_IRQ_CLEAR, irq);
  return( retval);
} 

int
pevx_vme_irq_armwait( uint crate, 
                     struct pev_ioctl_vme_irq *irq, 
                     uint tmo, 
                     uint *vector)
{
  int retval;
  int op;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
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
pevx_vme_conf_read( uint crate, struct pev_ioctl_vme_conf *conf)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_VME_CONF_RD, conf));
}
 
int
pevx_vme_conf_write( uint crate, struct pev_ioctl_vme_conf *conf)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_VME_CONF_WR, conf));
} 

int
pevx_vme_crcsr( uint crate, struct pev_ioctl_vme_crcsr *crcsr)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_VME_CRCSR, crcsr));
} 

int
pevx_vme_rmw( uint crate, struct pev_ioctl_vme_rmw *rmw_p)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_VME_RMW, rmw_p));
}

int
pevx_vme_lock( uint crate, struct pev_ioctl_vme_lock *lock_p)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_VME_LOCK, lock_p));
}

int
pevx_vme_unlock( uint crate)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_VME_UNLOCK, 0));
}

int
pevx_vme_init( uint crate)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_VME_SLV_INIT, 0));
}

int
pevx_vme_sysreset( uint crate, uint usec)
{
  struct pev_ioctl_rw wr;

  if( pevx[crate])
  {
    pev = pevx[crate];
    if( pev->fd > 0)
    {
      wr.offset = 0x10;
      wr.data = 0x1f;

      ioctl( pev->fd, PEV_IOCTL_WR_IO_32, &wr);
      usleep( usec);
      wr.data = 0x0;
      ioctl( pev->fd, PEV_IOCTL_WR_IO_32, &wr);
      return( 0);
    }
  }
  return(-1);
} 

int
pevx_timer_start( uint crate, uint mode,
		 uint msec)
{
  struct pev_ioctl_timer tmr;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  tmr.mode = mode;
  tmr.time = msec;
  return( ioctl( pev->fd, PEV_IOCTL_TIMER_START, &tmr));
} 

int
pevx_timer_restart(  uint crate)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_TIMER_RESTART, 0));
}

int
pevx_timer_stop(  uint crate)
{
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_TIMER_STOP, 0));
}

uint
pevx_timer_read( uint crate,
                 struct pevx_time *tm)
{
  struct pev_ioctl_timer tmr;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  ioctl( pev->fd, PEV_IOCTL_TIMER_READ, &tmr);
  if( tm)
  {
    tm->time = tmr.time;
    tm->utime = tmr.utime;
  }
  return( tmr.time); 
} 

int
pevx_fifo_init( uint crate)
{
  int retval;
  
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  retval = ioctl( pev->fd, PEV_IOCTL_FIFO_INIT, NULL);
  return( retval);
}

int
pevx_fifo_status( uint crate,
	          uint idx,
	 	 uint *sts)
{
  struct pev_ioctl_fifo fifo;
  int retval;
  
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
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
pevx_fifo_clear( uint crate,
	         uint idx,
		 uint *sts)
{
  struct pev_ioctl_fifo fifo;
  int retval;
  
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
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
pevx_fifo_wait_ef( uint crate,
	           uint idx,
		   uint *sts,
		   uint tmo)
{
  struct pev_ioctl_fifo fifo;
  int retval;
  
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
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
pevx_fifo_wait_ff( uint crate,
	           uint idx,
		   uint *sts,
		   uint tmo)
{
  struct pev_ioctl_fifo fifo;
  int retval;
  
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
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
pevx_fifo_read( uint crate,
	        uint idx,
	        uint *data,
	        uint wcnt,
	        uint *sts)
{
  struct pev_ioctl_fifo fifo;
  int retval;
  
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
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
pevx_fifo_write( uint crate,
	         uint idx,
	         uint *data,
	         uint wcnt,
	         uint *sts)
{
  struct pev_ioctl_fifo fifo;
  int retval;
  
  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
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
pevx_eeprom_rd( uint crate,
	       uint offset,
	       char *data,
	       uint cnt)
{
  struct pev_ioctl_rdwr rdwr;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  rdwr.buf = data;
  rdwr.offset = offset;
  rdwr.len = cnt;
  return( ioctl( pev->fd, PEV_IOCTL_EEPROM_RD, &rdwr));
}

int
pevx_eeprom_wr( uint crate,
	       uint offset,
	       char *data,
	       uint cnt)
{
  struct pev_ioctl_rdwr rdwr;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  rdwr.buf = data;
  rdwr.offset = offset;
  rdwr.len = cnt;
  return( ioctl( pev->fd, PEV_IOCTL_EEPROM_WR, &rdwr));
}


struct pev_ioctl_evt *
pevx_evt_queue_alloc( uint crate,
	              int sig)
{
  struct pev_ioctl_evt *evt;

  if( !pevx[crate]) return(NULL);
  pev = pevx[crate];
  if( pev->fd < 0) return(NULL);
  evt = (struct pev_ioctl_evt *)malloc( sizeof(struct pev_ioctl_evt));
  evt->sig = sig;
  ioctl( pev->fd, PEV_IOCTL_EVT_ALLOC, evt);
  return( evt);
}
 
int
pevx_evt_queue_free( uint crate,
	             struct pev_ioctl_evt *evt)
{
  int retval;

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  retval = ioctl( pev->fd, PEV_IOCTL_EVT_FREE, evt);
  free( evt);
  return( retval);
}

int
pevx_evt_register( uint crate,
	           struct pev_ioctl_evt *evt,
	           int src_id)
{

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  evt->src_id = src_id;
  return( ioctl( pev->fd, PEV_IOCTL_EVT_REGISTER, evt));
}
 
int
pevx_evt_unregister( uint crate,
	             struct pev_ioctl_evt *evt,
		     int src_id)
{

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  evt->src_id = src_id;
  return( ioctl( pev->fd, PEV_IOCTL_EVT_UNREGISTER, evt));
}
 
int
pevx_evt_queue_enable( uint crate,
	               struct pev_ioctl_evt *evt)
{

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_EVT_ENABLE, evt));
}
 
int
pevx_evt_queue_disable( uint crate,
	                struct pev_ioctl_evt *evt)
{

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  return( ioctl( pev->fd, PEV_IOCTL_EVT_DISABLE, evt));
}
 
int
pevx_evt_mask( uint crate,
	       struct pev_ioctl_evt *evt,
	       int src_id)
{

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  evt->src_id = src_id;
  return( ioctl( pev->fd, PEV_IOCTL_EVT_MASK, evt));
}
 
int
pevx_evt_unmask( uint crate,
	         struct pev_ioctl_evt *evt,
		 int src_id)
{

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  evt->src_id = src_id;
  return( ioctl( pev->fd, PEV_IOCTL_EVT_UNMASK, evt));
}
 
int
pevx_evt_clear( uint crate,
	        struct pev_ioctl_evt *evt,
		int src_id)
{

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  evt->src_id = src_id;
  return( ioctl( pev->fd, PEV_IOCTL_EVT_CLEAR, evt));
}
 
int
pevx_evt_read( uint crate,
	       struct pev_ioctl_evt *evt,
	       int wait)
{

  if( !pevx[crate]) return(-1);
  pev = pevx[crate];
  if( pev->fd < 0) return(-1);
  evt->wait = wait;
  ioctl( pev->fd, PEV_IOCTL_EVT_READ, evt);
  return( (evt->src_id << 8) | evt->vec_id);
}
