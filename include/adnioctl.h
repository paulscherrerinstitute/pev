/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : adnioctl.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : January 15,2013
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations and definitions needed to perform
 *    ioctl() operation on the ADN device driver.
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
 * $Log: adnioctl.h,v $
 * Revision 1.1  2013/06/07 15:02:43  zimoch
 * update to latest version
 *
 * Revision 1.4  2013/04/15 14:15:19  ioxos
 * read/write for SHM, CSR, HCR, SPI [JFG]
 *
 * Revision 1.3  2013/03/14 11:14:23  ioxos
 * rename spi to sflash [JFG]
 *
 * Revision 1.2  2013/03/13 08:04:55  ioxos
 * set version to 1.00 [JFG]
 *
 * Revision 1.1  2013/03/08 09:37:31  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef _H_ADNIOCTL
#define _H_ADNIOCTL

#define ADN_RELEASE "1.00"

#ifdef PPC
typedef unsigned char uchar;
#endif

#define ADN_IOCTL_OP_MASK        0xFFFF0000

#define ADN_IOCTL_RW             0x00010000
#define ADN_IOCTL_RD_CBM         ( ADN_IOCTL_RW | 0x0100)
#define ADN_IOCTL_WR_CBM         ( ADN_IOCTL_RW | 0x0200)
#define ADN_IOCTL_RD_SHM         ( ADN_IOCTL_RW | 0x0100)
#define ADN_IOCTL_WR_SHM         ( ADN_IOCTL_RW | 0x0200)
#define ADN_IOCTL_RD_CSR         ( ADN_IOCTL_RW | 0x0101)
#define ADN_IOCTL_WR_CSR         ( ADN_IOCTL_RW | 0x0201)
#define ADN_IOCTL_RD_HCR         ( ADN_IOCTL_RW | 0x0102)
#define ADN_IOCTL_WR_HCR         ( ADN_IOCTL_RW | 0x0202)
#define ADN_IOCTL_RD_SPI         ( ADN_IOCTL_RW | 0x0104)
#define ADN_IOCTL_WR_SPI         ( ADN_IOCTL_RW | 0x0204)

#define ADN_IOCTL_VERSION      0x00000101

#define ADN_IOCTL_HRMS             0x000a0000
#define ADN_IOCTL_HRMS_INIT        0x000a0000
#define ADN_IOCTL_HRMS_CMD         0x000a0001
#define ADN_IOCTL_HRMS_LOAD        0x000a0002
#define ADN_IOCTL_HRMS_RDWR        0x000a0003

#define ADN_IOCTL_CAM             0x000c0000
#define ADN_IOCTL_CAM_RDWR        0x000c0003

#define ADN_IOCTL_SFLASH             0x000d0000
#define ADN_IOCTL_SFLASH_RDWR        0x000d0003

#define RDWR_CLEAR      2

struct adn_ioctl_rw
{
  uint offset;
  uint data;
  ulong data_l;
};

struct adn_ioctl_rdwr
{
  void *buf;            /* data buffer pointer in user space */
  uint offset;          /* address offset in remote space    */
  uint len;             /* data transfer size in byte        */
  struct adn_rdwr_mode
  {
    char ds;            /* data size                         */
    char swap;          /* swap mode                         */
    char as;            /* address size                      */
    char dir;           /* transfer direction                */
    char crate;         /* remote crate identifier           */
    char am;            /* address modifier                  */
    char space;         /* remote space identifier           */
    char rsv3;
  } mode;
  void *k_addr;
};

#define HRMS_TXC    0x1
#define HRMS_RXC    0x2
#define HRMS_HCC    0x3

#define HRMS_CMD_START    0x00000000
#define HRMS_CMD_STOP     0x00000000
#define HRMS_CMD_RD_REG   0x00000000
#define HRMS_CMD_RD_IO    0x00000000
#define HRMS_CMD_RD_DATA  0x00000000
#define HRMS_CMD_RD_CBM   0x00000000
#define HRMS_CMD_WR_REG   0x00000000
#define HRMS_CMD_WR_IO    0x00000000
#define HRMS_CMD_WR_DATA  0x00000000
#define HRMS_CMD_WR_CBM   0x00000000

struct adn_ioctl_hrms
{
  uint dev;
  uint cmd;
  uint addr;
  uint data;
};

#define HRMS_CODE_SIZE_MAX    2048

struct adn_ioctl_hrms_code
{
  uint dev;
  uint offset;
  uint size;
  uint rsv;
  uint ins[HRMS_CODE_SIZE_MAX];
};

#endif /*  _H_ADNIOCTL */
