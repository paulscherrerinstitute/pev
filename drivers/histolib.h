/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : histolib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : march 17,2009
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    histolib.c
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
 * $Log: histolib.h,v $
 * Revision 1.3  2012/03/15 14:59:02  kalantari
 * added exact copy of tosca-driver_4.04 from afs
 *
 * Revision 1.1  2009/04/06 09:47:13  ioxos
 * first check-in [JFG]
 *
 *=============================< end file header >============================*/

#ifndef _H_HISTOLIB
#define _H_HISTOLIB

int  *histo_init( void);
void  histo_clear( int);
int  histo_inc( int, int, int);
int  *histo_get( int, int);
int  histo_print( int, int);

#endif /*  _H_HISTOLIB */

/*================================< end file >================================*/
