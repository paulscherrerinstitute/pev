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
 * $Log: EvtTst.c,v $
 * Revision 1.6  2012/09/04 07:34:34  kalantari
 * added tosca driver 4.18 from ioxos
 *
 * Revision 1.4  2012/07/10 09:49:07  ioxos
 * check 16 sources from user area [JFG]
 *
 * Revision 1.3  2012/06/28 13:41:45  ioxos
 * use USR1 as interrupt sources [JFG]
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
typedef void (*sighandler_t)(int);
int ivec[256];
int icnt[256];

int evt_sig, evt_cnt;
struct pev_ioctl_evt *evt;

void
 myhandler( int sig)
{
  int cnt;
  printf("in myhandler...%d\n", sig);
  do
  {
    pev_evt_read( evt, 0);
    cnt = evt->evt_cnt;
    if( evt->src_id)
    {
      //usleep(1000);
      ivec[evt_cnt] = evt->vec_id;
      icnt[evt_cnt] = cnt;
      evt_cnt++;
      //printf("%x - %x - %d - %d - %d\n", evt->src_id, evt->vec_id, evt->evt_cnt, evt_cnt, cnt);
      pev_evt_unmask( evt, evt->src_id);
      //if( evt->vec_id == 0xff) evt_sig = 0;
      if( evt->src_id == 0x4f) evt_sig = 0;
    }
    else
    {
      printf("evt queue empty...%d - %d\n", evt_cnt, cnt);
    }
  } while( cnt);
  return;
}



int
main( int argc,
      char **argv)
{
  int i;
  int src_id;

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

  evt = pev_evt_queue_alloc( SIGUSR2);
  //src_id = 0x10;
  src_id = 0x40;
  for( i = 0; i < 16; i++)
  {
    pev_evt_register( evt, src_id++);
  }
  evt->wait = -1;
  evt_sig = 1;
  evt_cnt = 0;
  signal( evt->sig, myhandler);
  printf("waiting for signal %d...\n", evt->sig);
  pev_evt_queue_enable( evt);
  while( evt_sig)
  {
    usleep(100000);
    //myhandler(0);
  }
  for( i = 0; i < evt_cnt; i++)
  {
    printf("%3d - %02x - %2d\n", i, ivec[i], icnt[i]);
  }
  pev_evt_queue_disable( evt);
  pev_evt_queue_free( evt);
  printf("evt_cnt = %d\n", evt_cnt);

  pev_exit( pev);

  exit(0);
}
