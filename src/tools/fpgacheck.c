/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : fpgabuild.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : july 10,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This program allows to build a 2 MBytes FPGA binary file ready to be
 *    loaded in the PEV110 SFLASH, using the XprsMon command
 *    -> sflash load <offset> <filename>
 *    This program asks for interactively for two source file as input:
 *    - a .mcs file containing the FPGA bitstream
 *    - a .fsm file containing the PEV1100 power-on sequencer instruction
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
 * $Log: fpgacheck.c,v $
 * Revision 1.10  2012/10/01 14:56:50  kalantari
 * added verion 4.20 of tosca-driver from IoxoS
 *
 * Revision 1.1  2008/11/12 14:11:38  ioxos
 * first cvs checkin [JFG]
 *
 * Revision 1.1  2008/07/18 15:00:03  ioxos
 * file creation [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: fpgacheck.c,v 1.10 2012/10/01 14:56:50 kalantari Exp $";
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

struct fpga_para
{
  int mcs_offset;
  int mcs_size;
  uint mcs_cks;
  int fsm_offset;
  int fsm_size;
  uint fsm_cks;
} fpga_para;


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

int
get_sign_para( char *sign,
	       struct fpga_para *para)
{
  int data;

  if( sscanf( sign, "mcs_offset:0x%x", &data) == 1)
  {
    para->mcs_offset = data;
    return( 0);
  }
  if( sscanf( sign, "mcs_size:0x%x", &data) == 1)
  {
    para->mcs_size = data;
    return( 0);
  }
  if( sscanf( sign, "mcs_checksum:0x%x", &data) == 1)
  {
    para->mcs_cks = data;
    return( 0);
  }
  if( sscanf( sign, "fsm_offset:0x%x", &data) == 1)
  {
    para->fsm_offset = data;
    return( 0);
  }
  if( sscanf( sign, "fsm_size:0x%x", &data) == 1)
  {
    para->fsm_size = data;
    return( 0);
  }
  if( sscanf( sign, "fsm_checksum:0x%x", &data) == 1)
  {
    para->fsm_cks = data;
    return( 0);
  }
  return( -1);
}

main( int argc,
      void *argv[])
{
  char *mcs_filename, *fsm_filename, *bin_filename;
  FILE *bin_file;
  int len, cks;
  int mcs_size, mcs_cks;
  int fsm_size, fsm_cks;
  char *buf, *p, *sign;
  time_t tm;
  struct stat fsm_stat, mcs_stat, bin_stat;

  printf("\n");
  printf("     +----------------------------------------------+\n");
  printf("     |    FpgaCheck - PEV1100 FPGA building tool    |\n");
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
    bin_file = fopen( argv[1], "r");
    if( !bin_file)
    {
      printf("cannot open FPGA binary file %s\n", argv[1]);
      exit(0);
    }
  }
  printf("checking binary file %s\n", argv[1]);

  fseek( bin_file, 0, SEEK_END);
  len = ftell( bin_file);
  if( len != 0x200000)
  {
    printf("file does not have the right size: expected = 0x200000 (2MBytes) -> actual = 0x%x\n", len);
    fclose( bin_file);
    exit( 0);
  }
  buf = (char *)malloc( 0x200000);
  if( !buf)
  {
    printf("cannot allocate buffer for bistream file\n");
    fclose( bin_file);
    exit(0);
  }
  fseek( bin_file, 0, SEEK_SET);
  fread( buf, 0x200000, 1, bin_file);

  printf("Checking FPGA signature...\n");

  sign = buf + 0x1f0000;
  if( strncmp( "Signature", sign, strlen("Signature")))
  {
    printf("Cannot find a well formatted signature at offset 0x1f0000n\n");
    goto fpgacheck_end;
  }
  p = strtok( sign, "$");
  while( p)
  {
    p = strtok( 0, ">");
    if( p)
    {
      printf("%s\n", p);
      get_sign_para( p, &fpga_para);
    }
    p = strtok( 0, "$");
  }

  printf("verifying checksums...");
  p = buf + fpga_para.mcs_offset;
  cks = checksum( p, fpga_para.mcs_size);
  if( cks != fpga_para.mcs_cks)
  {
    printf("\nFPGA bitstream checksum inconsistent with signature : calculated = 0x%08x -> expected = 0x%08x\n", 
	   cks, fpga_para.mcs_cks);
  }
  else
  {
    printf("mcs OK...");
  }
  p = buf + fpga_para.fsm_offset;
  cks = checksum( p, fpga_para.fsm_size);
  if( cks != fpga_para.fsm_cks)
  {
    printf("\nFSM file checksum inconsistent with signature : calculated = 0x%08x -> expected = 0x%08x\n", 
	   cks, fpga_para.fsm_cks);
  }
  else
  {
    printf("fsm OK\n");
  }

fpgacheck_end:
  free( buf);
  fclose( bin_file);
  exit( 0);
}
