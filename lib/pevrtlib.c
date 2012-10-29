/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pevrtlib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : 
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That library contains a set of function to access the PEV1000 interface
 *     through the real time functions of the /dev/pev device driver.
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
 *  $Log: pevrtlib.c,v $
 *  Revision 1.11  2012/10/29 10:06:56  kalantari
 *  added the tosca driver version 4.22 from IoxoS
 *
 *  Revision 1.3  2010/01/13 16:51:24  ioxos
 *  add real time support for DMA list [JFG]
 *
 *  Revision 1.2  2009/06/03 12:36:24  ioxos
 *  add pev_rt_dma_status() [JFG]
 *
 *  Revision 1.1  2009/05/20 14:50:46  ioxos
 *  first cvs checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: pevrtlib.c,v 1.11 2012/10/29 10:06:56 kalantari Exp $";
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
#include <pevulib.h>
#include <rtdm/rtdm.h>
#include <native/task.h>

#define DEVICE_NAME		"pev_rt"

RT_TASK rt_task_desc;
static int rt_fd = -1;
static int rt_crate = -1;


int
pev_rt_init( int crate)
{
  int ret;

  if( (crate < 0) || (crate > 15))
  {
    return( -1);
  }
  /* no memory-swapping for this programm */
  if( mlockall(MCL_CURRENT | MCL_FUTURE)) 
  {
    return(-1);
  }

  if( rt_task_shadow(&rt_task_desc, NULL, 10, T_CPU(1)))
  {
    return( -1);
  }

  rt_fd = rt_dev_open( "pev_rt", 0);
  if( rt_fd >= 0)
  {
    rt_crate = crate;
  }
  return( rt_fd);
}

int
pev_rt_set_crate( uint crate)
{
  if( (crate < 0) || (crate > 15))
  {
    return( -1);
  }
  rt_crate = crate;
  return( rt_crate);
}

int
pev_rt_exit()
{
  int ret;

  ret = 0;
  if( rt_fd > 0)
  {
    ret = close( rt_fd);
    rt_fd = -1;
  }
  return(ret);
}


int
pev_rt_dma_move( struct pev_ioctl_dma_req *dr_p)
{
  return( rt_dev_ioctl( rt_fd, (rt_crate << 28) | PEV_IOCTL_DMA_MOVE, dr_p));
}

int
pev_rt_dma_vme_list_rd( void *uaddr, 
		          struct pev_ioctl_dma_list *list_p,
		          int list_size)
{
  struct pev_ioctl_dma_req req;

  req.src_addr = (ulong)list_p;      /* source is VME list           */
  req.des_addr = (ulong)uaddr;       /* destination is DMA buffer    */
  req.size = list_size;                  
  req.src_space = DMA_SPACE_VME;
  req.des_space = DMA_SPACE_PCIE|DMA_PCIE_UADDR;
  req.src_mode = 0;
  req.des_mode = 0;
  req.start_mode = DMA_MODE_LIST_RD;
  req.end_mode = 0;
  req.intr_mode = 0;
  req.wait_mode = 0;

  return( rt_dev_ioctl( rt_fd, (rt_crate << 28) | PEV_IOCTL_DMA_MOVE, &req));
}
int
pev_rt_dma_status( struct pev_ioctl_dma_sts *ds_p)
{
  return( rt_dev_ioctl( rt_fd, (rt_crate << 28) | PEV_IOCTL_DMA_STATUS, ds_p));
}

int
pev_rt_timer_start( uint mode,
		 uint msec)
{
  struct pev_ioctl_timer tmr;

  tmr.mode = mode;
  tmr.time = msec;
  return( rt_dev_ioctl( rt_fd, (rt_crate << 28) | PEV_IOCTL_TIMER_START, &tmr));
} 

void
pev_rt_timer_restart( void)
{
  rt_dev_ioctl( rt_fd, (rt_crate << 28) | PEV_IOCTL_TIMER_RESTART, 0);
  return;
}

void
pev_rt_timer_stop( void)
{
  rt_dev_ioctl( rt_fd, (rt_crate << 28) | PEV_IOCTL_TIMER_STOP, 0);
  return;
}

uint
pev_rt_timer_read( struct pev_time *tm)
{
  struct pev_ioctl_timer tmr;

  rt_dev_ioctl( rt_fd, (rt_crate << 28) | PEV_IOCTL_TIMER_READ, &tmr);
  if( tm)
  {
    tm->time = tmr.time;
    tm->utime = tmr.utime;
  }
  return( tmr.time); 
} 

