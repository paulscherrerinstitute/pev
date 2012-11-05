/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : TimerTst.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : october 10,2012
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *  This test programe uses the tick timer located in the VME interface as
 *  source to mesure the interrupt response time (latency).
 *  The tick timer generates an interrupt every msec
 *  A 100 MHz micro counter is restarted at every tick
 *  every time a tick interrupt is detect by Linux, a kernel interrupt handler 
 *  is activated:
 *  - read the micro timer current value ( -> build histo #0)
 *  - read tick counter to track lost interrupts ( -> build histo #3)
 *  - register event in event queue
 *  - read again the micro timer current value ( -> build histo #1)
 *  - signal SIGUSR2
 * - re-enable tick interrupts
 * This test program install a user handler myhandler() waiting to be waked-up 
 * by SIGUSR2. When activated:
 *  - read the micro timer current value ( -> build histo #2 and #4)
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
 * $Log: TimerTst.c,v $
 * Revision 1.1  2012/11/05 13:00:43  kalt_r
 * received 2012-11-05 from Jean-Francois, to be used with define HISTO in ./drivers/pevklib.c
 *
 *=============================< end file header >============================*/
#include <stdlib.h>
#include <stdio.h>
#include <pty.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <pevioctl.h>
#include <pevulib.h>

struct pev_node *pev;
typedef void (*sighandler_t)(int);
int ivec[256];
int icnt[256];
int max_cnt = 100;

int evt_sig, evt_cnt;
struct pev_ioctl_evt *evt;

/* declare 4 histograms */
struct pev_ioctl_histo hst0_tmr;
struct pev_ioctl_histo hst1_tmr;
struct pev_ioctl_histo hst2_tmr;
struct pev_ioctl_histo hst3_tmr;
struct pev_ioctl_histo hst4_tmr;
int src_id;
  struct pev_time tmi, tmo;

/* interrupt handler */
/* enter here when linux signal SIGUSR2 has been set */
void
myhandler( int sig)
{
  int cnt, clk;
  struct pev_time tm;
  int chan;

  //printf("in myhandler...%d\n", sig);
  do
  {
    /* read next event from event queue (no wait) */
    pev_evt_read( evt, 0);
    cnt = evt->evt_cnt;
    /* check if queue not empty (more then 1 event in queue */
    if( cnt)
    {
      //printf("evt queue not empty...%d - %d\n", evt_cnt, cnt);
    }
    /* check source id of event just read out */
    if( evt->src_id)
    {
      evt_cnt++; /* increment eventcount */
      pev_timer_read( &tm); /* read currnt value of micro timer */
      /* update histo #2 (user latency 100 nsec) */
      chan = (tm.utime & 0x1ffff)/10; /* 1 channel = 100 nsec */
      if( chan > 998) chan = 998;     /* keep overflow in channel 994 */
      if(cnt) chan = 999;
      hst2_tmr.histo[chan] += 1;
      hst2_tmr.histo[1000] += 1;
      /* update histo #4 (user latency usec) */
      chan = (tm.utime & 0x1ffff)/100;           /* 1 channel = 1 usec */
      hst4_tmr.histo[1000] += 1;                 /* keep event count in channel 1000 */
      clk = tm.time - tmi.time;                  /* calculate current timer value (in msec) */
      hst4_tmr.histo[1000 + clk - evt_cnt] += 1; /* update channel keeping overflow counter */
#ifdef JFG
      /* verify if event have been missed (event counter != clock counter */
      if( clk > evt_cnt + cnt + 1)
      {
        printf("evt missed...%d + %d  -> %d\n", evt_cnt, cnt, clk);
        pev_evt_mask( evt, src_id);
	evt_sig = 0;               /* set exit flag */
      }
#endif
      /* exit if expected number of event has been reached */
      if( evt_cnt >= max_cnt)
      {
        pev_evt_mask( evt, src_id); /* mask events */
	evt_sig = 0;                /* set exit flag */
      }
    }
    /* event queue was empty (no event) */
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
  FILE *h_file;

  printf("Entering interrupt latency test program...");
  pev = pev_init( 0);
  if( !pev)
  {
    printf("\nCannot allocate data structures to control PEV1100\n");
    exit( -1);
  }
  if( pev->fd < 0)
  {
    printf("\nCannot find PEV1100 interface\n");
    exit( -1);
  }

  if( argc > 1)
  {
    sscanf( argv[1], "%d", &max_cnt);
  }
  printf("max_cnt = %d\n", max_cnt);

  evt = pev_evt_queue_alloc( SIGUSR2);
  src_id = 0x1a; /* source identifier = tick timer */
  //src_id = 0x40;
  pev_evt_register( evt, src_id);
  evt->wait = -1;
  evt_sig = 1;
  evt_cnt = 0;
  signal( evt->sig, myhandler);

  /* histo #0 track kernel interrupt latency (100 nsec/channel) */
  hst0_tmr.idx = 0;
  hst0_tmr.size = 1024;
  hst0_tmr.histo = malloc( hst0_tmr.size * sizeof(int));
  ioctl( pev->fd, PEV_IOCTL_HISTO_CLEAR, &hst0_tmr);


  /* histo #1 track kernel interrupt exit latency (100 nsec/channel) */
  hst1_tmr.idx = 1;
  hst1_tmr.size = 1024;
  hst1_tmr.histo = malloc( hst1_tmr.size * sizeof(int));
  ioctl( pev->fd, PEV_IOCTL_HISTO_CLEAR, &hst1_tmr);

  /* histo #2 track user interrupt latency (100 nsec/channel) */
  hst2_tmr.idx = 2;
  hst2_tmr.size = 1024;
  hst2_tmr.histo = malloc( hst2_tmr.size * sizeof(int));
  for( i = 0; i < hst2_tmr.size; i++)
  { 
    hst2_tmr.histo[i] = 0;
  }

  /* histo #3 track timing of lost interrupts (1 msec/channel) */
  hst3_tmr.idx = 3;
  hst3_tmr.size = 1024;
  hst3_tmr.histo = malloc( hst3_tmr.size * sizeof(int));
  ioctl( pev->fd, PEV_IOCTL_HISTO_CLEAR, &hst3_tmr);

  /* histo #4 track user interrupt latency (1 usec/channel) */
  hst4_tmr.idx = 4;
  hst4_tmr.size = 1024;
  hst4_tmr.histo = malloc( hst4_tmr.size * sizeof(int));
  for( i = 0; i < hst4_tmr.size; i++)
  { 
    hst4_tmr.histo[i] = 0;
  }

  printf("waiting for signal %d...\n", evt->sig);
  pev_timer_read( &tmi);            /* read timer reference (initial value) */
  pev_evt_queue_enable( evt);       /* enable event queue                   */
  /* wait until number of events reached or error */
  while( evt_sig)
  {
    usleep(100000);
    //myhandler(0);
  }
  pev_evt_queue_disable( evt);     /* disable event queue                   */
  pev_timer_read( &tmo);           /* read timer reference (final value) */
  pev_evt_queue_free( evt);

  /* print number of tick -> should be equal to event count */
  printf("timer_cnt = %d - %d [%d]\n", tmi.time, tmo.time, tmo.time-tmi.time);

  ioctl( pev->fd, PEV_IOCTL_HISTO_READ, &hst0_tmr);     /* read histo #0 */
  ioctl( pev->fd, PEV_IOCTL_HISTO_READ, &hst1_tmr);     /* read histo #1 */
  ioctl( pev->fd, PEV_IOCTL_HISTO_READ, &hst3_tmr);     /* read histo #3 */
  pev_exit( pev);

  /* save histo #0 in file hst0.his */
  h_file = fopen( "hst0.his", "w");
  if( !h_file)
  {
    printf("cannot open file hst0.his\n");
    exit(0);
  }
  for( i = 0; i < 1024; i++)
  {
    fprintf( h_file, "%d %d\n", i, hst0_tmr.histo[i]);
  }
  fclose( h_file);

  /* save histo #1 in file hst1.his */
  h_file = fopen( "hst1.his", "w");
  if( !h_file)
  {
    printf("cannot open file hst1.his\n");
    exit(0);
  }
  for( i = 0; i < 1024; i++)
  {
    fprintf( h_file, "%d %d\n", i, hst1_tmr.histo[i]);
  }
  fclose( h_file);

  /* save histo #2 in file hst2.his */
  h_file = fopen( "hst2.his", "w");
  if( !h_file)
  {
    printf("cannot open file hst2.his\n");
    exit(0);
  }
  for( i = 0; i < 1024; i++)
  {
    fprintf( h_file, "%d %d\n", i, hst2_tmr.histo[i]);
  }
  fclose( h_file);

  /* save histo #3 in file hst3.his */
  h_file = fopen( "hst3.his", "w");
  if( !h_file)
  {
    printf("cannot open file hst3.his\n");
    exit(0);
  }
  for( i = 0; i < 1024; i++)
  {
    fprintf( h_file, "%d %d\n", i, hst3_tmr.histo[i]);
  }
  fclose( h_file);

  /* save histo #4 in file hst4.his */
  h_file = fopen( "hst4.his", "w");
  if( !h_file)
  {
    printf("cannot open file hst3.his\n");
    exit(0);
  }
  for( i = 0; i < 1024; i++)
  {
    fprintf( h_file, "%d %d\n", i, hst4_tmr.histo[i]);
  }
  fclose( h_file);



  exit(0);
}
