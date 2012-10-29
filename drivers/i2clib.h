/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : i2clib.h
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
 * $Log: i2clib.h,v $
 * Revision 1.13  2012/10/29 10:06:55  kalantari
 * added the tosca driver version 4.22 from IoxoS
 *
 * Revision 1.5  2012/09/27 09:49:24  ioxos
 * release CPU for 1 tick in wait function [JFG]
 *
 * Revision 1.4  2012/08/28 13:37:46  ioxos
 * cleanup + i2c reset [JFG]
 *
 * Revision 1.3  2012/05/23 15:40:32  ioxos
 * bug in calling i2c_read [JFG]
 *
 * Revision 1.2  2012/01/27 13:13:04  ioxos
 * prepare release 4.01 supporting x86 & ppc [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:06  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_I2CLIB
#define _H_I2CLIB

void i2c_set_elb( uint);
uint i2c_cmd( uint, uint, uint);
uint i2c_read( uint, uint, uint *, uint);
uint i2c_write( uint, uint, uint, uint);
uint i2c_reset( uint, uint);
uint i2c_wait( uint, uint);

#endif /*  _H_I2CLIB */


/*================================< end file >================================*/
