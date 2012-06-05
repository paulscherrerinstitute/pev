/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : hppev.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : july 10,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This program allows to disconnect/reconnect the PCIe cable from the PEV1100
 *    It relies on the presence of the PEV1100 hotplug driver hppev.ko 
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
 * $Log: hppev.c,v $
 * Revision 1.4  2012/06/05 13:37:31  kalantari
 * linux driver ver.4.12 with intr Handling
 *
 * Revision 1.3  2010/08/16 15:23:36  ioxos
 * save/restore 256 registers [JFG]
 *
 * Revision 1.2  2010/07/13 09:44:59  ioxos
 * change command number [JFG]
 *
 * Revision 1.1  2009/06/03 12:57:12  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: hppev.c,v 1.4 2012/06/05 13:37:31 kalantari Exp $";
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

unsigned char buf[0x4000];

main( int argc,
      void *argv[])
{
  FILE *cfg_file;
  int fd;
  int n;

  n = argc-1;

  fd = open("/dev/hppev", O_RDWR);
  if( fd < 0)
  {
    printf("cannot access PEV100 hotplug driver\n");
    exit(0);
  }
  if( argc > 1)
  {
    if( !strcmp( "disconnect", argv[1]))
    {
      if( argc > 2)
      {
	bzero( buf, 0x4000);
	cfg_file = fopen( argv[2], "w");
        if( !cfg_file)
        {
          printf("cannot create configuration file %s\n", argv[2]);
          exit(0);
        }
	ioctl(fd, 11, buf);
	fwrite( buf, 0x4000, 1, cfg_file);
	fclose(cfg_file);
      }
      else
      {
	ioctl(fd, 11, 0);
      }
    }
    if( !strcmp( "connect", argv[1]))
    {
      if( argc > 2)
      {
	cfg_file = fopen( argv[2], "r");
        if( !cfg_file)
        {
          printf("cannot access configuration file %s\n", argv[2]);
          exit(0);
        }
	fread( buf, 0x4000, 1, cfg_file);
	fclose(cfg_file);
	ioctl(fd, 12, buf);
      }
      else
      {
	ioctl(fd, 12, 0);
      }
    }
  }
  close(fd);
}
