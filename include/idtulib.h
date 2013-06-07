/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : idtulib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    idtulib.c
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
 * $Log: idtulib.h,v $
 * Revision 1.1  2013/06/07 15:02:43  zimoch
 * update to latest version
 *
 * Revision 1.1  2013/02/05 10:42:19  ioxos
 * first checkin [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_IDTULIB
#define _H_IDTULIB

#include <idtioctl.h>

int idt_init( int);
void idt_exit( void);
int idt_mbx_read( int, struct idt_ioctl_mbx *);
int idt_mbx_write( int, struct idt_ioctl_mbx *);
int idt_mbx_mask( int, int);
int idt_mbx_unmask( int, int);
int idt_mbx_wait( int,  struct idt_ioctl_mbx *);
int idt_dma_move( int, struct idt_ioctl_dma_req *);
int idt_dma_wait( int, int, int *);

#endif /*  _H_IDTULIB */
