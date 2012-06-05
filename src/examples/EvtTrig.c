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
 * Revision 1.1  2012/06/05 13:40:26  kalantari
 * linux driver ver.4.12 with intr Handling
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

#include <pevioctl.h>
#include <pevulib.h>

struct pev_node *pev;

main( int argc,
      void *argv[])
{
  int i;
  int src_id, vec_id;

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

  vec_id = 0x11;
  for( i = 0; i < 3; i++)
  {
    pev_csr_wr( 0x8000040c, 0x1200 | vec_id++);
    while( pev_csr_rd( 0x8000040c) & 0x800);
    pev_csr_wr( 0x8000040c, 0x1300 | vec_id++);
    while( pev_csr_rd( 0x8000040c) & 0x800);
    pev_csr_wr( 0x8000040c, 0x1400 | vec_id++);
    while( pev_csr_rd( 0x8000040c) & 0x800);
    pev_csr_wr( 0x8000040c, 0x1500 | vec_id++);
    while( pev_csr_rd( 0x8000040c) & 0x800);
    pev_csr_wr( 0x8000040c, 0x1600 | vec_id++);
    while( pev_csr_rd( 0x8000040c) & 0x800);
  }
  pev_csr_wr( 0x8000040c, 0x11ff);

  pev_exit( pev);

  exit(0);
}
