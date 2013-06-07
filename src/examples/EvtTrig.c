/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : template.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : october 10,2008
 *    version  : 0.0.1
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
 * $Log: EvtTrig.c,v $
 * Revision 1.9  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.4  2012/07/10 09:49:07  ioxos
 * check 16 sources from user area [JFG]
 *
 * Revision 1.3  2012/06/28 14:00:43  ioxos
 * set USR1 interrupt [JFG]
 *
 * Revision 1.2  2012/06/01 14:00:14  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.1  2012/05/23 15:17:10  ioxos
 * first checkin [JFG]
 *
 * Revision 1.1  2009/01/08 08:19:03  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <pevioctl.h>
#include <pevulib.h>

struct pev_node *pev;

int
main( int argc,
      char **argv)
{
  int i;

  pev = pev_init( 0);
  if( !pev)
  {
    printf("Cannot allocate data structures to control PEV1100\n");
    exit( -1);
  }
  if( pev->fd < 0)
  {
    printf("Cannot find PEV1100 interface\n");
    exit( -1);
  }

  pev_csr_wr( 0x80001008, 0xffff);
  for( i = 0; i < 16; i++)
  {
    pev_csr_wr( 0x8000100c, 1<<i);
    usleep(1000);
  }

  pev_exit( pev);

  exit(0);
}
