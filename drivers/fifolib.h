/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : fifolib.h
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
 * $Log: fifolib.h,v $
 * Revision 1.3  2012/06/14 14:00:04  kalantari
 * added support for r/w PCI_IO bus registers, also added read USR1 generic area per DMA and distribute the readout into individual records
 *
 * Revision 1.1  2012/03/27 09:17:39  ioxos
 * add support for FIFOs [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_FIFOLIB
#define _H_FIFOLIB

void fifo_init( uint);
int fifo_read( uint, uint, uint *, uint, uint *);
int fifo_write( uint, uint, uint *, uint, uint *);
void fifo_clear( uint, uint, uint *);
void fifo_status( uint, uint, uint *);

#endif /*  _H_FIFOLIB */


/*================================< end file >================================*/
