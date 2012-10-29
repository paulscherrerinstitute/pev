/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : rdwr.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : may 30,2012
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    pevulib.c
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
 * $Log: rdwr.h,v $
 * Revision 1.7  2012/10/29 10:06:56  kalantari
 * added the tosca driver version 4.22 from IoxoS
 *
 * Revision 1.1  2012/06/01 14:02:28  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef _H_RDWR
#define _H_RDWR

int xprs_rdwr_dma( struct cli_cmd_para *);
int xprs_rdwr_evt( struct cli_cmd_para *);
int xprs_rdwr_dm( struct cli_cmd_para *);
int xprs_rdwr_fm( struct cli_cmd_para *);
int xprs_rdwr_lm( struct cli_cmd_para *);
int xprs_rdwr_pm( struct cli_cmd_para *);
int xprs_rdwr_rmw( struct cli_cmd_para *);
int xprs_rdwr_tm( struct cli_cmd_para *);
int  rdwr_init( void);
int  rdwr_exit( void);

#endif /* _H_RDWR */
