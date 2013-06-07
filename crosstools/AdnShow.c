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
 *   This program 
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
 * $Log: AdnShow.c,v $
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
static char rcsid[] = "$Id: AdnShow.c,v 1.1 2013/06/07 15:02:51 zimoch Exp $";
#endif

#define DEBUGno

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <adnioctl.h>
#include <adnulib.h>
#include "a664.h"

struct a664_tx_vl_ctl *tx_vl_tbl;
struct a664_rx_vl_ctl *rx_vl_tbl;
struct a664_tx_port_ctl *tx_port_tbl;
struct a664_rx_port_ctl *rx_port_tbl;

unsigned int adn_swap_32( unsigned int);
unsigned short adn_swap_16( unsigned short);
struct adn_node *adn;

char *
AdnShow_rcsid()
{
  return( rcsid);
}

char *default_file = "a664.cbm";

int
main( int argc,
      char *argv[])
{
  FILE *file_cbm;
  char *infilename;
  struct a664_conf_tbl_hdr *cth;
  uint i, j, k, size, off;
  char *cbm;

#ifdef JFG
  if( argc < 2)
  {
    adn = adn_init();
    if( !adn)
    {
      printf("Cannot allocate data structures to control ADN\n");
      exit( -1);
    }
    if( adn->fd < 0)
    {
      printf("Cannot find ADN interface\n");
      adn_exit( adn);
      exit( -1);
    }
    size = adn_reg_rd( 0x8008);
    cbm = (char *)malloc( size);
    adn_hrms_rd( (void *)cbm, 0x8000, size, 0);
    adn_exit( adn);
  }
  else
#endif
  {
    infilename = argv[1];
    file_cbm = fopen( infilename, "r");
    if( !file_cbm)
    {
      printf("A664 configuration file_bm %s doesn't exist\n", infilename);
      exit(-1);
    }
    fseek( file_cbm, 0, SEEK_END);
    size = ftell( file_cbm);
    fseek( file_cbm, 0, SEEK_SET);
    cbm = (char *)malloc( size);
    fread( cbm, 1, size, file_cbm);
    fclose( file_cbm);
  }
  
  /* read configuration table header */
  cth = (struct a664_conf_tbl_hdr *)cbm;

  /* get TX VL descriptor table */
  off = cth->tx_vl_tbl_ga - cth->conf_tbl_ga;
  tx_vl_tbl = (struct a664_tx_vl_ctl *)&cbm[off];
  printf("tx_vl_tbl_ga = %x - %x\n", off, cth->tx_vl_tbl_ga);

  /* get RX VL descriptor table */
  off = cth->rx_vl_tbl_ga - cth->conf_tbl_ga;
  rx_vl_tbl = (struct a664_rx_vl_ctl *)&cbm[off];
  printf("rx_vl_tbl_ga = %x - %x\n", off, cth->rx_vl_tbl_ga);

  /* get TX PORT descriptor table */
  off = cth->tx_port_tbl_ga - cth->conf_tbl_ga;
  tx_port_tbl = (struct a664_tx_port_ctl *)&cbm[off];
  printf("tx_port_tbl_ga = %x - %x\n", off, cth->tx_port_tbl_ga);

  /* get RX PORT descriptor table */
  off = cth->rx_port_tbl_ga - cth->conf_tbl_ga;
  rx_port_tbl = (struct a664_rx_port_ctl *)&cbm[off];
  printf("tx_port_tbl_ga = %x - %x\n", off, cth->rx_port_tbl_ga);

  printf("tx_port_num = %d - rx_port_num = %d\n",  cth->tx_port_num,  cth->rx_port_num);
  /* print TX VL/Port list */
  printf("+-----+-------------------------+-----+--------+------+------+------+------+-------------+-------+-------------+-------+\n");
  printf("|     |             TX VL Table                |                         TX Port Table                                 |\n");
  printf("|     +----+-+-----+------+-----+-----+--------+------+------+------+------+-------------+-------+-------------+-------+\n");
  printf("|     |  id  | bag | size | net | svl |  queue |  idx |   id | size | type |    ip_des   | udp_d |    ip_src   | udp_s |\n");
  printf("+-----+------+-----+------+-----+-----+--------+------+------+------+------+-------------+-------+-------------+-------+\n");
  for( i = 0; i < cth->tx_vl_num; i++)
  {
    struct a664_tx_vl_ctl *txv;
    int ret, svl;

    txv = &tx_vl_tbl[i];
    printf("| %3d | %04x | %3d | %4d | ", i, txv->vl_id, 1 << txv->bag, txv->max_size);
    if( txv->network& 2)printf(" A");
    else printf(" X");
    if( txv->network&1)printf("B |");
    else printf("X |");
    ret = 0;
    for( k = 0; k < 4; k++)
    {
      svl = 0;
      for(j = 0; j < cth->tx_port_num; j++)
      {
        struct a664_tx_port_ctl *txp;

        txp =  &tx_port_tbl[j];
        if(( txv->vl_id == txp->vl_id) && ( txp->svl_id == k))
        {
    	  if( !ret)
 	  {
	    ret = 1;
	  }
	  else
	  {
            printf("\n|     |      |     |      |     |");
	  }
    	  if( !svl)
 	  {
	    svl = 1;
	    printf(" %3d | %06x |", txp->svl_id, txv->svl_ctl[txp->svl_id].buf_ga);
	  }
	  else
	  {
            printf("     |        |");
	  }
	  printf(" %4d | %04x | %4d |", j, txp->port_id, txp->size & 0x3fff);
	  if( txp->size & A664_TX_PORT_TYPE_SAP)
	  {
	    printf("  sap | --.--.--.-- |  ---- |");
	  }
	  else 
	  { 
	    printf(" afdx |");
	    printf(" %02x.%02x.%02x.%02x |  %04x |", txp->fh.ip_des[0], txp->fh.ip_des[1], txp->fh.ip_des[2], txp->fh.ip_des[3], adn_swap_16( txp->fh.udp_des));
	  }
	  printf(" %02x.%02x.%02x.%02x |  %04x |", txp->fh.ip_src[0], txp->fh.ip_src[1], txp->fh.ip_src[2], txp->fh.ip_src[3], adn_swap_16( txp->fh.udp_src));
        }
      }
    }
    if( !ret)
    {
       printf("     |        |      |      |      |      |             |       |             |       |");
    }
    printf("\n+-----+------+-----+------+-----+-----+--------+------+------+------+------+-------------+-------+-------------+-------+\n");
  }

    printf("\n\n");

  /* print RX VL/Port- list */
  printf("+-----+-------------------+-----------------------------------------------------------+\n");
  printf("|     |    RX VL Table    |                      RX Port Table                        |\n");
  printf("|     +------+----+-------+------+------+------+--------+-------+-------------+-------+\n");
  printf("|     |  id  | ir |  skew |  idx |  id  | size |   buf  |  type |    ip_des   | udp_d |\n");
  printf("+-----+------+----+-------+------+------+------+--------+-------+-------------+-------+\n");
  for( i = 0; i < cth->rx_vl_num; i++)
  {
    struct a664_rx_vl_ctl *rxv;
    int ret;
    char I,R;

    I='X'; R= 'X';
    rxv = &rx_vl_tbl[i];
    if( rxv->ic & 0x80) I = 'I';
    if( rxv->rm & 0x80) R = 'R';
    printf("| %3d | %04x | %c%c | %5d |", i, rxv->vl_id, I, R, rxv->skew);
    ret = 0;
    for(j = 0; j < cth->rx_port_num; j++)
    {
      struct a664_rx_port_ctl *rxp;

      rxp =  &rx_port_tbl[j];
      if( rxv->vl_id == rxp->vl_id)
      {
    	if( !ret)
 	{
	    ret = 1;
	}
	else
	{
	  printf("\n|     |      |    |       |");
	}
	printf(" %4d | %04x | %4d | %06x |", j, rxp->port_id, rxp->size & 0x3fff, rxp->buf_ga);
	if( rxp->size & A664_RX_PORT_TYPE_QUEUING)
	{
	  printf(" queue |");
	}
	else 
	{ 
	  printf("  smpl |");
	}
	printf(" %02x.%02x.%02x.%02x |  %04x |", rxp->ip_des[0], rxp->ip_des[1], rxp->ip_des[2], rxp->ip_des[3], rxp->udp_des);
      }
    }
    if( !ret)
    {
      printf("      |      |      |        |       |             |       |");
    }
    printf("\n+-----+------+----+-------+------+------+------+--------+-------+-------------+-------+\n");
  }

  exit(0);
}
