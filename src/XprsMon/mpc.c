/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : mcp.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to control the
 *     MCP boards.
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
 * $Log: mpc.c,v $
 * Revision 1.3  2012/04/25 13:18:28  kalantari
 * added i2c epics driver and updated linux driver to v.4.10
 *
 * Revision 1.1  2011/03/15 09:25:04  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: mpc.c,v 1.3 2012/04/25 13:18:28 kalantari Exp $";
#endif

#define DEBUGno
#include <debug.h>

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <cli.h>
#include <pevioctl.h>
#include <pevulib.h>
#include <mpcioctl.h>


struct mpc
{
  int fd;
  struct pev_ioctl_map_pg a24_map;
  struct mpc_ioctl_map map_io;
} mpc[16];

struct mpc_i2c_devices i2c_devices[] =
{
  { "max5970",  0x45000030},
  { "bmr450_0", 0x45000000},
  { "bmr450_1", 0x45000032},
  { "lm95235_0",0x0400004c},
  { "lm95235_1",0x0400001c},
  { "pca9502",  0x05000048},
  { "ics8n3q01",0xe500006e},
  { NULL,       0x00000000}
};

uint mpc_idx = -1;
static uchar xilinx_head13[] = {0, 9, 15, 240, 15, 240, 15, 240, 15, 240, 0, 0, 1};

int 
mpc_init( void)
{
  int i;

  for( i = 1; i < 16; i++)
  {
    mpc[i].fd = -1;
    mpc[i].a24_map.rem_addr = 0;
    mpc[i].a24_map.size = 0;
    mpc[i].map_io.paddr = 0;
    mpc[i].map_io.vaddr = 0;
    mpc[i].map_io.size = 0;
  }
  return(0);
}
int 
mpc_exit( void)
{
  int i;

  for( i = 1; i < 16; i++)
  {
    if( mpc[i].fd >= 0)
    {
      if( mpc[i].map_io.size)
      {
        ioctl( mpc[i].fd, MPC_IOCTL_UNMAP_CSR, 0);
      }
      close(mpc[i].fd );
    }
    if( mpc[i].a24_map.size)
    {
      pev_map_free( &mpc[i].a24_map);
      mpc[i].a24_map.size = 0;
    }
  }
  return(0);
}

int
mpc_get_idx( struct cli_cmd_para *c)
{
  int idx;

  if( c->ext)
  {
    if( sscanf( c->ext,"%d", &idx) == 1)
    {
      if( ( idx > 0) && ( idx < 16))
      {
	return(idx);
      }
    }
  }
  return( -1);
}

int
mpc_map( struct cli_cmd_para *c)
{
  int i, idx, fd;
  uint base, size;

  idx = mpc_get_idx( c);
  if( idx != -1)
  {
    mpc_idx = idx;
  }

  if( mpc_idx == -1)
  {
    return( -1);
  }

  printf("mapping MPF#%d\n", mpc_idx);
  for( i = 1; i < c->cnt; i++)
  {
    printf("c->para[%d] = %s\n", i, c->para[i]);
    if( sscanf( c->para[i],"cr:%x.%x", &base, &size) == 2)
    {
      if( mpc[mpc_idx].a24_map.size)
      {
	pev_map_free( &mpc[mpc_idx].a24_map);
      }
      mpc[mpc_idx].a24_map.rem_addr = base;
      mpc[mpc_idx].a24_map.mode = MAP_SPACE_VME | MAP_VME_CR | MAP_SWAP_AUTO | MAP_ENABLE_WR | MAP_ENABLE;
      mpc[mpc_idx].a24_map.flag = 0x0;
      mpc[mpc_idx].a24_map.sg_id = MAP_MASTER_32;
      mpc[mpc_idx].a24_map.size = size;
      pev_map_alloc( &mpc[mpc_idx].a24_map);
    }
    if( sscanf( c->para[i],"a24:%x.%x", &base, &size) == 2)
    {
      if( mpc[mpc_idx].a24_map.size)
      {
	pev_map_free( &mpc[mpc_idx].a24_map);
      }
      mpc[mpc_idx].a24_map.rem_addr = base;
      mpc[mpc_idx].a24_map.mode = MAP_SPACE_VME | MAP_VME_A24 | MAP_SWAP_AUTO | MAP_ENABLE_WR | MAP_ENABLE;
      mpc[mpc_idx].a24_map.flag = 0x0;
      mpc[mpc_idx].a24_map.sg_id = MAP_MASTER_32;
      mpc[mpc_idx].a24_map.size = size;
      pev_map_alloc( &mpc[mpc_idx].a24_map);
    }
  }
  printf("a24 = %lx - %lx [%x]\n", mpc[mpc_idx].a24_map.rem_addr, mpc[mpc_idx].a24_map.loc_addr, mpc[mpc_idx].a24_map.size);
  printf("pci_base = %lx\n", mpc[mpc_idx].a24_map.pci_base);

  if( mpc[mpc_idx].fd < 0)
  {
    mpc[mpc_idx].fd = open("/dev/mpc0", O_RDWR);
    if( mpc[mpc_idx].fd < 0)
    {
      printf("cannot open mpc%d\n", mpc_idx);
      return(-1);
    }
  }
  fd = mpc[mpc_idx].fd;

  if( mpc[mpc_idx].map_io.size)
  {
    ioctl( fd, MPC_IOCTL_UNMAP_CSR, 0);
  }
  mpc[mpc_idx].map_io.paddr = mpc[mpc_idx].a24_map.pci_base + mpc[mpc_idx].a24_map.loc_addr;
  mpc[mpc_idx].map_io.vaddr = mpc[mpc_idx].a24_map.rem_addr;
  mpc[mpc_idx].map_io.size = mpc[mpc_idx].a24_map.size;
  printf("mapping MPC#%d at PCI address %lx [%x]\n", 
	  mpc_idx, mpc[mpc_idx].map_io.paddr, mpc[mpc_idx].map_io.size);
  ioctl( fd, MPC_IOCTL_MAP_CSR, &mpc[mpc_idx].map_io);

  return(0);
}

int
mpc_unmap( struct cli_cmd_para *c)
{
  int i, idx;
  uint base, size;

  idx = mpc_get_idx( c);
  if( idx != -1)
  {
    mpc_idx = idx;
  }

  if( mpc_idx == -1)
  {
    return( -1);
  }
  printf("unmapping MPC#%d\n", mpc_idx);
  if( mpc[mpc_idx].fd >= 0)
  {
    ioctl( mpc[mpc_idx].fd, MPC_IOCTL_UNMAP_CSR, 0);
    close(mpc[mpc_idx].fd );
  }
  if( mpc[mpc_idx].a24_map.size)
  {
    pev_map_free( &mpc[mpc_idx].a24_map);
    mpc[mpc_idx].a24_map.size = 0;
  }

  return(0);
}



static int
sflash_read( int fd,
             int start,
	     char *buf,
	     int size,
	     int blk,
	     int dev)
{
  struct mpc_ioctl_sflash_rw sflash_rw;
  int n, last;

  sflash_rw.dev = dev;
  if( blk > size)
  {
    printf("%08x\b\b\b\b\b\b\b\b", start);
    fflush( stdout);
    usleep( 100000);
    sflash_rw.len = size;
    sflash_rw.buf = buf;
    sflash_rw.offset = start;
    ioctl( fd, MPC_IOCTL_SFLASH_RD, &sflash_rw);
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
      sflash_rw.len = blk;
      sflash_rw.buf = buf;
      sflash_rw.offset = start;
      ioctl( fd, MPC_IOCTL_SFLASH_RD, &sflash_rw);
      start += blk;
      buf += blk;
    }
    if( last)
    {
      printf("%08x\b\b\b\b\b\b\b\b", start);
      fflush( stdout);
      usleep( 100000);
      sflash_rw.len = last;
      sflash_rw.buf = buf;
      sflash_rw.offset = start;
      ioctl( fd, MPC_IOCTL_SFLASH_RD, &sflash_rw);
      start += last;
      buf += last;
    }
  }
  printf("%08x", start);
  return( size);
}


static int
sflash_write(  int fd,
               uint offset,
	       char *buf_src,
	       uint size,
	       uint dev)
{
  struct mpc_ioctl_sflash_rw sflash_rw;
  uint i, n, nblk, blk_size;
  uint start, first, last;
  char *p, *buf_des;
  int retval;

  retval = 0;
  blk_size = 0x10000;
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
  printf("!! It requires millions of physical accesses loading the CPU at 100%\n");
  printf("!! During that process the system will be hanging for periods of 3 seconds\n");
  printf("!! This is the time needed to program one SFLASH sector\n");
  printf("-> Just relax and sit back...\n\n");

  start = offset;
  p = buf_src;
  sflash_rw.dev = dev;
  printf("Writing device will take about %d seconds.....", (nblk + 1)*3);
  printf("%08x\b\b\b\b\b\b\b\b", start);
  fflush( stdout);
  usleep(100000);
  sflash_rw.len = first;
  sflash_rw.buf = p;
  sflash_rw.offset = start;
  ioctl( fd, MPC_IOCTL_SFLASH_WR, &sflash_rw);

  p += first;
  start += first;
  n = nblk;
  while( n--)
  {
     printf("%08x\b\b\b\b\b\b\b\b", start);
     fflush( stdout);
     usleep(100000);
     sflash_rw.len = blk_size;
     sflash_rw.buf = p;
     sflash_rw.offset = start;
     ioctl( fd, MPC_IOCTL_SFLASH_WR, &sflash_rw);
     p += blk_size;
     start += blk_size;
  }

  printf("%08x\b\b\b\b\b\b\b\b", start);
  fflush( stdout);
  usleep(100000);
  sflash_rw.len = last;
  sflash_rw.buf = p;
  sflash_rw.offset = start;
  ioctl( fd, MPC_IOCTL_SFLASH_WR, &sflash_rw);
  p += last;
  start += last;
  printf("%08x ", start);

  printf(" -> done\n");
  printf("Verifying device will take about %d seconds...", (nblk + 1)*4);
  fflush( stdout);
  usleep(100000);
  buf_des = malloc( size);

  sflash_read( fd, offset, buf_des, size, 0x4000, dev);

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

  return( 0);
}
int xilinx_header( char *hp)
{
  uint i,j;
  short len;
  char letter;

  i = 0;
  if( strncmp( xilinx_head13, hp, 13))
  {
    printf("file doesn't seem to be Xilinx bit stream\n");
    return(-1);
  }
  i += 13;
  letter = hp[i];
  while( letter < 0x65)
  {
    len = ( hp[i+1] << 8) + hp[i+2];
    printf("%c:%02x:%s\n", letter, len, &hp[i+3]);
    i += len+3;
    letter = hp[i];
  }

  return(i);
}

int 
xilinx_load_sign(  struct cli_cmd_para *c,
		  int fd)
{
  struct mpc_ioctl_sflash_rw sflash_rw;
  unsigned char *buf;
  uint i, size;
  FILE *file;
  uint off, len;

  if( c->cnt < 4)
  {
    printf("usage: mpc.<i> sflash load SIGN <filename>\n");
    return(-1);
  }

  file = fopen( c->para[3], "r");
  if( !file)
  {
    printf("\nFile %s doesn't exist\n", c->para[3]);
    return( -1);
  }
  fseek( file, 0, SEEK_END);
  size = ftell( file);
  fseek( file, 0, SEEK_SET);
  buf = malloc( size);
  fread( buf, 1, size, file);
  fclose( file);


  printf("loading signature file %s [size 0x%x]\n", c->para[3], len);
  sflash_write( fd, 0x100000, buf, size, 1); 

  free( buf);

  return(0);
}

int 
xilinx_load_pon(  struct cli_cmd_para *c,
		  int fd)
{
  struct mpc_ioctl_sflash_rw sflash_rw;
  unsigned char *buf;
  uint i, size, idx;
  FILE *file;
  uint off, len;

  if( c->cnt < 4)
  {
    printf("usage: mpc.<i> sflash load PON#i <filename>\n");
    return(-1);
  }
  if( sscanf( c->para[2], "PON#%d", &idx) != 1)
  {
    printf("\nBad FPGA identifier: %s\n", c->para[2]);
    return( -1);
  }
  if( (idx < 0)||(idx >3))
  {
    printf("\nFPGA index out of range: %d\n", idx);
    return( -1);
  }

  file = fopen( c->para[3], "r");
  if( !file)
  {
    printf("\nFile %s doesn't exist\n", c->para[3]);
    return( -1);
  }
  fseek( file, 0, SEEK_END);
  size = ftell( file);
  fseek( file, 0, SEEK_SET);
  buf = malloc( size);
  fread( buf, 1, size, file);
  fclose( file);

  off = 0x200000 + idx*0x80000;

  printf("loading PON fsm file %s [size 0x%x]\n", c->para[3], len);
  sflash_write( fd, off, buf, size, 1); 

  free( buf);

  return(0);
}

int 
xilinx_load_sp1(  struct cli_cmd_para *c,
		  int fd)
{
  struct mpc_ioctl_sflash_rw sflash_rw;
  unsigned char *buf;
  uint i, size;
  FILE *file;
  uint off, len;

  if( c->cnt < 4)
  {
    printf("usage: mpc.<i> sflash load SP1 <filename>\n");
    return(-1);
  }

  file = fopen( c->para[3], "r");
  if( !file)
  {
    printf("\nFile %s doesn't exist\n", c->para[3]);
    return( -1);
  }
  fseek( file, 0, SEEK_END);
  size = ftell( file);
  fseek( file, 0, SEEK_SET);
  buf = malloc( size);
  fread( buf, 1, size, file);
  fclose( file);

  i = xilinx_header( buf);
  if( i < 0)
  {
    return( -1);
  }
  len = (buf[i+1] << 24) + (buf[i+2] << 16) + (buf[i+3] << 8) + buf[i+4];

  printf("loading SP file %s [size 0x%x]\n", c->para[3], len);
  sflash_write( fd, 0, buf + i + 5, len, 1); 

  free( buf);

  return(0);
}

int 
xilinx_load_vx6(  struct cli_cmd_para *c,
		  int fd)
{
  unsigned char *buf_d, *d, *buf_s, *s;
  uint i, size, offset, len, idx;
  FILE *file;

  if( c->cnt < 4)
  {
    printf("usage: mpc.<i> sflash load VX6#<i> <filename>\n");
    return(-1);
  }

  if( sscanf( c->para[2], "VX6#%d", &idx) != 1)
  {
    printf("\nBad FPGA identifier: %s\n", c->para[2]);
    return( -1);
  }
  if( (idx < 0)||(idx >3))
  {
    printf("\nFPGA index out of range: %d\n", idx);
    return( -1);
  }

  file = fopen( c->para[3], "r");
  if( !file)
  {
    printf("\nFile %s doesn't exist\n", c->para[3]);
    return( -1);
  }
  fseek( file, 0, SEEK_END);
  size = ftell( file);
  printf("[size 0x%x]\n", size);
  fseek( file, 0, SEEK_SET);
  buf_s = malloc( size);
  fread( buf_s, 1, size, file);
  fclose( file);

#ifdef JFG
  i = xilinx_header( buf_s);
  if( i < 0)
  {
    return( -1);
  }
  len = ((buf_s[i+1] << 24) + (buf_s[i+2] << 16) + (buf_s[i+3] << 8) + buf_s[i+4])/2;
  buf_d = (char *)malloc( len);
  offset = 0;
  printf("loading VX6 file %s [size 0x%x]\n", c->para[3], len);
#endif
  len = size;
  buf_d = (char *)malloc( len/2);
  offset = idx*0x400000;
  printf("loading VX6 file %s [size 0x%x]\n", c->para[3], len);

  d = buf_d;
  s = buf_s;
  for( i = 0; i < len; i+=2)
  {
    *d = ((*s&0xf)<<4) | (*(s+1)&0xf);
    d += 1;
    s += 2;
  }
  printf("transferring first half...\n");
  sflash_write( fd, offset, buf_d, len/2, 2); 

  d = buf_d;
  s = buf_s;
  for( i = 0; i < len; i+=2)
  {
    *d = (*s&0xf0) | ((*(s+1)&0xf0)>>4);
    d += 1;
    s += 2;
  }
  printf("transferring second half...\n");
  sflash_write( fd, offset, buf_d, len/2, 3); 


  free( buf_d);
  free( buf_s);
  return(0);
}

int 
xilinx_load_sp23(  struct cli_cmd_para *c,
		   int fd)
{
  unsigned char *buf, *buf1, *buf2;
  uint i, j, idx;
  uint offset;
  uint size1, size2;
  uint len1, len2;
  FILE *file1, *file2;

#ifdef JFG
  if( c->cnt < 5)
  {
    printf("usage: mpc.<i> sflash load SP23#i <file1> <file2>\n");
    return(-1);
  }
#endif

  if( c->cnt < 4)
  {
    printf("usage: mpc.<i> sflash load SP23#i <file1>\n");
    return(-1);
  }
  file1 = fopen( c->para[3], "r");
  if( !file1)
  {
    printf("\nFile %s doesn't exist\n", c->para[3]);
    return( -1);
  }
  if( sscanf( c->para[2], "SP23#%d", &idx) != 1)
  {
    printf("\nBad FPGA identifier: %s\n", c->para[2]);
    return( -1);
  }
  if( (idx < 0)||(idx >3))
  {
    printf("\nFPGA index out of range: %d\n", idx);
    return( -1);
  }
#ifdef JFG
  file2 = fopen( c->para[4], "r");
  if( !file2)
  {
    fclose( file2);
    printf("\nFile %s doesn't exist\n", c->para[4]);
    return( -1);
  }
  fseek( file1, 0, SEEK_END);
  size1 = ftell( file1);
  printf("[size1 0x%x]\n", size1);
  fseek( file1, 0, SEEK_SET);
  buf1 = malloc( size1);
  fread( buf1, 1, size1, file1);
  fclose( file1);

  fseek( file2, 0, SEEK_END);
  size2 = ftell( file2);
  printf("[size2 0x%x]\n", size2);
  fseek( file2, 0, SEEK_SET);
  buf2 = malloc( size2);
  fread( buf2, 1, size2, file2);
  fclose( file2);

  i = xilinx_header( buf1);
  if( i < 0)
  {
    return( -1);
  }
  len1 = (buf1[i+1] << 24) + (buf1[i+2] << 16) + (buf1[i+3] << 8) + buf1[i+4];

  j = xilinx_header( buf2);
  if( j < 0)
  {
    return( -1);
  }
  len2 = (buf2[j+1] << 24) + (buf2[j+2] << 16) + (buf2[j+3] << 8) + buf2[j+4];

  buf = malloc(len1 + len2);
  offset = 0x400000;
  memcpy( buf, buf1, len1);
  memcpy( buf+len1, buf2, len2);
  printf("loading SP files %s + %s [size 0x%x + 0x%x]\n", c->para[3], c->para[4], len1, len2);
  sflash_write( fd, offset, buf, len1+len2, 1); 
#endif
  fseek( file1, 0, SEEK_END);
  size1 = ftell( file1);
  printf("[size1 0x%x]\n", size1);
  fseek( file1, 0, SEEK_SET);
  buf1 = malloc( size1);
  fread( buf1, 1, size1, file1);
  fclose( file1);

  offset = 0x400000 + idx*0x100000;
  len1 = size1;
  buf = buf1;
  printf("loading SP files %s at offset 0x%x[size 0x%x]\n", c->para[3], offset, len1);
  sflash_write( fd, offset, buf, len1, 1); 

#ifdef JFG
  free( buf1);
  free( buf2);
#endif
  free( buf);

  return(0);
}

int
mpc_sflash( struct cli_cmd_para *c)
{
  uint flash_id;
  int i, idx;
  uint base, size;

  idx = mpc_get_idx( c);
  if( idx != -1)
  {
    mpc_idx = idx;
  }

  if( mpc_idx == -1)
  {
    return( -1);
  }
  if( mpc[mpc_idx].fd < 0)
  {
    //return( -1);
  }
  if( c->cnt < 2)
  {
    printf("mpc sflash command needs more arguments\n");
    return(-1);
  }

  if( !strcmp( "wrsr", c->para[1]))
  {
  }
  if( !strcmp( "rdsr", c->para[1]))
  {
    flash_id = 0;
    ioctl( mpc[mpc_idx].fd, MPC_IOCTL_SFLASH1_RDSR, &flash_id);
    printf("flash#1 status = %x\n", flash_id);
    flash_id = 0;
    ioctl( mpc[mpc_idx].fd, MPC_IOCTL_SFLASH2_RDSR, &flash_id);
    printf("flash#2 status = %x\n", flash_id);
    flash_id = 0;
    ioctl( mpc[mpc_idx].fd, MPC_IOCTL_SFLASH3_RDSR, &flash_id);
    printf("flash#3 status = %x\n", flash_id);
  }
  if( !strcmp( "id", c->para[1]))
  {
    flash_id = 0;
    ioctl( mpc[mpc_idx].fd, MPC_IOCTL_SFLASH1_ID, &flash_id);
    printf("flash#1 id = %x\n", flash_id);
    flash_id = 0;
    ioctl( mpc[mpc_idx].fd, MPC_IOCTL_SFLASH2_ID, &flash_id);
    printf("flash#2 id = %x\n", flash_id);
    flash_id = 0;
    ioctl( mpc[mpc_idx].fd, MPC_IOCTL_SFLASH3_ID, &flash_id);
    printf("flash#3 id = %x\n", flash_id);
  }
  if( !strcmp( "read", c->para[1]))
  {
    struct mpc_ioctl_sflash_rw sflash_rw;
    unsigned char *p;
    uint i, j, offset, dev, size;
    FILE *file;

    if( c->cnt < 5)
    {
      printf("usage: mpc.<i> sflash read fpga#i offset size\n");
      return(-1);
    }
    if( sscanf( c->para[2],"fpga#%d", &dev) != 1)
    {
      printf("invalid fpga identifier: %s\n", c->para[2]);
      printf("usage: mpc.<i> sflash read fpga#i offset size\n");
      return(-1);
    }
    if( sscanf( c->para[3],"%x", &offset) != 1)
    {
      printf("invalid offset: %s\n", c->para[3]);
      printf("usage: mpc.<i> sflash read fpga#i offset size\n");
      return(-1);
    }
    if( sscanf( c->para[4],"%x", &size) != 1)
    {
      printf("invalid size: %s\n", c->para[4]);
      printf("usage: mpc.<i> sflash read fpga#i offset size\n");
      return(-1);
    }
    sflash_rw.len = size;
    sflash_rw.buf = (void *)malloc(sflash_rw.len);
    sflash_rw.offset = offset;
    sflash_rw.dev = dev;

    printf("reading FPGA#%d at offset %x [%x]\n", dev, offset, size);
    p = (char *)sflash_rw.buf;
    for( i = 0; i < sflash_rw.len; i++)
    {
      *p++ = 0;
    }
    ioctl( mpc[mpc_idx].fd, MPC_IOCTL_SFLASH_RD, &sflash_rw);
    p = (char *)sflash_rw.buf;
    for( j = 0; j < size; j += 16)
    {
      unsigned char *pp;

      printf("%08x ", offset + j);
      pp = p;
      for( i = 0; i < 16; i++)
      {
        printf("%02x ", *p++);
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
    free( sflash_rw.buf);
  }
  if( !strcmp( "load", c->para[1]))
  {

    if( c->cnt < 3)
    {
      printf("usage: mpc.<i> sflash load <fpga> ...\n");
      return(-1);
    }
    if( !strncmp( c->para[2], "VX6#", 4))
    {
      return( xilinx_load_vx6( c,  mpc[mpc_idx].fd));
    }
    if( !strncmp( c->para[2], "SP1", 3))
    {
      return( xilinx_load_sp1( c,  mpc[mpc_idx].fd));
    }
    if( !strncmp( c->para[2], "SP23#", 5))
    {
      return( xilinx_load_sp23( c, mpc[mpc_idx].fd));
    }
    if( !strncmp( c->para[2], "SIGN", 4))
    {
      return( xilinx_load_sign( c, mpc[mpc_idx].fd));
    }
    if( !strncmp( c->para[2], "PON", 3))
    {
      return( xilinx_load_pon( c, mpc[mpc_idx].fd));
    }
  }
  return(-1);
}

int
mpc_i2c( struct cli_cmd_para *c)
{
  struct mpc_ioctl_i2c i2c;
  struct mpc_i2c_devices *i2d;
  uint flash_id;
  int i, idx;
  uint dev, reg, data;

  idx = mpc_get_idx( c);
  if( idx != -1)
  {
    mpc_idx = idx;
  }

  if( mpc_idx == -1)
  {
    return( -1);
  }
  if( mpc[mpc_idx].fd < 0)
  {
    //return( -1);
  }
  if( c->cnt < 4)
  {
    printf("mpc i2c command needs more arguments\n");
    printf("usage: mpc.<i> <dev> <op> <reg> [<data>]\n");
    printf("i2c device list:\n");
    i2d = &i2c_devices[0];
    while( i2d->name)
    {
      printf("   - %s\n", i2d->name);
      i2d++;
    }
    return(-1);
  }
  i2d = &i2c_devices[0];
  i2c.device = 0;
  printf("scanning i2d dev table\n");
  while( i2d->name)
  {
    if( !strcmp( i2d->name, c->para[1]))
    {
      i2c.device = i2d->id;
      break;
    }
    i2d++;
  }
  if( !i2c.device)
  {

    printf("wrong device name\n");
    printf("usage: mpc.<i> <dev> <op> <reg> [<data>]\n");
    printf("i2c device list:\n");
    i2d = &i2c_devices[0];
    while( i2d->name)
    {
      printf("   - %s\n", i2d->name);
      i2d++;
    }
    return(-1);
  }
  if( sscanf( c->para[3],"%x", &i2c.cmd) != 1)
  {
    printf("wrong register number\n");
    printf("usage: mpc.<i> <dev> <op> <reg> [<data>]\n");
    return(-1);
  }
  if( !strcmp( "read", c->para[2]))
  {
    i2c.data = 0;
    ioctl( mpc[mpc_idx].fd, MPC_IOCTL_I2C_DEV_RD, &i2c);
    printf("%s: reg=%x -> data = %x\n", i2d->name, i2c.cmd, i2c.data);
  }
  if( !strcmp( "write", c->para[2]))
  {
    if( c->cnt < 5)
    {
      printf("mpc i2c write command needs more arguments\n");
      printf("usage: mpc.<i> <dev> write <reg> <data>\n");
      return(-1);
    }
    if( sscanf( c->para[4],"%x", &i2c.data) != 1)
    {
      printf("wrong data value\n");
      return(-1);
    }
    ioctl( mpc[mpc_idx].fd, MPC_IOCTL_I2C_DEV_WR, &i2c);
  }
  return(0);
}

int 
xprs_mpc( struct cli_cmd_para *c)
{
  if( c->cnt < 1)
  {
    printf("mpc command needs more arguments\n");
    return(-1);
  }

  if( !strcmp( "map", c->para[0]))
  {
    return( mpc_map( c));
  }
  if( !strcmp( "unmap", c->para[0]))
  {
    return( mpc_unmap( c));
  }
  if( !strcmp( "sflash", c->para[0]))
  {
    return( mpc_sflash( c));
  }
  if( !strcmp( "i2c", c->para[0]))
  {
    return( mpc_i2c( c));
  }
  return(0);
}
