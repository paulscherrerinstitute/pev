/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : AdnSflashLoad.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
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
 * $Log: AdnSflashLoad.c,v $
 * Revision 1.1  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.2  2013/04/15 14:55:28  ioxos
 * add support multiple APX2300/ADN400x [JFG]
 *
 * Revision 1.1  2013/03/14 12:41:30  ioxos
 * rename spi to sflash [JFG]
 *
 * Revision 1.2  2013/03/14 12:38:35  ioxos
 * rename spi to sflash [JFG]
 *
 * Revision 1.1  2013/03/08 09:23:48  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: AdnSflashLoad.c,v 1.1 2013/06/07 14:59:54 zimoch Exp $";
#endif

#include <debug.h>

#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <pty.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <aio.h>
#include <errno.h>
#include <signal.h>

typedef unsigned int u32;
#include <pevioctl.h>
#include <adnioctl.h>
#include <adnulib.h>

struct adn_node *adn;
char buf[0x10000];
char cmp[0x10000];
char *get_rcsid(){return(rcsid);}

int
main( int argc,
      char *argv[])
{
  int i, off, size;
  int err = 0;
  FILE *infile;
  uint cks;
  int adn_idx;

  adn_idx = 0;
  if( argc < 4)
  {
    printf("usage: AdnSflashLoad <dev> <offset> <filenane>\n");
    exit(0);
  }
  sscanf(argv[1], "%x", &adn_idx);
  sscanf(argv[2], "%x", &off);
  if( ( adn_idx < 0) || ( adn_idx > 3))
  {
    printf("AdnSflashLoad : bad device index [0 - 3] ->  %d\n", adn_idx);
    exit( -1);
  } 
  infile = fopen( argv[3], "r");
  if( !infile)
  {
    printf("cannot open file %s\n", argv[3]);
    exit(0);
  }
  fseek( infile, 0, SEEK_END);
  size = ftell( infile);
  fseek( infile, 0, SEEK_SET);
  if( (off<0) || (off+size)>0x20000)
  {
    printf("AdnSflashLoad : offset out of range [0x0 - 0x20000] -> [0x%x - 0x%x]\n", off, off+size);
    exit( -1);
  } 

  adn = adn_init( adn_idx);
  if( !adn)
  {
    printf("Cannot allocate data structures to control ADN\n");
    exit( -1);
  }
  if( adn->fd < 0)
  {
    printf("Cannot find ADN interface\n");
    exit( -1);
  }

  printf("Loading SFLASH at offset 0x%x to 0x%x [0x%x]\n", off, off+size, size);
  fread( buf, 1, size, infile);
  fclose( infile);

  adn_sflash_wr( buf, off, size);

  printf("Verifying SFLASH...");
  adn_sflash_rd( cmp, off, size);

  cks = 0;
  for( i = 0; i < size; i++)
  {
    if( buf[i] != cmp[i])
    {
      err = 1;
      printf("compare error at offset 0x%x : 0x%2x - 0x%2x\n", i, buf[i], cmp[i]);
      break;
    }
    if(!(i&3))  cks += adn_swap_32(*(uint *)&buf[i]);
  }
  if( !err)
  {
    printf("OK %08x\n", cks);
  }
  adn_exit( adn);
  exit(0);
}
