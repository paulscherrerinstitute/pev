/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : idtlib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    idtlib.c
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


#ifndef _H_IDTLIB
#define _H_IDTLIB

int idt_swap_32( int);
int idt_csr_rd( struct pci_dev *, int, int *);
int idt_csr_wr( struct pci_dev *, int, int);
int idt_dma_init( struct idt_dev *);
int idt_dma_move( struct idt_dev *, struct idt_ioctl_dma_req *);
void idt_dma_exit( struct idt_dev *);
int idt_mbx_wait( struct idt_dev *,  struct idt_ioctl_mbx *);

#endif /*  _H_IDTLIB */

/*================================< end file >================================*/
