/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : AdnLib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *   This file program extracts the Rx port list from a AFDX configuration file
 *   and sorts them in ascending order according to their VL identifier, IP 
 *   destination and UDP destination:
 *                 vl_id    -> 16 bit
 *                 ip_des   -> 4*8 bit
 *                 udp_des  -> 16 bit
 *   The sorting is done in 4 passes, first vl_id then ip_des byte 0 and 1,
 *   then ip_des byte 2 and 3 and finally udp_des.
 *   At the end of the process a file suitable for the ADNESC CAM is generated
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
 * $Log: AdnLib.c,v $
 * Revision 1.1  2013/06/07 15:02:51  zimoch
 * update to latest version
 *
 * Revision 1.2  2013/03/11 16:53:22  ioxos
 * adapt to pev environmentd
 *
 * Revision 1.1  2013/03/11 16:28:28  ioxos
 * first checkin [JFG]
 *
 * Revision 1.1  2012/09/20 13:48:22  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/


#ifndef lint
static char rcsid[] = "$Id: AdnLib.c,v 1.1 2013/06/07 15:02:51 zimoch Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <adnioctl.h>
#include "a664.h"

char *
AdnLib_rcsid()
{
  return( rcsid);
}

int
adn_swap_32( int data)
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
adn_swap_16( short data)
{
  char ci[2];
  char co[2];

  *(short *)ci = data;
  co[0] = ci[1];
  co[1] = ci[0];

  return( *(short *)co);
}


static int
diff_vl( const void *x,
	  const void  *y)
{
  int diff;
  struct a664_rx_port_ctl *p, *q;

  p = (struct a664_rx_port_ctl *)(*(struct a664_rx_port_ctl **)x);
  q = (struct a664_rx_port_ctl *)(*(struct a664_rx_port_ctl **)y);
  diff = p->vl_id - q->vl_id;
  return( diff);
}

static int
diff_ip_h( const void *x,
	   const void  *y)
{
  int diff;
  struct a664_rx_port_ctl *p, *q;

  p = (struct a664_rx_port_ctl *)(*(struct a664_rx_port_ctl **)x);
  q = (struct a664_rx_port_ctl *)(*(struct a664_rx_port_ctl **)y);
  diff = ((p->ip_des[0] << 8) | p->ip_des[1]) - ((q->ip_des[0] << 8) | q->ip_des[1]);
  return( diff);
}

static int
diff_ip_l( const void *x,
	   const void  *y)
{
  int diff;
  struct a664_rx_port_ctl *p, *q;

  p = (struct a664_rx_port_ctl *)(*(struct a664_rx_port_ctl **)x);
  q = (struct a664_rx_port_ctl *)(*(struct a664_rx_port_ctl **)y);
  diff = ((p->ip_des[2] << 8) | p->ip_des[3]) - ((q->ip_des[2] << 8) | q->ip_des[3]);
  return( diff);
}

static int
diff_udp( const void *x,
	  const void  *y)
{
  int diff;
  struct a664_rx_port_ctl *p, *q;

  p = (struct a664_rx_port_ctl *)(*(struct a664_rx_port_ctl **)x);
  q = (struct a664_rx_port_ctl *)(*(struct a664_rx_port_ctl **)y);
  diff = p->udp_des - q->udp_des;
  return( diff);
}

void
adn_rx_sort( struct a664_rx_port_ctl **rx_port_list,
	     int port_num)
{
  struct a664_rx_port_ctl *p;
  void *next;
  int i, cnt, vl_id, ip;

  /* sort port list according to VL identifier */
  qsort( (void *)rx_port_list, port_num + 1, sizeof(struct a664_rx_port_ctl *), diff_vl);

  /* sort port list according to IP destination byte 0 and 1 */
  p = rx_port_list[0];
  vl_id = p->vl_id;
  next = (void *)&rx_port_list[0];
  cnt = 0;
  for( i = 1; i <= port_num; i++)
  {
    p = rx_port_list[i];
    cnt += 1;
    if( p->vl_id != vl_id)
    {
      if( cnt > 1)
      {
	qsort( next, cnt, sizeof(struct a664_rx_port_ctl *), diff_ip_h);
      }
      vl_id = p->vl_id;
      cnt = 0;
      next = (void *)&rx_port_list[i];
    }
  }

  /* sort port list according to IP destination byte 0 and 1 */
  p = rx_port_list[0];
  vl_id = p->vl_id;
  ip = (p->ip_des[0] << 8) | p->ip_des[1];
  next = (void *)&rx_port_list[0];
  cnt = 0;
  for( i = 1; i <= port_num; i++)
  {
    p = rx_port_list[i];
    cnt += 1;
    if( ( p->vl_id != vl_id) || ( ( (p->ip_des[0] << 8) | p->ip_des[1]) != ip))
    {
      if( cnt > 1)
      {
	qsort( next, cnt, sizeof(struct a664_rx_port_ctl *), diff_ip_l);
      }
      vl_id = p->vl_id;
      ip = (p->ip_des[0] << 8) | p->ip_des[1];
      cnt = 0;
      next = (void *)&rx_port_list[i];
    }
  }

  p = rx_port_list[0];
  vl_id = p->vl_id;
  ip = (p->ip_des[0] << 24) | (p->ip_des[1] << 16) | (p->ip_des[2] << 8) | p->ip_des[3];
  next = (void *)&rx_port_list[0];
  cnt = 0;
  for( i = 1; i <= port_num; i++)
  {
    p = rx_port_list[i];
    cnt += 1;
    if( ( p->vl_id != vl_id) || (((p->ip_des[0] << 24) | (p->ip_des[1] << 16) | (p->ip_des[2] << 8) | p->ip_des[3]) != ip))
    {
      if( cnt > 1)
      {
	qsort( next, cnt, sizeof(struct a664_rx_port_ctl *), diff_udp);
      }
      vl_id = p->vl_id;
      ip = (p->ip_des[0] << 24) | (p->ip_des[1] << 16) | (p->ip_des[2] << 8) | p->ip_des[3];
      cnt = 0;
      next = (void *)&rx_port_list[i];
    }
  }
  return;
}

struct a664_tx_vl_ctl **tx_vl_list;
struct a664_sched_seq sched_tbl[A664_TSLOT_NUM+1][80];

static int
diff_bag( const void *x,
	  const void  *y)
{
  int diff;
  struct a664_tx_vl_ctl *p, *q;

  p = (struct a664_tx_vl_ctl *)(*(struct a664_tx_vl_ctl **)x);
  q = (struct a664_tx_vl_ctl *)(*(struct a664_tx_vl_ctl **)y);
  diff = ((p->bag << 16) - p->max_size) - ((q->bag << 16) - q->max_size);
  return( diff);
}

int 
adn_sched_next_tts( long start,
		    long size )
{
  return( start + ( ( size + A664_EXTRA_SIZE) *  A664_TIME_PER_BYTE));
}

void
adn_sched_seq( struct a664_tx_vl_ctl *txv,
	       int txv_num,
	       struct a664_sched_seq *ss,
	       int ss_num)

{
  int i, j, iex;
  int vl_idx, bag;
  struct a664_tx_vl_ctl *v;
  int tts[A664_TSLOT_NUM], nvl[A664_TSLOT_NUM];
  int n_rej, n_ok;;
  int next_tts;

  tx_vl_list = (struct a664_tx_vl_ctl **)malloc( txv_num*sizeof(struct a664_tx_vl_ctl *));
  for( i = 0; i < txv_num; i++)
  {
    tx_vl_list[i] = &txv[i];
  }
  /* sort VL list according to BAG and max size */
  qsort( (void *)tx_vl_list, txv_num, sizeof(struct a664_tx_vl_ctl *), diff_bag);
#ifdef DEBUG
  for( i = 0; i < txv_num; i++)
  {
    printf("vl: %x - %x - %x\n", tx_vl_list[i]->vl_id, tx_vl_list[i]->bag, tx_vl_list[i]->max_size);
  }
#endif

  for( i = 0; i < A664_TSLOT_NUM; i++)
  {
    tts[i] = 0;
    nvl[i] = 0;
    for( j = 0; j < 80; j++)
    {
      sched_tbl[i][j].vl_idx = -1;
      sched_tbl[i][j].tts = 0;
    }
  }
  iex = 1;
  n_rej = 0;
  n_ok = 0;
  for( vl_idx = 0; vl_idx < txv_num; vl_idx++)  
  {
    v = tx_vl_list[vl_idx];
    bag = v->bag;
    for( j = 0; j < ( 1 << bag); j++)
    {
      next_tts = adn_sched_next_tts( tts[j], v->max_size);
      if( next_tts < A664_MSEC)
      {
	iex = 0;
	break;
      }
    }
    if( iex)
    {
      /* no space for VL */
      n_rej++;
    }
    else
    {
      /* place VL in scheduling list */
      for( i = j; i < A664_TSLOT_NUM; i += ( 1 << bag))
      {
        //printf("%3d : %04x - %x - %d:%d\n", i, v->vl_id, v->max_size, tts[i], next_tts);
	sched_tbl[i][nvl[i]].vl_idx = vl_idx;
	sched_tbl[i][nvl[i]].tts = tts[i];
	tts[i] = next_tts;
	nvl[i] += 1;
      }
    }
  }
#ifdef DEBUG
  for( i = 0; i < A664_TSLOT_NUM; i++)
  {
    printf("%3d:", i);
    for( j = 0; j < nvl[i]; j++)
    {
      printf(" %02d:%06d", sched_tbl[i][j].vl_idx,  sched_tbl[i][j].tts);
      n_ok++;
    }
    printf("\n");
  }
  printf("n_ok = %d - ss_num = %d\n", n_ok, ss_num);
#endif

  /* load scheduling table */
  n_ok = 0;
  for( i = 0; i < A664_TSLOT_NUM; i++)
  {
    for( j = 0; j < nvl[i]; j++)
    {
      ss[n_ok].tts = (i*A664_MSEC + sched_tbl[i][j].tts)/A664_SCHED_TICK;
      ss[n_ok].vl_idx = sched_tbl[i][j].vl_idx;
      n_ok++;
    }
  }
  ss[n_ok-1].tts |= A664_SCHED_LAST;
}


uint
ecc_16( uint data)
{
  uint ecc, i;
  uint d[16], e[6];

  for( i = 0; i < 16; i++)
  {
    d[i] = (data >> i) & 1;
  }
  e[0] = d[0]^d[1]^d[3]^d[4]^d[6]^d[8]^d[10]^d[11]^d[13]^d[15];
  e[1] = d[0]^d[2]^d[3]^d[5]^d[6]^d[9]^d[10]^d[12]^d[13];
  e[2] = d[1]^d[2]^d[3]^d[7]^d[8]^d[9]^d[10]^d[14]^d[15];
  e[3] = d[4]^d[5]^d[6]^d[7]^d[8]^d[9]^d[10];
  e[4] = d[11]^d[12]^d[13]^d[14]^d[15];
  e[5] = d[0]^d[1]^d[2]^d[3]^d[4]^d[5]^d[6]^d[7]^d[8]^d[9]^d[10]^d[11]^d[12]^d[13]^d[14]^d[15]^e[0]^e[1]^e[2]^e[3]^e[4];

  ecc = 0;
  for( i = 0; i < 6; i++)
  {
    ecc |= (e[i]&1) << i;
  }
  return( ecc);
}

uint
ecc_32( uint data)
{
  uint ecc, i;
  uint d[32], e[7];

  for( i = 0; i < 32; i++)
  {
    d[i] = (data >> i) & 1;
  }
  e[0] = d[0]^d[1]^d[3]^d[4]^d[6]^d[8]^d[10]^d[11]^d[13]^d[15];
  e[0] = e[0]^d[17]^d[19]^d[21]^d[23]^d[25]^d[26]^d[28]^d[30];

  e[1] = d[0]^d[2]^d[3]^d[5]^d[6]^d[9]^d[10]^d[12]^d[13];
  e[1] = e[1]^d[16]^d[17]^d[20]^d[21]^d[24]^d[25]^d[27]^d[28]^d[31];

  e[2] = d[1]^d[2]^d[3]^d[7]^d[8]^d[9]^d[10]^d[14]^d[15];
  e[2] = e[2]^d[16]^d[17]^d[22]^d[23]^d[24]^d[25]^d[29]^d[30]^d[31];

  e[3] = d[4]^d[5]^d[6]^d[7]^d[8]^d[9]^d[10]^d[18]^d[19]^d[20]^d[21]^d[22]^d[23]^d[24]^d[25];

  e[4] = d[11]^d[12]^d[13]^d[14]^d[15]^d[16]^d[17]^d[18]^d[19]^d[20]^d[21]^d[22]^d[23]^d[24]^d[25];

  e[5] = d[26]^d[27]^d[28]^d[29]^d[30]^d[31];

  e[6] = d[0]^d[1]^d[2]^d[3]^d[4]^d[5]^d[6]^d[7]^d[8]^d[9]^d[10]^d[11]^d[12]^d[13]^d[14]^d[15];
  e[6] = e[6]^d[16]^d[17]^d[18]^d[19]^d[20]^d[21]^d[22]^d[23]^d[24]^d[25]^d[26]^d[27]^d[28]^d[29]^d[30]^d[31];
  e[6] = e[6]^e[0]^e[1]^e[2]^e[3]^e[4]^e[5];

  ecc = 0;
  for( i = 0; i < 7; i++)
  {
    ecc |= (e[i]&1) << i;
  }
  return( ecc);
}

static uint
*adn_cam_set( char *b, 
              ushort vid, 
              uchar ipd[4],
	      ushort udpd,
	      uint vidx)
{
  uint ecc[3];
  struct a664_cam_entry *c;
  c = (struct a664_cam_entry *)b;
  c->vl_id = vid ;
  c->ip0 = ipd[0];
  c->ip1 = ipd[1];
  c->ip2 = ipd[2];
  c->ip3 = ipd[3];
  c->udp = udpd;
  //printf("cam_reg[0]=%08x\n", *(uint *)&b[0]);
  //printf("cam_reg[1]=%08x\n", *(uint *)&b[4]);
  ecc[0] = ecc_16( c->vl_id);
  ecc[1] = ecc_32( (c->ip0 << 24) | (c->ip1 << 16) |  (c->ip2 << 8) | c->ip3);
  ecc[2] = ecc_32( (c->udp << 12) | vidx);
  *(uint *)&b[8] = (vidx << 20) | (ecc[0]<<14) | (ecc[1]<<7) | ecc[2];  
  //printf("cam_reg[2]=%08x\n",*(uint *)&b[8]);

  return( 0);

}

void
adn_cam_fill( struct a664_rx_port_ctl **rxpl,
	      int rxp_num,
	      struct a664_cam_entry *cam,
	      int *rx_vl_hash)

{
  int i;
  struct a664_cam_entry *c;
  struct a664_rx_port_ctl *p;

  c = cam;
  for( i = 0; i < A664_CAM_SIZE; i++)
  {
    uchar ip_ff[4] = { 0xff, 0xff, 0xff, 0xff};
    adn_cam_set( (char *)c++, 0xffff, ip_ff, i, rxp_num);
  }

  c = cam;
  for( i = 0; i < rxp_num; i++)
  {
    p = rxpl[i];
    adn_cam_set( (char *)c, p->vl_id, &p->ip_des[0], p->udp_des, rx_vl_hash[p->vl_id]);
#ifdef DEBUG
    printf("%2d : %04x %08x %04x %2d -> ", i, p->vl_id, *(uint *)&p->ip_des[0], p->udp_des, rx_vl_hash[p->vl_id]);
    b = (uchar *)c;
    for( j=0; j<12; j++)
    {
      printf("%02x ", b[j]);
    }
    printf("\n");
#endif
    c++;
  }

  return;
}
