/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : rdwrlib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    rdwrlib.c
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
 * $Log: rdwrlib.h,v $
 * Revision 1.14  2013/06/07 14:58:31  zimoch
 * update to latest version
 *
 * Revision 1.2  2012/01/27 13:13:05  ioxos
 * prepare release 4.01 supporting x86 & ppc [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *  
 *=============================< end file header >============================*/


#ifndef _H_RDWRLIB
#define _H_RDWRLIB

int rdwr_cfg_wr( struct pci_dev *, void *, int, struct pev_rdwr_mode *);
int rdwr_cfg_rd( struct pci_dev *, void *, int, struct pev_rdwr_mode *);
int rdwr_io_wr( uint, void *, int, struct pev_rdwr_mode *);
int rdwr_io_rd( uint, void *, int, struct pev_rdwr_mode *);
int rdwr_wr_sgl( void *, void *, struct pev_rdwr_mode *);
int rdwr_rd_sgl( void *, void *, struct pev_rdwr_mode *);
int rdwr_wr_blk( void *, void *, int, struct pev_rdwr_mode *);
int rdwr_rd_blk( void *, void *, int, struct pev_rdwr_mode *);
int rdwr_loop( void *, void *, int, struct pev_rdwr_mode *);
int rdwr_idt_wr( struct pci_dev *, void *, int, struct pev_rdwr_mode *);
int rdwr_idt_rd( struct pci_dev *, void *, int, struct pev_rdwr_mode *);

#endif /*  _H_RDWRLIB */

/*================================< end file >================================*/
