/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : adnulib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : january 29,2010
 *    version  : 1.0.0
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    adnulib.c
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
 * $Log: adnulib.h,v $
 * Revision 1.1  2013/06/07 15:02:43  zimoch
 * update to latest version
 *
 * Revision 1.3  2013/04/15 14:16:24  ioxos
 * read/write for CSR, HCR [JFG]
 *
 * Revision 1.2  2013/03/14 11:14:23  ioxos
 * rename spi to sflash [JFG]
 *
 * Revision 1.1  2013/03/08 09:37:31  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef _H_ADNULIB
#define _H_ADNULIB

struct adn_node
{
  int fd;
  int crate;
  int tmp1;
  int tmp2;
};

struct adn_time
{
  uint time;
  uint utime;
};

int adn_swap_32( int);
short adn_swap_16( short);
struct adn_node *adn_init();
int adn_exit( struct adn_node *);
char *adn_get_driver_version();
char *adn_get_lib_version();
void adn_hrms_start( uint);
void adn_hrms_stop( uint);
void adn_hrms_load( struct adn_ioctl_hrms_code *);
int adn_cbm_rd( void *, int , int );
int adn_cbm_wr( void *, int , int);
void adn_hrms_rd( void *, uint , uint, char );
void adn_hrms_wr( void *, uint , uint, char);
void adn_cam_rd( void *, uint , uint );
void adn_cam_wr( void *, uint , uint);
void adn_sflash_rd( void *, uint , uint );
void adn_sflash_wr( void *, uint , uint);
void adn_sflash_erase( uint , uint);
void adn_ctl( uint);
void adn_snmp_trap( uint);
void adn_reg_wr( int, int);
int adn_reg_rd( int);
void adn_csr_wr( int, int);
int adn_csr_rd( int);
void adn_hcr_wr( int, int);
int adn_hcr_rd( int);

#endif /*  _H_ADNULIB */
