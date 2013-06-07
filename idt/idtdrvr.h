/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : idt.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations and definition needed by the IDT1100
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
 * $Log: idtdrvr.h,v $
 * Revision 1.1  2013/06/07 15:03:26  zimoch
 * update to latest version
 *
 * Revision 1.2  2013/02/21 10:58:04  ioxos
 * support for 2 dma channels [JFG]
 *
 * Revision 1.1  2013/02/05 10:46:16  ioxos
 * first checkin [JFG]
 *
 *  
 *=============================< end file header >============================*/


#ifndef _H_IDTDRVR
#define _H_IDTDRVR

// Number of idt devices
#ifndef IDT_COUNT
#define IDT_COUNT 5
#endif

// Name of the idt driver
#ifndef IDT_NAME
#define IDT_NAME "idt"
#endif

// First minor number
#ifndef IDT_MINOR_START
#define IDT_MINOR_START 0
#endif

struct idt_dev
{
  int idx;
  struct pci_dev *dev;
  uint nt_csr_base;
  uint nt_csr_len;
  char *nt_csr_ptr;
  uint dma_csr_base;
  uint dma_csr_len;
  char *dma_csr_ptr;
  uint dma_shm_base;
  uint dma_shm_offset;
  uint dma_shm_len;
  char *dma_shm_ptr;
  uint dma_des_base[2];
  uint dma_mbx_base;
  void *dma_ctl;
  uint dma_status[2];
  struct semaphore dma_lock[2];
  int msi; 
  struct semaphore sem_msi[4];
};

struct idt_drv
{
  struct cdev cdev;
  dev_t dev_id;
  struct pci_dev *dev;
  int idx_max;
  struct idt_dev *idt[5];
};


#endif /*  _H_IDTDRVR */

/*================================< end file >================================*/
