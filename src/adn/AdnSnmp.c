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
 * $Log: AdnSnmp.c,v $
 * Revision 1.1  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.2  2013/04/15 14:55:28  ioxos
 * add support multiple APX2300/ADN400x [JFG]
 *
 * Revision 1.1  2013/03/08 09:23:48  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: AdnSnmp.c,v 1.1 2013/06/07 14:59:54 zimoch Exp $";
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

#ifdef OLD_ADP4003
#define ADN_RD_SNMP adn_reg_rd( 0x7600)
#define ADN_WR_SNMP(x) adn_reg_wr( 0x7600, x)
#else
#define ADN_RD_SNMP adn_csr_rd( 0x200)
#define ADN_WR_SNMP(x) adn_csr_wr( 0x200, x)
#endif

struct adn_node *adn;

char *get_rcsid(){return(rcsid);}

int
main( int argc,
      char *argv[])
{
  int status;
  int adn_idx;

  adn_idx = 0;
  if( argc < 3)
  {
    printf("usage: AdnSnmp <dev> <operation>\n");
    exit(0);
  }
  sscanf(argv[1], "%d", &adn_idx);
  if( ( adn_idx < 0) || ( adn_idx > 3))
  {
    printf("AdnSnmp : bad device index [0 - 3] ->  %d\n", adn_idx);
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
  status = ADN_RD_SNMP;
  if( status & 1)
  {
    printf("ADN SNMP trap active\n");
    if( argc > 1)
    {  
      if( !strcmp( argv[2], "stop"))
      {
	ADN_WR_SNMP( 0x0);
	printf(" -> stopped\n");
      }
    }
  }
  else
  {
    printf("ADN SNMP trap stopped\n");
    if( argc > 1)
    {  
      if( !strcmp( argv[2], "start"))
      {
	ADN_WR_SNMP(0x1);
	printf(" -> started\n");
      }
    }
  }
  adn_exit( adn);
  exit(0);
}
