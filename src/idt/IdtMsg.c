/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : idt.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : october 10,2012
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
 * $Log: IdtMsg.c,v $
 * Revision 1.1  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.2  2013/02/21 14:44:09  ioxos
 * allow to choose mbx index [JFG]
 *
 * Revision 1.1  2013/02/05 10:49:55  ioxos
 * first checkin [JFG]
 *
 * Revision 1.1  2012/10/12 12:55:55  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#include <stdlib.h>
#include <stdio.h>
#include <pty.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <idtioctl.h>
#include <pevioctl.h>
#include <pevulib.h>
#include <idtulib.h>

struct idt_ioctl_mbx mbx;
char dev_name[32];

int
main( int argc,
      char **argv)
{
  int devid, idx;
  char yn[8];
  int mask;

  printf("Entering idt test\n");

  devid = 0;
  if( argc > 1)
  {
    devid = argv[1][0] - '0';
  }
  if( ( devid < 0) || ( devid > 4))
  {
    printf("bad device number\n");
    exit(1);
  }

  idx = 0;
  if( argc > 2)
  {
    idx = argv[2][0] - '0';
  }
  if( ( idx < 0) || ( idx > 4))
  {
    printf("bad maibox number\n");
    exit(1);
  }
  if( idt_init( devid) < 0)
  {
    printf("cannot open idt device #%d\n", devid);
    exit(-1);
  }

  mbx.idx = idx;
  mbx.tmo = 5000;
  mask = 1 << mbx.idx;
  idt_mbx_read( devid,  &mbx);
  printf("mbx status: %08x -> %08x\n", mbx.sts, mbx.msg);
  while(1)
  {
    printf(" wait for mbx interrupt...\n");
    idt_mbx_unmask( devid, mask);
    idt_mbx_wait( devid,  &mbx);
    printf("mbx status: ");
    if( mbx.sts < 0)
    {
      printf(" timeout\n");
    }
    else
    {
      printf(" %08x -> data = %08x\n", mbx.sts, mbx.msg);
    }
    printf("exit ? [y/n] ");
    fflush( stdout);
    yn[0] = 0;
    gets(yn);
    if( yn[0] == 'y') break;
  }
  idt_mbx_mask( devid, 0x8000000f);

  idt_exit();

  exit(0);

}
