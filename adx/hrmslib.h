/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : hrmslib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    hrmslib.c
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

#ifndef _H_HRMSLIB
#define _H_HRMSLIB

void hrms_set_board_type( uint);
int hrms_init( uint);
int hrms_spi_read( uint, uint);
int hrms_spi_write( uint, uint, uint);
int hrms_cmd( uint, uint);
int hrms_load( uint, struct adn_ioctl_hrms_code *);
void hrms_read_cbm( uint, uint, char *, uint, char);
void hrms_write_cbm( uint, uint, char *, uint, char);
void hrms_read_shm( uint, uint, char *, uint, char);
void hrms_write_shm( uint, uint, char *, uint, char);
void hrms_read_cam( uint, uint, unsigned char *, uint);
void hrms_write_cam( uint, uint, unsigned char *, uint);
int hrms_read_sflash( uint, uint, unsigned char *, uint);
int hrms_write_sflash( uint, uint, unsigned char *, uint);
int hrms_erase_sflash( uint, uint, uint);
int hrms_sts_sflash( uint);

#endif /*  _H_HRMSLIB */

/*================================< end file >================================*/
