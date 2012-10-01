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
 * Revision 1.12  2012/10/01 14:56:49  kalantari
 * added verion 4.20 of tosca-driver from IoxoS
 *
 * Revision 1.20  2012/09/04 13:19:49  ioxos
 * new function to map system memory statically allocated [JFG]
 *
 * Revision 1.19  2012/09/03 13:10:34  ioxos
 * pointer to data as arg of read function and return i2c cycle status [JFG]
 *
 * Revision 1.18  2012/08/29 11:30:15  ioxos
 * declare pev_map_finf() [JFG]
 *
 * Revision 1.17  2012/08/28 13:53:43  ioxos
 * cleanup + update i2c status + reset [JFG]
 *
 * Revision 1.16  2012/08/27 08:45:14  ioxos
 * include pevioctl.h [JFG]
 *
 * Revision 1.15  2012/08/07 09:21:12  ioxos
 * support for BMR DC-DC converter [JFG]
 *
 * Revision 1.14  2012/06/01 14:20:06  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.13  2012/05/23 08:14:39  ioxos
 * add support for event queues [JFG]
 *
 * Revision 1.12  2012/04/12 13:45:39  ioxos
 * bug in eeprom_.. declaration [JFG]
 *
 * Revision 1.11  2012/04/12 13:43:50  ioxos
 * support for eeprom access [JFG]
 *
 * Revision 1.10  2012/03/27 09:17:40  ioxos
 * add support for FIFOs [JFG]
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

#include <pevioctl.h>

#ifdef _cplusplus
extern "C" {
#endif
struct pevx_node
{
  int fd;
  int crate;
  int tmp1;
  int tmp2;
};

struct pevx_time
{
  uint time;
  uint utime;
};

struct pevx_node *pevx_init( uint);
char *pevx_id( void);
uint pevx_board( void);
char *pevx_board_name(void);
char *pevx_get_driver_version(void);
char *pevx_get_lib_version(void);
struct pev_reg_remap *pevx_io_remap( void);
struct pevx_node *pevx_set_crate( uint);
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
int pevx_i2c_cmd( uint, uint, uint);
int pevx_i2c_read( uint, uint, uint, uint *);
int pevx_i2c_write( uint, uint, uint, uint);
int pevx_i2c_reset( uint, uint);
int pevx_pex_read( uint, uint, uint *);
int pevx_pex_write( uint, uint, uint);
int pevx_bmr_read( uint, uint, uint, uint *, uint);
int pevx_bmr_write( uint, uint, uint, uint, uint);
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
uint pevx_timer_read( uint, struct pevx_time *);
void *pevx_buf_alloc( uint, struct pev_ioctl_buf *);
int pevx_buf_free( uint, struct pev_ioctl_buf *);
void *pevx_buf_map( uint, struct pev_ioctl_buf *);
int pevx_buf_unmap( uint, struct pev_ioctl_buf *);
int pevx_dma_move( uint, struct pev_ioctl_dma_req *);
int pevx_dma_vme_list_rd( uint, void *, struct pev_ioctl_dma_list *, int);
int pevx_dma_status( uint, struct pev_ioctl_dma_sts *);
int pevx_vme_init( uint);
int pevx_vme_irq_init( uint);
int pevx_vme_sysreset( uint, uint);
int pevx_vme_irq_mask( uint, uint);
int pevx_vme_irq_unmask( uint, uint);
int pevx_vme_conf_read( uint, struct pev_ioctl_vme_conf *);
int pevx_vme_conf_write( uint, struct pev_ioctl_vme_conf *);
int pevx_vme_lock( uint, struct pev_ioctl_vme_lock *);
int pevx_vme_unlock( uint);
struct pev_ioctl_vme_irq *pevx_vme_irq_alloc( uint, uint);
int pevx_vme_irq_free( uint, struct pev_ioctl_vme_irq *);
int pevx_vme_irq_arm( uint, struct pev_ioctl_vme_irq *);
int pevx_vme_irq_wait( uint, struct pev_ioctl_vme_irq *, uint, uint *);
int pevx_vme_irq_armwait( uint, struct pev_ioctl_vme_irq *, uint, uint *);
int pevx_vme_irq_clear( uint, struct pev_ioctl_vme_irq *);
int pevx_elb_rd( uint, int);
int pevx_elb_wr( uint, int, int);
int pevx_fifo_init( uint);
int pevx_fifo_status( uint, uint, uint *);
int pevx_fifo_clear( uint, uint, uint *);
int pevx_fifo_wait_ef( uint, uint,  uint *, uint);
int pevx_fifo_wait_ff( uint, uint,  uint *, uint);
int pevx_fifo_read( uint, uint,  uint *, uint, uint *);
int pevx_fifo_write( uint, uint,  uint *, uint, uint *);
int pevx_eeprom_rd( uint,  uint, char *, uint);
int pevx_eeprom_wr( uint, uint, char *, uint);
int pevx_evt_queue_free( uint, struct pev_ioctl_evt *);
int pevx_evt_register( uint, struct pev_ioctl_evt *, int);
int pevx_evt_unregister( uint, struct pev_ioctl_evt *, int);
int pevx_evt_queue_enable( uint, struct pev_ioctl_evt *evt);
int pevx_evt_queue_disable( uint, struct pev_ioctl_evt *evt);
int pevx_evt_mask( uint, struct pev_ioctl_evt *, int);
int pevx_evt_unmask( uint, struct pev_ioctl_evt *,int);
int pevx_evt_read( uint, struct pev_ioctl_evt *, int);
#ifdef _cplusplus
}
#endif

#endif /*  _H_PEVXULIB */
