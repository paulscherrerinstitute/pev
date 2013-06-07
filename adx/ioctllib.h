/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : ioctllib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    adnioctl.c
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
 * $Log: ioctllib.h,v $
 * Revision 1.1  2013/06/07 15:03:25  zimoch
 * update to latest version
 *
 * Revision 1.2  2013/03/14 11:14:11  ioxos
 * rename spi to sflash [JFG]
 *
 * Revision 1.1  2013/03/08 16:04:17  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef _H_IOCTLLIB
#define _H_IOCTLLIB

int adn_ioctl_hrms( struct adn_dev *, unsigned int,  unsigned long);
int adn_ioctl_cam( struct adn_dev *, unsigned int,  unsigned long);
int adn_ioctl_sflash( struct adn_dev *, unsigned int,  unsigned long);
int adn_ioctl_rw( struct adn_dev *, unsigned int,  unsigned long);

#endif /*  _H_IOCTLLIB */

/*================================< end file >================================*/
