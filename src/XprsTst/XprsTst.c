/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : PevTst.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2009
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
 * $Log: XprsTst.c,v $
 * Revision 1.9  2012/09/04 07:34:34  kalantari
 * added tosca driver 4.18 from ioxos
 *
 * Revision 1.6  2012/06/01 14:00:06  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.5  2012/03/21 10:14:18  ioxos
 * XprsTst .c replaces PevTst.c [JFG]
 *
 * Revision 1.6  2012/03/05 09:31:04  ioxos
 * support for execution on PPC [JFG]
 *
 * Revision 1.5  2010/06/11 11:56:26  ioxos
 * add test status report [JFG]
 *
 * Revision 1.4  2009/12/15 17:18:07  ioxos
 * modification for short io window + tst_09 [JFG]
 *
 * Revision 1.3  2009/12/02 15:14:11  ioxos
 * use multicrate library [JFG]
 *
 * Revision 1.2  2009/08/25 13:14:01  ioxos
 * cleanup + move cmd handling function from tstlib to here [JFG]
 *
 * Revision 1.1  2009/08/18 13:21:49  ioxos
 * first cvs checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: XprsTst.c,v 1.9 2012/09/04 07:34:34 kalantari Exp $";
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
#include <sys/mman.h>
#include <time.h>
#include <signal.h>

#include <pevioctl.h>
#include <pevulib.h>
#include <pevxulib.h>
#include <cli.h>

#include <xprstst.h>
#include <tstlib.h>
#include <tstxlib.h>
#include "tstlist.h"

#define DEBUGnono

void tst_signal( int);
int tst_init( struct xprstst *);
void tst_exit( struct xprstst *);

struct cli_cmd_para cmd_para;
struct pev_node *pev;
char cmdline[0x101];
int fd_in, fd_out;
int cmd_pending = 0;
struct tst_ctl tst_ctl;
char log_filename[0x101];
int debug = 0;
struct pev_reg_remap *reg_remap;
char XprsTst_version[] = "1.00";

char *
XprsTst_rcsid()
{
  return( rcsid);
}

void
tst_signal( int signum)
{
  int n;

  n = read( fd_in, cmdline, 0x100);
  if( n <= 0)
  {
      printf("PevTst->Exiting:Connexion lost with XprsTst");
      strcpy( cmdline, "exit");
  }
  else
  {
    cmdline[n] = 0;
    cmd_pending = 1;
  }
}

int
tst_init( struct xprstst *xt)
{
  xt->pev = (struct pev_node *)pevx_init( xt->pev_para.crate);
  if( !xt->pev)
  {
    printf("Cannot allocate data structures to control PEV1100\n");
    return( -1);
  }
  if( xt->pev->fd < 0)
  {
    printf("Cannot find PEV1100 interface\n");
    exit( -1);
  }
  reg_remap = pevx_io_remap();

  if( tstx_cpu_map_shm( xt->pev_para.crate, &xt->cpu_map_shm, 0, 0) == MAP_FAILED)
  {
    return( -1);
  }

  if( tstx_cpu_map_vme( xt->pev_para.crate, &xt->cpu_map_vme_shm, 0, 0, 0) == MAP_FAILED)
  {
    return( -1);
  }

  if( tstx_cpu_map_kbuf( xt->pev_para.crate, &xt->cpu_map_kbuf, 0x0) == MAP_FAILED)
  {
    return( -1);
  }

  if( tstx_cpu_map_vme( xt->pev_para.crate, &xt->cpu_map_vme_kbuf, 0, 0, 0) == MAP_FAILED)
  {
    return( -1);
  }

  return( 0);
}

int 
tst_status( struct cli_cmd_para *c)
{
  write( fd_out, &tst_ctl, sizeof( tst_ctl));  
  return(0);
}

int 
tst_set( struct cli_cmd_para *c)
{
  int i, cnt;
  struct tst_ctl *tc;

  tc = &tst_ctl;

  cnt = c->cnt;
  i = 0;
  while( cnt--)
  {
    int tmp;
    printf("%s\n", c->para[i]);
    if( sscanf( c->para[i], "loop=%d", &tmp) == 1)
    {
      tc->loop_mode = tmp;
    }
    if( !strncmp( c->para[i], "log=off", 7) == 1)
    {
      if( tc->log_mode != TST_LOG_OFF)
      {
        TST_LOG( tc, (logline, "PevTst->LoggingOff:%s\n", tc->log_filename));
	fclose( tc->log_file);
      }
      tc->log_mode = TST_LOG_OFF;
    }
    if( !strncmp( c->para[i], "log=new", 7) == 1)
    {
      if( tc->log_mode != TST_LOG_OFF)
      {
	fclose( tc->log_file);
      }
      tc->log_file = fopen( tc->log_filename, "w");
      if( !tc->log_file)
      {
	tc->log_mode = TST_LOG_OFF;
      }
      else
      {
        TST_LOG( tc, (logline, "PevTst->LoggingNew:%s\n", tc->log_filename));
      }
      tc->log_mode = TST_LOG_NEW;
    }
    if( !strncmp( c->para[i], "log=add", 7) == 1)
    {
      if( tc->log_mode == TST_LOG_OFF)
      {
         tc->log_file = fopen( tc->log_filename, "a");
         if( !tc->log_file)
         {
	   tc->log_mode = TST_LOG_OFF;
         }
         else
         {
            TST_LOG( tc, (logline, "PevTst->LoggingAdd:%s\n", tc->log_filename));
         }
      }
      tc->log_mode = TST_LOG_ADD;
    }
    if( sscanf( c->para[i], "logfile=%s", log_filename) == 1)
    {
      strcpy( log_filename, &c->para[i][8]);
      printf("log_filename=%s\n", log_filename);
    }
    if( !strncmp( c->para[i], "err=cont", 6) == 1)
    {
      tc->err_mode = TST_ERR_CONT;
    }
    if( !strncmp( c->para[i], "err=halt", 6) == 1)
    {
      tc->err_mode = TST_ERR_HALT;
    }
    if( !strncmp( c->para[i], "exec=fast", 8) == 1)
    {
      tc->exec_mode = TST_EXEC_FAST;
    }
    if( !strncmp( c->para[i], "exec=val", 8) == 1)
    {
      tc->exec_mode = 0;
    }
    i++;
  }
  return(0);
}

int 
tst_start( struct cli_cmd_para *c)
{
  struct tst_list *t;
  int first, last;
  int i, cnt, loop;
  int iex;

  cnt = c->cnt;
  i = 0;
  if( cnt > 0)
  {
    tst_get_range( c->para[0], &first, &last);
    i++;
    cnt--;
  }
  else
  {
    first = 1;
    last = 0xff;
  }
  tst_ctl.para_cnt = cnt;
  tst_ctl.para_p = &c->para[i];

  printf("PevTst->Starting test %x..%x\n", first, last);
  iex = 0;
  loop = tst_ctl.loop_mode;
  do
  {
    t = &tst_list[0];
    while( t->idx)
    {
      if( ( t->idx >= first) && ( t->idx <= last))
      {
	tst_ctl.test_idx = t->idx;
	tst_ctl.status = TST_STS_STARTED;
        tst_ctl.status = t->func( &tst_ctl);
	t->status = tst_ctl.status;
	if( ( tst_ctl.status & TST_STS_ERR) &&
	    ( tst_ctl.err_mode & TST_ERR_HALT))
	{
	  iex = 1;
	  break;
	}

      }
      if( cmd_pending)
      { 
        if( !strncmp( cmdline, "tstop", 5))
        {
	  printf("PevTst->Stopping test %x\n", t->idx);
	  iex = 1;
	  break;
	}
      }
      t++;
    }
  } while( --loop && !iex);
  return(0);
}

int 
tst_tlist( struct cli_cmd_para *c)
{
  struct tst_list *t;
  int first, last;
  int i, cnt;

  cnt = c->cnt;
  i = 0;
  if( cnt > 0)
  {
    tst_get_range( c->para[0], &first, &last);
    i++;
    cnt--;
  }
  else
  {
    first = 1;
    last = 0xff;
  }

  i = 0;
  t = &tst_list[0];
  while( t->idx)
  {
    if( ( t->idx >= first) && ( t->idx <= last))
    {
      printf( "Tst:%02x ", t->idx);
      if( t->status)
      {
	if( t->status & TST_STS_STOPPED)
	{
	  printf( "[STP]");
	}
	else if( t->status & TST_STS_ERR)
	{
  	  printf( "[NOK]");
	}
	else
	{
  	  printf( "[ OK]");
	}
      }
      else
      {
	printf( "[NEX]");
      }
      printf( " -> %s\n", t->msg[0]);

    }
    t++;
  }
  return(0);
}


void
tst_exit( struct xprstst *xt)
{
  pevx_munmap( xt->pev_para.crate, &xt->cpu_map_shm);
  munmap( xt->cpu_map_kbuf.u_addr, xt->cpu_map_kbuf.size);
  pevx_exit( xt->pev_para.crate);
}

int
tst_get_cmd_pending()
{
  return( cmd_pending);
}

char *
tst_get_cmdline()
{
  return( cmdline);
}

int
tst_check_cmd_tstop()
{
  if( cmd_pending)
  { 
    if( !strncmp( cmdline, "tstop", 5))
    {
      return( 1);
    }
    if( !strncmp( cmdline, "tstatus", 7))
    {
      tst_status( 0);
      cmd_pending = 0;
      return( 0);
    }
  }
  return( 0);
}

int
main( int argc,
      char *argv[])
{
  char data[64];
  int pid;
  FILE *cfg_file;
  time_t tm;
  struct tst_ctl *tc;
  struct xprstst *xt;
#ifdef DEBUG
  debug = 1;
#endif

  if( argv[4])
  {
    //freopen("/dev/pts/0", "r+", stdout);
    freopen( argv[4], "r+", stdout);
  }

  tm = time(0);
  //printf("arg = %s - %s - %s - %s - %s\n", argv[0], argv[1], argv[2], argv[3], argv[4]);
  signal( SIGUSR1, tst_signal);

  xt = (struct xprstst *)malloc( sizeof( struct xprstst));
  tc = &tst_ctl;
  tst_ctl.xt = xt;
  printf("PevTst->Entering:%s", ctime(&tm));
  printf("PevTst->Initialization");

  cfg_file = fopen( argv[1], "r");
  if( !cfg_file)
  {
    printf("->Error:cannot open configuration file %s\n", argv[1]);
    goto tst_exit;
  }
  fread( xt, sizeof( struct xprstst), 1, cfg_file);
  fclose( cfg_file);
  fd_in = atoi( argv[2]);
  fd_out = atoi( argv[3]);

  tst_init( xt);
  pid = getpid();
  sprintf( data, "%s %d", argv[0], pid);
  write( fd_out, data, strlen( data));  
  printf("->Done\n");
  printf("PevTst->Board type : %s\n", pevx_board_name());
  printf("PevTst->Version %s - %s %s\n", XprsTst_version, __DATE__, __TIME__);

  if( debug) printf("arg = %s - %s - %s - %s\n", argv[0], argv[1], argv[2], argv[3]);
  if( debug) printf("shared memory local address = %lx\n", xt->cpu_map_shm.loc_addr);
  if( debug) printf("shared memory user address = %p - %x\n", xt->cpu_map_shm.usr_addr, *(int *)xt->cpu_map_shm.usr_addr);
  if( debug) printf("DMA buffer user address    = %p - %x\n", xt->cpu_map_kbuf.u_addr, *(int *)xt->cpu_map_kbuf.u_addr);
  tst_ctl.loop_mode = 1;
  tst_ctl.log_mode = TST_LOG_OFF;
  tst_ctl.err_mode = TST_ERR_CONT;
  strcpy(log_filename, "PevTst.log");
  tc->log_filename = log_filename;
  tst_ctl.exec_mode = TST_EXEC_FAST;
  
  while( 1)
  {
    if( !cmd_pending)
    {
      pause();
    }
    cmd_pending = 0;
    TST_LOG( tc, (logline, "PevTst->Command:%s\n", cmdline));
    cli_cmd_parse( cmdline, &cmd_para);
    if( !strncmp( cmd_para.cmd, "exit", 4))
    {
      TST_LOG( tc, (logline, "PevTst->Exiting:XprsTst command"));
      break;
    }

    if( !strncmp( cmd_para.cmd, "tstart", 6))
    {
      tst_start( &cmd_para);
    }
    if( !strncmp( cmd_para.cmd, "tset", 6))
    {
      tst_set( &cmd_para);
    }
    if( !strncmp( cmd_para.cmd, "tlist", 6))
    {
      tst_tlist( &cmd_para);
    }
    if( !strncmp( cmd_para.cmd, "tstatus", 7))
    {
      tst_status( &cmd_para);
    }
  }
  tst_exit( xt);
  TST_LOG( tc, (logline, "->Done\n"));
tst_exit:
  close( fd_in);
  close( fd_out);
  if( tc->log_file)
  {
    fclose( tc->log_file);
  }
  exit(0);
}
