/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : AdnInit.c
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
 * $Log: AdnInit.c,v $
 * Revision 1.1  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.2  2013/04/15 14:55:28  ioxos
 * add support multiple APX2300/ADN400x [JFG]
 *
 * Revision 1.1  2013/03/08 09:23:47  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: AdnInit.c,v 1.1 2013/06/07 14:59:54 zimoch Exp $";
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
  int adn_idx;

  adn_idx = 0;
  if( argc < 2)
  {
    printf("usage: AdnInit <dev>\n");
    exit(0);
  }
  sscanf(argv[1], "%d", &adn_idx);
  if( ( adn_idx < 0) || ( adn_idx > 3))
  {
    printf("AdnInit : bad device index [0 - 3] ->  %d\n", adn_idx);
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
  printf("Initializing ADN interface...\n");
#ifdef OLD_ADP4003
  adn_reg_wr( 0x7200, 0xe);
  usleep(100000);
  adn_reg_wr( 0x7200, 0xf);
  usleep(100000);
  adn_reg_wr( 0x7200, 0x0);
  printf("ADN magic = %08x\n", adn_reg_rd( 0x8000));
#else
  adn_csr_wr( 0x80, 0xe);
  usleep(100000);
  adn_csr_wr( 0x80, 0xf);
  usleep(100000);
  adn_csr_wr( 0x80, 0x0);
  printf("ADN magic = %08x [%08x]\n", adn_csr_rd( 0x0), adn_csr_rd( 0x80));
#endif
  adn_exit( adn);
  exit(0);
}
