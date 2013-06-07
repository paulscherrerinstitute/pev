/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : adn.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations and definition needed by the ADN1100
 *    device driver.
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
 * $Log: adndrvr.h,v $
 * Revision 1.1  2013/06/07 15:03:25  zimoch
 * update to latest version
 *
 * Revision 1.2  2013/04/15 14:34:20  ioxos
 * add support multiple APX2300/ADN400x [JFG]
 *
 * Revision 1.1  2013/03/08 16:04:16  ioxos
 * first checkin [JFG]
 *
 * Revision 1.2  2013/02/21 10:58:04  ioxos
 * support for 2 dma channels [JFG]
 *
 * Revision 1.1  2013/02/05 10:46:16  ioxos
 * first checkin [JFG]
 *
 *  
 *=============================< end file header >============================*/


#ifndef _H_ADNDRVR
#define _H_ADNDRVR

// Number of adn devices
#ifndef ADN_COUNT
#define ADN_COUNT 4
#endif

// Name of the adn driver
#ifndef ADN_NAME
#define ADN_NAME "adn"
#endif

// First minor number
#ifndef ADN_MINOR_START
#define ADN_MINOR_START 0
#endif

struct adn_dev
{
  struct pci_dev *dev;
  int board;
  int io_base;
  void *csr_ptr;
  void *shm_ptr;
  void *hcr_ptr;
  void *adr_ptr;
};

struct adn_drv
{
  struct cdev cdev;
  dev_t dev_id;
  struct adn_dev *adn[4];
};

#endif /*  _H_ADNDRVR */

/*================================< end file >================================*/
