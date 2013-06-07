/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : a664ulib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    a664ulib.c
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
 * $Log: a664lib.h,v $
 * Revision 1.1  2013/06/07 15:02:43  zimoch
 * update to latest version
 *
 * Revision 1.1  2013/04/15 15:08:15  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef _H_A664ULIB
#define _H_A664ULIB

#ifndef _H_A664
#include <a664.h>
#endif

int a664_init( void);
int a664_exit( void);
int *a664_tx_port_list( void);
int *a664_rx_port_list( void);
int a664_write_afdx( short, char *, int);
int a664_write_sap( short, char *, int, int, short);
int a664_read_sampling( short, char *, int *, int *);
int a664_read_queueing( short, char *, int *);
int a664_read_sap( short, char *, int *, int *, short *);
int a664_status_tx( unsigned short, struct a664_port_status *);
int a664_status_rx( unsigned short, struct a664_port_status *);
int a664_statistics( struct a664_stats *);

#endif /*  _H_A664ULIB */
