/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : XprsTst.c
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
 * Revision 1.1  2012/03/15 14:50:11  kalantari
 * added exact copy of tosca-driver_4.04 from afs
 *
 * Revision 1.3  2009/08/25 13:11:33  ioxos
 * cleanup + accept filenam for tinit [JFG]
 *
 * Revision 1.2  2009/08/20 07:40:28  ioxos
 * cleanup [JFG]
 *
 * Revision 1.1  2009/08/18 13:21:49  ioxos
 * first cvs checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: XprsTst.c,v 1.1 2012/03/15 14:50:11 kalantari Exp $";
#endif

#include <debug.h>

#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <pty.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <aio.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>

#include <pevioctl.h>
#include <cli.h>
#include <pevulib.h>

#include <xprstst.h>
#include <tstlib.h>

int xt_init( struct xprstst *);
void xt_exit( struct xprstst *);
int xt_launch_test( char *);

int pid, X_pid, T_pid;
char *tst_argv[16];

struct termios termios_old;
struct termios termios_new;
char cli_prompt[16];
struct cli_cmd_para cmd_para;
struct cli_cmd_history cmd_history;

struct xprstst *xt;
char *cfg_filename = "XprsTst.cfg";
int fd_p[2][2];
char *cmdline;
int cmd_pending;
int debug = 0;

main( int argc,
      char *argv[])
{
  struct cli_cmd_history *h;
  FILE *cfg_file;

  printf("\n");
  printf("     +-----------------------------------------+\n");
  printf("     |    XprsTst - PEV1100 Test Environment   |\n");
  printf("     |    IOxOS Technologies Copyright 2009    |\n");
  printf("     +-----------------------------------------+\n");
  printf("\n");

#ifdef DEBUG
  debug = 1;
#endif

  tcgetattr( 0, &termios_old);
  memcpy( &termios_new, &termios_old, sizeof( struct termios)); 
  termios_new.c_lflag &= ~(ECHOCTL | ECHO | ICANON);
  tcsetattr( 0, TCSANOW, &termios_new);
  h = cli_history_init( &cmd_history);

  xt = (struct xprstst *)malloc( sizeof( struct xprstst));
  xt->pev_para.crate = 0;
  if( argc > 1)
  {
    sscanf(argv[1], "%d", &xt->pev_para.crate);
  }
  if( xt_init( xt) < 0)
  {
    goto XprsTst_exit;
  }
  tst_argv[0] =  "/usr/bin/xterm";
  tst_argv[1] =  "-e";
  tst_argv[2] =  (char *)malloc( 64);
  tst_argv[3] =  0;

  cfg_file = fopen( cfg_filename, "w");
  if( !cfg_file)
  {
    printf("cannot create configuration file %s\n", cfg_filename);
    goto XprsTst_exit;
  }
  fwrite( xt, sizeof( struct xprstst), 1, cfg_file);
  fclose( cfg_file);
  if( debug)
  {
    *(int *)xt->cpu_map_shm.usr_addr = 0x11223344;
    *(int *)xt->cpu_map_kbuf.u_addr = 0xaabbccdd;
  }
  sprintf(cli_prompt, "XprsTst>");
  T_pid = 0;
  while(1)
  {
    cmdline = cli_get_cmd( h, cli_prompt);
    if( cmdline[0] == 'q')
    {
      if( T_pid)
      {
	write( fd_p[0][1], "tstop", 6);
        kill( T_pid, SIGUSR1);
	usleep(300000);
	write( fd_p[0][1], "exit", 6);
        kill( T_pid, SIGUSR1);
        close( fd_p[0][1]);
        close( fd_p[1][0]);
	kill( T_pid, SIGKILL);
	T_pid = 0;
      }
      break;
    }
    cli_cmd_parse( cmdline, &cmd_para);
    if( !strncmp( cmd_para.cmd, "tinit", 5))
    {
      if( T_pid)
      {
	printf("Test program already running...(see tkill)\n");
      }
      else
      {
	if( !cmd_para.cnt)
	{
	  xt_launch_test("./PevTst");
	}
	else
	{
	  xt_launch_test(cmd_para.para[0]);
	}

      }
    }
    if( ( !strncmp( cmd_para.cmd, "tstart", 6)) ||
        ( !strncmp( cmd_para.cmd, "tset",   4)) ||
        ( !strncmp( cmd_para.cmd, "tlist",  5)) ||
        ( !strncmp( cmd_para.cmd, "tstop",  5))    )
    {
      if( T_pid)
      {
	write( fd_p[0][1], cmdline, strlen( cmdline));
	cmd_pending = 1;
	kill( T_pid, SIGUSR1);
      }
      else
      {
	printf("Test program not loaded...(see tinit)\n");
      }
    }
    if( !strncmp( cmd_para.cmd, "tkill", 5))
    {
      if( T_pid)
      {
	write( fd_p[0][1], "tstop", 6);
        kill( T_pid, SIGUSR1);
	sleep(1);
	write( fd_p[0][1], "exit", 6);
        kill( T_pid, SIGUSR1);
        close( fd_p[0][1]);
        close( fd_p[1][0]);
	kill( T_pid, SIGKILL);
	T_pid = 0;
      }
    }
  }
XprsTst_exit:
  xt_exit( xt);
  tcsetattr( 0, TCSANOW, &termios_old);
  exit(0);

}

int
xt_init( struct xprstst *xt)
{
  ulong vme_base_addr;
  ulong vme_shm_addr;
  ulong vme_kbuf_addr;
  ulong pci_kbuf_addr;

  xt->pev = pev_init( xt->pev_para.crate);
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

  vme_base_addr = (ulong)tst_vme_conf_read( &xt->vme_conf);
  if( debug) printf("VME base address = %x\n", vme_base_addr);

  if( tst_cpu_map_shm( &xt->cpu_map_shm, 0, 0x100000) == MAP_FAILED)
  {
    printf("cannot map SHM in user space\n");
    return( -1);
  }

  vme_shm_addr = tst_vme_map_shm( &xt->vme_map_shm, 0, 0x100000);
  if( vme_shm_addr == -1)
  {
    printf("cannot map SHM in VME space\n");
    return( -1);
  }

  vme_shm_addr += vme_base_addr;
  if( debug) printf("VME SHM address = %x\n", vme_shm_addr);
  if( tst_cpu_map_vme( &xt->cpu_map_vme_shm, vme_shm_addr , 0x100000, MAP_VME_A32) == MAP_FAILED)
  {
    printf("cannot map VME_SHM in user space\n");
    return( -1);
  }

  if( tst_cpu_map_kbuf( &xt->cpu_map_kbuf, 0x100000) == MAP_FAILED)
  {
    printf("cannot map KBUF in user space\n");
    return( -1);
  }

  pci_kbuf_addr = (ulong)xt->cpu_map_kbuf.b_addr;
  vme_kbuf_addr = tst_vme_map_kbuf( &xt->vme_map_kbuf, pci_kbuf_addr, 0x100000);
  if( vme_kbuf_addr == -1)
  {
    printf("cannot map KBUF in VME space\n");
    return( -1);
  }

  vme_kbuf_addr += vme_base_addr;
  if( debug) printf("VME KBUF address = %x\n", vme_kbuf_addr);
  if( tst_cpu_map_vme( &xt->cpu_map_vme_kbuf, vme_kbuf_addr , 0x100000, MAP_VME_A32) == MAP_FAILED)
  {
    printf("cannot map VME_KBUF in user space\n");
    return( -1);
  }

  return( 0);
}

void
xt_exit( struct xprstst *xt)
{
  tst_cpu_unmap_shm( &xt->cpu_map_shm);
  tst_cpu_unmap_vme( &xt->cpu_map_vme_shm);
  tst_vme_unmap_shm( &xt->vme_map_shm);
  tst_cpu_unmap_kbuf( &xt->cpu_map_kbuf);
  tst_cpu_unmap_vme( &xt->cpu_map_vme_kbuf);
  tst_vme_unmap_kbuf( &xt->vme_map_kbuf);
  pev_exit( xt->pev);
}

int
xt_launch_test( char *tst_file)
{
  int sts;
  char tmp[64];

  if( pipe( fd_p[0]) < 0)
  {
    /* cannot create communication pipe to write  */
    return( -1);
  }
  //printf("write pipe = %d - %d\n", fd_p[0][0], fd_p[0][1]);
  if( pipe( fd_p[1]) < 0)
  {
    /* cannot create communication pipe to read  */
    close( fd_p[0][0]);
    close( fd_p[0][1]);
    return( -1);
  }
  sprintf( tst_argv[2], "%s %s %d %d", tst_file, cfg_filename, fd_p[0][0], fd_p[1][1]);
  pid = fork();
  if( pid < 0)
  {
    /* cannot fork...  */
    return( -1);
  }
  else
  {
    if( pid)
    {
      int n, i;

      /* parent process -> we stay in XprsTst */
      X_pid = pid;
      close( fd_p[0][0]);
      close( fd_p[1][1]);
      n = read( fd_p[1][0], tmp, 63);  
      if( n <= 0)
      {
        printf("->Error:Exiting:Connexion lost\n");
      }
      else
      {
	tmp[n] = 0;
	i = strlen(tst_file);
	if( !strncmp( tst_file, tmp, i))
	{
	  printf("->Done:%s\n", tmp);
	  sscanf( &tmp[i], "%d", &T_pid);
	}
      }
    }
    else
    {
      /* child process -> we execute PevTst */
      close( fd_p[0][1]);
      close( fd_p[1][0]);
      printf("XprsTst->Launching:%s", tst_argv[2]);
      fflush( stdout);
      if( execv( tst_argv[0], tst_argv) == -1)
      {
	printf("->ERROR:cannot execute %s\n", tst_argv[0]);
      }
      exit( -1);
    }
  }
  return( 0);
}
