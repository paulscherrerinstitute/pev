/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : AdnConf.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *   This program converts an ASCII configuration file in a binary version
 *   suitable for loading in the ADN SPI Flash.
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
 * $Log: AdnConf.c,v $
 * Revision 1.1  2013/06/07 15:02:51  zimoch
 * update to latest version
 *
 * Revision 1.3  2013/03/19 08:41:03  ioxos
 * suprres debug [JFG]
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
static char rcsid[] = "$Id: AdnConf.c,v 1.1 2013/06/07 15:02:51 zimoch Exp $";
#endif

#define DEBUGno

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <adnioctl.h>
#include "a664.h"

struct a664_conf_tbl_hdr conf_tbl_hdr;
struct a664_tx_vl_ctl *tx_vl_tbl;
struct a664_rx_vl_ctl *rx_vl_tbl;
struct a664_tx_port_ctl *tx_port_tbl;
struct a664_rx_port_ctl *rx_port_tbl;
struct a664_sched_seq *sched_seq;
struct a664_cam_entry *cam_tbl;
struct a664_rx_port_ctl **rx_port_list;

static char line[256];
static int tx_vl_hash[0x10000];
static int rx_vl_hash[0x10000];
static int tx_port_hash[0x10000];
static int rx_port_hash[0x10000];
static int snmp_ip_addr;
static int snmp_port_id;

void adn_rx_sort( struct a664_rx_port_ctl **, int);
void adn_sched_seq( struct a664_tx_vl_ctl *, int, struct a664_sched_seq *, int);
void adn_cam_fill( struct a664_rx_port_ctl **, int, struct a664_cam_entry *, int *); 
unsigned int adn_swap_32( unsigned int);
unsigned short adn_swap_16( unsigned short);

char *default_file = "a664.cbm";
uchar a664_mac_des[6] = { 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
uchar a664_mac_src[6] = { 0x02, 0x00, 0x00, 0x01, 0x22, 0x22};
uint a664_netid=0x03000000;

char *
AdnConf_rcsid()
{
  return( rcsid);
}

int
main( int argc,
      char *argv[])
{
  char *infilename, *cbmfilename, *camfilename;
  FILE *file_cfg, *file_cbm, *file_cam;
  uint i, j, size, off;
  char *cbm, *file_root;
  uint *p;
  ulong cks;
  struct a664_conf_tbl_hdr *cth;
  int txv_i, rxv_i, txp_i, rxp_i;
  int svl_buf_ga;
  int port_buf_ga;
  int sched_seq_cnt;
  int conf_file_size;
  int cbm_idx;

  i = 1;
  infilename = (char *)0;
  file_root = (char *)0;
  while( i < argc)
  {
    char *a;

    a = argv[i++];
    //printf("%s\n", a);
    if( a[0] == '-')
    {
      switch( a[1])
      {
        case 'o':
        {
	  a = argv[i++];
	  file_root = a;
	  //printf("%s\n", a);
	  break;
	}
        default:
        {
	  printf("-%c : bad option -> ", a[1]);
	  printf("usage: AdnConf -o cbmfile infile\n");
	  exit(0);
	}
      }
    }
    else
    {
      infilename = a;
      break;
    }
  }
  if( !infilename)
  {
    printf("need input file -> usage: AdnConf -o cbmfile infile\n");
    exit(0);
  }
  file_cfg = fopen( infilename, "r");
  if( !file_cfg)
  {
    printf("A664 configuration file_cfg %s doesn't exist\n", infilename);
    exit(-1);
  }
  printf("Scanning configuration file : %s ...\n", infilename);
  if( !file_root)
  {
    file_root = strtok( infilename,".");
  }
  cbmfilename = malloc( strlen(file_root) + 4);
  strcpy( cbmfilename, file_root);
  strcat( cbmfilename,".cbm");
  camfilename = malloc( strlen(file_root) + 4);
  strcpy( camfilename, file_root);
  strcat( camfilename,".cam");
  printf("output files : %s - %s\n", cbmfilename, camfilename);

  cth = &conf_tbl_hdr;
  bzero(  cth, sizeof(struct a664_conf_tbl_hdr));
  fseek( file_cfg, 0, SEEK_END);
  size = ftell( file_cfg);
  fseek( file_cfg, 0, SEEK_SET);

  while( fgets( line, 256, file_cfg))
  {
    if( line[0] == '$')
    {
      if( !strncmp( line, "$netid", 6))
      {
	sscanf( line, "$netid:%x", &a664_netid);
      }
      if( !strncmp( line, "$txvl", 5))
      {
	cth->tx_vl_num += 1;
      }
      if( !strncmp( line, "$rxvl", 5))
      {
	cth->rx_vl_num += 1;
      }
      if( !strncmp( line, "$txport", 7))
      {
	cth->tx_port_num += 1;
      }
      if( !strncmp( line, "$rxport", 7))
      {
	cth->rx_port_num += 1;
      }
    }
  }
  printf("tx_vl_num=%d rx_vl_num=%d tc_port_num=%d rx_port_num=%d\n", cth->tx_vl_num, cth->rx_vl_num, cth->tx_port_num, cth->rx_port_num);


  /* prepare TX vl table */
  tx_vl_tbl = (struct a664_tx_vl_ctl *)malloc( cth->tx_vl_num*sizeof(struct a664_tx_vl_ctl));
  for( i = 0; i < cth->tx_vl_num; i++)
  {
    bzero( &tx_vl_tbl[i], sizeof(struct a664_tx_vl_ctl));
  }

  /* prepare RX vl table */
  rx_vl_tbl = (struct a664_rx_vl_ctl *)malloc( cth->rx_vl_num*sizeof(struct a664_rx_vl_ctl));
  for( i = 0; i <  cth->rx_vl_num; i++)
  {
    bzero( &rx_vl_tbl[i], sizeof(struct a664_rx_vl_ctl));
  }

  /* prepare TX port table */
  tx_port_tbl = (struct a664_tx_port_ctl *)malloc( cth->tx_port_num*sizeof(struct a664_tx_port_ctl));
  for( i = 0; i < cth->tx_port_num; i++)
  {
    bzero( &tx_port_tbl[i], sizeof(struct a664_tx_port_ctl));
  }

  /* prepare RX port table */
  rx_port_tbl = (struct a664_rx_port_ctl *)malloc( (cth->rx_port_num + 1)*sizeof(struct a664_rx_port_ctl));
  rx_port_list = (struct a664_rx_port_ctl **)malloc( (cth->rx_port_num + 1)*sizeof(struct a664_rx_port_ctl *));
  for( i = 0; i <= cth->rx_port_num; i++)
  {
    rx_port_list[i] = &rx_port_tbl[i];
    bzero( rx_port_list[i], sizeof(struct a664_rx_port_ctl));
  }
  /* create dummy port indicating end of list */
  rx_port_list[cth->rx_port_num]->vl_id = 0xffff;
  rx_port_list[cth->rx_port_num]->ip_des[0] = 0xff;
  rx_port_list[cth->rx_port_num]->ip_des[1] = 0xff;
  rx_port_list[cth->rx_port_num]->ip_des[2] = 0xff;
  rx_port_list[cth->rx_port_num]->ip_des[3] = 0xff;
  rx_port_list[cth->rx_port_num]->udp_des = 0xffff;

  /* scan configuration file_cfg                          */
  /* initialize tx_vl, rx_vl, tx_port, rx_port tables */
  fseek( file_cfg, 0, SEEK_SET);
  i = 0;
  txv_i = 0;
  rxv_i = 0;
  txp_i = 0;
  rxp_i = 0;
  while( fgets( line, 256, file_cfg))
  {
    if( line[0] == '$') /* line is an object descriptor */
    {
      char *argv[16], *s;
      int argc;

      /* scan object parameters */
      argc = 0;
      argv[argc] = strtok( line,":");
      while( argv[argc++])
      {
	argv[argc] = strtok( 0,":");
      }
      argc -= 1;

      /* MIB Information */
      if( !strncmp( line, "$AE_des", 7))
      {
	strncpy( (char *)&cth->AE_des, argv[1], 32);
      }
      if( !strncmp( line, "$AE_pn", 6))
      {
	strncpy( (char *)&cth->AE_pn, argv[1], 32);
      }
      if( !strncmp( line, "$AE_sn", 6))
      {
	strncpy( (char *)&cth->AE_sn, argv[1], 32);
      }
      if( !strncmp( line, "$AE_lpn", 7))
      {
	strncpy( (char *)&cth->AE_lpn, argv[1], 32);
      }
      if( !strncmp( line, "$AE_hwv", 7))
      {
	strncpy( (char *)&cth->AE_hwv, argv[1], 8);
      }
      if( !strncmp( line, "$AE_swv", 7))
      {
	strncpy( (char *)&cth->AE_swv, argv[1], 8);
      }
      if( !strncmp( line, "$AE_tblv", 8))
      {
	strncpy( (char *)&cth->AE_tblv, argv[1], 8);
      }
      if( !strncmp( line, "$AE_loc", 7))
      {
	strncpy( (char *)&cth->AE_loc, argv[1], 12);
      }
      /* SNMP Information */
      if( !strncmp( line, "$snmp", 5))
      {
	snmp_port_id = strtoul( argv[1], &s, 0);
	snmp_ip_addr = strtoul( argv[2], &s, 0);
      }
      /* transmission VL */
      if( !strncmp( line, "$txvl", 5))
      {
	struct a664_tx_vl_ctl *txv;

	txv = &tx_vl_tbl[txv_i++];
	txv->vl_id = (ushort)strtoul( argv[1], &s, 0);
	txv->max_size = (ushort)strtoul( argv[2], &s, 0);
	txv->bag =( (uchar)strtoul( argv[3], &s, 0)) & 0x7;
	txv->network = 0;
	if( (argv[4][0] == 'A') ||  (argv[4][1] == 'A')) txv->network |= 2;
	if( (argv[4][0] == 'B') ||  (argv[4][1] == 'B')) txv->network |= 1;
      }
      /* reception VL */
      if( !strncmp( line, "$rxvl", 5))
      {
	struct a664_rx_vl_ctl *rxv;

	rxv = &rx_vl_tbl[rxv_i++];
	rxv->vl_id = (ushort)strtoul( argv[1], &s, 0);
	rxv->ic = 0;
	if( (argv[2][0] == 'I') ||  (argv[2][1] == 'I')) rxv->ic |= 0x80;
	if( (argv[2][1] == 'R') ||  (argv[2][1] == 'R')) rxv->rm |= 0x80;
	rxv->skew = (ushort)strtoul( argv[3], &s, 0);
	rxv->raq_idx = 0xf;
      }
      /* transmission port */
      if( !strncmp( line, "$txport", 7))
      {
	struct a664_tx_port_ctl *txp;
	int ip;
	unsigned short udp;

	txp = &tx_port_tbl[txp_i++];
	txp->port_id = (ushort)strtoul( argv[1], &s, 0);
	txp->size = (ushort)strtoul( argv[2], &s, 0);
        if( !strncmp( argv[3], "sap", 3))
	{
	  txp->size |= A664_TX_PORT_TYPE_SAP;
	}
	txp->vl_id = strtoul( argv[4], &s, 0);
	txp->svl_id = strtoul( argv[5], &s, 0);
	txp->fh.mac_des[0] = a664_mac_des[0];
	txp->fh.mac_des[1] = a664_mac_des[1];
	txp->fh.mac_des[2] = a664_mac_des[2];
	txp->fh.mac_des[3] = a664_mac_des[3];
	txp->fh.mac_des[4] = (txp->vl_id >> 8) & 0xff;
	txp->fh.mac_des[5] = txp->vl_id & 0xff;
	txp->fh.mac_src[0] = a664_mac_src[0];
	txp->fh.mac_src[1] = a664_mac_src[1];
	txp->fh.mac_src[2] = a664_mac_src[2];
	txp->fh.mac_src[3] = a664_mac_src[3];
	txp->fh.mac_src[4] = a664_mac_src[4];
	txp->fh.mac_src[5] = a664_mac_src[5];
	txp->fh.ip_vhl     = 0x45;
	txp->fh.ip_tos     = 0;
	txp->fh.ip_len     = 0;
	txp->fh.ip_id      = 0;
	txp->fh.ip_off     = 0;
	txp->fh.ip_ttl     = 1;
	txp->fh.ip_prot    = 17;
	txp->fh.protocol   = adn_swap_16( 0x0800);
	ip = strtoul( argv[7], &s, 0);
	txp->fh.ip_src[0] = (ip >> 24)&0xff;
	txp->fh.ip_src[1] = (ip >> 16)&0xff;
	txp->fh.ip_src[2] = (ip >> 8)&0xff;
	txp->fh.ip_src[3] = (ip)&0xff;
	udp = (unsigned short)strtoul( argv[8], &s, 0);
	txp->fh.udp_src = adn_swap_16(udp);
	ip = strtoul( argv[9], &s, 0);
	txp->fh.ip_des[0] = (ip >> 24)&0xff;
	txp->fh.ip_des[1] = (ip >> 16)&0xff;
	txp->fh.ip_des[2] = (ip >> 8)&0xff;
	txp->fh.ip_des[3] = (ip)&0xff;
	udp = (unsigned short)strtoul( argv[10], &s, 0);
	txp->fh.udp_des = adn_swap_16(udp);
	txp->fh.udp_len = 0;
	txp->fh.udp_cks = 0;
      }
      /* reception port */
      if( !strncmp( line, "$rxport", 7))
      {
	struct a664_rx_port_ctl *rxp;
	int ip;


	rxp = rx_port_list[rxp_i++];
	rxp->port_id = (ushort)strtoul( argv[1], &s, 0);
	rxp->size = (ushort)strtoul( argv[2], &s, 0);
        if( !strncmp( argv[3], "queue", 3))
	{
	  rxp->size |= A664_RX_PORT_TYPE_QUEUING;
	}
        if( !strncmp( argv[3], "smpl", 3))
	{
	  rxp->size |= A664_RX_PORT_TYPE_SAMPLING;
	}
	rxp->vl_id = strtoul( argv[4], &s, 0);
	ip = strtoul( argv[5], &s, 0);
	rxp->ip_des[0] = (ip >> 24)&0xff;
	rxp->ip_des[1] = (ip >> 16)&0xff;
	rxp->ip_des[2] = (ip >> 8)&0xff;
	rxp->ip_des[3] = (ip)&0xff;
	rxp->udp_des = strtoul( argv[6], &s, 0);
	rxp->wr_idx = 0x0;
	rxp->rd_idx = 0x0;
      }
    }
  }

#ifdef DEBUG
  /* Display transmission VLs */
  for( i = 0; i < cth->tx_port_num; i++)
  {
    struct a664_tx_vl_ctl *txv;
    txv = &tx_vl_tbl[i];
  }
  /* Display transmission ports */
  for( i = 0; i < cth->tx_port_num; i++)
  {
    struct a664_tx_port_ctl *txp;
    txp = &tx_port_tbl[i];

    printf("%04x:%02x.%02x.%02x.%02x:%04x:", txp->vl_id, txp->fh.ip_src[0], txp->fh.ip_src[1], txp->fh.ip_src[2], txp->fh.ip_src[3], txp->fh.udp_src);
    printf("%02x.%02x.%02x.%02x:%04x |  ", txp->fh.ip_des[0], txp->fh.ip_des[1], txp->fh.ip_des[2], txp->fh.ip_des[3], txp->fh.udp_des);
    if( (i & 3) == 3) printf("\n");
  }
  printf("\n");

  /* Display reception port */
  for( i = 0; i < cth->rx_port_num; i++)
  {
    p = rx_port_list[i];
    printf("%04x:%02x.%02x.%02x.%02x:%04x |  ", p->vl_id, p->ip_des[0], p->ip_des[1], p->ip_des[2], p->ip_des[3], p->udp_des);
    if( (i & 3) == 3) printf("\n");
  }
  printf("\n");
#endif


  /* RX port sorting */
  adn_rx_sort(rx_port_list, cth->rx_port_num);

#ifdef DEBUG
  /* Display reception port after sorting */
  for( i = 0; i < cth->rx_port_num; i++)
  {
    p = rx_port_list[i];
    printf("%04x:%02x.%02x.%02x.%02x:%04x |  ", p->vl_id, p->ip_des[0], p->ip_des[1], p->ip_des[2], p->ip_des[3], p->udp_des);
    if( (i & 3) == 3) printf("\n");
  }
  printf("\n");
#endif

  /* update hash table id->idx */
  /* reset tx_vl hash table */
  for( i = 0; i < 0x10000; i++)
  {
    tx_vl_hash[i] = -1;
  }
  /* register tx_vl list */
  for( i = 0; i < cth->tx_vl_num; i++)
  {
    tx_vl_hash[tx_vl_tbl[i].vl_id] = i;
  }
  /* reset rx_vl hash table */
  for( i = 0; i < 0x10000; i++)
  {
    rx_vl_hash[i] = -1;
  }
  /* register rx_vl list */
  for( i = 0; i < cth->rx_vl_num; i++)
  {
    rx_vl_hash[rx_vl_tbl[i].vl_id] = i;
  }
  /* reset tx_port hash table */
  for( i = 0; i < 0x10000; i++)
  {
    tx_port_hash[i] = -1;
  }
  /* register tx_port list */
  for( i = 0; i < cth->tx_port_num; i++)
  {
    tx_port_hash[tx_port_tbl[i].port_id] = i;
  }
  /* reset rx_port hash table */
  for( i = 0; i < 0x10000; i++)
  {
    rx_port_hash[i] = -1;
  }
  /* register rx_port list */
  for( i = 0; i < cth->rx_port_num; i++)
  {
    rx_port_hash[rx_port_tbl[i].port_id] = i;
  }

  /* Build CAM table */
  cam_tbl = (struct a664_cam_entry *)malloc( A664_CAM_SIZE*sizeof(struct a664_cam_entry));
  adn_cam_fill( rx_port_list, cth->rx_port_num, cam_tbl, rx_vl_hash);

  /* update SVL queue size according to list of tx port to which they are connect */
  /* queue_size = sum of port_size + room for msg headers + rounding to 16        */
  for( i = 0; i < cth->tx_port_num; i++)
  {
    int msg_size, vl_idx, svl_idx;
    struct a664_tx_vl_ctl *txv;
    struct a664_tx_svl_ctl *sv;

    vl_idx =  tx_vl_hash[tx_port_tbl[i].vl_id];
    txv = &tx_vl_tbl[vl_idx];

    svl_idx = tx_port_tbl[i].svl_id;
    sv = &txv->svl_ctl[svl_idx];

    msg_size = ((tx_port_tbl[i].size & 0x3fff) + 31) & 0xfff0;
    sv->buf_size += msg_size;

#ifdef DEBUG
    printf("port %d: %x - %x -%x\n", i, vl_idx, svl_idx, msg_size);
#endif
  }

  /* update Tx SVL queue addresses */
  sched_seq_cnt = 0;
  for( i = 0; i < cth->tx_vl_num; i++)
  {
    sched_seq_cnt += 1<<(7-tx_vl_tbl[i].bag);
  }

  /* prepare scheduling sequence */
  sched_seq = (struct a664_sched_seq *)malloc(sched_seq_cnt*sizeof(struct a664_sched_seq));
  adn_sched_seq( tx_vl_tbl,  cth->tx_vl_num, sched_seq, sched_seq_cnt);

  /* prepare address offset of control tables */
  cth->magic =  0x41363634;
  cth->conf_tbl_ga =  0x8000;
  cth->tx_vl_tbl_ga = cth->conf_tbl_ga + sizeof(struct a664_conf_tbl_hdr);
  cth->rx_vl_tbl_ga = cth->tx_vl_tbl_ga + (cth->tx_vl_num*sizeof(struct a664_tx_vl_ctl));
  cth->tx_port_tbl_ga = cth->rx_vl_tbl_ga + (cth->rx_vl_num*sizeof(struct a664_rx_vl_ctl));
  cth->rx_port_tbl_ga = cth->tx_port_tbl_ga + (cth->tx_port_num*sizeof(struct a664_tx_port_ctl));
  cth->tx_sched_tbl_ga = cth->rx_port_tbl_ga + (cth->rx_port_num*sizeof(struct a664_rx_port_ctl));
  cbm_idx =  cth->tx_sched_tbl_ga + (sched_seq_cnt*sizeof(struct a664_sched_seq));
  conf_file_size =  cbm_idx - cth->conf_tbl_ga;
  printf("configuration table size : 0x%x [%d]\n", conf_file_size, conf_file_size);


  /* update SVL queues global addresses */
  svl_buf_ga = cbm_idx;
  for( i = 0; i < cth->tx_vl_num; i++)
  {
    struct a664_tx_vl_ctl *txv;
    struct a664_tx_svl_ctl *sv;

    txv = &tx_vl_tbl[i];
    for( j =  0; j < 4; j++)
    {
      sv = &txv->svl_ctl[j];
      if( sv->buf_size)
      {
	sv->buf_size += 0x10; /* add space for queue rollover header */
	sv->buf_ga = svl_buf_ga;
	svl_buf_ga += sv->buf_size;
      }
#ifdef DEBUG
      printf("vl %d:%d - %x - %x\n", i, j, sv->buf_ga, sv->buf_size);
#endif
    }
  }
  cbm_idx = svl_buf_ga;
  //printf("cbm index : 0x%x [%d]\n", cbm_idx, cbm_idx);

  /* update Rx port buffer global addresses */
  port_buf_ga = cbm_idx;
  for( i = 0; i < cth->rx_port_num; i++)
  {
    struct a664_rx_port_ctl *rxp;

    rxp = rx_port_list[i];
    rxp->buf_ga = port_buf_ga;
    port_buf_ga += (rxp->size + 0x1f) & 0x3ff0;
#ifdef DEBUG
      printf("port %d:%04x - %x - %x\n", i,  rxp->port_id, rxp->buf_ga, rxp->size);
#endif
  }
  cbm_idx = port_buf_ga;
  //printf("cbm index : 0x%x [%d]\n", cbm_idx, cbm_idx);

  /* update Tx port buffer global addresses */
  for( i = 0; i < cth->tx_port_num; i++)
  {
    struct a664_tx_port_ctl *txp;

    txp = &tx_port_tbl[i];
    txp->svl_desc_ga = cth->tx_vl_tbl_ga + (sizeof(struct a664_tx_vl_ctl) * tx_vl_hash[txp->vl_id]) + (sizeof(struct a664_tx_svl_ctl) * txp->svl_id) + 0x30  ;
#ifdef DEBUG
      printf("tx port %d:%04x - %x - %x\n", i,  txp->port_id, txp->svl_desc_ga, txp->size & 0x3fff);
#endif
  }

  /* build configuration file */
  cbm = (char *)malloc( A664_CBM_SIZE);

  /* copy table header */
  memcpy( cbm, cth, sizeof(struct a664_conf_tbl_hdr));
  cth = (struct a664_conf_tbl_hdr *)cbm;
  cth->len = conf_file_size;

  /* update SNMP trap parameters */
  cth->snmp_ip_addr[0] = (snmp_ip_addr >> 24)&0xff;
  cth->snmp_ip_addr[1] = (snmp_ip_addr >> 16)&0xff;
  cth->snmp_ip_addr[2] = (snmp_ip_addr >> 8)&0xff;
  cth->snmp_ip_addr[3] = (snmp_ip_addr)&0xff;
  cth->snmp_port_index =  tx_port_hash[snmp_port_id];
  {
    struct a664_tx_port_ctl *txp;
    int vl_idx;

    txp = &tx_port_tbl[cth->snmp_port_index];
    vl_idx =  tx_vl_hash[txp->vl_id];
    cth->snmp_wr_idx_ga = cth->tx_vl_tbl_ga + (sizeof(struct a664_tx_vl_ctl) * vl_idx) + (4*txp->svl_id);
    cth->snmp_svl_buf_ga = tx_vl_tbl[vl_idx].svl_ctl[txp->svl_id].buf_ga;
  }
  printf("snmp port = %d [%04x]- ga = %x - %x\n",  cth->snmp_port_index, snmp_port_id, cth->snmp_wr_idx_ga, cth->snmp_svl_buf_ga);
  /* copy tx vl table */
  //printf("tx_vl_tbl: %x [%x*%lx]\n", cth->tx_vl_tbl_ga, cth->tx_vl_num, sizeof(struct a664_tx_vl_ctl));
  off = cth->tx_vl_tbl_ga - cth->conf_tbl_ga;
  memcpy( &cbm[off], tx_vl_tbl, cth->tx_vl_num*sizeof(struct a664_tx_vl_ctl));

  /* copy rx vl table */
  //printf("rx_vl_tbl: %x [%x*%lx]\n", cth->rx_vl_tbl_ga, cth->rx_vl_num, sizeof(struct a664_rx_vl_ctl));
  off = cth->rx_vl_tbl_ga - cth->conf_tbl_ga;
  memcpy( &cbm[off], rx_vl_tbl, cth->rx_vl_num*sizeof(struct a664_rx_vl_ctl));

  /* copy tx port table */
  //printf("tx_port_tbl: %x [%x*%lx]\n", cth->tx_port_tbl_ga, cth->tx_port_num, sizeof(struct a664_tx_port_ctl));
  off = cth->tx_port_tbl_ga - cth->conf_tbl_ga;
  memcpy( &cbm[off], tx_port_tbl, cth->tx_port_num*sizeof(struct a664_tx_port_ctl));

  /* copy rx port table */
  //printf("rx_port_tbl: %x [%x*%lx]\n", cth->rx_port_tbl_ga, cth->rx_port_num, sizeof(struct a664_rx_port_ctl));
  off = cth->rx_port_tbl_ga - cth->conf_tbl_ga;
  for( i = 0; i < cth->rx_port_num; i++)
  {
    memcpy( &cbm[off], rx_port_list[i] , sizeof(struct a664_rx_port_ctl));
    off += sizeof(struct a664_rx_port_ctl);
  }

  /* copy scheduling table */
  //printf("sched_seq: %x [%x*%lx] %p\n", cth->tx_sched_tbl_ga, sched_seq_cnt, sizeof(struct a664_sched_seq), sched_seq);
  off = cth->tx_sched_tbl_ga - cth->conf_tbl_ga;
  memcpy( &cbm[off], sched_seq, sched_seq_cnt*sizeof(struct a664_sched_seq));
  cth->tx_sched_period =  A664_SCHED_PERIOD;

  file_cbm = fopen( cbmfilename, "w");
  p = (uint *)cbm;
  cks = 0;
  for( i = 0; i < conf_file_size; i+=4)
  {
    cks += (ulong)(*p++);
  }
  while( cks >> 32)
  {
    cks = (cks & 0xffffffff) + (cks >>32);
  }
  cth->cks = ~((uint)cks);
  fwrite( cbm, 1, conf_file_size, file_cbm);
  fclose( file_cbm);
  free( cbm);

  /* build cam file */
  file_cam = fopen( camfilename, "w");
  fwrite( cam_tbl, 1, A664_CAM_SIZE*12, file_cam);
  fclose( file_cam);

  fclose( file_cfg);

  exit(0);
}

