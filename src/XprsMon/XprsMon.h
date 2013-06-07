/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : .h
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
 * $Log: XprsMon.h,v $
 * Revision 1.8  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.3  2013/04/15 14:02:22  ioxos
 * support for ADC3110 [JFG]
 *
 * Revision 1.2  2012/12/13 15:38:36  ioxos
 * add support for IDF switch configuration [JFG]
 *
 * Revision 1.1  2012/06/01 14:02:29  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef _H_XPRSMON
#define _H_XPRSMON

#include "eeprom.h"  
#include "fpga.h"  
#include "map.h"
#include "rdwr.h"    
#include "sflash.h"  
#include "tst.h"
#include "vme.h"
#include "conf.h"   
#include "fifo.h"    
#include "i2c.h"   
#include "idt.h"   
#ifdef MPC
#include "mpc.h"  
#endif
#include "script.h"  
#include "timer.h"   
#include "tty.h"  
#include "adc3110.h"  

#endif /* _H_XPRSMON */
