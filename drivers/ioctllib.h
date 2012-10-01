/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : ioctllib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    pevioctl.c
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
 * $Log: ioctllib.h,v $
 * Revision 1.12  2012/10/01 14:56:49  kalantari
 * added verion 4.20 of tosca-driver from IoxoS
 *
 * Revision 1.5  2012/03/27 09:17:39  ioxos
 * add support for FIFOs [JFG]
 *
 * Revision 1.4  2012/01/27 13:13:04  ioxos
 * prepare release 4.01 supporting x86 & ppc [JFG]
 *
 * Revision 1.3  2009/06/04 13:24:14  ioxos
 * use buf_alloc instead of dma_alloc [JFG]
 *
 * Revision 1.2  2009/04/06 10:35:19  ioxos
 * add prototypes of functions added in pevioctl.c [JFG]
 *
 * Revision 1.1  2008/12/12 13:34:29  jfg
 * first cvs checkin [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_IOCTLLIB
#define _H_IOCTLLIB

int pev_ioctl_buf( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_dma( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_evt( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_histo( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_i2c( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_map( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_reg( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_rdwr( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_rw( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_sflash( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_fpga( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_eeprom( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_timer( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_vme( struct pev_dev *, unsigned int,  unsigned long);
int pev_ioctl_fifo( struct pev_dev *, unsigned int,  unsigned long);

#endif /*  _H_IOCTLLIB */

/*================================< end file >================================*/
