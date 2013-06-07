/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : AdnPost.c
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
 * $Log: AdnGetMsg.c,v $
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
static char *rcsid = "$Id: AdnGetMsg.c,v 1.1 2013/06/07 14:59:54 zimoch Exp $";
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
#include <ctype.h>
#include <aio.h>
#include <errno.h>
#include <signal.h>


typedef unsigned int u32;
#include <pevioctl.h>
#include <adnioctl.h>
#include <adnulib.h>

struct adn_msg
{
  int cmd;
  int buf_size;
  int rsv1[2];
  int port;
  int msg_size;
  int msg_cnt;
  unsigned int time;
} adn_msg;

#ifdef PPC
struct port_status
{
  int cmd;
  int rsv1[3];
  ushort size;
  ushort port_id;
  ushort rsv2;
  ushort vl_id;
  uint msg_cnt;
  uint err_cnt;
  uint para[4];
}adn_sts;
#else
struct port_status
{
  int cmd;
  int rsv1[3];
  ushort port_id;
  ushort size;
  ushort vl_id;
  ushort rsv2;
  uint msg_cnt;
  uint err_cnt;
  uint para[4];
}adn_sts;
#endif

struct adn_node *adn;
unsigned char buf[0x4000];

int
main( int argc,
      char *argv[])
{
  char *outfilename;
  FILE *outfile;
  int tmo, err;
  int cnt, i, j;
  unsigned char c;
  struct adn_msg *msg;
  struct port_status *sts;
  int port, size;
  uint sec, usec;

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
  size = 0x40000;
  i = 1;
  outfilename = (char *)0;
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
	  outfilename = a;
	  //printf("%s\n", a);
	  break;
	}
        case 's':
        {
	  a = argv[i++];
	  sscanf(a, "%d", &size);
	  break;
	}
        default:
        {
	  printf("-%c : bad option -> ", a[1]);
	  printf("usage: AdnGetMsg -s size port\n");
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
    printf("need port index -> usage: AdnGetMsg -s size port\n");
    exit(0);
  }
  printf("Reading Rx Status from port index #%d\n", port);
  sts = &adn_sts;
  sts->cmd = 0x44000000 + port;
  adn_hrms_wr( (void *)sts, 0x1800, sizeof( struct port_status), RDWR_NOSWAP);
  adn_reg_wr( 0x4030, 0x181800);
  usleep(1000);
  tmo = 100;
  while(( !(adn_reg_rd( 0x4030) & 0x80000000)) && tmo--);
  if( !tmo)
  {
    printf("Request timeout...exiting!!\n");
    goto AdnGetMsg_exit_1;
  }
  adn_hrms_rd( (void *)sts, 0x1800, sizeof( struct port_status), RDWR_NOSWAP);
  if( (sts->cmd & (0x55000000 + port))!= (0x55000000 + port))
  {
    printf("Request error...exiting!!\n");
    goto AdnGetMsg_exit_1;
  }
  err = sts->cmd & 0x70000;
  if( err)
  {
    switch( err)
    {
      case 0x10000:
      {
	printf("Bad port index..!!\n");
	break;
      }
      default:
      {
	printf("Unknown error type..!!\n");
      }
    }
    goto AdnGetMsg_exit_1;
  }

  if( sts->size & 0x4000)
  {
    char rd, wr;
    printf("Reading Rx Message from queuing port #%d : port_id = 0x%04x [%d]\n", port, sts->port_id, sts->port_id);

    printf("  message received  : %d\n", sts->msg_cnt);
    printf("  message discarded : %d\n", sts->err_cnt);
    rd =(char)( sts->para[2] >> 24);
    wr =(char)( sts->para[0] >> 24);
    printf("  message pending   : %d\n", wr-rd);
    if( rd == wr)
    {
      goto AdnGetMsg_exit_1;
    }
  }
  else
  {
    printf("Reading Rx Message from sampling port 0x%04x [%d]\n", sts->port_id, sts->port_id);
    printf("  message received  : %d\n", sts->msg_cnt);
    printf("  message discarded : %d\n", sts->err_cnt);
  }

  msg = &adn_msg;
  cnt = 0;
  bzero( buf, 0x4000);
  bzero( msg, sizeof( struct adn_msg));
  msg->cmd = 0x44000000 + port;
  msg->buf_size = 0x300;
  adn_hrms_wr( (void *)msg, 0x1800, sizeof( struct adn_msg), RDWR_NOSWAP);
  adn_reg_wr( 0x4030, 0x141800);
  usleep(1000);
  tmo = 100;
  while(( !(adn_reg_rd( 0x4030) & 0x80000000)) && tmo--);
  if( !tmo)
  {
    printf("Request timeout...exiting!!\n");
    goto AdnGetMsg_exit;
  }
  adn_hrms_rd( (void *)msg, 0x1800, sizeof( struct adn_msg), RDWR_NOSWAP);
  if( (msg->cmd & (0x55000000 + port))!= (0x55000000 + port))
  {
    printf("Request error...exiting!!\n");
    goto AdnGetMsg_exit;
  }
  err = msg->cmd & 0x70000;
  if( err)
  {
    switch( err)
    {
      case 0x10000:
      {
	printf("Bad port index..!!\n");
	break;
      }
      case 0x40000:
      {
	printf("No message in port..!!\n");
	break;
      }
      default:
      {
	printf("Unknown error type..!!\n");
      }
    }
    goto AdnGetMsg_exit;
  }
  printf("  msg_size = 0x%x [%d]\n", msg->msg_size, msg->msg_size);
  if( sts->size & 0x4000)
  {
     unsigned char byte[4];

    *(uint *)&byte[0] =  msg->msg_cnt;
#ifdef BIG_ENDIAN
    printf("  ip_src = %02x.%02x.%02x.%02x [%d.%d.%d.%d]\n", byte[3],  byte[2],  byte[1],  byte[0],  byte[3],  byte[2],  byte[1],  byte[0]);
#else
    printf("  ip_src = %02x.%02x.%02x.%02x [%d.%d.%d.%d]\n", byte[0],  byte[1],  byte[2],  byte[3],  byte[0],  byte[1],  byte[2],  byte[3]);
#endif
    *(uint *)&byte[0] =  msg->time;
#ifdef BIG_ENDIAN
    printf("  udp_src = 0x%02x%02x\n", byte[3],  byte[2]);
#else
    printf("  udp_src = 0x%02x%02x\n", byte[0],  byte[1]);
#endif
  }
  else
  {
    printf("  msg_cnt = 0x%x [%d]\n", msg->msg_cnt, msg->msg_cnt);                  
    sec = msg->time/1000000;                                        
    usec = msg->time%1000000;                                       
    printf("  timestamp = 0x%x [%d.%d sec]\n", msg->time, sec, usec);
  }

  if( size)
  {
    if( size < msg->msg_size)
    {
      cnt = (size + 0xf)&0x3ff0;
    }
    else
    {
      cnt = (msg->msg_size + 0xf)&0x3ff0;
    }
#ifdef BIG_ENDIAN
    adn_hrms_rd( (void *)buf, 0x1800 + sizeof( struct adn_msg), cnt, RDWR_SWAP);
#else
    adn_hrms_rd( (void *)buf, 0x1800 + sizeof( struct adn_msg), cnt, RDWR_NOSWAP);
#endif
    if( outfilename)
    {
      outfile = fopen( outfilename, "w");
      fwrite( (void *)msg, 1, sizeof( struct adn_msg), outfile);
      fwrite( (void *)buf, 1, cnt, outfile);
      fclose( outfile);
    }
    for( i = 0; i < cnt; i += 16)
    {
      printf("%04x : ", i);
      for( j= 0; j < 16; j++)
      {
        printf("%02x ", buf[i+j]);
      }
      for( j= 0; j < 16; j++)
      {
        c = buf[i+j];
        if( !isprint(c)) c = '.';
        printf("%c", c);
      }
      printf("\n");
    }
  }
 
AdnGetMsg_exit:
AdnGetMsg_exit_1:
  adn_exit( adn);
  exit(0);
}
