/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : sflash.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to access the
 *     PEV1000 SFLASH device.
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
 * $Log: sflash.c,v $
 * Revision 1.7  2012/07/10 10:21:48  kalantari
 * added tosca driver release 4.15 from ioxos
 *
 * Revision 1.12  2012/06/01 13:59:44  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.11  2012/03/15 15:10:07  ioxos
 * add board signature [JFG]
 *
 * Revision 1.10  2012/03/14 14:02:02  ioxos
 * update id, rdsr and wrsr commands [JFG]
 *
 * Revision 1.9  2012/01/27 15:34:58  ioxos
 * support for IFC1210 FPGAs [JFG]
 *
 * Revision 1.8  2011/10/19 13:35:03  ioxos
 * support for powerpc [JFG]
 *
 * Revision 1.7  2009/12/15 17:16:21  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.6  2009/10/15 11:45:51  ioxos
 * add rdsr and wrsr operations [JFG]
 *
 * Revision 1.5  2009/10/02 08:02:40  ioxos
 * Support for 2Mb and 4Mb and check FPGA device [CM]
 *
 * Revision 1.4  2008/11/12 13:23:44  ioxos
 * add info in signature, add check command, improve read an load commands [JFG]
 *
 * Revision 1.3  2008/08/08 12:48:08  ioxos
 *  reorganize code (cosmetic)  + support for FPGA signature (not finished yet) [JFG]
 *
 * Revision 1.2  2008/07/18 14:07:00  ioxos
 * re-organize code [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 * 
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: sflash.c,v 1.7 2012/07/10 10:21:48 kalantari Exp $";
#endif

#define DEBUGno
#include <debug.h>

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <cli.h>
#include <pevioctl.h>
#include <pevulib.h>
#include <xilinx.h>

extern struct pev_reg_remap *reg_remap;

struct sflash_fpga_para
{
  int id;
  int offset;
  int fpga_size;
  int fpga_ck;
  int len;
  char *filename;
  int mcs_offset;
  int mcs_size;
  uint mcs_cks;
  int fsm_offset;
  int fsm_size;
  uint fsm_cks;
  uint board;
};

char *
sflash_rcsid()
{
  return( rcsid);
}

struct xilinx_device
*fpga_identify( void *buf,
	  int size)
{
  int *p, i;
  struct xilinx_device *xd;

  p = (int *)buf;
  i = size;
  for( i = size; i > 0; i-=4)
  {
    /* check for synchronization pattern */
#ifdef PPC
    if( *p == pev_swap_32(0x665599aa))
#else
    if( *p == 0x665599aa)
#endif
    {
      p += 0x14;
      /* return device identifier */
      xd = &xilinx_device[0];
      while( xd->name)
      {
#ifdef PPC
	if( *p == pev_swap_32(xd->id))
#else
	if( *p == xd->id)
#endif
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
#ifdef PPC
    cks += pev_swap_32(*p++);
#else
    cks += *p++;
#endif
    size -= 4;
  }
  return(cks);
}

int
get_sign_para( char *sign,
	       struct sflash_fpga_para *para)
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

int
sflash_fpga_id( char *arg,
                struct sflash_fpga_para *p)
{
  char *q;
  int st_reg;

  p->board = pev_board();
  if( !strncmp( "pon", arg, 3))
  {
    p->fpga_size = 0x200000;
    p->fpga_ck = -1;
    p->id = 0x10;
    p->offset = 0;
    return(0);
  }
  if( !strncmp( "fsm#", arg, 4) ||
      !strncmp( "FSM#", arg, 4)   )
  {
    p->fpga_size = 0x400000;
    p->fpga_ck = -1;
    switch( arg[4])     
    {
      case '0':
      {
	p->id = 0x111;
	p->offset = 0x400000;
	return(0);
      }
      case '1':
      {
	p->id = 0x112;
	p->offset = 0x500000;
	return(0);
      }
      case '2':
      {
	p->id = 0x113;
	p->offset = 0x600000;
	return(0);
      }
      case '3':
      {
	p->id = 0x114;
	p->offset = 0x700000;
	return(0);
      }
      default:
      {
	return( -1);
      }
    }
  }
  if( !strncmp( "io#", arg, 3) ||
      !strncmp( "IO#", arg, 3)   )
  {
    p->fpga_size = 0x400000;
    p->fpga_ck = -1;
    switch( arg[3])     
    {
      case '0':
      {
	p->id = 0x11;
	p->offset = 0x800000;
	return(0);
      }
      case '1':
      {
	p->id = 0x12;
	p->offset = 0xa00000;
	return(0);
      }
      case '2':
      {
	p->id = 0x13;
	p->offset = 0xc00000;
	return(0);
      }
      case '3':
      {
	p->id = 0x14;
	p->offset = 0xe00000;
	return(0);
      }
      default:
      {
	return( -1);
      }
    }
  }
  if( !strncmp( "vx6#", arg, 4) ||
      !strncmp( "VX6#", arg, 4) ||
      !strncmp( "central#", arg, 8) ||
      !strncmp( "CENTRAL#", arg, 8)   )
  {
    char idx;

    p->fpga_size = 0x400000;
    p->fpga_ck = -1;
    if( (arg[0] == 'v') ||  (arg[0] == 'V')) idx = arg[4];
    if( (arg[0] == 'c') ||  (arg[0] == 'C')) idx = arg[8];
    switch( idx)     
    {
      case '0':
      {
	p->id = 0x20;
	p->offset = 0x000000;
	return(0);
      }
      case '1':
      {
	p->id = 0x21;
	p->offset = 0x400000;
	return(0);
      }
      case '2':
      {
	p->id = 0x22;
	p->offset = 0x800000;
	return(0);
      }
      case '3':
      {
	p->id = 0x23;
	p->offset = 0xc00000;
	return(0);
      }
      default:
      {
	return( -1);
      }
    }
  }
  if( !strncmp( "fpga#", arg, 5))
  {
    st_reg = pev_csr_rd( reg_remap->iloc_ctl);         //cm:28.09.09 Check for little or big FPGA 
    if (st_reg & (1 << 5))
    {
      p->fpga_size = 0x400000;
    }
    else
    {
      p->fpga_size = 0x200000;
    }
    p->fpga_ck = 0;
    switch( arg[5])                  //cm:28.09.09 Modify the offset management.
    {
      case '0':
      {
	p->id = 0;
	p->offset = 0;
	return(0);
      }
      case '1':
      {
	p->id = 1;
	p->offset = p->fpga_size;
	return(0);
      }
      case '2':
      {
	p->id = 2;
	p->offset = p->fpga_size * 2;
	return(0);
      }
      case '3':
      {
	p->id = 3;
	p->offset = p->fpga_size * 3;
	return(0);
      }

      default:
      {
	return( -1);
      }
    }
  }
  if (!strncmp( "all", arg, 3))
  {
    st_reg = pev_csr_rd( reg_remap->iloc_ctl);         //cm:28.09.09 Check for little or big FPGA 
    if (st_reg & (1 << 5))
    {
      p->fpga_size = 0x400000;
    }
    else
    {
      p->fpga_size = 0x200000;
    }
    p->id = 0xff;
    p->fpga_ck = -1;
    p->offset = p->fpga_size;
    return( 0);
  }
  p->id = -1;
  p->fpga_ck = -1;
  p->offset = strtoul( arg, &q, 16);
  if( q == arg)
  {
    p->offset = -1;
    return(-1);
  }
  return( 0);
}

struct sflash_cmd_arg
{
  int operation;
  struct sflash_fpga_para fpga;
};

#define SFLASH_OP_ID     1
#define SFLASH_OP_READ   2
#define SFLASH_OP_LOAD   3
#define SFLASH_OP_SIGN   4
#define SFLASH_OP_CHECK  5
#define SFLASH_OP_RDSR   6
#define SFLASH_OP_WRSR   7
#define SFLASH_OP_DUMP   8

int
sflash_scan_arg(struct cli_cmd_para *c,
                struct sflash_cmd_arg *a)
{
  int cnt, i;

  i = 0;
  cnt = c->cnt;

  a->operation = 0;
  if( cnt--)
  {
    if( !strcmp( "id", c->para[i]))
    {
      a->operation = SFLASH_OP_ID;
      return( 0);
    }
    if( !strcmp( "read", c->para[i]))
    {
      a->operation = SFLASH_OP_READ;
    }
    if( !strcmp( "dump", c->para[i]))
    {
      a->operation = SFLASH_OP_DUMP;
    }
    if( !strcmp( "load", c->para[i]))
    {
      a->operation = SFLASH_OP_LOAD;
    }
    if( !strcmp( "sign", c->para[i]))
    {
      a->operation = SFLASH_OP_SIGN;
    }
    if( !strcmp( "check", c->para[i]))
    {
      a->operation = SFLASH_OP_CHECK;
    }
    if( !strcmp( "rdsr", c->para[i]))
    {
      a->operation = SFLASH_OP_RDSR;
      return( 0);
    }
    if( !strcmp( "wrsr", c->para[i]))
    {
      a->operation = SFLASH_OP_WRSR;
      return( 0);
    }
    if( a->operation)
    {      
      i++;
      if( cnt--)
      {
	if( sflash_fpga_id( c->para[i], &a->fpga) < 0)
	{
	  return( -1);
	}
      }
      return( 0);
    }
  }
  return( -1);
}

int
sflash_sign( char *sign,
	     struct sflash_fpga_para *para)
{
  char *p;
  if( strncmp( "Signature", sign, strlen("Signature")))
  {
    return( -1);
  }
  
  printf("\n");
  p = strtok( sign, "$");
  while( p)
  {
    p = strtok( 0, ">");
    if( p)
    {
      printf("   + %s\n", p);
      get_sign_para( p, para);
    }
    p = strtok( 0, "$");
  }
  return(0);
}

int
sflash_read( int start,
	     char *buf,
	     int size,
	     int blk)
{
  int n, last;

  if( blk > size)
  {
    printf("%08x\b\b\b\b\b\b\b\b", start);
    fflush( stdout);
    usleep( 100000);
    pev_sflash_read( start, buf, size);
    start += size;
    buf += size;
  }
  else
  {
    n = size/blk;
    last = size%blk;
    while( n--)
    {
      printf("%08x\b\b\b\b\b\b\b\b", start);
      fflush( stdout);
      usleep( 100000);
      pev_sflash_read( start, buf, blk);
      start += blk;
      buf += blk;
    }
    if( last)
    {
      printf("%08x\b\b\b\b\b\b\b\b", start);
      fflush( stdout);
      usleep( 100000);
      pev_sflash_read( start, buf, last);
      start += last;
      buf += last;
    }
  }
  printf("%08x", start);
  return( size);
}

static int
sflash_write(  uint offset,
	       char *buf_src,
	       uint size)
{
  char *p, *buf_des;
  int i, n, nblk, blk_size;
  int start, first, last;
  int retval;

  if( !(offset & 0xf0000000))
  {
    blk_size = 0x40000;
  }
  else
  {
    blk_size = 0x10000;
  }

  first = offset & (blk_size -1);
  if( first)
  {
    first = blk_size - first;
  }
  if( size <= first)
  {
    first = size;
    nblk = 0;
    last = 0;
  }
  else
  {
    size -= first;
    nblk = size/blk_size;
    last = size - (nblk * blk_size);
  }
  printf("\n");
  printf("!! Programming the SFLASH device is done one bit at a time\n");
  //printf("!! It requires millions of physical accesses loading the CPU at 100%\n");
  printf("!! During that process the system will be hanging for periods of 10 seconds\n");
  printf("!! This is the time needed to program one SFLASH sector\n");
  printf("-> Just relax and sit back...\n\n");

  start = offset;
  p = buf_src;
  printf("Writing device will take about %d seconds.....", (nblk + 1)*10);
  printf("%08x\b\b\b\b\b\b\b\b", start);
  fflush( stdout);
  usleep(100000);
  retval = pev_sflash_write( start, p, first);
  p += first;
  start += first;
  n = nblk;
  while( n--)
  {
    printf("%08x\b\b\b\b\b\b\b\b", start);
    fflush( stdout);
    usleep(100000);
    retval = pev_sflash_write( start, p, blk_size);
    p += blk_size;
    start += blk_size;
  }

  printf("%08x\b\b\b\b\b\b\b\b", start);
  fflush( stdout);
  usleep(100000);
  retval = pev_sflash_write( start, p, last);
  p += last;
  start += last;
  printf("%08x ", start);

  printf(" -> done\n");
  printf("Verifying device will take about %d seconds...", (nblk + 1)*12);
  fflush( stdout);
  usleep(100000);
  buf_des = malloc( size);

  sflash_read( offset, buf_des, size, 0x4000);

  retval = 0;
  for( i = 0; i < size; i++)
  {
    if( buf_src[i] != buf_des[i])
    {
      printf(" -> compare error at offset 0x%x\n", i);
      retval = -1;
      break;
    }
  }
  if( !retval)
  {
    printf(" -> OK\n");
  }
  free( buf_des);

  return( retval);
}

int 
xprs_sflash( struct cli_cmd_para *c)
{
  int retval;
  int cnt, i;
  char *p;
  struct sflash_cmd_arg cmd;
  struct xilinx_device *xd, *xd_sflash;

  retval = -1;
  cnt = c->cnt;
  i = 0;
  cmd.operation = 0;
  cmd.fpga.id = -1;

  if( cnt--)
  {
    uint offset;
    uint size;
    char *buf_src, *buf_des;
    char *filename;
    FILE *file;

    if( sflash_scan_arg( c, &cmd) < 0)
    {
      printf("%s : bad argument\n", c->cmd);
      return( -1);
    }

    if( cmd.operation == SFLASH_OP_ID)
    {
      unsigned char id[4];

      if( pev_board() == PEV_BOARD_IFC1210)
      {
	for( i = 1; i < 4; i++)
	{
          pev_sflash_id( (char *)id, i);
          printf("SFLASH %d identifier %02x:%02x:%02x\n", i, id[0], id[1], id[2]);
	}
      }
      else
      {
        pev_sflash_id( (char *)id, 0);
        printf("SFLASH identifier %02x:%02x:%02x\n", id[0], id[1], id[2]);
      }
      return( 0); 
    }

    if( cmd.operation == SFLASH_OP_RDSR)
    {
      unsigned short sr;

      if( pev_board() == PEV_BOARD_IFC1210)
      {
	for( i = 1; i < 4; i++)
	{
	  sr = (unsigned short)pev_sflash_rdsr( i);
	  printf("SFLASH %d status %04x\n", i, sr);
	}
      }
      else
      {
	sr = (unsigned short)pev_sflash_rdsr( 0);
	printf("SFLASH status %04x\n", sr);
      }
      return( 0); 
    }

    if( cmd.operation == SFLASH_OP_WRSR)
    {
      int sr;
      int dev;

      dev = 0;
      if( c->cnt < 3)
      {
	printf("usage: sflash wrsr <dev> <sr>\n");
	return( -1);
      }
      dev = strtoul( c->para[1], &p, 16);
      sr = strtoul( c->para[2], &p, 16);
      pev_sflash_wrsr( sr, dev);
      return( 0); 
    }

    if( cmd.operation == SFLASH_OP_SIGN)
    {
      char *sign;
      int i;

      if( cmd.fpga.id == -1)
      {
	printf("Invalid FPGA identifier : %s\n", c->para[1]);
	return( -1);
      }

      sign = malloc( 0x1000);
      if( sign)
      {
	if( cmd.fpga.id == 0xff)
	{
	  offset =  cmd.fpga.fpga_size - 0x10000;             // cm:28.09.09 Modify the offset management
	  for (i=0; i<4; i++)
	  {
	    printf("FPGA#%d Signature at offset 0x%x\n", i, offset);
	    pev_sflash_read( offset, sign, 0x1000);
	    sflash_sign( sign, &cmd.fpga);
	    offset += cmd.fpga.fpga_size;
	  }
	  free( sign);
	  return( 0);
	}
	else
	{
	  offset = cmd.fpga.offset + cmd.fpga.fpga_size - 0x10000;             // cm:28.09.09 Modify the offset management 
	  printf("FPGA#%d Signature at offset 0x%x\n", cmd.fpga.id, offset);
	  pev_sflash_read( offset, sign, 0x1000);
	  sflash_sign( sign, &cmd.fpga);
	  free( sign);
	  return( 0);
	}
      }
      return( -1); 
    }
    if( cmd.operation == SFLASH_OP_CHECK)
    {
      char *sign, *buf;
      int cks;

      if( cmd.fpga.id == -1)
      {
	printf("Invalid FPGA identifier : %s\n", c->para[1]);
	return( -1);
      }
      buf = malloc( cmd.fpga.fpga_size);                // cm:28.09.09
      sign = buf + (cmd.fpga.fpga_size - 0x10000);
      if( buf)
      {
	offset = cmd.fpga.offset + cmd.fpga.fpga_size - 0x10000;             // cm:28.09.09
	printf("FPGA#%d Signature at offset 0x%x\n", cmd.fpga.id, offset);
	pev_sflash_read( offset, sign, 0x1000);
	sflash_sign( sign, &cmd.fpga);
	offset = cmd.fpga.offset + cmd.fpga.mcs_offset;
	printf("Reading FPGA bitstream from offset 0x%x [0x%x] -> ", offset, cmd.fpga.mcs_size);
	sflash_read( offset, buf + cmd.fpga.mcs_offset, cmd.fpga.mcs_size, 0x4000);
	printf("\nVerifying FPGA bitstream checksum...");
	cks = checksum( buf + cmd.fpga.mcs_offset,  cmd.fpga.mcs_size);
        if( cks != cmd.fpga.mcs_cks)
        {
          printf("\nFPGA bitstream checksum inconsistent with signature : calculated = 0x%08x -> expected = 0x%08x\n", 
		 cks, cmd.fpga.mcs_cks);
        }
        else
        {
          printf("OK\n");
        }
	offset = cmd.fpga.offset + cmd.fpga.fsm_offset;       //cm:28.09.09
	printf("Reading FSM instructions from offset 0x%x [0x%x] -> ", offset, cmd.fpga.fsm_size);
	sflash_read( offset, buf + cmd.fpga.fsm_offset, cmd.fpga.fsm_size, 0x4000);
	printf("\nVerifying FSM instruction checksum...");
	cks = checksum( buf + cmd.fpga.fsm_offset,  cmd.fpga.fsm_size);
        if( cks != cmd.fpga.fsm_cks)
        {
          printf("\nFSM instruction checksum inconsistent with signature : calculated = 0x%08x -> expected = 0x%08x\n", 
		 cks, cmd.fpga.fsm_cks);
        }
        else
        {
          printf("OK\n");
        }
	free( buf);
	return( 0);
      }
      return( -1); 
    }
    if( cmd.operation == SFLASH_OP_READ)
    {
      i++;
      if( cnt > 2)
      {
	offset = cmd.fpga.offset;
	if( cmd.fpga.id != -1)
	{
	  offset |= (cmd.fpga.id & 0xf0) << 24;
	}
	size = strtoul( c->para[i+1], &p, 16);
	if( p == c->para[i+1])
        {
	  return(-1);
	}
	filename = (char *)c->para[i+2];
	printf("Reading SFLASH from offset %x [size %x] into file %s...", offset, size, c->para[i+2]);
	buf_des = malloc( size);

	sflash_read( offset, buf_des, size, 0x4000);


	printf(" -> done\n");

	printf("Creating file %s\n", filename);
	file = fopen( filename, "w");
        if( !file)
        {
	  printf("Cannot create file %s\n", filename);
	}
	else
	{
	  fwrite( buf_des, 1, size, file);
	  fclose( file);
	}
	free( buf_des);
	return(0);
      }
      return( -1);
    }

    if( cmd.operation == SFLASH_OP_DUMP)
    {
      char *p;
      int j;


      i++;
      if( cnt > 1)
      {
	offset = cmd.fpga.offset;
	if( cmd.fpga.id != -1)
	{
	  offset |= (cmd.fpga.id & 0xf0) << 24;
	}
	size = strtoul( c->para[i+1], &p, 16);
	if( p == c->para[i+1])
        {
	  return(-1);
	}
	printf("Dumping SFLASH %d from offset %x [size %x] ...", offset >> 28, offset & 0xfffffff, size);
	buf_des = malloc( size);

	sflash_read( offset, buf_des, size, 0x4000);


	printf(" -> done\n");

	p = (char *)buf_des;
	for( j = 0; j < size; j += 16)
        {
          char *pp;

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


	free( buf_des);
	return(0);
      }
      return( -1);
    }
    if( cmd.operation == SFLASH_OP_LOAD)
    {
      char yn, *buf_check;
      int split;

      i++;
      if( cnt > 1)
      {
	if( sflash_fpga_id( c->para[i], &cmd.fpga) < 0)
	{
	  printf("Bad offset parameter\n");
	  return( -1);
	}
	offset = cmd.fpga.offset;
	if( (offset < cmd.fpga.fpga_size) && (cmd.fpga.id == 0))
	{
	  printf("!! You are going to overwrite the SFLASH first sector [%x]!!!!\n", offset);
	  printf("!! That operation can kill the PEV1100 interface...\n");
	  printf("!! Do you want to continue [y/n] n ->  ");
	  scanf("%c", &yn);
	  printf("\n");
	  if( yn != 'y')
	  {
	    return( 0);
	  }
	  printf("Are you sure  [y/n] n ->  ");
	  scanf("%c", &yn);
	  printf("\n");
	  if( yn != 'y')
	  {
	    return( 0);
	  }
	}
	printf("Loading SFLASH from file %s at offset 0x%x ", c->para[i+1], offset);
	file = fopen( c->para[i+1], "r");
        if( !file)
        {
	  printf("\nFile %s doesn't exist\n", c->para[i+1]);
	  return( -1);
	}
	fseek( file, 0, SEEK_END);
	size = ftell( file);
	printf("[size 0x%x]\n", size);
	split = 1;
	if(  (cmd.fpga.id & 0xf0) == 0x20) split = 2;
	if( offset + (size/split) > 0x1000000)
	{
	  printf("parameter out of range -> offset+size overflows SFLASH\n");
	  fclose( file);
	  return( -1);
	}
	fseek( file, 0, SEEK_SET);
	buf_src = malloc( size);
	fread( buf_src, 1, size, file);
	fclose( file);
	if( cmd.fpga.fpga_ck != -1)
	{
	  printf("FPGA check device ...");
	  buf_check = malloc(0x200);
	  xd = fpga_identify( buf_src, 0x200);
	  pev_sflash_read( cmd.fpga.fpga_ck, buf_check, 0x200);
	  xd_sflash = fpga_identify( buf_check, 0x200);
	  
	  if( !xd || !xd_sflash)
	  {
	      printf("-> unknown FPGA device in file\n");
	      return( -1);
	  }
	  else 
	  {
	    if( xd->id == xd_sflash->id)
	    {
	      printf("-> done for this device [%s]",xd->name);
	    }
	    else
	    {
	      printf("-> %s not ok for this device [%s]\n",xd->name,xd_sflash->name);
	      return( -1);
	    }
	  }
	  free(buf_check);
	}

	if( cmd.fpga.id != -1)
	{
	  offset |= (cmd.fpga.id & 0xf0) << 24;
	}
	if(  (cmd.fpga.id & 0xf0) == 0x20)
	{
	  char *buf_d, *d, *s;

	  buf_d = (char *)malloc( size/2);
          d = buf_d;
          s = buf_src;
          for( i = 0; i < size; i+=2)
          {
            *d = ((*s&0xf)<<4) | (*(s+1)&0xf);
            d += 1;
            s += 2;
          }
          printf("transferring first half...\n");
	  retval = sflash_write( offset, buf_d, size/2);

          d = buf_d;
          s = buf_src;
          for( i = 0; i < size; i+=2)
          {
            *d = (*s&0xf0) | ((*(s+1)&0xf0)>>4);
             d += 1;
             s += 2;
          }
          printf("transferring second half...\n");
	  offset += 0x10000000;
	  retval = sflash_write( offset, buf_d, size/2);
	  free( buf_d);
	}
	else
        {
	  retval = sflash_write( offset, buf_src, size);
	}

	free( buf_src);
	return( retval);
      }
      return( -1);
    }
  }
  return( retval);
}

static char sign_para[16][16];
struct cli_cmd_history sign_history;

int 
xprs_sign( struct cli_cmd_para *c)
{
  char *para_p;
  char *board_sign;
  int sign_offset;
  char *p;

  if( pev_board() != PEV_BOARD_IFC1210)
  {
    printf("Board signature not supported on this type of board\n");
    return(-1);
  }
  sign_offset = 0x103c0000;
  board_sign = (char *)malloc(0x1000);
  bzero( board_sign, 0x1000);
  p = board_sign;
  if( c->cnt)
  {
    if( !strcmp( "set", c->para[0]))
    {
      cli_history_init( &sign_history);
      printf("Setting board signature...\n");
      para_p = cli_get_cmd( &sign_history, "password: ");
      if( strcmp( "goldorak", para_p))
      {
	printf("Wrong password !!\n");
      }
      printf("\n");
      para_p = cli_get_cmd( &sign_history, "Board type: ");
      strcpy( sign_para[0], para_p);
      sprintf( p,"Board type: %s\n",  sign_para[0]);
      p += strlen(p);
      para_p = cli_get_cmd( &sign_history, "Serial number: ");
      strcpy( sign_para[1], para_p);
      sprintf( p, "Serial number: %s\n", sign_para[1]);
      p += strlen(p);
      para_p = cli_get_cmd( &sign_history, "Hardware revision: ");
      strcpy( sign_para[2], para_p);
      sprintf( p, "Hardware revision: %s\n", sign_para[2]);
      p += strlen(p);
      para_p = cli_get_cmd( &sign_history, "Fabrication date: ");
      strcpy( sign_para[3], para_p);
      sprintf( p, "Fabrication date: %s\n", sign_para[3]);
      p += strlen(p);
      para_p = cli_get_cmd( &sign_history, "Test date: ");
      strcpy( sign_para[4], para_p);
      sprintf( p, "Test date: %s\n", sign_para[4]);
      p += strlen(p);
      para_p = cli_get_cmd( &sign_history, "End customer: ");
      strcpy( sign_para[5], para_p);
      sprintf( p, "End customer: %s\n", sign_para[5]);
      p += strlen(p);
      para_p = cli_get_cmd( &sign_history, "Comment: ");
      strcpy( sign_para[6], para_p);
      sprintf( p, "Comment: %s\n", sign_para[6]);
      p += strlen(p)+1;
      *p = 0;
      printf("\n%s\n", board_sign);
      pev_sflash_write( sign_offset, board_sign, 0x1000);
      return( 0);
    }
  }
  pev_sflash_read( sign_offset, board_sign, 0x1000);
  printf("\n%s\n", board_sign);

  return( 0);
}
