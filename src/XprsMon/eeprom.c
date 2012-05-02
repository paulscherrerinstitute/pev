/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : eeprom.c
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
 * $Log: eeprom.c,v $
 * Revision 1.1  2012/05/02 13:13:07  kalantari
 * update fifo.c and eeprom.c files
 *
 * Revision 1.2  2012/04/18 07:45:20  ioxos
 * add load function [JFG]
 *
 * Revision 1.1  2012/04/12 13:14:02  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: eeprom.c,v 1.1 2012/05/02 13:13:07 kalantari Exp $";
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cli.h>
#include <pevioctl.h>
#include <pevulib.h>


int 
xprs_eeprom( struct cli_cmd_para *c)
{
  uint i, j;
  uint data, offset, cnt;
  char *buf, *p;
  FILE *file;

  if( c->cnt < 3)
  {
    printf("eeprom command needs more arguments\n");
    printf("usage: eeprom dump <offset> <cnt>\n");
    printf("       eeprom load <offset> <filename>\n");
    return(-1);
  }
  if( !strcmp( "dump", c->para[0]))
  {
    if( sscanf( c->para[1],"%x", &offset) != 1)
    {
      printf("wrong offset value\n");
      return(-1);
    }
    if( sscanf( c->para[2],"%x", &cnt) != 1)
    {
      printf("wrong count value\n");
      return(-1);
    }
    printf("Dumping EEPROM from offset %x [size %x] ...", offset, cnt);
    buf = malloc( cnt);
    pev_eeprom_rd( offset, buf, cnt);
    printf(" -> done\n");

    p = (char *)buf;
    for( j = 0; j < cnt; j += 16)
    {
      unsigned char *pp;

      printf("%08x ", offset + j);
      pp = p;
      for( i = 0; i < 16; i++)
      {
        printf("%02x ", *(unsigned char *)p++);
      }
      for( i = 0; i < 16; i++)
      {
        char c;
	c = *pp++;
	if(isalpha(c))
	{ 
          printf("%c", c);
	}
	else
	{ 
          printf(".");
        }
      }
      printf("\n");
    }
    printf("\n");
    free( buf);
  }
  if( !strcmp( "load", c->para[0]))
  {
    if( sscanf( c->para[1],"%x", &offset) != 1)
    {
      printf("wrong offset value\n");
      return(-1);
    }
    file = fopen( c->para[2], "r");
    if( !file)
    {
      printf("\nFile %s doesn't exist\n", c->para[2]);
      return( -1);
    }
    fseek( file, 0, SEEK_END);
    cnt = ftell( file);
    printf("Loading EEPROM from file %s at offset %x [size %x] ...", c->para[2], offset, cnt);
    buf = (char *)malloc( cnt);
    fseek( file, 0, SEEK_SET);
    fread( buf, 1, cnt, file);
    fclose( file);
    pev_eeprom_wr( offset, buf, cnt);
    printf(" -> done\n");
    free( buf);
  }
  return(0);

}
