/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : vme.h
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
 * $Log: vme.h,v $
 * Revision 1.6  2012/10/01 14:56:49  kalantari
 * added verion 4.20 of tosca-driver from IoxoS
 *
 * Revision 1.1  2012/06/01 14:02:29  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef _H_VME
#define _H_VME

int xprs_vme( struct cli_cmd_para *);
int xprs_vme_rmw( struct cli_cmd_para *);
int  vme_init( void);

#endif /* _H_VME */
