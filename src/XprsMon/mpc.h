/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : mpc.h
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
 * $Log: mpc.h,v $
 * Revision 1.8  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.2  2012/12/05 14:37:12  ioxos
 * support for S10, S20 and cleanup [JFG]
 *
 * Revision 1.1  2012/06/01 14:02:28  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef _H_MPC
#define _H_MPC
int xprs_map( struct cli_cmd_para *);
int mpc_init( void);
int mpc_exit( void);
int xprs_mpc( struct cli_cmd_para *);


#endif /* _H_MPC */
