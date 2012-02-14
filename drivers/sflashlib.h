/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : sflashlib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    sflashlib.c
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
 * $Log: sflashlib.h,v $
 * Revision 1.1  2012/02/14 14:15:45  kalantari
 * added IoxoS driver and module version 3_13 under drivers and modules
 *
 * Revision 1.3  2009/09/29 12:43:38  ioxos
 * support to read/write sflash status [JFG]
 *
 * Revision 1.2  2009/04/06 10:27:27  ioxos
 * first arg in function is register base address[JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *  
 *=============================< end file header >============================*/


#ifndef _H_SFLASHLIB
#define _H_SFLASHLIB


void sflash_read_ID( uint, unsigned char *);
unsigned char sflash_read_status( uint);
void sflash_write_status( uint, unsigned char);
void sflash_read_data( uint, uint, unsigned char *, uint);
int sflash_write_sector( uint, uint, unsigned char *, uint);

#endif /*  _H_SFLASHLIB */

/*================================< end file >================================*/
