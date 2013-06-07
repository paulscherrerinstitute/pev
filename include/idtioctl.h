/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : idtioctl.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : January 15,2013
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations and definitions needed to perform
 *    ioctl() operation on the IDT device driver.
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
 * $Log: idtioctl.h,v $
 * Revision 1.1  2013/06/07 15:02:43  zimoch
 * update to latest version
 *
 * Revision 1.1  2013/02/05 10:42:37  ioxos
 * first checkin [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_IDTIOCTL
#define _H_IDTIOCTL

typedef unsigned char uchar;

#define IDT_IOCTL_VERSION      0x00000101
#define IDT_IOCTL_MBX_READ     0x00010010
#define IDT_IOCTL_MBX_WRITE    0x00010020
#define IDT_IOCTL_MBX_MASK     0x00010030
#define IDT_IOCTL_MBX0_UNMASK  0x00010040
#define IDT_IOCTL_MBX1_UNMASK  0x00010041
#define IDT_IOCTL_MBX2_UNMASK  0x00010042
#define IDT_IOCTL_MBX3_UNMASK  0x00010043
#define IDT_IOCTL_MBX_UNMASK   IDT_IOCTL_MBX0_UNMASK
#define IDT_IOCTL_MBX_WAIT     0x00010050
#define IDT_IOCTL_DMA_MOVE     0x00010110
#define IDT_IOCTL_DMA_WAIT     0x00010150

struct idt_ioctl_mbx
{
  int idx;
  int sts;
  int tmo;
  int msg;
};

struct idt_ioctl_dma_req
{
  ulong src_addr;
  ulong des_addr;
  uint size;
  uchar src_space; uchar src_mode; uchar des_space; uchar des_mode;
  uchar start_mode; uchar end_mode; uchar intr_mode; uchar wait_mode;
  uint dma_status;
  uint mbx_data;
};
#endif /*  _H_IDTIOCTL */
