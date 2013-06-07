/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : AdnPost.c
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
 * $Log: AdnStats.c,v $
 * Revision 1.1  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.2  2013/04/15 14:55:29  ioxos
 * add support multiple APX2300/ADN400x [JFG]
 *
 * Revision 1.1  2013/03/08 09:23:48  ioxos
 * first checkin [JFG]
 *
 * Revision 1.1  2012/09/20 13:39:52  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: AdnStats.c,v 1.1 2013/06/07 14:59:54 zimoch Exp $";
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
#include <aio.h>
#include <errno.h>
#include <signal.h>


typedef unsigned int u32;
#include <pevioctl.h>
#include <adnioctl.h>
#include <adnulib.h>

struct adn_node *adn;

char *get_rcsid(){return(rcsid);}

int
main( int argc,
      char *argv[])
{
  int cnt, i;
  char *buf;
  uint *p;
  int adn_idx;

  adn_idx = 0;
  if( argc < 2)
  {
    printf("usage: AdnStats <dev>\n");
    exit(0);
  }
  sscanf(argv[1], "%x", &adn_idx);
  if( ( adn_idx < 0) || ( adn_idx > 3))
  {
    printf("AdnStats : bad device index [0 - 3] ->  %d\n", adn_idx);
  } 
  adn = adn_init( adn_idx);
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
  printf("ADN Live Statistics (1 sec refresh rate)\n");
  buf = (char *)malloc( 0x200);

  cnt = 0;
  while(1)
  {
#ifdef OLD_ADP4003
    adn_hrms_rd( (void *)buf, 0x7400, 0x200,  RDWR_NOSWAP);
#else
    for( i = 0; i < 0x100; i += 4)
    {
      *(int *)&buf[i*2] = adn_csr_rd( 0x100 + i);
    }
#endif

    p = (uint *)buf;
    printf("+----------+-------------------------+-------------------------+\n");
    printf("|          |       Transmission      |        Reception        |\n");
    printf("|   MAC    +------------+------------+------------+------------+\n");
    printf("|          |      A     |      B     |      A     |      B     |\n");
    printf("+----------+------------+------------+------------+------------+\n");
    printf("| Bytes    | %10d | %10d | %10d | %10d |\n", p[0x00], p[0x08], p[0x20], p[0x28]);
    printf("| Frames   | %10d | %10d | %10d | %10d |\n", p[0x02], p[0x0a], p[0x22], p[0x2a]);
    printf("| Errors   | %10d | %10d | %10d | %10d |\n", p[0x04], p[0x0c], p[0x24], p[0x2c]);
    printf("| No Dest  |            |            | %10d | %10d |\n", p[0x26], p[0x2e]);
    printf("+----------+-------------------------+-------------------------+\n");
    printf("|          |                MAC Reception Errors               |\n");
    printf("|   CHAN   +------------+------------+------------+------------+\n");
    printf("|          |   Align    |    CRC     |    FLen    |  Internal  |\n");
    printf("+----------+------------+------------+------------+------------+\n");
    printf("|    A     | %10d | %10d | %10d | %10d |\n", p[0x30], p[0x32], p[0x34], p[0x36]);
    printf("|    B     | %10d | %10d | %10d | %10d |\n", p[0x38], p[0x3a], p[0x3c], p[0x3e]);
    printf("+----------+------------+------------+------------+------------+\n");
    printf("|          |                IP Reception Counter               |\n");
    printf("+----------+------------+------------+------------+------------+\n");
    printf("|  IC/RM   | %10d | %10d |          X |          X |\n", p[0x40], p[0x42]);
    printf("|  Errors  | %10d | %10d | %10d | %10d |\n", p[0x48], p[0x4a], p[0x4c], p[0x4e]);
    printf("| Reassem  | %10d | %10d | %10d |          X |\n", p[0x50], p[0x52], p[0x54]);
    printf("|    UDP   | %10d | %10d | %10d | %10d |\n", p[0x58], p[0x5a], 0, 0);
    printf("+----------+------------+------------+------------+------------+\n");
    cnt++;
    printf("%d\r", cnt);
    sleep(1);
    i = 24;
    while( i--)printf("\x1b\x5b\x41");
  }
  free( buf);

  adn_exit( adn);
  exit(0);
}
