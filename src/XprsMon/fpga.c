/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : i2c.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : november 14,2011
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to perform read
 *     or write cycles through the I"C interface.
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
 * $Log: fpga.c,v $
 * Revision 1.2  2012/03/15 16:15:37  kalantari
 * added tosca-driver_4.05
 *
 * Revision 1.1  2012/01/27 13:39:15  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: fpga.c,v 1.2 2012/03/15 16:15:37 kalantari Exp $";
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


int 
fpga_prun( struct cli_cmd_para *c)
{
  int data;

  data = pev_elb_rd( 0x20);
  data |= 0x80000000;
  pev_elb_wr( 0x20, data);
  data = pev_elb_rd( 0x20);
  printf("prun fpga: %08x\n", data);
  return( 0);
}

int 
fpga_reload( struct cli_cmd_para *c)
{
  int data;
  int idx;
  char *p;

  if( c->cnt < 2)
  {
    printf("usage: fpga reload <idx>\n");
    return(-1);
  }
  idx = strtoul( c->para[1], &p, 16);
  if( p == c->para[1])
  {
    printf("usage: fpga reload <idx>\n");
    return(-1);
  }
  data = pev_elb_rd( 0x20);
  data |= 0x40000000 | ((idx & 3) << 28);
  pev_elb_wr( 0x20, data);
  data = pev_elb_rd( 0x20);
  printf("reload fpga: %08x\n", data);
  return( 0);
}

int 
fpga_load( struct cli_cmd_para *c)
{
  int fpga, size;
  char *buf;
  FILE *file;

  if( c->cnt < 3)
  {
    printf("usage: fpga pcfg <dev> <filename>\n");
    return(-1);
  }
  if( (!strcmp( "io", c->para[1])) ||
      (!strcmp( "IO", c->para[1]))    )
  {
    fpga = 1;
  }
  else if( (!strcmp( "vx6", c->para[1])) || 
           (!strcmp( "VX6", c->para[1])) ||
           (!strncmp( "central", c->para[1], 2)) ||
           (!strncmp( "CENTRAL", c->para[1], 2))    )
  {
    fpga = 2;
  }
  else
  {
    printf("bad fpga identifier -> usage: fpga load <dev> <filename>\n");
    return(-1);
  }

  printf("Loading FPGA from file %s...", c->para[2]);

  file = fopen( c->para[2], "r");
  if( !file)
  {
    printf("\nFile %s doesn't exist\n", c->para[2]);
    return( -1);
  }
  fseek( file, 0, SEEK_END);
  size = ftell( file);
  printf("[size 0x%x]\n", size);

  buf = (char *)malloc( size);
  fseek( file, 0, SEEK_SET);
  fread( buf, 1, size, file);
  fclose( file);
  if( pev_fpga_load( fpga, buf, size) < 0)
  {
    printf("FPGA programmation error...\n");
  }
  else
  {
    printf("FPGA programmation done...\n");
  }

  free( buf);

  return(0);

}

int 
xprs_fpga( struct cli_cmd_para *c)
{
  if( c->cnt < 1)
  {
    printf("fpga command needs more arguments\n");
    return(-1);
  }

  if( (!strcmp( "load", c->para[0])) ||
      (!strcmp( "pcfg", c->para[0]))    )
  {
    return( fpga_load( c));
  }
  if( (!strcmp( "prun", c->para[0])))
  {
    return( fpga_prun( c));
  }
  if( (!strcmp( "reload", c->para[0])))
  {
    return( fpga_reload( c));
  }
  printf("fpga: %s -> unsupported operation\n",  c->para[0]);
  return(-1);
}
