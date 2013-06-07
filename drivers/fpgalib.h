/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : fpgalib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : november 30,2011
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    fpgalib.c
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
 *----------------------------------------------------------------------------d
 *  Change History
 *  
 * $Log: fpgalib.h,v $
 * Revision 1.12  2013/06/07 14:58:31  zimoch
 * update to latest version
 *
 * Revision 1.1  2012/01/27 09:56:49  ioxos
 * first checkin [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_FPGALIB
#define _H_FPGALIB

void fpga_set_dev( uint);
uint fpga_load( ulong, unsigned char *, uint, uint);

#endif /*  _H_FPGALIB */


/*================================< end file >================================*/
