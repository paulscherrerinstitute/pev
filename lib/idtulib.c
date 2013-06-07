/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pevulib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : 
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That library contains a set of function to access the IDT PCI Express
 *     switch
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
 *  $Log: idtulib.c,v $
 *  Revision 1.1  2013/06/07 15:03:26  zimoch
 *  update to latest version
 *
 *  Revision 1.5  2013/02/27 08:41:24  ioxos
 *  include pevioctl.h [JFG]
 *
 *  Revision 1.4  2013/02/26 15:35:03  ioxos
 *  release 4.29 [JFG]
 *
 *  Revision 1.3  2013/02/21 15:27:59  ioxos
 *  cosmetics & optimizations -> release 4.28 [JFG]
 *
 *  Revision 1.2  2013/02/15 11:32:30  ioxos
 *  release 4.27 [JFG]
 *
 *  Revision 1.1  2013/02/05 10:42:00  ioxos
 *  first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char rcsid[] = "$Id: idtulib.c,v 1.1 2013/06/07 15:03:26 zimoch Exp $";
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
#include <idtioctl.h>
#include <idtulib.h>

char *idt_name[5] =
{
  "/dev/idt_mas",
  "/dev/idt_slv1",
  "/dev/idt_slv2",
  "/dev/idt_slv3",
  "/dev/idt_slv4"
};
int idt_fd[5] = { -1, -1, -1, -1, -1};
char idt_lib_version[] = PEV1100_RELEASE;

char *
idt_rcsid()
{
  return( rcsid);
}

char *
idt_get_lib_version()
{
  return( idt_lib_version);
}

int
idt_init( int devid)
{
  int fd;

  if( (devid < 0) || (devid > 4))
  {
    return( -1);
  }
  if( idt_fd[devid] == -1)
  {
    printf("opening %s\n", idt_name[devid]);
    fd = open( idt_name[devid], O_RDWR);
    if( fd < 0)
    {
      return(-1);
    }
    idt_fd[devid] = fd;
  }
  return( idt_fd[devid]);
}

void
idt_exit( void)
{
  int devid;

  for( devid = 0; devid < 5; devid++)
  {
    if( idt_fd[devid] != -1)
    {
      close( idt_fd[devid]);
    }
  }
  return;
}
int 
idt_mbx_read( int devid, 
              struct idt_ioctl_mbx *mbx)
{
  if( (devid < 0) || (devid > 4)) return(-1);
  if( idt_fd[devid] < -1) return(-1);
  return( ioctl( idt_fd[devid], IDT_IOCTL_MBX_READ, mbx));
}

int 
idt_mbx_write( int devid,
	       struct idt_ioctl_mbx *mbx)
{
  if( (devid < 0) || (devid > 4)) return(-1);
  if( idt_fd[devid] < -1) return(-1);
  return( ioctl( idt_fd[devid], IDT_IOCTL_MBX_WRITE, mbx));
}

int 
idt_mbx_mask( int devid, 
	      int mask)
{
  if( (devid < 0) || (devid > 4)) return(-1);
  if( idt_fd[devid] < -1) return(-1);
  return( ioctl( idt_fd[devid], IDT_IOCTL_MBX_MASK, &mask));
}

int 
idt_mbx_unmask( int devid, 
                int mask)
{
  if( (devid < 0) || (devid > 4)) return(-1);
  if( idt_fd[devid] < -1) return(-1);
  return( ioctl( idt_fd[devid], IDT_IOCTL_MBX_UNMASK, &mask));
}

int 
idt_mbx_wait( int devid,  
	      struct idt_ioctl_mbx *mbx)
{
  if( (devid < 0) || (devid > 4)) return(-1);
  if( idt_fd[devid] < -1) return(-1);
  return( ioctl( idt_fd[devid], IDT_IOCTL_MBX_WAIT, mbx));
}


int
idt_dma_move( int devid,
	      struct idt_ioctl_dma_req *dr_p)
{
  if( (devid < 0) || (devid > 4)) return(-1);
  if( idt_fd[devid] < -1) return(-1);
  return( ioctl( idt_fd[devid], IDT_IOCTL_DMA_MOVE, dr_p));
}
int
idt_dma_wait( int devid,
	      int mode,
	      int *sts)
{
  int retval;
  int arg;
  if( (devid < 0) || (devid > 4)) return(-1);
  if( idt_fd[devid] < -1) return(-1);
  arg = mode;
  retval = ioctl( idt_fd[devid], IDT_IOCTL_DMA_WAIT, &arg);
  *sts = arg;
  return( retval);
}
