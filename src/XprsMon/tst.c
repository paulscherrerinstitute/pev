/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : tty.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to send command
 *     through the ttyUSB0 interface.
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
 * $Log: tst.c,v $
 * Revision 1.7  2012/07/10 10:21:48  kalantari
 * added tosca driver release 4.15 from ioxos
 *
 * Revision 1.11  2012/06/06 12:33:20  ioxos
 * change rcsid [JFG]
 *
 * Revision 1.10  2012/06/01 13:59:44  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.9  2012/03/21 11:29:41  ioxos
 * adjust default test file for PEV, IPV, IFC [JFG]
 *
 * Revision 1.8  2012/03/05 09:33:05  ioxos
 * support for execution on PPC [JFG]
 *
 * Revision 1.7  2010/06/11 11:52:24  ioxos
 * add test status report [JFG]
 *
 * Revision 1.6  2009/11/10 09:14:24  ioxos
 * use constant instead of raw data for memory offset [JFG]
 *
 * Revision 1.5  2009/08/27 14:46:57  ioxos
 * don't execute tinit more than once [JFG]
 *
 * Revision 1.4  2009/08/26 12:01:17  ioxos
 * add tlist [JFG]
 *
 * Revision 1.3  2009/08/25 14:34:01  ioxos
 * tkill test program if needed when exiting [JFG]
 *
 * Revision 1.2  2009/08/25 13:37:38  ioxos
 * add support for XprsTst [JFG]
 *
 * Revision 1.1  2008/09/17 13:05:21  ioxos
 * file creation [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: tst.c,v 1.7 2012/07/10 10:21:48 kalantari Exp $";
#endif

#define DEBUGno
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

#include <cli.h>
#include <pevioctl.h>
#include <pevulib.h>
#include <sys/time.h>

#include <xprstst.h>
#include <tstlib.h>

#include "tst.h"

int debug;
#define TST_SHM_BASE 0x100000
#define TST_SHM_SIZE 0x100000
#define TST_KBUF_SIZE 0x100000

struct xprstst *xt;
int pid, X_pid, T_pid;
char *tst_argv[16];
int fd_p[2][2];
char *cfg_filename = "XprsTst.cfg";

extern char *cmdline;

char *
Xtst_rcsid()
{
  return( rcsid);
}

void
tst_init( void)
{
  xt = (struct xprstst *)malloc( sizeof( struct xprstst));
  xt->pev_para.crate = pev_get_crate();
  T_pid = 0;
  X_pid = 0;

  return;
}
void
tst_exit( void)
{
  if( T_pid)
  {
    xprs_tkill( 0);
  }
  free( xt);
  return;
}

int
launch_test( char *tst_file, char *tty)
{
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
#ifdef PPC
  tst_argv[0] =  (char *)malloc( 64);
  strcpy(  tst_argv[0], tst_file);
  tst_argv[1] =  "XprsTst.cfg";
  tst_argv[2] =  (char *)malloc( 64);
  sprintf( tst_argv[2], "%d", fd_p[0][0]);
  tst_argv[3] =  (char *)malloc( 64);
  sprintf( tst_argv[3], "%d", fd_p[1][1]);
  //tst_argv[4] =  "/dev/pts/0";
  tst_argv[4] =  (char *)malloc( 64);
  strcpy(  tst_argv[4], tty);
  tst_argv[5] =  0;
#else
  tst_argv[0] =  "/usr/bin/xterm";
  tst_argv[1] =  "-e";
  tst_argv[2] =  (char *)malloc( 64);
  tst_argv[3] =  0;
  sprintf( tst_argv[2], "%s %s %d %d", tst_file, cfg_filename, fd_p[0][0], fd_p[1][1]);
#endif
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
	return( -1);
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
#ifdef PPC
      printf("XprsTst->Launching:%s", tst_argv[0]);
#else
      printf("XprsTst->Launching:%s", tst_argv[2]);
#endif
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

int 
xprs_tinit( struct cli_cmd_para *c)
{
  FILE *cfg_file;
  ulong vme_base_addr;
  ulong vme_shm_addr;
  ulong vme_kbuf_addr;
  ulong pci_kbuf_addr;
  int retval;

  if( T_pid)
  {
    printf("Test program already running...(see tkill)\n");
    return(-1);
  }
  vme_base_addr = (ulong)tst_vme_conf_read( &xt->vme_conf);
  if( debug) printf("VME base address = %lx\n", vme_base_addr);

  if( tst_cpu_map_shm( &xt->cpu_map_shm, TST_SHM_BASE, TST_SHM_SIZE) == MAP_FAILED)
  {
    printf("cannot map SHM in user space\n");
    return( -1);
  }

  vme_shm_addr = tst_vme_map_shm( &xt->vme_map_shm, TST_SHM_BASE, TST_SHM_SIZE);
  if( vme_shm_addr == -1)
  {
    printf("cannot map SHM in VME space\n");
    goto xprs_cpu_unmap_shm;
  }

  vme_shm_addr += vme_base_addr;
  if( debug) printf("VME SHM address = %lx\n", vme_shm_addr);
  if( tst_cpu_map_vme( &xt->cpu_map_vme_shm, vme_shm_addr , 0x100000, MAP_VME_A32) == MAP_FAILED)
  {
    printf("cannot map VME_SHM in user space\n");
    goto xprs_vme_unmap_shm;
  }

  if( tst_cpu_map_kbuf( &xt->cpu_map_kbuf, 0x100000) == MAP_FAILED)
  {
    printf("cannot map KBUF in user space\n");
    goto xprs_cpu_unmap_vme_shm;
  }

  pci_kbuf_addr = (ulong)xt->cpu_map_kbuf.b_addr;
  vme_kbuf_addr = tst_vme_map_kbuf( &xt->vme_map_kbuf, pci_kbuf_addr, 0x100000);
  if( vme_kbuf_addr == -1)
  {
    printf("cannot map KBUF in VME space\n");
    goto xprs_cpu_unmap_kbuf;
  }

  vme_kbuf_addr += vme_base_addr;
  if( debug) printf("VME KBUF address = %lx\n", vme_kbuf_addr);
  if( tst_cpu_map_vme( &xt->cpu_map_vme_kbuf, vme_kbuf_addr , 0x100000, MAP_VME_A32) == MAP_FAILED)
  {
    printf("cannot map VME_KBUF in user space\n");
    goto xprs_vme_unmap_kbuf;
  }
  cfg_file = fopen( cfg_filename, "w");
  if( !cfg_file)
  {
    printf("cannot create configuration file %s\n", cfg_filename);
    goto xprs_cpu_unmap_vme_kbuf;
  }
  fwrite( xt, sizeof( struct xprstst), 1, cfg_file);
  fclose( cfg_file);
  switch( c->cnt)
  {
    case 0:
    {
      retval = -1;
      if( pev_board() == PEV_BOARD_PEV1100)
      {
	retval = launch_test("./PevTst",  "/dev/pts/0");
      }
      if( pev_board() == PEV_BOARD_IPV1102)
      {
	retval = launch_test("./IpvTst",  "/dev/pts/0");
      }
      if( pev_board() == PEV_BOARD_IFC1210)
      {
	retval = launch_test("./IfcTst",  "/dev/pts/0");
      }
      break;
    }
    case 1:
    {
      retval = launch_test(c->para[0],  "/dev/pts/0");
      break;
    }
    default:
    {
      retval = launch_test(c->para[0],  c->para[1]);
    }
  }

  if( retval == 0) return( 0);

xprs_cpu_unmap_vme_kbuf:
  tst_cpu_unmap_vme( &xt->cpu_map_vme_kbuf);
xprs_vme_unmap_kbuf:
  tst_vme_unmap_kbuf( &xt->vme_map_kbuf);
xprs_cpu_unmap_kbuf:
   tst_cpu_unmap_kbuf( &xt->cpu_map_kbuf);
xprs_cpu_unmap_vme_shm:
   tst_cpu_unmap_vme( &xt->cpu_map_vme_shm);
xprs_vme_unmap_shm:
   tst_vme_unmap_shm( &xt->vme_map_shm);
xprs_cpu_unmap_shm:
   tst_cpu_unmap_shm( &xt->cpu_map_shm);
  return( -1);
}

int 
xprs_tkill( struct cli_cmd_para *c)
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
    tst_cpu_unmap_shm( &xt->cpu_map_shm);
    tst_cpu_unmap_vme( &xt->cpu_map_vme_shm);
    tst_vme_unmap_shm( &xt->vme_map_shm);
    tst_cpu_unmap_kbuf( &xt->cpu_map_kbuf);
    tst_cpu_unmap_vme( &xt->cpu_map_vme_kbuf);
    tst_vme_unmap_kbuf( &xt->vme_map_kbuf);
    return( 0);
  }
  else
  {
    printf("tkill -> no test program launched..\n");
    return( -1);
  }
}

int 
xprs_tcmd( struct cli_cmd_para *c)
{
  if( T_pid)
  {
    write( fd_p[0][1], cmdline, strlen( cmdline));
    kill( T_pid, SIGUSR1);
    return( 0);
  }
  else
  {
    printf("Test program not loaded...(see tinit)\n");
    return( -1);
  }
}

int 
xprs_tlist( struct cli_cmd_para *c)
{
  return( xprs_tcmd( c));
}

int 
xprs_tset( struct cli_cmd_para *c)
{
  return( xprs_tcmd( c));
}

int 
xprs_tstart( struct cli_cmd_para *c)
{
  return( xprs_tcmd( c));
}

int 
xprs_tstop( struct cli_cmd_para *c)
{
  return( xprs_tcmd( c));
}

int 
xprs_tstatus( struct cli_cmd_para *c)
{
  struct tst_ctl tc;
  char tmp[64];
  int n;

  if( !xprs_tcmd( c))
  {
    bzero( tmp, 64);
    n = read( fd_p[1][0], &tc, sizeof( tc));  
    if( n <= 0)
    {
      printf("Connexion lost with test program\n");
      return( -1);
    }
    if( !tc.status)
    {
      printf("PevTST -> IDLE\n");
      return(0);
    }
    if( tc.status & TST_STS_STARTED)
    {
      printf("Tst:%02x -> STARTED\n", tc.test_idx);
      return(0);
    }
    if( tc.status & TST_STS_DONE)
    {
      printf("Tst:%02x -> DONE:", tc.test_idx);
      if( tc.status & TST_STS_STOPPED)
      {
	printf("STOPPED\n");
      }
      else if( tc.status & TST_STS_ERR)
      {
	printf("ERROR\n");
      }
      else
      {
	printf("OK\n");
      }
    }

    return(0);
  }
  return( -1);
}
