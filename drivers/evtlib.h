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
 * Revision 1.9  2013/06/07 14:58:31  zimoch
 * update to latest version
 *
 * Revision 1.3  2012/10/25 12:27:57  ioxos
 * eeprom delay set to 5 msec + clear evt + mask SMI while reading event (need new FPGA)[JFG]
 *
 * Revision 1.2  2012/06/28 12:06:16  ioxos
 * support up to 8 ITC [JFG]
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
  int src_id[8];
  //struct semaphore lock;
  int lock;
  int lock_cnt;
};

void evt_init( void);
void evt_irq( int, void *);
struct evt_queue *evt_queue_alloc( int);
int evt_queue_free( struct evt_queue *);
int evt_register( struct evt_queue *, int);
int evt_unregister( struct evt_queue *, int);
int evt_read( struct evt_queue *, int *, int, char *);
int *evt_get_src_id( struct evt_queue *);

#endif /*  _H_FIFOLIB */


/*================================< end file >================================*/
