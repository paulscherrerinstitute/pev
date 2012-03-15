/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : maplib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the low level functions to drive the address mappers
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
 * $Log: maplib.c,v $
 * Revision 1.3  2012/03/15 14:59:02  kalantari
 * added exact copy of tosca-driver_4.04 from afs
 *
 * Revision 1.6  2009/04/06 12:19:24  ioxos
 * remove pevdrvr.h [JFG]
 *
 * Revision 1.5  2009/01/27 14:37:30  ioxos
 * remove ref to semaphore.h [JFG]
 *
 * Revision 1.4  2008/11/12 09:55:16  ioxos
 * update map_blk_alloc() and map_blk_find() [JFG]
 *
 * Revision 1.3  2008/07/18 14:28:41  ioxos
 * update local address in map_blk_modify [JFG]
 *
 * Revision 1.2  2008/07/04 07:40:12  ioxos
 * update address mapping functions [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:06  ioxos
 * Import sources for PEV1100 project [JFG]
 *
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
#include <asm/dma.h>

#include "../include/pevioctl.h"

#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

int
map_blk_alloc( struct pev_ioctl_map_ctl *map_ctl_p,
               struct pev_ioctl_map_pg *map_pg_p)
{
  struct pev_map_blk *p;
  int i;
  u32 size;
  ushort npg;
  int off;

  p = map_ctl_p->map_p;
  size = map_pg_p->size + (u32)(map_pg_p->rem_addr % map_ctl_p->pg_size);
  npg = (ushort)(((size-1)/map_ctl_p->pg_size) + 1);
  off = -1;
   /* scan list block per block */
  for( i = 0; i < map_ctl_p->pg_num; i += p[i].npg)
  {
    /* check only free blocks */
    if( (p[i].flag == MAP_FLAG_FREE))
    {
      /* check if this block is big enough */
      if( p[i].npg >= npg)
      {
	/* check if a block big enough was already found*/
	if( off >= 0)
        {
	  /* if this block is a better fit select it */
	  if( p[i].npg < p[off].npg)
          {
	    off = i;
	  }
        }
	/* if not select this block */
        else
        {
	  off = i;
	}
      }
    }
  }
  /* if block was found, update block list */
  if( off >= 0)
  {
    /* if block found is to big, create new free block with residue */
      if( p[off].npg > npg)
      {
         p[off + (long)npg].npg = p[off].npg - npg;
         p[off + (long)npg].flag = MAP_FLAG_FREE;
      }
      /* update block status */
      p[off].npg = npg;
      p[off].flag = MAP_FLAG_BUSY;
      if( map_pg_p->flag & MAP_FLAG_PRIVATE)
      {
         p[off].flag |= MAP_FLAG_PRIVATE;
      }
      p[off].usr = 1;
      p[off].rem_addr = map_pg_p->rem_addr &  ~((u64)map_ctl_p->pg_size-1);
      p[off].mode = map_pg_p->mode;

      /* return local address in map parameters */
      map_pg_p->win_size = map_ctl_p->pg_size * p[off].npg;
      map_pg_p->rem_base = p[off].rem_addr;
      map_pg_p->loc_base = off * map_ctl_p->pg_size;
      map_pg_p->loc_addr = map_pg_p->rem_addr - p[off].rem_addr;
      map_pg_p->loc_addr += map_pg_p->loc_base;
  }
  map_pg_p->offset = off;

  return( off);
}

int
map_blk_find( struct pev_ioctl_map_ctl *map_ctl_p,
              struct pev_ioctl_map_pg *map_pg_p)
{
  struct pev_map_blk *p;
  int i;
  int off;
  u64 h_addr;

  off = -1;
  p = map_ctl_p->map_p;
  for( i = 0; i < map_ctl_p->pg_num; i += p[i].npg)
  {
    /* check only sharable busy blocks */
    if( (p[i].flag & MAP_FLAG_BUSY) && !(p[i].flag & MAP_FLAG_PRIVATE))
    {
      /* check if bus parameters are matching */
      h_addr = p[i].rem_addr + ((u64)map_ctl_p->pg_size * p[i].npg);
      if( (  map_pg_p->rem_addr >= p[i].rem_addr) &&
          ( (map_pg_p->rem_addr + map_pg_p->size) <=  h_addr ) &&
          ( (map_pg_p->mode & MAP_MODE_MASK) == (p[i].mode & MAP_MODE_MASK)))
      {
	off = i;
	/* if block is not locked, increment user's count */
	p[off].usr += 1;
	break;
      }
    }
  }
  if( off >= 0)
  {
    /* return local address in map parameters */
    map_pg_p->win_size = map_ctl_p->pg_size * p[off].npg;
    map_pg_p->rem_base = p[off].rem_addr;
    map_pg_p->loc_base = off * map_ctl_p->pg_size;
    map_pg_p->loc_addr = map_pg_p->rem_addr - p[off].rem_addr;
    map_pg_p->loc_addr += map_pg_p->loc_base;
  }
  map_pg_p->offset = off;

  return( off);
}

int
map_blk_modify( struct pev_ioctl_map_ctl *map_ctl_p,
                struct pev_ioctl_map_pg *map_pg_p)
{
  struct pev_map_blk *p;
  int off;

  p = map_ctl_p->map_p;
  off = map_pg_p->offset;
  if( !(p[off].flag & MAP_FLAG_BUSY))
  {
    return( -1);
  }
  p[off].rem_addr = map_pg_p->rem_addr &  ~((u64)map_ctl_p->pg_size-1);
  p[off].mode = map_pg_p->mode;
  map_pg_p->win_size = map_ctl_p->pg_size * p[off].npg;
  map_pg_p->rem_base = p[off].rem_addr;
  map_pg_p->loc_base = off * map_ctl_p->pg_size;
  map_pg_p->loc_addr = map_pg_p->loc_base + map_pg_p->rem_addr - map_pg_p->rem_base;

  return( off);
}

int
map_blk_free( struct pev_ioctl_map_ctl *map_ctl_p,
              int off)
{
  struct pev_map_blk *p;
  int i;
  int tmp_off;
  unsigned short new_size;

  p = map_ctl_p->map_p;
  if( !(p[off].flag & MAP_FLAG_BUSY))
  {
    return( -1);
  }
   /* check if the block has been taken by more then one user */
   if( p[off].usr > 1)
   {
      p[off].usr -= 1;
      return( 0);
   }

   /* free block */
   p[off].flag = MAP_FLAG_FREE;
   p[off].usr = 0;
   p[off].rem_addr = 0;
   p[off].mode = 0;

   /* clear corresponding pages in mmu */

   /* check if next block in list is a free block */
   if( p[off + p[off].npg].flag == MAP_FLAG_FREE)
   {
      /* merge next block with current one */
      new_size = p[off].npg + p[off + p[off].npg].npg;
      p[off + p[off].npg].npg = 0;
      p[off].npg = new_size;
   }

   /* scan list to get offset of previous block */
   tmp_off = 0;
   for( i = 0; i < off; i += p[i].npg)
   {
      tmp_off = i;
   }
   if( i != off)
   {
    return( -1);
   }
   if( tmp_off < off)
   {
     /* check if previous block in list is a free block */
     if( p[tmp_off].flag == MAP_FLAG_FREE)
     {
       /* merge two bloks */
       p[tmp_off].npg += p[off].npg;
       p[off].npg = 0;
     }
   }
   return(0);
}
