/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : fifo.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to control
 *     PEV1000 fifo.
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
 * $Log: fifo.c,v $
 * Revision 1.6  2012/08/16 09:11:39  kalantari
 * added version 4.16 of tosca driver
 *
 * Revision 1.3  2012/06/01 13:59:43  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.2  2012/04/05 13:52:06  ioxos
 * add wait operation [JFG]
 *
 * Revision 1.1  2012/03/27 09:17:41  ioxos
 * add support for FIFOs [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: fifo.c,v 1.6 2012/08/16 09:11:39 kalantari Exp $";
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
#define pev_fifo_read(x) pev_rt_fifo_read(x)
#endif

char *
fifo_rcsid()
{
  return( rcsid);
}

int 
xprs_fifo( struct cli_cmd_para *c)
{

  int retval;
  int cnt, i, idx;
  char *p;

  retval = -1;
  cnt = c->cnt;
  i = 0;

  idx = -1;
  if( c->ext)
  {
    idx = strtoul( c->ext, &p, 16);
    if(( idx < 0) || ( idx > 3))
    {
      printf("bad FIFO index : %d\n", idx);
      return( -1);
    }
  }
  if( cnt--)
  {
    if( !strcmp( "init", c->para[i]))
    {
      printf("Initializing FIFOs..\n");
      pev_fifo_init();
      return(0); 
    }
    if( !strcmp( "status", c->para[i]))
    {
      uint sts;

      if( idx == -1)
      {
        for( idx = 0; idx < 4; idx++)
	{
	  pev_fifo_status( idx, &sts);
	  printf("FIFO#%d status = %08x\n", idx, sts);
	}
      }
      else
      {
	pev_fifo_status( idx, &sts);
	printf("FIFO#%d status = %08x\n", idx, sts);
      }
      return(0); 
    }
    if( !strcmp( "clear", c->para[i]))
    {
      uint sts;

      if( idx == -1)
      {
        for( idx = 0; idx < 4; idx++)
	{
	  pev_fifo_clear( idx, &sts);
	  printf("FIFO#%d status = %08x\n", idx, sts);
	}
      }
      else
      {
	pev_fifo_clear( idx, &sts);
	printf("FIFO#%d status = %08x\n", idx, sts);
      }
      return(0); 
    }

    if( !strcmp( "read", c->para[i]))
    {
      uint sts, data;

      if( idx != -1)
      {
	if( pev_fifo_read( idx, &data, 1, &sts) > 0)
	{
	  printf("FIFO#%d data = %08x [%08x]\n", idx, data, sts);
	}
	else
	{
	  printf("FIFO#%d is empty\n", idx);
        }
      }
      return(0); 
    }
    if( !strcmp( "write", c->para[i]))
    {
      uint sts, data;

      if( idx != -1)
      {
        if( !cnt)
        {
	  printf("usage: fifo.<idx> write <data>\n");
	}
	data = strtoul( c->para[i+1], &p, 16);
	if( !pev_fifo_write( idx, &data, 1, &sts))
	{
	  printf("FIFO#%d is full\n", idx);
        }
      }
      return(0); 
    }
    if( !strcmp( "wait", c->para[i]))
    {
      uint sts, tmo;

      if( idx != -1)
      {
	tmo = 0;
	if( cnt--)
        {
          tmo = strtoul( c->para[i+1], &p, 10);
          if( p ==  c->para[i+1])
          {
            printf("%s : bad timeout value\n", c->para[i+1]);
	    return(-1);
   	  }
        }
	printf("FIFO#%d waiting [%d msec]...", idx, tmo);
	retval = pev_fifo_wait_ef( idx, &sts, tmo);
	if( retval == -1)
	{
	  printf("timeout : ");
	}
	printf("status = %08x\n", sts);
      }
    }
  }

  return( -1);
}

