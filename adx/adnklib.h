/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : adnklib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    adnklib.c
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
 *  
 *=============================< end file header >============================*/


#ifndef _H_ADNKLIB
#define _H_ADNKLIB

int adn_hrms_init( struct adn_dev *);
int adn_hrms_cmd( struct adn_dev *, struct adn_ioctl_hrms *);
int adn_hrms_load( struct adn_dev *, struct adn_ioctl_hrms_code *);
int adn_hrms_rdwr( struct adn_dev *, struct adn_ioctl_rdwr *);
int adn_shm_rdwr( struct adn_dev *, struct adn_ioctl_rdwr *);
int adn_cam_rdwr( struct adn_dev *, struct adn_ioctl_rdwr *);
int adn_sflash_rdwr( struct adn_dev *, struct adn_ioctl_rdwr *);

#endif /*  _H_ADNKLIB */

/*================================< end file >================================*/
