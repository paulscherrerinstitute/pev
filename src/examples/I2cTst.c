/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : I2cTst.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : october 10,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
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
 * $Log: I2cTst.c,v $
 * Revision 1.1  2013/06/07 15:01:17  zimoch
 * update to latest version
 *
 * Revision 1.1  2012/10/09 14:12:23  ioxos
 * first checkin [JFG]
 *
 * Revision 1.3  2012/09/04 13:32:42  ioxos
 * map statically allocated sysmem [JFG]
 *
 * Revision 1.2  2012/08/29 11:32:27  ioxos
 * build three mapping and check for error [JFG]
 *
 * Revision 1.1  2012/08/27 12:02:55  ioxos
 * first checkin [JFG]
 *
 * Revision 1.4  2012/06/01 14:00:14  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.3  2010/08/26 14:29:24  ioxos
 * cleanup void pointers and char * [JFG]
 *
 * Revision 1.2  2009/05/20 15:18:57  ioxos
 * support for multicrate + test using PMEM windoe [JFG]
 *
 * Revision 1.1  2009/01/08 08:19:03  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

typedef unsigned int u32;
#include <pevioctl.h>
#include <pevulib.h>


struct pev_node *pev;
struct timeval ti, to;
struct timezone tz;

int
main( int argc,
      char **argv)
{
  uint crate;
  uint i, sts, data;
  struct pev_ioctl_i2c i2c;

  crate = 0;

  printf("Entering test program for I2C\n");

  /* call PEV1100 user library initialization function */
  pev = pev_init( crate);
  if( !pev)
  {
    printf("Cannot allocate data structures to control PEV1100\n");
    exit( -1);
  }
  /* verify if the PEV1100 is accessible */
  if( pev->fd < 0)
  {
    printf("Cannot find PEV1100 interface\n");
    exit( -1);
  }

  i = 1;
  while(1)
  {
    i2c.device = 0x0100004c | 0x80;
    i2c.cmd = 0;
    sts = pev_i2c_read( i2c.device, i2c.cmd, &i2c.data);
    if((i%1000) == 0)printf("\rLM#0 : %d -> %02x   %08x    ", i, (unsigned char)i2c.data, sts);
    fflush( stdout);
    usleep(1000);
    i2c.device = 0x01000018 | 0x80;
    sts = pev_i2c_read( i2c.device, i2c.cmd, &i2c.data);
    if((i%1000) == 250)printf("\rLM#1 : %d -> %02x  %08x    ", i, (unsigned char)i2c.data, sts);
    fflush( stdout);
    usleep(1000);
    sts = pev_bmr_read( 0, 0x88, &data, 2);/*0x88*/
    if( ((data & 0xff) == 0xff) || !( data & 0xff))
    {
      printf("\nBMR#0 : %d -> %04x %08x  \n", i, (unsigned short)data, sts);
      fflush( stdout);
      break;
    }
    if((i%1000) == 500)printf("\rBMR#0 : %d -> %04x %08x  ", i, (unsigned short)data, sts);
    fflush( stdout);
    usleep(1000);
    sts = pev_bmr_read( 1, 0x8c, &data, 2);/*0x88*/
    if( ((data & 0xff) == 0xff) || !( data & 0xff))
    {
      printf("\nBMR#0 : %d -> %04x %08x  \n", i, (unsigned short)data, sts);
      fflush( stdout);
      break;
    }
    if((i%1000) == 750)printf("\rBMR#1 : %d -> %04x  %08x   ", i, (unsigned short)data, sts);
    fflush( stdout);
    usleep(1000);
    i++;
  }

  pev_exit( pev);

  exit(0);
}

