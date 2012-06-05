/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : debug.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations and definitions used to enable
 *    debug messages
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
 * $Log: debug.h,v $
 * Revision 1.6  2012/06/05 13:37:31  kalantari
 * linux driver ver.4.12 with intr Handling
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 * Revision 1.1  2008/06/30 15:43:43  ioxos
 * file creation [JFG]
 *
 * Revision 1.2  2008/06/30 12:03:51  ioxos
 * change file description [JFG]
 *
 * Revision 1.1  2008/06/30 11:58:39  ioxos
 * first CVS checkin [JFG]
 *
 *  
 *=============================< end file header >============================*/
#ifdef DEBUG
#define debug(x) printf x
#else
#define debug(x)
#endif
