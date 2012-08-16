/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : dmalib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    dmalib.c
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
 * $Log: dmalib.h,v $
 * Revision 1.10  2012/08/16 09:11:38  kalantari
 * added version 4.16 of tosca driver
 *
 * Revision 1.9  2012/04/17 07:47:03  ioxos
 * support pipe mode on PPC [JFG]
 *
 * Revision 1.8  2012/04/12 13:33:37  ioxos
 * support for dma swapping [JFG]
 *
 * Revision 1.7  2010/01/13 16:49:35  ioxos
 * xenomai support for DMA list [JFG]
 *
 * Revision 1.6  2010/01/08 11:20:07  ioxos
 * add support to read DMA list from VME [JFG]
 *
 * Revision 1.5  2009/11/10 09:06:15  ioxos
 * add support for DMA transfer mode [JFG]
 *
 * Revision 1.4  2009/05/20 08:22:15  ioxos
 * multicrate support + pipeline mode [JFG]
 *
 * Revision 1.3  2009/01/09 13:09:54  ioxos
 * add support for DMA status [JFG]
 *
 * Revision 1.2  2008/12/12 13:41:54  jfg
 * delare new functions implemented in dmalib [JFG]
 *
 * Revision 1.1  2008/09/17 12:10:25  ioxos
 * file creation [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_DMALIB
#define _H_DMALIB

int  dma_init( int, void *, ulong, uint, uint, uint);
#ifdef XENOMAI
int  dma_init_xeno( int, rtdm_user_info_t *);
#endif
int dma_alloc_kmem( int, void *, ulong);
void *dma_get_buf_kaddr( int);
ulong dma_get_buf_baddr( int);
uint dma_set_ctl( uint, uint, uint, uint);
int dma_set_wr_desc( int, ulong, ulong,  uint, unsigned char, uint);
int dma_set_rd_desc( int, ulong, ulong,  uint, unsigned char, uint);
int dma_set_pipe_desc( int, ulong, ulong,  uint, char, char, uint);
int dma_get_desc( int, uint *);
int dma_set_list_desc( int,  struct pev_ioctl_dma_req *);
int dma_get_list( int,  struct pev_ioctl_dma_req *);

#endif /*  _H_DMALIB */

/*================================< end file >================================*/
