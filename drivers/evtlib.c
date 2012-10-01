/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : evtlib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the low level functions to drive the EVT busses
 *    implemented on the PEV1100.
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
 *  $log: $
 * 
 *=============================< end file header >============================*/
#include <asm/uaccess.h>         // copy_to_user and copy_from_user
#include <linux/init.h>          // modules
#include <linux/module.h>        // module
#include <linux/types.h>         // dev_t type
#include <linux/fs.h>            // chrdev allocation
#include <linux/slab.h>          // kmalloc and kfree
#include <linux/cdev.h>          // struct cdev
#include <linux/errno.h>         // error codes
#include <linux/pci.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/dma.h>

#include "../include/pevioctl.h"
#include "evtlib.h"

struct evt_queue evt_queue[16];

struct evt_tbl
{
  struct evt_queue *queue;
} evt_tbl[128];

void
evt_init()
{
  int i;
  for( i = 0; i < 16; i++)
  {
    evt_queue[i].queue_ptr = NULL;

    evt_queue[i].wr_idx = 0;
    evt_queue[i].rd_idx = 0;
    evt_queue[i].cnt = 0;
    evt_queue[i].size = 256;
    evt_queue[i].task_p = NULL;
  }
  return;
}


struct evt_queue *
evt_queue_alloc( int sig)
{
  int i;

  for( i = 0; i < 16; i++)
  {
    if( evt_queue[i].queue_ptr == NULL)
    {
      evt_queue[i].queue_ptr = (int *)kmalloc( sizeof(int)*evt_queue[i].size, GFP_KERNEL);
      evt_queue[i].wr_idx = 0;
      evt_queue[i].rd_idx = 0;
      evt_queue[i].cnt = 0;
      evt_queue[i].size = 256;
      if( sig)
      {
	evt_queue[i].task_p = current;
	evt_queue[i].signal = sig;
      }
      evt_queue[i].src_id[0] = 0;
      evt_queue[i].src_id[1] = 0;
      evt_queue[i].src_id[2] = 0;
      evt_queue[i].src_id[3] = 0;
      evt_queue[i].src_id[4] = 0;
      evt_queue[i].src_id[5] = 0;
      evt_queue[i].src_id[6] = 0;
      evt_queue[i].src_id[7] = 0;
      sema_init( &evt_queue[i].sem, 0);
      sema_init( &evt_queue[i].lock, 1);
      return( &evt_queue[i]);
    }
  }
  return( NULL);
}

int
evt_queue_free( struct evt_queue *q)
{
  int i, j;

  for( i = 0; i < 16; i++)
  {
    if( &evt_queue[i] == q)
    {
      for( j = 0; j < 128; j++)
      {
        if( evt_tbl[j].queue == q)
        {
          evt_tbl[j].queue = NULL;
        }
      }
      kfree( q->queue_ptr);
      q->queue_ptr = NULL;
      q->wr_idx = 0;
      q->rd_idx = 0;
      q->size = 256;
      q->task_p = NULL;
      evt_queue[i].signal = 0;
      q->src_id[0] = 0;
      q->src_id[1] = 0;
      q->src_id[2] = 0;
      q->src_id[3] = 0;
      q->src_id[4] = 0;
      q->src_id[5] = 0;
      q->src_id[6] = 0;
      q->src_id[7] = 0;
      return( 0);
    }
  }
  return(-1);
}

int
evt_register(  struct evt_queue *q,
	       int src_id)
{
  src_id &= 0x7f;
  if( !evt_tbl[src_id].queue)
  {
    evt_tbl[src_id].queue = q;
    q->src_id[(src_id >> 4)&7] |= 1 << ( src_id & 0xf);
    return(0);
  }
  else
  {
    return(-1);
  }
}

int *
evt_get_src_id(  struct evt_queue *q)
{
  int i;

  for( i = 0; i < 16; i++)
  {
    if( &evt_queue[i] == q)
    {
      return( &q->src_id[0]);
    }
  }
  return( NULL);
}

int
evt_read( struct evt_queue *q,
	  int *evt_id,
	  int wait)
{
  int i, *p;
  int cnt;
  int retval;

  retval = 0;
  for( i = 0; i < 16; i++)
  {
    if( &evt_queue[i] == q)
    {
      if( wait)
      {
	if( wait == -1)
	{
	  retval = down_interruptible( &q->sem);
	}
	else
	{
	  int jiffies;
	  jiffies = msecs_to_jiffies( wait);
	  retval = down_timeout( &q->sem, jiffies);
	}
        if( retval)
        {
	  *evt_id = 0;
	  return(0);
        }
      }
      retval = down_interruptible( &q->lock);
      cnt = q->cnt;
      if( cnt)
      {
	if( !wait)
	{
	  /* if cnt != 0 -> q->sem should have been set by evt_irq() -> decrement it*/
	  retval = down_interruptible( &q->sem);
	}
	p = q->queue_ptr;
        *evt_id = p[q->rd_idx];
	q->cnt = cnt - 1;                    /* decrement evt counter */
	q->rd_idx = (q->rd_idx + 1) & 0xff;  /* update read index     */
      }
      else
      {
        *evt_id = 0;
      }
      up( &q->lock);
      return( q->cnt);
    }
  }
  return(-1);
}

int
evt_unregister(  struct evt_queue *q,
		 int src_id)
{
  src_id &= 0x7f;
  if( evt_tbl[src_id].queue == q)
  {
    evt_tbl[src_id].queue = NULL;
    q->src_id[(src_id >> 4)&7] &= ~(1 << ( src_id & 0xf));
    return(0);
  }
  else
  {
    return(-1);
  }
}

void
evt_irq( int src,
	 void *arg)
{
  struct evt_queue *q;
  int src_id;
  int *p;
  int retval;

  src_id = ( src >> 8) & 0x7f;
  q = evt_tbl[src_id].queue;
  if( q)
  {
    retval = down_interruptible( &q->lock);
    p = q->queue_ptr;
    p[q->wr_idx] = src;
    q->wr_idx = (q->wr_idx + 1) & 0xff;
    if( q->wr_idx == q->rd_idx)
    {
      q->rd_idx = (q->rd_idx + 1) & 0xff;
    } 
    q->cnt += 1;
    if( q->signal)
    {
      if( q->cnt == 1)
      {
	send_sig( q->signal, q->task_p, 0);
      }
    }
    up( &q->lock);
    up( &q->sem);
  }
  return;
}

