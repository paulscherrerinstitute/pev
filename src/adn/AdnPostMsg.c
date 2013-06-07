/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : AdnPostMsg.c
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
 * $Log: AdnPostMsg.c,v $
 * Revision 1.1  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.1  2013/03/08 09:23:47  ioxos
 * first checkin [JFG]
 *
 * Revision 1.1  2012/09/20 13:39:52  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: AdnPostMsg.c,v 1.1 2013/06/07 14:59:54 zimoch Exp $";
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

struct adn_tx_msg
{
  int cmd;
  int msg_size;
  int ip_addr;
  int udp_addr;
} adn_msg;

struct adn_node *adn;
char buf[0x2000];

int
main( int argc,
      char *argv[])
{
  char *infilename;
  FILE *infile;
  int tmo, err;
  int cnt, i;
  struct adn_tx_msg *msg;
  int port, size;

  adn = adn_init();
  if( !adn)
  {
    printf("Cannot allocate data structures to control ADN\n");
    exit( -1);
  }
  if( adn->fd < 0)
  {
    printf("Cannot find ADN interface\n");
    adn_exit( adn);
    exit( -1);
  }

  port = -1;
  i = 1;
  infilename = (char *)0;
  bzero( buf, 0x2000);
  while( i < argc)
  {
    char *a;

    a = argv[i++];
    //printf("%s\n", a);
    if( a[0] == '-')
    {
      switch( a[1])
      {
        case 'f':
        {
	  a = argv[i++];
	  infilename = a;
	  //printf("%s\n", a);
	  break;
	}
        case 'm':
        {
	  a = argv[i++];
	  strcpy( buf, a);
	  printf("Adn message:%s\n", buf);
	  break;
	}
        default:
        {
	  printf("-%c : bad option -> ", a[1]);
	  printf("usage: AdnPostMsg -f datafile port\n");
	  exit(0);
	}
      }
    }
    else
    {
      sscanf(a, "%d", &port);
      break;
    }
  }
  if( port < 0)
  {
    printf("need port index -> usage: AdnPostMsg -f datafile port\n");
    exit(0);
  }
  if( !infilename)
  {
    if( !buf[0])
    {
      printf("enter message : ");
      fgets( buf, 0x2000, stdin);
    }
    size = strlen(buf);
    cnt = (size + 0xf)&0x3ff0;
  }
  else
  {
    infile = fopen( infilename, "r");
    if( !infile)
    {
      printf("cannot open message file %s\n", infilename);
      exit(0);
    }
    fseek( infile, 0, SEEK_END);
    size = ftell( infile);
    fseek( infile, 0, SEEK_SET);
    cnt = (size + 0xf)&0x3ff0;
    fread( buf, 1, size, infile);
    fclose( infile);
  }
  printf("Posting Tx Message [%d]\n", size);
  msg = &adn_msg;
  bzero( msg, sizeof( struct adn_tx_msg));
  msg->cmd = 0x44000000 + port;
  msg->msg_size = size;
  adn_hrms_wr( (void *)msg, 0x1800, sizeof( struct adn_tx_msg), RDWR_NOSWAP);
#ifdef BIG_ENDIAN
  adn_hrms_wr( (void *)buf, 0x1800 + sizeof( struct adn_tx_msg), cnt, RDWR_SWAP);
#else
  adn_hrms_wr( (void *)buf, 0x1800 + sizeof( struct adn_tx_msg), cnt, RDWR_NOSWAP);
#endif
  adn_reg_wr( 0x4030, 0x41800);
  usleep(10000);
  tmo = 100;
  while(( !(adn_reg_rd( 0x4030) & 0x80000000)) && tmo--);
  if( !tmo)
  {
    printf("Request timeout...exiting!!\n");
    goto AdnPostMsg_exit;
  }
  adn_hrms_rd( (void *)msg, 0x1800, sizeof( struct adn_tx_msg), RDWR_NOSWAP);
  if( (msg->cmd & (0x55000000 + port))!= (0x55000000 + port))
  {
    printf("Request error...exiting!!\n");
    goto AdnPostMsg_exit;
  }
  err = msg->cmd & 0x70000;
  if( err)
  {
    switch( err)
    {
      case 0x10000:
      {
	printf("Bad port index [%d]..!!\n", port);
	break;
      }
      case 0x20000:
      {
	printf("Message size [%d]bigger than port size..!!\n", size);
	break;
      }
      default:
      {
	printf("Unknown error type..!!\n");
      }
    }
    goto AdnPostMsg_exit;
  }
  printf("Request has been processed\n");
 
AdnPostMsg_exit:
  adn_exit( adn);
  exit(0);
}
