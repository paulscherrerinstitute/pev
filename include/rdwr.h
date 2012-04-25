/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : rdwr.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations and definitions used by XprsMon
 *    to perform read/write operations
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
 * $Log: rdwr.h,v $
 * Revision 1.5  2012/04/25 13:18:28  kalantari
 * added i2c epics driver and updated linux driver to v.4.10
 *
 * Revision 1.3  2012/01/27 13:16:06  ioxos
 * prepare release 4.01 supporting x86 & ppc [JFG]
 *
 * Revision 1.2  2008/07/18 14:21:48  ioxos
 * add parameter for loop count [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_RDWR
#define _H_RDWR

struct rdwr_cycle_para
{
  ulong addr;
  ulong data;
  char am; char crate; char dir; char as;
  char op; char space; char ds; char swap;
  uint len;
  int error;
  ulong para;
  ulong mode;
  int loop;
};

#define RDWR_READ_SGL  0x01
#define RDWR_WRITE_SGL 0x02
#define RDWR_READ_BLK  0x11
#define RDWR_WRITE_BLK 0x12

#define RDWR_SPACE_MEM  1
#define RDWR_SPACE_VME  2
#define RDWR_SPACE_PCI  4

#define RDWR_SIZE_BYTE  1
#define RDWR_SIZE_SHORT 2
#define RDWR_SIZE_LONG  4
#define RDWR_SIZE_DBL   8

#define RDWR_SWAP_DATA   1

#define RDWR_STS_ADDR   0x1
#define RDWR_STS_DATA   0x2
#define RDWR_STS_LEN    0x4
#define RDWR_STS_AM     0x8
#define RDWR_STS_CRATE  0x10
#define RDWR_STS_AS     0x20
#define RDWR_STS_LOOP   0x40
#define RDWR_STS_FILE   0x80

struct reg_cycle_para
{
  uint addr_off;
  uint data_off;
  uint reg_idx;
  uint reg_data;
};

#endif /*  _H_RDWR */
