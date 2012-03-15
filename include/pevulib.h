/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pevulib.h
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
 * $Log: pevulib.h,v $
 * Revision 1.4  2012/03/15 16:15:37  kalantari
 * added tosca-driver_4.05
 *
 * Revision 1.16  2012/01/26 15:57:09  ioxos
 * prepare for IFC1210 support [JFG]
 *
 * Revision 1.15  2011/12/06 13:17:04  ioxos
 * support for multi task VME IRQ [JFG]
 *
 * Revision 1.14  2011/03/03 15:42:47  ioxos
 * support for 1MBytes VME slave granularity [JFG]
 *
 * Revision 1.13  2010/08/26 14:49:54  ioxos
 * add vme_csr_set [JFG]
 *
 * Revision 1.12  2010/08/26 14:24:14  ioxos
 * add pec_csr.. and pev_vme.. declaarations + directive for C++ [JFG]
 *
 * Revision 1.11  2010/06/11 12:05:27  ioxos
 * update pev_buf_alloc() prototype [JFG]
 *
 * Revision 1.10  2010/01/12 15:42:51  ioxos
 * add function to read vme list [JFG]
 *
 * Revision 1.9  2009/12/15 17:14:39  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.8  2009/06/03 12:38:29  ioxos
 * use buf_alloc instead of dma_alloc [JFG]
 *
 * Revision 1.7  2009/06/02 11:47:09  ioxos
 * add driver identification [jfg]
 *
 * Revision 1.6  2009/04/06 12:55:52  ioxos
 * add set_crate() + update pev_init()[JFG]
 *
 * Revision 1.5  2008/12/12 13:55:09  jfg
 * declare memory mapping and dma functions [JFG]
 *
 * Revision 1.4  2008/11/12 12:17:18  ioxos
 * declare  pev_csr_set(), pev_map_find(), pev_timer_xxx() functions and a structure to hold VME time [JFG]
 *
 * Revision 1.3  2008/08/08 09:06:13  ioxos
 * add declaration for pev_sflash_xxx() functions [JFG]
 *
 * Revision 1.2  2008/07/04 07:41:00  ioxos
 * update address mapping functions [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_PEVULIB
#define _H_PEVULIB

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

struct pev_node *pev_init( uint);
char *pev_id( void);
uint pev_board( void);
struct pev_reg_remap *pev_io_remap( void);
struct pev_node *pev_set_crate( uint);
int pev_exit( struct pev_node *);
int pev_vme_write_byte( uint,  char, ulong, uint);
int pev_vme_write_short( uint,  short, ulong, uint);
int pev_vme_write_int( uint,  int, ulong, uint);
int pev_vme_write_long( uint,  long, ulong, uint);
int pev_vme_write_block( uint,  void *, ulong, uint, uint);
int pev_vme_read_byte( uint,  void *, ulong, uint);
int pev_vme_read_short( uint,  void *, ulong, uint);
int pev_vme_read_int( uint,  void *, ulong, uint);
int pev_vme_read_long( uint,  void *, ulong, uint);
int pev_vme_read_block( uint,  void *, ulong, uint, uint);
int pev_exit( struct pev_node *);
int pev_shm_write_byte( uint,  char, ulong);
int pev_shm_write_short( uint,  short, ulong);
int pev_shm_write_int( uint,  int, ulong);
int pev_shm_write_long( uint,  long, ulong);
int pev_shm_write_block( uint,  void *, ulong, uint);
int pev_shm_read_byte( uint,  void *, ulong);
int pev_shm_read_short( uint,  void *, ulong);
int pev_shm_read_int( uint,  void *, ulong);
int pev_shm_read_long( uint,  void *, ulong);
int pev_shm_read_block( uint,  void *, ulong, uint);
int pev_rdwr( struct pev_ioctl_rdwr *);
void *pev_mmap( struct pev_ioctl_map_pg *);
int pev_munmap( struct pev_ioctl_map_pg *);
void *pev_vmap( struct pev_ioctl_map_pg *);
int pev_map_alloc( struct pev_ioctl_map_pg *);
int pev_map_free( struct pev_ioctl_map_pg *);
int pev_map_modify( struct pev_ioctl_map_pg *);
int pev_map_read( struct pev_ioctl_map_ctl *);
int pev_map_clear( struct pev_ioctl_map_ctl *);
int pev_i2c_cmd( uint, uint);
int pev_i2c_read( uint, uint);
int pev_i2c_write( uint, uint, uint);
int pev_pex_read( uint);
int pev_pex_write( uint, uint);
int pev_sflash_id( char *, uint);
int pev_sflash_rdsr( uint);
int pev_sflash_wrsr( int, uint);
int pev_sflash_read( uint, void *, uint);
int pev_sflash_write( uint, void *, uint);
int pev_fpga_load( uint, void *, uint);
int pev_fpga_sign( uint, void *, uint);
int pev_timer_start( uint, uint);
void pev_timer_restart( void);
void pev_timer_stop( void);
uint pev_timer_read( struct pev_time *);
void *pev_buf_alloc( struct pev_ioctl_buf *);
int pev_buf_free( struct pev_ioctl_buf *);
int pev_dma_move( struct pev_ioctl_dma_req *);
int pev_dma_vme_list_rd( void *, struct pev_ioctl_dma_list *, int);
int pev_dma_status( struct pev_ioctl_dma_sts *);
int pev_vme_conf_read( struct pev_ioctl_vme_conf *);
int pev_vme_conf_write( struct pev_ioctl_vme_conf *);
int pev_vme_crcsr( struct pev_ioctl_vme_crcsr *);
int pev_vme_rmw( struct pev_ioctl_vme_rmw *);
int pev_vme_lock( struct pev_ioctl_vme_lock *);
int pev_vme_unlock( void);
struct pev_ioctl_vme_irq *pev_vme_irq_alloc( uint);
int pev_vme_irq_free( struct pev_ioctl_vme_irq *);
int pev_vme_irq_arm( struct pev_ioctl_vme_irq *);
int pev_vme_irq_wait( struct pev_ioctl_vme_irq *, uint, uint *);
int pev_vme_irq_armwait( struct pev_ioctl_vme_irq *, uint, uint *);
int pev_vme_irq_clear( struct pev_ioctl_vme_irq *);
int pev_csr_rd( int);
void pev_csr_wr( int, int);
void pev_csr_set( int, int);
int pev_elb_rd( int);
int pev_elb_wr( int, int);
#ifdef _cplusplus
}
#endif

#endif /*  _H_PEVULIB */
