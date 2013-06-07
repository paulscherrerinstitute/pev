/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pevrtlib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    pevrtlib.c
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
 * $Log: pevrtlib.h,v $
 * Revision 1.14  2013/06/07 15:02:43  zimoch
 * update to latest version
 *
 * Revision 1.3  2010/01/13 16:50:55  ioxos
 * add real time support for DMA list [JFG]
 *
 * Revision 1.2  2009/06/03 12:39:41  ioxos
 * define translation for real time functions [JFG]
 *
 * Revision 1.1  2009/05/26 09:47:16  ioxos
 * first checkin [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_PEVRTLIB
#define _H_PEVRTLIB

int pev_rt_init( int);

#define pev_dma_move(x)              pev_rt_dma_move(x)
#define pev_dma_vme_list_rd(x,y,z)   pev_rt_dma_vme_list_rd(x,y,z)
#define pev_dma_status(x)            pev_rt_dma_status(x)
#define pev_timer_start(x,y)         pev_rt_timer_start(x,y)
#define pev_timer_restart(x)         pev_rt_timer_restart(x)
#define pev_timer_stop(x)            pev_rt_timer_stop(x)
#define pev_timer_read(x)            pev_rt_timer_read(x)

#endif /*  _H_PEVRTLIB */
