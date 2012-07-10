/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : tst.h
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
 * $Log: tst.h,v $
 * Revision 1.3  2012/07/10 10:21:48  kalantari
 * added tosca driver release 4.15 from ioxos
 *
 * Revision 1.1  2012/06/01 14:02:29  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef _H_TST
#define _H_TST

int xprs_tinit( struct cli_cmd_para *);
int xprs_tkill( struct cli_cmd_para *);
int xprs_tlist( struct cli_cmd_para *);
int xprs_tset( struct cli_cmd_para *);
int xprs_tstart( struct cli_cmd_para *);
int xprs_tstatus( struct cli_cmd_para *);
int xprs_tstop( struct cli_cmd_para *);
void tst_init( void);
void tst_exit( void);

#endif /* _H_TST */
