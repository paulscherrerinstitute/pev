/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pevxulib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    pevulib.c
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
 * $Log: pevxulib.h,v $
 * Revision 1.4  2012/03/15 16:15:37  kalantari
 * added tosca-driver_4.05
 *
 * Revision 1.9  2012/01/26 15:57:09  ioxos
 * prepare for IFC1210 support [JFG]
 *
 * Revision 1.8  2011/12/06 14:29:24  ioxos
 * support for multi task VME IRQ [JFG]
 *
 * Revision 1.7  2011/03/03 15:42:47  ioxos
 * support for 1MBytes VME slave granularity [JFG]
 *
 * Revision 1.6  2010/08/26 15:01:04  ioxos
 * remove pevx_csr_csr_rd pevx_csr_wr [JFG]
 *
 * Revision 1.5  2010/08/26 14:24:14  ioxos
 * add pec_csr.. and pev_vme.. declaarations + directive for C++ [JFG]
 *
 * Revision 1.4  2010/06/11 12:05:27  ioxos
 * update pev_buf_alloc() prototype [JFG]
 *
 * Revision 1.3  2010/01/12 15:42:51  ioxos
 * add function to read vme list [JFG]
 *
 * Revision 1.2  2009/12/15 17:14:39  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.1  2009/10/23 09:09:47  ioxos
 * first checkin [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_PEVXULIB
#define _H_PEVXULIB

#ifdef _cplusplus
extern "C" {
#endif
struct pev_node
{
  int fd;
  int crate;
  int tmp1;
  int tmp2;
};

struct pev_time
{
  uint time;
  uint utime;
};

struct pev_node *pevx_init( uint);
char *pevx_id( void);
uint pevx_board( void);
struct pev_reg_remap *pevx_io_remap( void);
struct pev_node *pevx_set_crate( uint);
int pevx_exit( uint);
int pevx_rdwr( uint, struct pev_ioctl_rdwr *);
int pevx_smon_wr( uint, int, int);
int pevx_smon_rd( uint, int);
int pevx_csr_wr( uint, int, int);
int pevx_csr_rd( uint, int);
int pevx_csr_set( uint, int, int);
void *pevx_mmap( uint, struct pev_ioctl_map_pg *);
int pevx_munmap( uint, struct pev_ioctl_map_pg *);
void *pevx_vmap( uint, struct pev_ioctl_map_pg *);
int pevx_map_alloc( uint, struct pev_ioctl_map_pg *);
int pevx_map_free( uint, struct pev_ioctl_map_pg *);
int pevx_map_modify( uint, struct pev_ioctl_map_pg *);
int pevx_map_read( uint, struct pev_ioctl_map_ctl *);
int pevx_map_clear( uint, struct pev_ioctl_map_ctl *);
int pevx_pex_read( uint, uint);
int pevx_pex_write( uint, uint, uint);
int pevx_sflash_id( uint, char *, uint);
int pevx_sflash_rdsr( uint, uint);
int pevx_sflash_wrsr( uint, int, uint);
int pevx_sflash_read( uint, uint, void *, uint);
int pevx_sflash_write( uint, uint, void *, uint);
int pevx_fpga_load( uint, uint, void *, uint);
int pevx_fpga_sign( uint, uint, void *, uint);
int pevx_timer_start( uint, uint, uint);
int pevx_timer_restart( uint);
int pevx_timer_stop( uint);
uint pevx_timer_read( uint, struct pev_time *);
void *pevx_buf_alloc( uint, struct pev_ioctl_buf *);
int pevx_buf_free( uint, struct pev_ioctl_buf *);
int pevx_dma_move( uint, struct pev_ioctl_dma_req *);
int pevx_dma_vme_list_rd( uint, void *, struct pev_ioctl_dma_list *, int);
int pevx_dma_status( uint, struct pev_ioctl_dma_sts *);
int pevx_vme_conf_read( uint, struct pev_ioctl_vme_conf *);
int pevx_vme_conf_write( uint, struct pev_ioctl_vme_conf *);
int pevx_vme_lock( uint, struct pev_ioctl_vme_lock *);
int pevx_vme_unlock( uint);
struct pev_ioctl_vme_irq *pev_vme_irq_alloc( uint, uint);
int pev_vme_irq_free( uint, struct pev_ioctl_vme_irq *);
int pev_vme_irq_arm( uint, struct pev_ioctl_vme_irq *);
int pev_vme_irq_wait( uint, struct pev_ioctl_vme_irq *, uint, uint *);
int pev_vme_irq_armwait( uint, struct pev_ioctl_vme_irq *, uint, uint *);
int pev_vme_irq_clear( uint, struct pev_ioctl_vme_irq *);
int pevx_elb_rd( uint, int);
int pevx_elb_wr( uint, int, int);
#ifdef _cplusplus
}
#endif

#endif /*  _H_PEVXULIB */
