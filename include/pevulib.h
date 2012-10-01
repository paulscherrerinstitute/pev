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
 * Revision 1.15  2012/10/01 14:56:49  kalantari
 * added verion 4.20 of tosca-driver from IoxoS
 *
 * Revision 1.28  2012/09/04 13:18:59  ioxos
 * new function to map system memory statically allocated [JFG]
 *
 * Revision 1.27  2012/09/03 13:10:33  ioxos
 * pointer to data as arg of read function and return i2c cycle status [JFG]
 *
 * Revision 1.26  2012/08/29 11:30:15  ioxos
 * declare pev_map_finf() [JFG]
 *
 * Revision 1.25  2012/08/28 13:53:43  ioxos
 * cleanup + update i2c status + reset [JFG]
 *
 * Revision 1.24  2012/08/27 08:45:14  ioxos
 * include pevioctl.h [JFG]
 *
 * Revision 1.23  2012/08/07 09:21:12  ioxos
 * support for BMR DC-DC converter [JFG]
 *
 * Revision 1.22  2012/06/01 14:20:06  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.21  2012/05/23 08:14:39  ioxos
 * add support for event queues [JFG]
 *
 * Revision 1.20  2012/04/12 13:45:39  ioxos
 * bug in eeprom_.. declaration [JFG]
 *
 * Revision 1.19  2012/04/12 13:43:50  ioxos
 * support for eeprom access [JFG]
 *
 * Revision 1.18  2012/03/29 08:43:11  ioxos
 * get rid of old stuff [JFG]
 *
 * Revision 1.17  2012/03/27 09:17:40  ioxos
 * add support for FIFOs [JFG]
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

#include <pevioctl.h>

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
char *pev_board_name( void);
int pev_get_crate( void);
char *pev_get_driver_version(void);
char *pev_get_lib_version(void);
struct pev_reg_remap *pev_io_remap( void);
struct pev_node *pev_set_crate( uint);
int pev_exit( struct pev_node *);
#ifdef PPC
long long pev_swap_64( long long);
#else
long pev_swap_64( long);
#endif
int pev_swap_32( int);
short pev_swap_16( short);
int pev_rdwr( struct pev_ioctl_rdwr *);
void *pev_mmap( struct pev_ioctl_map_pg *);
int pev_munmap( struct pev_ioctl_map_pg *);
void *pev_vmap( struct pev_ioctl_map_pg *);
int pev_map_alloc( struct pev_ioctl_map_pg *);
int pev_map_free( struct pev_ioctl_map_pg *);
int pev_map_modify( struct pev_ioctl_map_pg *);
int pev_map_find( struct pev_ioctl_map_pg *);
int pev_map_read( struct pev_ioctl_map_ctl *);
int pev_map_clear( struct pev_ioctl_map_ctl *);
int pev_i2c_cmd( uint, uint);
int pev_i2c_read( uint, uint, uint *);
int pev_i2c_write( uint, uint, uint);
int pev_i2c_reset( uint);
int pev_pex_read( uint, uint *);
float pev_bmr_conv_11bit_u( unsigned short);
float pev_bmr_conv_11bit_s( unsigned short);
float pev_bmr_conv_16bit_u( unsigned short);
int pev_pex_write( uint, uint);
int pev_bmr_read( uint, uint, uint *, uint);
int pev_bmr_write( uint, uint, uint, uint);
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
void *pev_buf_map( struct pev_ioctl_buf *);
int pev_buf_unmap( struct pev_ioctl_buf *);
int pev_dma_move( struct pev_ioctl_dma_req *);
int pev_dma_vme_list_rd( void *, struct pev_ioctl_dma_list *, int);
int pev_dma_status( struct pev_ioctl_dma_sts *);
int pev_vme_init( void);
void pev_vme_irq_init( void);
void pev_vme_sysreset( int);
void pev_vme_irq_mask( uint);
void pev_vme_irq_unmask( uint);
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
int pev_smon_rd( int);
void pev_smon_wr( int, int);
int pev_csr_rd( int);
void pev_csr_wr( int, int);
void pev_csr_set( int, int);
int pev_elb_rd( int);
int pev_elb_wr( int, int);
int pev_fifo_init( void);
int pev_fifo_status( uint, uint *);
int pev_fifo_clear( uint, uint *);
int pev_fifo_wait_ef( uint,  uint *, uint);
int pev_fifo_wait_ff( uint,  uint *, uint);
int pev_fifo_read( uint,  uint *, uint, uint *);
int pev_fifo_write( uint,  uint *, uint, uint *);
int pev_eeprom_rd( uint,  char *, uint);
int pev_eeprom_wr( uint,  char *, uint);
struct pev_ioctl_evt *pev_evt_queue_alloc( int);
int pev_evt_queue_free( struct pev_ioctl_evt *);
int pev_evt_register( struct pev_ioctl_evt *, int);
int pev_evt_unregister( struct pev_ioctl_evt *, int);
int pev_evt_queue_enable( struct pev_ioctl_evt *evt);
int pev_evt_queue_disable( struct pev_ioctl_evt *evt);
int pev_evt_mask( struct pev_ioctl_evt *, int);
int pev_evt_unmask( struct pev_ioctl_evt *,int);
int pev_evt_read( struct pev_ioctl_evt *, int);
#ifdef _cplusplus
}
#endif

#endif /*  _H_PEVULIB */
