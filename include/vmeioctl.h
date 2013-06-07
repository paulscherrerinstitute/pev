/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : vmeioctl.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations and definitions needed to perform
 *    ioctl() operation on the VME device driver.
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
 * $Log: vmeioctl.h,v $
 * Revision 1.1  2013/06/07 15:02:43  zimoch
 * update to latest version
 *
 * Revision 1.1  2012/10/12 12:56:58  ioxos
 * first checkin [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_VMEIOCTL
#define _H_VMEIOCTL

#define VME_BOARD_REGISTER  0x1
#define VME_IRQ_MASK      0x2
#define VME_IRQ_UNMASK    0x3

struct vme_board
{
  uint base; /* vme base address */
  int am;    /* address modifier */
  uint size; /* address window size */
  int irq;   /* irq level */
  int vec;   /* irq vector */
};

#endif /*  _H_VMEIOCTL */
