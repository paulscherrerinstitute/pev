/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : timer.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to control
 *     PEV1000 timer.
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
 * $Log: timer.c,v $
 * Revision 1.9  2012/09/04 07:34:33  kalantari
 * added tosca driver 4.18 from ioxos
 *
 * Revision 1.5  2012/06/01 13:59:44  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.4  2009/05/20 14:56:24  ioxos
 * add support for XENOMAI real time driver [JFG]
 *
 * Revision 1.3  2009/04/15 13:00:25  ioxos
 * base frequency for timer is now 100 MHz [JFG]
 *
 * Revision 1.2  2009/04/06 12:38:13  ioxos
 * set timer mode to 125 MHz [JFG]
 *
 * Revision 1.1  2008/11/12 12:25:01  ioxos
 * first cvs checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: timer.c,v 1.9 2012/09/04 07:34:33 kalantari Exp $";
#endif

#define DEBUGno
#include <debug.h>

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cli.h>
#include <pevioctl.h>
#include <pevulib.h>

#ifdef XENOMAI
#define pev_timer_read(x) pev_rt_timer_read(x)
#endif


char *
timer_rcsid()
{
  return( rcsid);
}

int 
xprs_timer( struct cli_cmd_para *c)
{

  int cnt, i;

  cnt = c->cnt;
  i = 0;

  if( !cnt)
  {
    uint msec;

    msec = pev_timer_read( 0);
    printf("current timer value : %d msec\n", msec);
    return(0);;
  }

  if( cnt--)
  {
    if( !strcmp( "start", c->para[i]))
    {
      uint msec;
      uint mode;

      mode = 3;
      msec = 0;

      pev_timer_start( mode, msec);
      return(0); 
    }
    if( !strcmp( "restart", c->para[i]))
    {
      pev_timer_restart();
      return(0); 
    }
    if( !strcmp( "stop", c->para[i]))
    {
      pev_timer_stop();
      return(0); 
    }
    if( !strcmp( "read", c->para[i]))
    {
      struct pev_time tm;

      pev_timer_read( &tm);
      tm.utime &= 0x1ffff;
      printf("current timer value : %d.%06d msec\n", tm.time, tm.utime*10);
      return(0); 
    }
  }

  return(0);
}

