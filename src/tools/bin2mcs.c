/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : mcs2bin.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This program allows to convert an ASCII "mcs" file in its binary form
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
 * $Log: bin2mcs.c,v $
 * Revision 1.12  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.1  2009/11/10 09:16:46  ioxos
 * first check in [JFG]
 *
 * Revision 1.2  2008/11/12 14:22:46  ioxos
 * add support for a pex instruction [JFG]
 *
 * Revision 1.1  2008/07/18 14:59:29  ioxos
 * file creation [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: bin2mcs.c,v 1.12 2013/06/07 14:59:54 zimoch Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

unsigned char line[256];
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

unsigned char data;
unsigned char *bitstream;

main( int argc,
      void *argv[])
{
  char *infilename, *outfilename;
  FILE *infile, *outfile;
  int i, j, k, n;
  int type, len, ns, offset;
  int addr, addr_lo, addr_hi;
  unsigned char cks;

  i = 1;
  infilename = (char *)0;
  outfilename = (char *)0;
  while( i < argc)
  {
    char *p;

    p = argv[i++];
    printf("%s\n", p);
    if( p[0] == '-')
    {
      switch( p[1])
      {
        case 'o':
        {
	  p = argv[i++];
	  outfilename = p;
	  printf("%s\n", p);
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
    printf("usage: bin2mcs -o outfile infile");
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
    strcat( outfilename,".mcs");
  }
  outfile = fopen( outfilename, "w");
  if( !outfile)
  {
    printf("cannot open output file %s\n", outfilename);
    fclose( infile);
    exit(0);
  }
  printf("converting file %s -> %s\n", infilename, outfilename);
  n = 0;
  addr_hi = 0;
  addr_lo = 0;
  fseek( infile, 0, SEEK_END);
  len = ftell( infile);
  fseek( infile, 0, SEEK_SET);
  bitstream = (char *)malloc( (len+0xf)&0xfffffff0);
  if( !bitstream)
  {
    printf("cannot allocate buffer for bistream file\n");
    fclose( outfile);
    fclose( infile);
    exit( 0);
  }
  fread( bitstream, len, 1, infile);

  ns = 1+((len-1) >> 16);
  for( i = 0; i < ns; i++)
  {
    cks = 0x6 + (i & 0xff) + (( i >> 8) & 0xff);
    sprintf( line, ":02000004%04X%02X\n", i, (unsigned char)-cks);
    fprintf(outfile, "%s", line);
    for( j = 0; j < 0x1000; j++)
    {
      addr_lo = j<<4;
      sprintf( line, ":10%04X00", addr_lo);
      cks = 0x10 + (addr_lo & 0xff) + (( addr_lo >> 8) & 0xff);
      for( k= 0; k < 16; k++)
      {
	offset = (i<<16)+(j<<4)+k;
	data = bitstream[offset];
	sprintf( &line[9+(2*k)], "%02X", data);
	cks += data;
      }
      sprintf( &line[41], "%02X\n", (unsigned char)-cks);
      fprintf(outfile,"%s", line);
      if( offset >= len)
      {
	goto bin2mcs_end;
      }
    }
  }

bin2mcs_end:
  sprintf( line, ":00000001FF\n");
  fprintf(outfile,"%s", line);
  free( bitstream);
  fclose( outfile);
  fclose( infile);
  exit( 0);
}
