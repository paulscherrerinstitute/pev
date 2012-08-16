/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : fpgabuild.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This program allows to build a 2 MBytes FPGA binary file ready to be
 *    loaded in the HELIOS SFLASH, using the HeliosMon command
 *    -> sflash load <offset> <filename>
 *    This program asks for interactively for two source file as input:
 *    - a .mcs file containing the FPGA bitstream
 *    - a .fsm file containing the HELIOS power-on sequencer instruction
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
 * $Log: fpgabuild.c,v $
 * Revision 1.8  2012/08/16 09:11:39  kalantari
 * added version 4.16 of tosca driver
 *
 * Revision 1.4  2010/08/13 09:35:48  ioxos
 * get company and board from cfg file [JFG]
 *
 * Revision 1.2  2010/08/13 09:34:55  ioxos
 * get company and board from cfg file [JFG]
 *
 * Revision 1.1.1.1  2010/03/26 12:45:12  ioxos
 * HELIOS sources
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: fpgabuild.c,v 1.8 2012/08/16 09:11:39 kalantari Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <libgen.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xilinx.h>


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

char data[256];
char filename[1024];

#define BAD_CKS   2
#define BAD_ADDR  3
#define BAD_FORM  4
#define BAD_REC   5
#define BAD_INS   6

int
convert_fsm( FILE *infile,
	     char *seq)
{
  int i, n;

  n = 0;
  while( fgets( line, 256, infile))
  {
    bzero( ins, 8);
    if( !strncmp( "load", line, 4))
    {
      uint reg, data;
      sscanf( line, "load %x %x", &reg, &data);

      *(uint *)ins = 0x45000000 | reg;
      *(uint *)&ins[4] = data;
      for( i = 0; i < 8; i++)
      {
	seq[n++] = ins[i];
      }
    }
    if( !strncmp( "pex", line, 3))
    {
      uint reg, data;

      sscanf( line, "pex %x %x", &reg, &data);

      *(uint *)ins = 0x45000108;
      *(uint *)&ins[4] = swap_32(data);
      for( i = 0; i < 8; i++)
      {
	seq[n++] = ins[i];
      }
      *(uint *)ins = 0x45000104;
      *(uint *)&ins[4] = swap_32( 0x03003c00 | ((reg << 3) &0x78000) | ((( reg >> 2) & 0x3ff)));
      for( i = 0; i < 8; i++)
      {
	seq[n++] = ins[i];
      }
      *(uint *)ins = 0x45000100;
      *(uint *)&ins[4] = 0x018f0069;
      for( i = 0; i < 8; i++)
      {
	seq[n++] = ins[i];
      }
    }
    if( !strncmp( "wait", line, 4))
    {
      uint tmo;
      sscanf( line, "wait %d", &tmo);

      *(uint *)ins = 0x34000000 | tmo;
      *(uint *)&ins[4] = 0;
      for( i = 0; i < 8; i++)
      {
	seq[n++] = ins[i];
      }
    }
    if( !strncmp( "stop", line, 4))
    {
      *(uint *)ins = 0x22000000;
      *(uint *)&ins[4] = 0;
      for( i = 0; i < 8; i++)
      {
	seq[n++] = ins[i];
      }
    }
  }

  return(n);
}
int
convert_mcs( FILE *infile,
	     char *bitstream)
{
  int i, n;
  int type, len;
  int addr, addr_lo, addr_hi;
  unsigned char cks;

  n = 0;
  addr_hi = 0;
  addr_lo = 0;

  while( fgets( line, 256, infile))
  {
    if( line[0] == ':')
    {
      sscanf( line, ":%2x%4x%2x", &len, &addr_lo, &type);
      cks = len + type + (addr_lo & 0xff) + (( addr_lo >> 8) & 0xff);
      for( i = 0; i <= len; i++)
      {
	sscanf( &line[9+(2*i)], "%2x", &data[i]);
	cks += data[i];
      }
      if( cks)
      {
	return( -BAD_CKS);
      }
      switch( type)
      {
        case 0:
	{
	  addr = addr_hi + addr_lo;
	  if( addr > (0x400000 - len))
	  {
	    return( -BAD_ADDR);
	  }
	  for( i = 0; i < len; i++)
	  {
	    bitstream[addr++] = data[i];
	  }
	  break;
	}
        case 1:
	{
	  break;
	}
        case 2:
        case 4:
	{
	  if( addr_lo)
	  {
	    return( -BAD_FORM);
	  }
	  if( type == 4)
	  {
	    addr_hi = ( data[0] << 24) | ( data[1] << 16);
	  }
	  else
	  {
	    addr_hi = ( data[0] << 12) | ( data[1] << 4);
	  }
	  break;
	}
        default:
	{
	  return( -BAD_REC);
	}
      }
    }
      n++;
  }
  return( addr);
}


struct xilinx_device
*fpga_identify( void *buf,
	  int size)
{
  int *p, i, j;
  struct xilinx_device *xd;

  p = (int *)buf;
  i = size;
  for( i = size; i > 0; i-=4)
  {
    /* check for synchronization pattern */
    if( *p == 0x665599aa)
    {
      p += 0x14;
      /* return device identifier */
      xd = &xilinx_device[0];
      while( xd->name)
      {
	if( *p == xd->id)
	{
	  return( xd);
	}
	xd++;
      }
      return( NULL);
    }
    p++;
  }
  return( NULL);
}

int
checksum(void *buf,
	 int size)
{
  int *p;
  int cks;

  p = (int *)buf;
  cks = 0;

  while( size > 0)
  {
    cks += *p++;
    size -= 4;
  }
  return(cks);
}


main( int argc,
      void *argv[])
{
  char *mcs_filename, *fsm_filename, *bin_filename;
  char *company_name, *board_name;
  FILE *mcs_file, *fsm_file, *bin_file, *cfg_file;
  struct xilinx_device *xd;
  int len, offset_btr, offset_fsm, offset_sign;
  int mcs_size, mcs_cks;
  int fsm_size, fsm_cks;
  char *buf, *p;
  time_t tm;
  struct stat fsm_stat, mcs_stat, bin_stat;

  printf("\n");
  printf("     +----------------------------------------------+\n");
  printf("     |    FpgaBuild - PEV1100 FPGA building tool    |\n");
  printf("     |       IOxOS Technologies Copyright 2008      |\n");
  printf("     +----------------------------------------------+\n");
  printf("\n");

  tm = time(0);
  printf("time: %s\n", ctime( &tm));
  printf("login: %s\n", getlogin());

  fsm_filename = (char *)0;
  mcs_filename = (char *)0;
  bin_filename = (char *)0;

  if( argc > 1)
  {
    cfg_file = fopen( argv[1], "r");
    if( !cfg_file)
    {
      printf("cannot open configuration file %s\n", argv[1]);
      exit(0);
    }
    while( fgets( line, 256, cfg_file))
    {
      if( !strncmp( "company:", line, 8))
      {
	sscanf( line, "company:%s", filename);
	company_name = strdup( filename);
      }
      if( !strncmp( "board:", line, 6))
      {
	sscanf( line, "board:%s", filename);
	board_name = strdup( filename);
      }
      if( !strncmp( "mcs:", line, 4))
      {
	sscanf( line, "mcs:%s", filename);
	mcs_filename = strdup( filename);
      }
      if( !strncmp( "fsm:", line, 4))
      {
	sscanf( line, "fsm:%s", filename);
	fsm_filename = strdup( filename);
      }
      if( !strncmp( "sfl:", line, 4))
      {
	sscanf( line, "sfl:%s", filename);
	bin_filename = strdup( filename);
      }
    }
    fclose( cfg_file);
  }


  if( !company_name)
  {
    printf("company : ");
    scanf("%s", filename);
    company_name = strdup( filename);
  }
  else
  {
    printf("company : %s\n", company_name);
  }
  if( !board_name)
  {
    printf("board : ");
    scanf("%s", filename);
    board_name = strdup( filename);
  }
  else
  {
    printf("board : %s\n", board_name);
  }
  if( !fsm_filename)
  {
    printf("power on sequencer file : ");
    scanf("%s", filename);
    fsm_filename = strdup( filename);
  }
  else
  {
    printf("power on sequencer file : %s\n", fsm_filename);
  }
  fsm_file = fopen( fsm_filename, "r");
  if( !fsm_file)
  {
    printf("cannot open fsm file %s\n", fsm_filename);
    exit(0);
  }
  stat( fsm_filename, &fsm_stat);
  fsm_filename = basename( fsm_filename);

  if( !mcs_filename)
  {
    printf("FPGA bitstream file     : ");
    scanf("%s", filename);
    mcs_filename = strdup( filename);
  }
  else
  {
    printf("FPGA bitstream file     : %s\n", mcs_filename);
  }
  mcs_file = fopen( mcs_filename, "r");
  if( !mcs_file)
  {
    printf("cannot open mcs file %s\n", mcs_filename);
    fclose( fsm_file);
    exit(0);
  }
  stat( mcs_filename, &mcs_stat);
  mcs_filename = basename( mcs_filename);

  if( !bin_filename)
  {
    printf("binary file             : ");
    scanf("%s", filename);
    bin_filename = strdup( filename);
  }
  else
  {
    printf("binary file             : %s\n", bin_filename);
  }
  bin_file = fopen( bin_filename, "w");
  if( !bin_file)
  {
    printf("cannot open output file %s\n", bin_filename);
    fclose( mcs_file);
    fclose( fsm_file);
    exit(0);
  }
  bin_filename = basename( bin_filename);

  buf = (char *)malloc( 0x400000);
  if( !buf)
  {
    printf("cannot allocate buffer for bistream file\n");
    goto fpgabuild_error_end;
  }

  printf("converting mcs file %s ->  ", mcs_filename);
  mcs_size = convert_mcs( mcs_file, buf);
  if( mcs_size < 0)
  {
    printf("error %d in file conversion\n", mcs_size);
  }
  printf("size %08x \n",mcs_size);
  xd = fpga_identify( buf, mcs_size);
  if( !xd)
  {
    printf("unknown FPGA device\n");
  }
  if( xd->size != mcs_size)
  {
    printf("file doesnt have expected size [xd->size] -> actual [mcs_size]\n");
  }
  if (xd->size > 0x200000)
  {
    offset_btr  = 0x400000;
    offset_fsm  = 0x3c0000;
    offset_sign = 0x3f0000;
  }
  else
  {
    offset_btr  = 0x200000;
    offset_fsm  = 0x1c0000;
    offset_sign = 0x1f0000;
  }
  
  mcs_cks = checksum( buf, mcs_size);
  printf("%s : 0x%x bytes [cks = 0x%08x]\n", xd->name, xd->size, mcs_cks);

  printf("converting fsm file %s -> ", fsm_filename);
  
  fsm_size = convert_fsm( fsm_file, buf + offset_fsm);
  if( fsm_size < 0)
  {
    printf("error %d in file conversion\n", mcs_size);
  }
  fsm_cks = checksum( buf+offset_fsm, fsm_size);
  printf("0x%x bytes [cks = 0x%08x]\n", fsm_size, fsm_cks);

  printf("building FPGA signature...");
  p = buf + offset_sign;
  len = sprintf( p, "Signature:");
  p += len;
  len = sprintf( p, "<$company:%s>", company_name);
  p += len;
  len = sprintf( p, "<$board:%s>", board_name);
  p += len;
  len = sprintf( p, "<$filename:%s>", bin_filename);
  p += len;
  len = sprintf( p, "<$creation:%s>", ctime( &tm));
  p += len;
  len = sprintf( p, "<$mcs_file:%s>", mcs_filename);
  p += len;
  len = sprintf( p, "<$mcs_devname:%s>", xd->name);
  p += len;
  len = sprintf( p, "<$mcs_devid:0x%08x>", swap_32( xd->id));
  p += len;
  len = sprintf( p, "<$mcs_offset:0x%06x>", 0x000000);
  p += len;
  len = sprintf( p, "<$mcs_size:0x%06x>", mcs_size);
  p += len;
  len = sprintf( p, "<$mcs_checksum:0x%08x>", mcs_cks);
  p += len;
  len = sprintf( p, "<$mcs_creation:%s>", ctime( &mcs_stat.st_mtime));
  p += len;
  len = sprintf( p, "<$fsm_file:%s>", fsm_filename);
  p += len;
  len = sprintf( p, "<$fsm_offset:0x%06x>", offset_fsm);
  p += len;
  len = sprintf( p, "<$fsm_size:0x%06x>", fsm_size);
  p += len;
  len = sprintf( p, "<$fsm_checksum:0x%08x>", fsm_cks);
  p += len;
  len = sprintf( p, "<$fsm_creation:%s>", ctime( &fsm_stat.st_mtime));
  p += len;
  printf("done\n");


  printf("writing FPGA bitstream file %s\n", bin_filename);
  fwrite( buf, offset_btr, 1, bin_file);


fpgabuild_error_end:
  free( buf);
  fclose( bin_file);
  fclose( mcs_file);
  fclose( fsm_file);
  exit( 0);
}
