/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : XprsMon.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
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
 * $Log: XprsMon.c,v $
 * Revision 1.10  2012/10/01 14:56:49  kalantari
 * added verion 4.20 of tosca-driver from IoxoS
 *
 * Revision 1.43  2012/09/27 11:49:51  ioxos
 * tagging 4.20 [JFG]
 *
 * Revision 1.42  2012/09/04 13:33:15  ioxos
 * release 4.19 [JFG]
 *
 * Revision 1.41  2012/09/03 13:54:32  ioxos
 * tagging release 4.18 [JFG]
 *
 * Revision 1.40  2012/08/28 13:59:46  ioxos
 * release 4.17 [JFG]
 *
 * Revision 1.39  2012/08/08 08:05:00  ioxos
 * set release 4.16 [JFG]
 *
 * Revision 1.38  2012/07/10 09:47:45  ioxos
 * rel 4.15 [JFG]
 *
 * Revision 1.37  2012/06/28 14:01:11  ioxos
 * set release 4.14 [JFG]
 *
 * Revision 1.36  2012/06/06 15:26:01  ioxos
 * release 4.13 [JFG]
 *
 * Revision 1.35  2012/06/01 13:59:44  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.34  2012/04/19 08:40:39  ioxos
 * tagging rel-4-10 [JFG]
 *
 * Revision 1.33  2012/04/18 07:51:29  ioxos
 * release 4.09 [JFG]
 *
 * Revision 1.32  2012/04/10 08:32:03  ioxos
 * version 4.08 [JFG]
 *
 * Revision 1.31  2012/03/27 11:47:47  ioxos
 * set version to 4.07 [JFG]
 *
 * Revision 1.30  2012/03/21 14:43:20  ioxos
 * set software revision to 4.06 [JFG]
 *
 * Revision 1.29  2012/03/21 11:26:18  ioxos
 * update copyright [JFG]
 *
 * Revision 1.28  2012/03/15 15:12:16  ioxos
 * set version to 4.05 [JFG]
 *
 * Revision 1.27  2012/02/28 16:08:52  ioxos
 * set release to 4.04 [JFG]
 *
 * Revision 1.26  2012/02/14 16:18:44  ioxos
 * release 4.03 [JFG]
 *
 * Revision 1.25  2012/02/03 16:27:37  ioxos
 * dynamic use of elbc for i2c [JFG]
 *
 * Revision 1.24  2012/01/30 11:16:23  ioxos
 * cosmetics [JFG]
 *
 * Revision 1.23  2012/01/27 15:55:43  ioxos
 * prepare release 4.01 supporting x86 & ppc [JFG]
 *
 * Revision 1.22  2012/01/06 14:40:31  ioxos
 * release 3.13 [JFG]
 *
 * Revision 1.21  2011/12/06 14:42:53  ioxos
 * support for multi task VME IRQ [JFG]
 *
 * Revision 1.20  2011/10/19 13:37:51  ioxos
 * release 3.11 [JFG]
 *
 * Revision 1.19  2011/10/19 13:36:30  ioxos
 * support for IPV1102 [JFG]
 *
 * Revision 1.18  2011/10/03 10:01:05  ioxos
 * release 1.10 [JFG]
 *
 * Revision 1.17  2011/03/15 09:26:26  ioxos
 * support for MPC1200 [JFG]
 *
 * Revision 1.16  2011/03/03 15:42:38  ioxos
 * support for 1MBytes VME slave granularity [JFG]
 *
 * Revision 1.15  2010/06/11 11:50:55  ioxos
 * dont exit id PEV not found [JFG]
 *
 * Revision 1.14  2009/12/15 17:16:21  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.13  2009/10/02 07:59:14  ioxos
 * Copyright 2009 [CM]
 *
 * Revision 1.12  2009/08/25 14:34:01  ioxos
 * tkill test program if needed when exiting [JFG]
 *
 * Revision 1.11  2009/08/25 13:39:12  ioxos
 * export cmdline variable [JFG]
 *
 * Revision 1.10  2009/07/17 14:13:08  ioxos
 * allow for script execution when launching XprsMon [JFG]
 *
 * Revision 1.9  2009/06/03 12:29:32  ioxos
 * check driver identification [JFG]
 *
 * Revision 1.8  2009/05/20 14:59:43  ioxos
 * add support for XENOMAI real time driver [JFG]
 *
 * Revision 1.7  2009/04/06 12:39:09  ioxos
 * support for multicrate [JFG]
 *
 * Revision 1.6  2008/12/12 14:11:36  jfg
 * compare command with length of entry [JFG]
 *
 * Revision 1.5  2008/11/12 14:02:15  ioxos
 * add call to vme_init() function [JFG]
 *
 * Revision 1.4  2008/09/17 13:16:48  ioxos
 * init tst list + suppor loop mode [JFG]
 *
 * Revision 1.3  2008/08/08 11:46:13  ioxos
 * parse script arguments [JFG]
 *
 * Revision 1.2  2008/07/18 14:10:48  ioxos
 * add support tu run script [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char rcsid[] = "$Id: XprsMon.c,v 1.10 2012/10/01 14:56:49 kalantari Exp $";
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

#ifdef XENOMAI
#include <pevrtlib.h>
#include <rtdm/rtdm.h>
#include <native/task.h>

#define DEVICE_NAME		"pev_rt"
int rt_fd;
RT_TASK rt_task_desc;

#endif

typedef unsigned int u32;
#include <pevioctl.h>
#include <cli.h>
#include <pevulib.h>

#include "cmdlist.h"


char XprsMon_version[] = "4.20";

int xprs_cmd_exec( struct cli_cmd_list *, struct cli_cmd_para *);

struct termios termios_old;
struct termios termios_new;
char cli_prompt[16];
struct cli_cmd_para cmd_para;
struct cli_cmd_history cmd_history;
struct pev_node *pev;
struct aiocb aiocb;
char aio_buf[256];
char *cmdline;
struct pev_reg_remap *reg_remap;
uint pev_board_id;

char *
XprsMon_rcsid()
{
  return( rcsid);
}

int
main( int argc,
      char *argv[])
{
  struct cli_cmd_history *h;
  struct winsize winsize;
  int iex, ret;
  uint crate;
  char *drv_id;


  crate = 0;
  if( argc > 1)
  {
    sscanf(argv[1], "%d", &crate);
  }
  printf("initializing crate %d \n", crate);
  pev = pev_init( crate);
  if( !pev)
  {
    printf("Cannot allocate data structures to control PEV1100\n");
    exit( -1);
  }
  if( pev->fd < 0)
  {
    printf("Cannot find PEV1100 interface\n");
    //exit( -1);
  }
  printf("\n");
  printf("     +-----------------------------------------+\n");
  printf("     |  XprsMon - %s diagnostic tool      |\n", pev_board_name());
  printf("     |  IOxOS Technologies Copyright 2009-2012 |\n");
  printf("     |  Version %s - %s %s    |\n", XprsMon_version, __DATE__, __TIME__);
  printf("     +-----------------------------------------+\n");
  printf("\n");

  drv_id =  pev_id();
  printf("Device driver: %s\n", drv_id);
  reg_remap = pev_io_remap();

#ifdef XENOMAI
  if( strcmp( drv_id, "pev-xeno"))
  {
    printf("To run XprsMonRt> you need to have the pev-xeno driver installed !!!\n");
    exit( -1);
  }
  rt_fd = pev_rt_init( crate);
  if( rt_fd < 0)
  {
    printf("Cannot find real time PEV1100 interface\n");
    exit( -1);
  }
#else
  if( strcmp( drv_id, "pev-linux"))
  {
    printf("To run XprsMon> you need to have the pev-linux driver installed !!!\n");
    exit( -1);
  }
#endif
  rdwr_init();
  vme_init();
  tst_init();
#ifdef MPC
  mpc_init();
#endif

  ioctl( 0, TIOCGWINSZ, &winsize);
  //printf("winsize = %d * %d\n", winsize.ws_row, winsize.ws_col);
  tcgetattr( 0, &termios_old);
  memcpy( &termios_new, &termios_old, sizeof( struct termios)); 
  termios_new.c_lflag &= ~(ECHOCTL | ECHO | ICANON);
  tcsetattr( 0, TCSANOW, &termios_new);
  h = cli_history_init( &cmd_history);

  if( argc > 2)
  {
    struct cli_cmd_para script_para;

    if( argv[2][0] == '@')
    {
      cli_cmd_parse(  &argv[2][1], &script_para);
      iex = xprs_script( &argv[2][1], &script_para);
      if( iex)
      {
        goto XprsMon_exit;
      }
    }
    else
    {
      cli_cmd_parse(  argv[2], &cmd_para);
      xprs_cmd_exec( &cmd_list[0], &cmd_para);
      goto XprsMon_exit;
    }
  }

#ifdef XENOMAI
  sprintf(cli_prompt, "XprsMonRt#%d>", crate);
#else
  sprintf(cli_prompt, "XprsMon#%d>", crate);
#endif
  while(1)
  {
    cmdline = cli_get_cmd( h, cli_prompt);
    if( cmdline[0] == 'q')
    {
      break;
    }
    if( cmdline[0] == '@')
    {
      struct cli_cmd_para script_para;

      cli_cmd_parse(  &cmdline[1], &script_para);
      iex = xprs_script( &cmdline[1], &script_para);
      if( iex == 1)
      {
	break;
      }
      continue;
    }
    if( cmdline[0] == '&')
    {
      printf("entering loop mode [enter any character to stop loop]...\n");
      if( aio_error( &aiocb) != EINPROGRESS)
      {
        ret = aio_read( &aiocb);
        if( ret < 0)
        {
          perror("aio_read");
          goto XprsMon_end_loop;
        }
      }
      while( 1)
      {
	cli_cmd_parse( &cmdline[1], &cmd_para);
        if( xprs_cmd_exec( &cmd_list[0], &cmd_para) < 0)
	{
          if( aio_error( &aiocb) == EINPROGRESS)
          {
            ret = aio_cancel( aiocb.aio_fildes, &aiocb);
	  }
	  goto XprsMon_end_loop;
	}
        if( aio_error( &aiocb) != EINPROGRESS)
        {
          aio_return( &aiocb);
	  goto XprsMon_end_loop;
        }
      }
XprsMon_end_loop:
      continue;
    }
    cli_cmd_parse( cmdline, &cmd_para);
    if( cmdline[0] == '?')
    {
      xprs_func_help( &cmd_para);
      continue;
    }
    xprs_cmd_exec( &cmd_list[0], &cmd_para);
  }

XprsMon_exit:
  tcsetattr( 0, TCSANOW, &termios_old);
#ifdef MPC
  mpc_exit();
#endif
  tst_exit();
  rdwr_exit();
#ifdef XENOMAI
  pev_rt_exit();
#endif
  pev_exit( pev);
  exit(0);
}

int
xprs_cmd_exec( struct cli_cmd_list *l,
	       struct cli_cmd_para *c)
{
  long i;

  i = 0;
  if( strlen(c->cmd))
  {
    while(1)
    {
      if( !l->cmd)
      {
        break;
      }
      if( !strncmp( l->cmd, c->cmd, strlen( c->cmd)))
      {
        c->idx = i;
        return( l->func( c));
      }
      i++; l++;
    }
    printf("%s -> invalid command name ", c->cmd);
    printf("(\'help\' or \'?\' displays a list of valid commands)\n");
  }
  return(-1);
}

int 
xprs_func_xxx( struct cli_cmd_para *c)
{
  printf("entering mon_func_xxx()\n");
  return(0);
}


int 
xprs_func_help( struct cli_cmd_para *c)
{
  char *cmd;
  long i, j;

  i = 0;
  if( c->cnt > 0)
  {
    while(1)
    {
      cmd = cmd_list[i].cmd;
      if( cmd)
      {
        if( !strcmp( c->para[0], cmd))
        {
	  long j;

	  j = 0;
	  while( cmd_list[i].msg[j])
	  {
	    printf("%s\n", cmd_list[i].msg[j]);
	    j++;
	  }
	  return(0);
        }
	i++;
      }
      else
      {
	printf("%s -> invalid command name ", c->para[0]);
	printf("(\'help\' or \'?\' displays a list of valid commands)\n");
	return(-1);
      }
    }
  }

  while(1)
  {
    cmd = cmd_list[i++].cmd;
    if( cmd)
    {
      printf("%s", cmd);
      if( i&3)
      {
	j = 10 - strlen( cmd);
	while(j--)
	{
	  putchar(' ');
	}
      }
      else
      {
	putchar('\n');
      }
    }
    else
    {
      putchar('\n');
      break;
    }
  }
  return(0);
}
