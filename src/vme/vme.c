/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : vme.c
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
 * $Log: vme.c,v $
 * Revision 1.1  2013/06/07 14:59:54  zimoch
 * update to latest version
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
#include "../../include/vmeioctl.h"

struct vme_board vme_board;

int
main( int argc,
      char **argv)
{
  int fd;
  char yn;

  printf("Entering vme test\n");

  fd = open("/dev/vme", O_RDWR);
  if( fd < 0)
  {
    printf("cannot open vme\n");
    exit(-1);
  }

  vme_board.base = 0x00000000;
  vme_board.am = 0x9;
  vme_board.size = 0x100000;
  vme_board.irq = 4;
  vme_board.vec = 0x50;
  ioctl( fd, VME_BOARD_REGISTER, &vme_board);

  while(1)
  {
    ioctl( fd, VME_IRQ_UNMASK, 0);
    printf("exit ? [y/n] ");
    scanf("%c", &yn);
    if( yn == 'y') break;
  }
  ioctl( fd, VME_IRQ_MASK, 0);
  close( fd);

  exit(0);

}
