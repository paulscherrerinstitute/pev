/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : a664ulib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That library contains a set of function to access the A6641000 interface
 *     through the /dev/a664 device driver.
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
 *  $Log: a664lib.c,v $
 *  Revision 1.1  2013/06/07 15:03:26  zimoch
 *  update to latest version
 *
 *  Revision 1.1  2013/04/15 14:21:36  ioxos
 *  first checkin [JFG]
 *
 *  Revision 1.3  2013/03/14 11:14:48  ioxos
 *  rename spi to sflash [JFG]
 *
 *  Revision 1.2  2013/03/13 08:05:42  ioxos
 *  set version to A664_RELEASE [JFG]
 *
 *  Revision 1.1  2013/03/08 09:38:08  ioxos
 *  first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: a664lib.c,v 1.1 2013/06/07 15:03:26 zimoch Exp $";
#endif

#define DEBUGno
#ifdef DEBUG
#define debug(x) printf x
#else
#define debug(x) 
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
#include <adnioctl.h>
#include <adnulib.h>
#include <a664lib.h>

#ifdef BIG_ENDIAN
#define SWAP_32(x) adn_swap_32(x)
#define SWAP_16(x) adn_swap_16(x)
#else
#define SWAP_32(x) 
#define SWAP_16(x) 
#endif

#define A664_REQ_DONE        0x80000000
#define A664_REQ_POST_MSG      0x040000
#define A664_REQ_GET_MSG       0x140000
#define A664_REQ_STS_TX        0x080000
#define A664_REQ_STS_RX        0x180000

#define A664_REQ_TMO               1000
#define A664_REQ_OFFSET          0x1000
#define A664_REQ_IDX                  0

int a664_tx_port_idx[0x10000];
int a664_rx_port_idx[0x10000];
int a664_tx_port_id[0x1000];
int a664_rx_port_id[0x1000];

static struct adn_node *adn = (struct adn_node *)NULL;

struct a664_dev
{
  struct adn_node *adn;
  struct a664_port_list *tx_port;
  struct a664_port_list *rx_port;
} a664;

char a664_conf_tbl[0x8000];
struct a664_tx_port_ctl *a664_txp;
struct a664_rx_port_ctl *a664_rxp;

struct adn_req_tx_msg
{
  int cmd;
  int msg_size;
  int ip_addr;
  int udp_addr;
} adn_req_tx;

struct adn_req_rx_msg
{
  int cmd;
  int buf_size;
  int rsv1[2];
  int port;
  int msg_size;
  int msg_cnt;
  unsigned int time;
} adn_req_rx;

#ifdef BIG_ENDIAN
struct adn_req_port_status
{
  int cmd;
  int rsv1[3];
  ushort size;
  ushort port_id;
  ushort rsv2;
  ushort vl_id;
  uint msg_cnt;
  uint err_cnt;
};
#else
struct adn_req_port_status
{
  int cmd;
  int rsv1[3];
  ushort port_id;
  ushort size;
  ushort vl_id;
  ushort rsv2;
  uint msg_cnt;
  uint err_cnt;
};
#endif

char *
a664_rcsid()
{
  return( rcsid);
}

int
a664_init()
{
  struct a664_conf_tbl_hdr *cth;
  struct a664_tx_port_ctl *txp;
  struct a664_rx_port_ctl *rxp;
  int len, magic;
  uint cks;
  uint i, off;
  uint *p;
  int ntx, nrx;

  adn = adn_init();
  if( !adn)
  {
    return( -1);
  }
  if( adn->fd < 0)
  {
    return( -1);
  }
  adn_sflash_rd( a664_conf_tbl, A664_SFLASH_CBM_OFF, sizeof(struct a664_conf_tbl_hdr));

  cth = ( struct a664_conf_tbl_hdr *)a664_conf_tbl;
  magic = SWAP_32(cth->magic);
  if( magic != 0x41363634)
  {
    debug(("Wrong magic : %08x\n", magic));
    close(adn->fd);
    return( A664_ERR_CONF_INV);
  }
  len = SWAP_32( cth->len);
  debug(("Configuration table size = %x [magic = %08x]\n", len, SWAP_32(cth->magic)));
  adn_sflash_rd( a664_conf_tbl, A664_SFLASH_CBM_OFF, len);
  debug(("Verifying checksum..."));
  cks = 0;
  p = (uint *)a664_conf_tbl;
  for( i = 0; i < len; i+=4)
  {
    cks += SWAP_32(*p++);
  }
  if( cks != 0xffffffff)
  {
    debug(("Wrong checksum : %08x\n", cks));
    close(adn->fd);
    return( A664_ERR_CONF_INV);
  }
  else
  {
    debug(("%08x -> OK\n", cks));
  }


  for( i = 0; i < 0x10000; i++)
  {
    a664_tx_port_idx[i] = -1;
  }
  for( i = 0; i < 0x1000; i++)
  {
    a664_tx_port_id[i] = -1;
  }
  ntx = SWAP_16( cth->tx_port_num);
  off = SWAP_32( cth->tx_port_tbl_ga) - SWAP_32( cth->conf_tbl_ga);
  txp = (struct a664_tx_port_ctl *)&a664_conf_tbl[off];
  a664_txp = txp;
  debug(("tx port: %08x [%d]\n", off, ntx));
  for( i = 0; i < ntx; i++)
  {
    unsigned short id;

    id = (unsigned short)SWAP_16( txp->port_id);
    debug(("%2d : %04x\n", i, id));
    a664_tx_port_idx[id] = i;
    a664_tx_port_id[i] = id;
    txp++;
  }

  for( i = 0; i < 0x10000; i++)
  {
    a664_rx_port_idx[i] = -1;
  }
  for( i = 0; i < 0x1000; i++)
  {
    a664_rx_port_id[i] = -1;
  }
  nrx = SWAP_16( cth->rx_port_num);
  off = SWAP_32( cth->rx_port_tbl_ga) - SWAP_32( cth->conf_tbl_ga);
  rxp = (struct a664_rx_port_ctl *)&a664_conf_tbl[off];
  a664_rxp = rxp;
  debug(("rx port: %08x [%d]\n", off, nrx));
  for( i = 0; i < nrx; i++)
  {
    unsigned short id;

    id = (unsigned short)SWAP_16( rxp->port_id);
    debug(("%2d : %04x\n", i, id));
    a664_rx_port_idx[id] = i;
    a664_rx_port_id[i] = id;
    rxp++;
  }
  return( A664_OK);
}

int
a664_exit()
{
  int ret;

  ret = A664_OK;
  adn_exit( adn);
  return(ret);
}

int *
a664_tx_port_list()
{
  return( &a664_tx_port_id[0]);
}

int *
a664_rx_port_list()
{
  return( &a664_rx_port_id[0]);
}

int
a664_post_req( int idx,
	       int offset,
	       int req)
{
#ifdef OLD_ADP4003
  adn_reg_wr( 0x4000 | (idx << 2), req | offset);
#else
  adn_hcr_wr( (idx << 3), req | offset);
#endif
  return( A664_OK);
}


int
a664_wait_req( int idx,
	       int tmo)
{
  usleep(10000);
  return( A664_OK);
#ifdef OLD_ADP4003
  while(( !(adn_reg_rd( 0x4000 | (idx << 2)) & A664_REQ_DONE)) && --tmo);
#else
  while(( !(adn_hcr_rd( idx << 3) & A664_REQ_DONE)) && --tmo);
#endif
  if( !tmo)
  {
    return( A664_ERR_REQ_TMO);
  }
  return( A664_OK);
}


int
a664_write( short port_id,
	    char *msg,
	    int size,
	    int ip_des,
	    short udp_des,
	    int mode)
{
  int port_idx;
  struct a664_tx_port_ctl *txp;
  int port_size;
  int req_offset, req_idx;
  int retval;

  port_idx = a664_tx_port_idx[port_id];
  debug(("a664_write() : port -> %04x : %d\n", port_id, port_idx));
  if( port_idx < 0)
  {
    return( A664_ERR_PORT_INV);
  }
  txp = &a664_txp[port_idx];
  port_size = SWAP_16(txp->size);
  debug(("port size = %x\n", port_size));
  if( (port_size & 0xc000) != mode)
  {
    return( A664_ERR_PORT_INV);
  }
  if( size > (port_size & 0x3fff))
  {
    return( A664_ERR_BAD_SIZE);
  }
  bzero( &adn_req_tx, sizeof( struct adn_req_tx_msg));
  adn_req_tx.cmd = 0x44000000 + port_idx;
  adn_req_tx.msg_size = size;
  if( mode == A664_TX_PORT_TYPE_SAP)
  {
    adn_req_tx.ip_addr = ip_des;
    adn_req_tx.udp_addr = udp_des;
  }
  req_offset = A664_REQ_OFFSET;
  adn_hrms_wr( (void *)&adn_req_tx, req_offset, sizeof( struct adn_req_tx_msg), RDWR_NOSWAP);
  size = (size + 0xf)&0x3ff0;
#ifdef BIG_ENDIAN
  adn_hrms_wr( (void *)msg, req_offset + sizeof( struct adn_req_tx_msg), size, RDWR_SWAP);
#else
  adn_hrms_wr( (void *)msg, req_offset + sizeof( struct adn_req_tx_msg), size, RDWR_NOSWAP);
#endif
  req_idx = A664_REQ_IDX;
  a664_post_req( req_idx, req_offset, A664_REQ_POST_MSG);
  retval = a664_wait_req( req_idx, A664_REQ_TMO);
  if( retval < 0)
  {
    return( retval);
  }
  adn_hrms_rd( (void *)&adn_req_tx, req_offset, sizeof( struct adn_req_tx_msg), RDWR_NOSWAP);
  if( (adn_req_tx.cmd & 0xff00ffff)!= (0x55000000 + port_idx))
  {
    return( A664_ERR_REQ_ERR);
  }
  retval = adn_req_tx.cmd & 0x70000;
  if( retval)
  {
    switch( retval)
    {
      case 0x10000:
      {
	return( A664_ERR_PORT_INV);
      }
      case 0x20000:
      {
	return( A664_ERR_BAD_SIZE);
      }
      default:
      {
	return( A664_ERR_REQ_ERR);
      }
    }
  }
  return( A664_OK);
}

int
a664_write_afdx( short port_id,
	         char *msg,
	         int size)
{
  return( a664_write( port_id, msg, size, 0, 0, A664_TX_PORT_TYPE_AFDX));
}

int
a664_write_sap( short port_id,
	        char *msg,
	        int size,
		int ip_des,
		short udp_des)
{
  return( a664_write( port_id, msg, size, ip_des, udp_des, A664_TX_PORT_TYPE_SAP));
}

int
a664_read( short port_id,
	   char *msg,
	   int *size,
	   int *para_1,
	   int *para_2,
	   int mode)
{
  int port_idx;
  struct a664_rx_port_ctl *rxp;
  int port_size, len;
  int req_offset, req_idx;
  int retval;

  port_idx = a664_rx_port_idx[port_id];
  debug(("a664_read() : port -> %04x : %d\n", port_id, port_idx));
  if( port_idx < 0)
  {
    return( A664_ERR_PORT_INV);
  }
  rxp = &a664_rxp[port_idx];
  port_size = SWAP_16(rxp->size);
  debug(("port size = %x\n", port_size));
  if( (port_size & 0xc000) != mode)
  {
    return( A664_ERR_PORT_INV);
  }
  bzero( &adn_req_rx, sizeof( struct adn_req_rx_msg));
  adn_req_rx.cmd = 0x44000000 + port_idx;
  adn_req_rx.msg_size = *size;
  req_offset = A664_REQ_OFFSET;
  adn_hrms_wr( (void *)&adn_req_rx, req_offset, sizeof( struct adn_req_rx_msg), RDWR_NOSWAP);
  req_idx = A664_REQ_IDX;
  a664_post_req( req_idx, req_offset, A664_REQ_GET_MSG);
  retval = a664_wait_req( req_idx, A664_REQ_TMO);
  if( retval < 0)
  {
    return( retval);
  }
  adn_hrms_rd( (void *)&adn_req_rx, req_offset, sizeof( struct adn_req_rx_msg), RDWR_NOSWAP);
  if( (adn_req_rx.cmd & 0xff00ffff)!= (0x55000000 + port_idx))
  {
    return( A664_ERR_REQ_ERR);
  }
  retval = adn_req_rx.cmd & 0x70000;
  if( retval)
  {
    switch( retval)
    {
      case 0x10000:
      {
	return( A664_ERR_PORT_INV);
      }
      case 0x20000:
      {
	return( A664_ERR_BAD_SIZE);
      }
      default:
      {
	return( A664_ERR_REQ_ERR);
      }
    }
  }
  if( para_1)
  {
    *para_1 = adn_req_rx.msg_cnt;
  }
  if( para_2)
  {
    *para_2 = adn_req_rx.time;
  }
  len = 0;
  if( *size)
  {
    if( *size < adn_req_rx.msg_size)
    {
      len = (*size + 0xf)&0x3ff0;
    }
    else
    {
      len = (adn_req_rx.msg_size + 0xf)&0x3ff0;
    }
    *size = adn_req_rx.msg_size;
#ifdef BIG_ENDIAN
    adn_hrms_rd( (void *)msg, req_offset + sizeof( struct adn_req_rx_msg), len, RDWR_SWAP);
#else
    adn_hrms_rd( (void *)msg, req_offset + sizeof( struct adn_req_rx_msg), len, RDWR_NOSWAP);
#endif
  }
  return( len);
}

int
a664_read_sampling( short port_id,
	            char *msg,
	            int *size,
		    int *freshness)
{
  int len;
  int cnt, time;

  len = a664_read(port_id, msg, size, &cnt, &time, A664_RX_PORT_TYPE_SAMPLING);
  *freshness = cnt;

  return( len);
}

int
a664_read_queueing( short port_id,
	            char *msg,
	            int *size)
{
  return( a664_read(port_id, msg, size, 0, 0, A664_RX_PORT_TYPE_QUEUING));
}

int
a664_read_sap( short port_id,
	       char *msg,
	       int *size,
	       int *ip_src,
	       short *udp_src)
{
  int len;
  int ip, udp; 

  len =  a664_read(port_id, msg, size, &ip, &udp, A664_RX_PORT_TYPE_QUEUING);
  *ip_src = SWAP_32(ip);
  *udp_src = SWAP_16((short)(udp & 0xffff));

  return( len);
}

int
a664_port_status( unsigned short port_id,
		  struct a664_port_status *sts,
		  int type)
{
  int port_idx;
  struct adn_req_port_status *ps;
  int req_offset, req_idx, req;
  int retval;

  if( type)
  {
    req = A664_REQ_STS_RX;
    port_idx = a664_rx_port_idx[port_id];
    debug(("a664_status_rx() : port -> %04x : %d\n", port_id, port_idx));
  }
  else 
  {
    req = A664_REQ_STS_TX;
    port_idx = a664_tx_port_idx[port_id];
    debug(("a664_status_tx() : port -> %04x : %d\n", port_id, port_idx));
  }
  if( port_idx < 0)
  {
    return( A664_ERR_PORT_INV);
  }
  ps = (struct adn_req_port_status *)malloc( sizeof(struct adn_req_port_status));
  ps->cmd = 0x44000000 + port_idx;
  req_offset = A664_REQ_OFFSET;
  adn_hrms_wr( (void *)ps, req_offset, sizeof( struct adn_req_port_status), RDWR_NOSWAP);
  req_idx = A664_REQ_IDX;
  a664_post_req( req_idx, req_offset, req);
  retval = a664_wait_req( req_idx, A664_REQ_TMO);
  if( retval < 0)
  {
    free( ps);
    return( retval);
  }
  adn_hrms_rd( (void *)ps, req_offset, sizeof( struct adn_req_port_status), RDWR_NOSWAP);
  if( (ps->cmd & 0xff00ffff)!= (0x55000000 + port_idx))
  {
    free( ps);
    return( A664_ERR_REQ_ERR);
  }
  retval = ps->cmd & 0x70000;
  if( retval)
  {
    switch( retval)
    {
      case 0x10000:
      {
	free( ps);
	return( A664_ERR_PORT_INV);
      }
      default:
      {
	free( ps);
	return( A664_ERR_REQ_ERR);
      }
    }
  }
  bcopy( (char *)&ps->size, (char *)sts, sizeof( struct a664_port_status));
  free( ps);
  return( port_idx);
}

int
a664_status_tx( unsigned short port_id,
		struct a664_port_status *sts)
{

  return( a664_port_status( port_id, sts, 0));
}

int
a664_status_rx( unsigned short port_id,
		struct a664_port_status *sts)
{

  return( a664_port_status( port_id, sts, 1));
}


int
a664_statistics( struct a664_stats *stats)
{
  stats->mac_tx_A.bytes = adn_csr_rd( 0x100 + 0x00);
  stats->mac_tx_A.frames = adn_csr_rd( 0x100 + 0x04);
  stats->mac_tx_A.errors = adn_csr_rd( 0x100 + 0x08);
  stats->mac_rx_A.bytes = adn_csr_rd( 0x100 + 0x40);
  stats->mac_rx_A.frames = adn_csr_rd( 0x100 + 0x44);
  stats->mac_rx_A.errors.tot = adn_csr_rd( 0x100 + 0x48);
  stats->mac_rx_A.errors.nodest = adn_csr_rd( 0x100 + 0x4c);
  stats->mac_rx_A.errors.align = adn_csr_rd( 0x100 + 0x60);
  stats->mac_rx_A.errors.crc = adn_csr_rd( 0x100 + 0x64);
  stats->mac_rx_A.errors.flen = adn_csr_rd( 0x100 + 0x68);
  stats->mac_rx_A.errors.internal = adn_csr_rd( 0x100 + 0x6c);

  stats->mac_tx_B.bytes = adn_csr_rd( 0x100 + 0x10);
  stats->mac_tx_B.frames = adn_csr_rd( 0x100 + 0x14);
  stats->mac_tx_B.errors = adn_csr_rd( 0x100 + 0x18);
  stats->mac_rx_B.bytes = adn_csr_rd( 0x100 + 0x50);
  stats->mac_rx_B.frames = adn_csr_rd( 0x100 + 0x54);
  stats->mac_rx_B.errors.tot = adn_csr_rd( 0x100 + 0x58);
  stats->mac_rx_B.errors.nodest = adn_csr_rd( 0x100 + 0x5c);
  stats->mac_rx_B.errors.align = adn_csr_rd( 0x100 + 0x70);
  stats->mac_rx_B.errors.crc = adn_csr_rd( 0x100 + 0x74);
  stats->mac_rx_B.errors.flen = adn_csr_rd( 0x100 + 0x78);
  stats->mac_rx_B.errors.internal = adn_csr_rd( 0x100 + 0x7c);

  stats->rm_rx.first = adn_csr_rd( 0x100 + 0x84);
  stats->rm_rx.errors = adn_csr_rd( 0x100 + 0x80);

  stats->ip_rx.reasm.reqds = adn_csr_rd( 0x100 + 0xa0);
  stats->ip_rx.reasm.ok = adn_csr_rd( 0x100 + 0xa4);
  stats->ip_rx.reasm.fails = adn_csr_rd( 0x100 + 0xa8);

  stats->ip_rx.errors.tot = adn_csr_rd( 0x100 + 0x90);
  stats->ip_rx.errors.protos = adn_csr_rd( 0x100 + 0x94);
  stats->ip_rx.errors.checksum = adn_csr_rd( 0x100 + 0x98);
  stats->ip_rx.errors.discards = adn_csr_rd( 0x100 + 0x9c);

  stats->udp_rx.errors = adn_csr_rd( 0x100 + 0xb0);
  stats->udp_rx.noport = adn_csr_rd( 0x100 + 0xb4);

  return( 0);
}

int
a664_()
{
  return( 0);
}

