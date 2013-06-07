/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : AdnTxPorts.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
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
 * $Log: AdnTxPorts.c,v $
 * Revision 1.1  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.1  2013/03/08 09:23:48  ioxos
 * first checkin [JFG]
 *
 * Revision 1.1  2012/09/20 13:39:52  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: AdnTxPorts.c,v 1.1 2013/06/07 14:59:54 zimoch Exp $";
#endif

#include <debug.h>

#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <pty.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <aio.h>
#include <errno.h>
#include <signal.h>


typedef unsigned int u32;
#include <pevioctl.h>
#include <adnioctl.h>
#include <adnulib.h>


#ifdef BIG_ENDIAN
struct port_status
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
struct port_status
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

struct adn_node *adn;

int
main( int argc,
      char *argv[])
{
  int tmo;
  int cnt;
  char *buf;
  struct port_status *ps;
  int port;

  port = -1;
  if( argc > 1)
  {
    sscanf(argv[1], "%d", &port);
  }

  adn = adn_init();
  if( !adn)
  {
    printf("Cannot allocate data structures to control ADN\n");
    exit( -1);
  }
  if( adn->fd < 0)
  {
    printf("Cannot find ADN interface\n");
    exit( -1);
  }
  buf = (char *)malloc( 0x200);
  ps = (struct port_status *)buf;
  cnt = 0;
  printf("+-----+------+------+------+---------+---------+\n");
  printf("|                TX Port Status                |\n");
  printf("+-----+------+------+------+---------+---------+\n");
  printf("| idx |  id  | size |  vl  |   msg   |  error  |\n"); 
  printf("+-----+------+------+------+---------+---------+\n");
  if( port == -1)
  {
    while(1)
    {
      bzero( buf, 0x200);
      *(int *)buf = 0x44000000 + cnt;
      adn_hrms_wr( (void *)buf, 0x1800, 0x40, RDWR_NOSWAP);
      adn_reg_wr( 0x4030, 0x81800);
      //usleep(10000);
      tmo = 100;
      while(( !(adn_reg_rd( 0x4030) & 0x80000000)) && tmo--);
      if( !tmo)
      {
        printf("Request timeout...exiting!!\n");
        break;
      }
      adn_hrms_rd( (void *)buf, 0x1800, 0x40, RDWR_NOSWAP);
      if( *(int *)buf != (0x55000000 + cnt))
      {
        break;
      }
      printf("| %3d | %04x | %04x | %04x | %7d | %7d |\n", cnt, ps->port_id, ps->size & 0x3fff, ps->vl_id, ps->msg_cnt, ps->err_cnt);
      cnt++;
    }
  }
  else
  {
    bzero( buf, 0x200);
    *(int *)buf = 0x44000000 + port;
    adn_hrms_wr( (void *)buf, 0x1800, 0x40, RDWR_NOSWAP);
    adn_reg_wr( 0x4030, 0x81800);
    //usleep(10000);
    tmo = 100;
    while(( !(adn_reg_rd( 0x4030) & 0x80000000)) && tmo--);
    if( !tmo)
    {
      printf("|          Request timeout...exiting!!         |\n");
    }
    else
    {
      adn_hrms_rd( (void *)buf, 0x1800, 0x40, RDWR_NOSWAP);
      if( *(int *)buf != (0x55000000 + port))
      {
        printf("| %3d |      Port not in configuation table    |\n", port);
      }
      else
      {
        printf("| %3d | %04x | %04x | %04x | %7d | %7d |\n", port, ps->port_id, ps->size & 0x3fff, ps->vl_id, ps->msg_cnt, ps->err_cnt);
      }
    }
  }
  printf("+-----+------+------+------+---------+---------+\n");

  free( buf);

  adn_exit( adn);
  exit(0);
}
