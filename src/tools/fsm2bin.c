/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : fsm2bin.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This program allows to convert an ASCII "fsm" file in its binary form
 *    ready to be loaded in the PEV110 SFLASH, using the XprsMon command
 *    -> sflash load <offset> <filename>
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
 * $Log: fsm2bin.c,v $
 * Revision 1.1  2012/03/15 14:50:11  kalantari
 * added exact copy of tosca-driver_4.04 from afs
 *
 * Revision 1.3  2008/09/17 13:19:31  ioxos
 * cleanup (remove debug info) [JFG]
 *
 * Revision 1.2  2008/08/08 12:53:59  ioxos
 * define pex operation to access PEX86xx registers and allow for comments inside a command line [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: fsm2bin.c,v 1.1 2012/03/15 14:50:11 kalantari Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

char line[256];
unsigned char ins[8];

int
swap_32( int data)
{
  char ci[4];
  char co[4];

  *(int *)ci = data;
  co[0] = ci[3];
  co[1] = ci[2];
  co[2] = ci[1];
  co[3] = ci[0];

  return( *(int *)co);
}

main( int argc,
      void *argv[])
{
  char *infilename, *outfilename;
  FILE *infile, *outfile;
  int i, n;

  i = 1;
  infilename = (char *)0;
  outfilename = (char *)0;
  while( i < argc)
  {
    char *p;

    p = argv[i++];
    if( p[0] == '-')
    {
      switch( p[1])
      {
        case 'o':
        {
	  p = argv[i++];
	  outfilename = p;
	  break;
	}
        default:
        {
	  printf("bad option\n");
	  exit(0);
	}
      }
    }
    else
    {
      infilename = p;
      break;
    }
  }
  if( !infilename)
  {
    printf("usage: fsm2bin -o outfile infile");
    exit(0);
  }
  infile = fopen( infilename, "r");
  if( !infile)
  {
    printf("cannot open input file %s\n", infilename);
    exit(0);
  }
  if( !outfilename)
  {
    outfilename = malloc( strlen(infilename) + 4);
    strcpy( outfilename, infilename);
    strcat( outfilename,".sfl");
  }
  outfile = fopen( outfilename, "w");
  if( !outfile)
  {
    printf("cannot open output file %s\n", outfilename);
    exit(0);
  }
  printf("converting file %s -> %s\n", infilename, outfilename);
  n = 0;
  while( fgets( line, 256, infile))
  {
    bzero( ins, 8);
    strtok( line, "\n\r#");
    if( !strncmp( "load", line, 4))
    {
      uint reg, data;
      sscanf( line, "load %x %x", &reg, &data);

      *(uint *)ins = 0x45000000 | reg;
      *(uint *)&ins[4] = data;
      fwrite( ins,1,8,outfile);
    }
    if( !strncmp( "pex", line, 3))
    {
      uint reg, data;

      sscanf( line, "pex %x %x", &reg, &data);

      *(uint *)ins = 0x45000108;
      *(uint *)&ins[4] = swap_32(data);
      fwrite( ins,1,8,outfile);
      *(uint *)ins = 0x45000104;
      *(uint *)&ins[4] = swap_32( 0x03003c00 | ((reg << 3) &0x78000) | ((( reg >> 2) & 0x3ff)));
      fwrite( ins,1,8,outfile);
      *(uint *)ins = 0x45000100;
      *(uint *)&ins[4] = 0x018f0069;
      fwrite( ins,1,8,outfile);
    }
    if( !strncmp( "wait", line, 4))
    {
      uint tmo;
      sscanf( line, "wait %d", &tmo);

      *(uint *)ins = 0x34000000 | tmo;
      fwrite( ins,1,8,outfile);
    }
    if( !strncmp( "stop", line, 4))
    {
      *(uint *)ins = 0x22000000;
      fwrite( ins,1,8,outfile);
    }
  }
  fclose( outfile);
  fclose( infile);
  exit( 0);
}
