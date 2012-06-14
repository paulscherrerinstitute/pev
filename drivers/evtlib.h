/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : evtlib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    evtlib.c
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
 * $Log: evtlib.h,v $
 * Revision 1.2  2012/06/14 14:00:04  kalantari
 * added support for r/w PCI_IO bus registers, also added read USR1 generic area per DMA and distribute the readout into individual records
 *
 * Revision 1.1  2012/05/23 15:32:50  ioxos
 * first checkin [JFG]
 *
 * Revision 1.1  2012/03/27 09:17:39  ioxos
 * add support for EVTs [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_EVTLIB
#define _H_EVTLIB

struct evt_queue
{
  int *queue_ptr;
  int wr_idx;
  int rd_idx;
  int cnt;
  int size;
  struct task_struct *task_p;
  int signal;
  struct semaphore sem;
  int src_id[4];
  struct semaphore lock;
};

void evt_init( void);
void evt_irq( int, void *);
struct evt_queue *evt_queue_alloc( int);
int evt_queue_free( struct evt_queue *);
int evt_register( struct evt_queue *, int);
int evt_unregister( struct evt_queue *, int);
int evt_read( struct evt_queue *, int *, int);
int *evt_get_src_id( struct evt_queue *);

#endif /*  _H_FIFOLIB */


/*================================< end file >================================*/
