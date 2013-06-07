/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : adnulib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That library contains a set of function to access the ADN1000 interface
 *     through the /dev/adn device driver.
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
 *  $Log: adnulib.c,v $
 *  Revision 1.1  2013/06/07 15:03:26  zimoch
 *  update to latest version
 *
 *  Revision 1.4  2013/04/15 14:17:39  ioxos
 *  read/write for SHM, CSR, HCR, SPI [JFG]
 *
 *  Revision 1.3  2013/03/14 11:14:48  ioxos
 *  rename spi to sflash [JFG]
 *
 *  Revision 1.2  2013/03/13 08:05:42  ioxos
 *  set version to ADN_RELEASE [JFG]
 *
 *  Revision 1.1  2013/03/08 09:38:08  ioxos
 *  first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: adnulib.c,v 1.1 2013/06/07 15:03:26 zimoch Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <pty.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

typedef unsigned int u32;
#include <pevioctl.h>
#include <adnioctl.h>
#include <adnulib.h>

static struct adn_node *adn = (struct adn_node *)NULL;

char adn_driver_version[16];
char *adn_lib_version = ADN_RELEASE;

char *
adn_rcsid()
{
  return( rcsid);
}

struct adn_node
*adn_init( int idx)
{
  char dev_name[16];

  if( idx > 3)
  {
    return( (struct adn_node *)0);
  }
  if( !adn)
  {
    adn = (struct adn_node *)malloc( sizeof( struct adn_node));
    if( !adn)
    {
      return( adn);
    }
    adn->fd = -1;
  }
  if( adn->fd < 0)
  {
    sprintf( dev_name, "/dev/adn%d", idx);
    adn->fd = open( dev_name, O_RDWR);
  }
  return(adn);
}

int
adn_exit( struct adn_node *adn)
{
  int ret;

  ret = 0;
  if( adn->fd > 0)
  {
    ret = close( adn->fd);
    adn->fd = -1;
  }
  return(ret);
}

char *
adn_get_driver_version()
{
  ioctl( adn->fd, ADN_IOCTL_VERSION, adn_driver_version);
  return( adn_driver_version);
}

char *
adn_get_lib_version()
{
  return( adn_lib_version);
}

long
adn_swap_64( long data)
{
  char ci[8];
  char co[8];

  *(long *)ci = data;
  co[0] = ci[7];
  co[1] = ci[6];
  co[2] = ci[5];
  co[3] = ci[4];
  co[4] = ci[3];
  co[5] = ci[2];
  co[6] = ci[1];
  co[7] = ci[0];

  return( *(long *)co);
}

int
adn_swap_32( int data)
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

short
adn_swap_16( short data)
{
  char ci[2];
  char co[2];

  *(short *)ci = data;
  co[0] = ci[1];
  co[1] = ci[0];

  return( *(short *)co);
}

 

void
adn_hrms_start( uint idx)
{
  struct adn_ioctl_hrms hrms;

  printf("HRMS start...\n");
  hrms.cmd = HRMS_CMD_START;
  hrms.dev = idx;
  ioctl( adn->fd, ADN_IOCTL_HRMS_CMD, &hrms);
  return;
}

void
adn_hrms_stop( uint idx)
{
  struct adn_ioctl_hrms hrms;

  printf("HRMS stop...\n");
  hrms.cmd = HRMS_CMD_STOP;
  hrms.dev = idx;
  ioctl( adn->fd, ADN_IOCTL_HRMS_CMD, &hrms);
  return;
}

uint
adn_hrms_rd_reg( uint idx,
                    uint reg)
{
  struct adn_ioctl_hrms hrms;

  printf("HRMS read registrer %d -> ", reg);
  hrms.dev = idx;
  hrms.cmd = HRMS_CMD_RD_REG;
  hrms.addr = reg;
  ioctl( adn->fd, ADN_IOCTL_HRMS_CMD, &hrms);
  printf("%08x\n", hrms.data);
  return(hrms.data);
}

void
adn_hrms_wr_reg( uint idx,
                    uint reg,
		    uint data)
{
  struct adn_ioctl_hrms hrms;

  printf("HRMS write registrer %d -> %08x\n", reg, data);
  hrms.dev = idx;
  hrms.cmd = HRMS_CMD_WR_REG;
  hrms.addr = reg;
  hrms.data = data;
  ioctl( adn->fd, ADN_IOCTL_HRMS_CMD, &hrms);
  return;
}

void
adn_hrms_load( struct adn_ioctl_hrms_code *hc)
{
  ioctl( adn->fd, ADN_IOCTL_HRMS_LOAD, hc);
  return;
}

int
adn_cbm_wr( void *buf,
	       int off,
	       int size)
{
  lseek( adn->fd, 0x20000000 | off, 0);
  return( write( adn->fd, buf, size));
} 

int
adn_cbm_rd( void *buf,
	       int off,
	       int size)
{
  lseek( adn->fd, 0x20000000 | off, 0);
  return( read( adn->fd, buf, size));
} 

void
adn_hrms_rd( void *buf, 
                uint offset, 
	     uint len,
	     char swap)
{
  struct adn_ioctl_rdwr rw;

  rw.buf = buf;
  rw.offset = offset;
  rw.len = len;
  rw.mode.dir = RDWR_READ;
  rw.mode.swap = swap;
  ioctl( adn->fd, ADN_IOCTL_HRMS_RDWR, &rw);
  return;
}

void
adn_hrms_wr( void *buf, 
             uint offset, 
             uint len,
	     char swap)
{
  struct adn_ioctl_rdwr rw;

  rw.buf = buf;
  rw.offset = offset;
  rw.len = len;
  rw.mode.dir = RDWR_WRITE;
  rw.mode.swap = swap;
  ioctl( adn->fd, ADN_IOCTL_HRMS_RDWR, &rw);
  return;
}


void
adn_cam_rd( void *buf, 
               uint offset, 
               uint len)
{
  struct adn_ioctl_rdwr rw;

  rw.buf = buf;
  rw.offset = offset;
  rw.len = len;
  rw.mode.dir = RDWR_READ;
  ioctl( adn->fd, ADN_IOCTL_CAM_RDWR, &rw);
  return;
}

void
adn_cam_wr( void *buf, 
               uint offset, 
               uint len)
{
  struct adn_ioctl_rdwr rw;

  rw.buf = buf;
  rw.offset = offset;
  rw.len = len;
  rw.mode.dir = RDWR_WRITE;
  ioctl( adn->fd, ADN_IOCTL_CAM_RDWR, &rw);
  return;
}

void
adn_sflash_rd( void *buf, 
              uint offset, 
               uint len)
{
  struct adn_ioctl_rdwr rw;

  rw.buf = buf;
  rw.offset = offset;
  rw.len = len;
  rw.mode.dir = RDWR_READ;
  ioctl( adn->fd, ADN_IOCTL_SFLASH_RDWR, &rw);
  return;
}

void
adn_sflash_wr( void *buf, 
               uint offset, 
               uint len)
{
  struct adn_ioctl_rdwr rw;

  rw.buf = buf;
  rw.offset = offset;
  rw.len = len;
  rw.mode.dir = RDWR_WRITE;
  ioctl( adn->fd, ADN_IOCTL_SFLASH_RDWR, &rw);
  return;
}

void
adn_sflash_erase( uint offset, 
                  uint len)
{
  struct adn_ioctl_rdwr rw;

  rw.buf = 0;
  rw.offset = offset;
  rw.len = len;
  rw.mode.dir = RDWR_CLEAR;
  ioctl( adn->fd, ADN_IOCTL_SFLASH_RDWR, &rw);
  return;
}

void
adn_ctl( uint op)
{
  struct adn_ioctl_rw rw;

  rw.offset = 0x7200;
  if( op == 1)
  {
    rw.data = 0xe; /* stop all HRMS */
    ioctl( adn->fd, ADN_IOCTL_WR_CBM, &rw);
    usleep(100000);
    rw.data = 0xf; /* load HRMS + CAM + Config table */
    ioctl( adn->fd, ADN_IOCTL_WR_CBM, &rw);
    usleep(100000);
    rw.data = 0x0; /* releae HRMS */
    ioctl( adn->fd, ADN_IOCTL_WR_CBM, &rw);
    sleep(1);
  }
  if( op == 0x80)
  {
    rw.data = 0xe; /* stop all HRMS */
    ioctl( adn->fd, ADN_IOCTL_WR_CBM, &rw);
  }
  return;
}

void
adn_snmp_trap( uint op)
{
  struct adn_ioctl_rw rw;

  rw.offset = 0x7600;
  if( op == 1)
  {
    rw.data = 0x1; /* enable SNMP trap */
    ioctl( adn->fd, ADN_IOCTL_WR_CBM, &rw);
  }
  if( op == 0x80)
  {
    rw.data = 0x0; /* enable SNMP trap */
    ioctl( adn->fd, ADN_IOCTL_WR_CBM, &rw);
  }
  return;
}

void
adn_reg_wr( int off, int data)
{
  struct adn_ioctl_rw rw;
  rw.offset = off;
  rw.data = data;
  ioctl( adn->fd, ADN_IOCTL_WR_CBM, &rw);

  return;
} 

int
adn_reg_rd( int off)
{
  struct adn_ioctl_rw rw;

  rw.offset = off;
  rw.data = 0;
  ioctl( adn->fd, ADN_IOCTL_RD_CBM, &rw);

  return( rw.data);
}

void
adn_shm_wr( int off, int data)
{
  struct adn_ioctl_rw rw;
  rw.offset = off;
  rw.data = data;
  ioctl( adn->fd, ADN_IOCTL_WR_SHM, &rw);

  return;
} 

int
adn_shm_rd( int off)
{
  struct adn_ioctl_rw rw;

  rw.offset = off;
  rw.data = 0;
  ioctl( adn->fd, ADN_IOCTL_RD_SHM, &rw);

  return( rw.data);
}

void
adn_csr_wr( int off, int data)
{
  struct adn_ioctl_rw rw;
  rw.offset = off;
  rw.data = data;
  ioctl( adn->fd, ADN_IOCTL_WR_CSR, &rw);

  return;
} 

int
adn_csr_rd( int off)
{
  struct adn_ioctl_rw rw;

  rw.offset = off;
  rw.data = 0;
  ioctl( adn->fd, ADN_IOCTL_RD_CSR, &rw);

  return( rw.data);
}

void
adn_hcr_wr( int off, int data)
{
  struct adn_ioctl_rw rw;
  rw.offset = off;
  rw.data = data;
  ioctl( adn->fd, ADN_IOCTL_WR_HCR, &rw);

  return;
} 

int
adn_hcr_rd( int off)
{
  struct adn_ioctl_rw rw;

  rw.offset = off;
  rw.data = 0;
  ioctl( adn->fd, ADN_IOCTL_RD_HCR, &rw);

  return( rw.data);
}

void
adn_spi_wr( int off, int data)
{
  struct adn_ioctl_rw rw;
  rw.offset = off;
  rw.data = data;
  ioctl( adn->fd, ADN_IOCTL_WR_SPI, &rw);

  return;
} 

int
adn_spi_rd( int off)
{
  struct adn_ioctl_rw rw;

  rw.offset = off;
  rw.data = 0;
  ioctl( adn->fd, ADN_IOCTL_RD_SPI, &rw);

  return( rw.data);
}
