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
 * $Log: AdnSflashRead.c,v $
 * Revision 1.1  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.1  2013/03/14 12:46:02  ioxos
 * rename spi to sflash [JFG]
 *
 * Revision 1.1  2013/03/14 12:41:30  ioxos
 * rename spi to sflash [JFG]
 *
 * Revision 1.2  2013/03/14 12:38:35  ioxos
 * rename spi to sflash [JFG]
 *
 * Revision 1.1  2013/03/08 09:23:48  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: AdnSflashRead.c,v 1.1 2013/06/07 14:59:54 zimoch Exp $";
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
char buf[0x10000];
int
main( int argc,
      char *argv[])
{
  int i;
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

  adn_sflash_rd( buf, 0x1800, 0x100);

  for( i = 0; i < 0x100; i++)
  {
    printf("%02x ", (unsigned char)buf[i]);
    if( (i&0xf) == 0xf) printf("\n");
  }
  adn_exit( adn);
  exit(0);
}
