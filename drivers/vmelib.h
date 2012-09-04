/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : vmelib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    vmelib.c
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
 * $Log: vmelib.h,v $
 * Revision 1.11  2012/09/04 07:34:33  kalantari
 * added tosca driver 4.18 from ioxos
 *
 * Revision 1.6  2011/03/03 15:42:15  ioxos
 * support for 1MBytes VME slave granularity [JFG]
 *
 * Revision 1.5  2011/01/25 13:40:43  ioxos
 * support for VME RMW [JFG]
 *
 * Revision 1.4  2009/12/15 17:13:25  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.3  2008/11/12 10:36:27  ioxos
 * add timer functions [JFG]
 *
 * Revision 1.2  2008/09/17 11:47:54  ioxos
 * declare VME configuration functions [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_VMELIB
#define _H_VMELIB

struct vme_ctl
{
  uint arb;
  uint master;
  uint slave;
  uint itgen;
  uint crcsr;
  uint bar;
  uint ader;
};
struct vme_reg
{
  uint ctl;
  uint timer;
  uint ader;
  uint csr;
};

#define VME_TIMER_ENA   0x80000000
#define VME_TIMER_START       0x80

struct vme_time
{
  uint time;
  uint utime;
};

int  vme_conf_get( struct vme_reg *, struct vme_ctl *);
int  vme_conf_set( struct vme_reg *, struct vme_ctl *);
int  vme_crcsr_set( uint, int);
int  vme_crcsr_clear( uint, int);
int  vme_itgen_set( uint, uint, uint);
int  vme_itgen_get( uint);
int  vme_itgen_clear( uint);
int  vme_it_ack( uint);
void vme_it_enable( uint);
void vme_it_disable( uint);
void vme_it_clear( uint);
void vme_it_mask( uint, uint);
void vme_it_unmask( uint, uint);
void vme_it_restart( uint, uint);
int vme_timer_start( uint, uint, struct vme_time *);
void vme_timer_restart( uint);
void vme_timer_stop( uint);
void vme_timer_read( uint, struct vme_time *);
uint vme_cmp_swap( uint, uint, uint, uint, uint);
uint vme_lock( uint, uint, uint);
uint vme_unlock( uint);

#endif /*  _H_VMELIB */

/*================================< end file >================================*/
