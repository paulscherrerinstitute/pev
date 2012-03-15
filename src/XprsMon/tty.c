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
 * $Log: tty.c,v $
 * Revision 1.2  2012/03/15 16:15:37  kalantari
 * added tosca-driver_4.05
 *
 * Revision 1.1  2008/09/17 13:05:21  ioxos
 * file creation [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: tty.c,v 1.2 2012/03/15 16:15:37 kalantari Exp $";
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
#include <cli.h>
#include <pevioctl.h>
#include <pevulib.h>

int fd_tty = -1;
char tty_cmd[256];

int
tty_open( void)
{
  fd_tty = open("/dev/ttyUSB0", O_RDWR);
  return( fd_tty);
}

int
tty_send( char *cmd)
{
  if( fd_tty > 0)
  {
    return( write( fd_tty, cmd, strlen(cmd)));
  }
  return( 0);
}

void
tty_close( void)
{
  if( fd_tty > 0)
  {
    close(fd_tty);
  }
  return;
}


int 
xprs_tty( struct cli_cmd_para *c)
{
  int i;

  if( !strcmp( "open", c->para[0]))
  {
    tty_open();
  }
  if( !strcmp( "close", c->para[0]))
  {
    tty_close();
  }
  if( !strcmp( "send", c->para[0]))
  {
    if( c->cnt >= 1)
    {
      strcpy( tty_cmd, c->para[1]);
      strcat( tty_cmd, "\n\r");
      tty_send( tty_cmd);
    }
  }
  return(0);
}

