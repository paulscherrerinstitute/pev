/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pevlib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    pevlib.c
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


#ifndef _H_PEVLIB
#define _H_PEVLIB

void pev_irq_register(struct pev_dev *, int, void (*)( struct pev_dev*, int, void *), void *);
int pev_rdwr(struct pev_dev *, struct pev_ioctl_rdwr *);
void pev_sflash_id(struct pev_dev *, unsigned char *, uint);
unsigned short pev_sflash_rdsr(struct pev_dev *, uint);
void pev_sflash_wrsr(struct pev_dev *, unsigned short, uint);
int pev_sflash_read(struct pev_dev *, struct pev_ioctl_sflash_rw *);
int pev_sflash_write(struct pev_dev *, struct pev_ioctl_sflash_rw *);
int pev_fpga_load(struct pev_dev *, struct pev_ioctl_sflash_rw *);
int pev_idt_eeprom_read(struct pev_dev *, struct pev_ioctl_rdwr *);
int pev_idt_eeprom_write(struct pev_dev *, struct pev_ioctl_rdwr *);
int pev_map_init(struct pev_dev *, struct pev_ioctl_map_ctl *);
int pev_map_alloc(struct pev_dev *,  struct pev_ioctl_map_pg *);
int pev_map_free(struct pev_dev *,  struct pev_ioctl_map_pg *);
int pev_map_modify(struct pev_dev *,  struct pev_ioctl_map_pg *);
int pev_map_find(struct pev_dev *,  struct pev_ioctl_map_pg *);
int pev_map_read(struct pev_dev *,  struct pev_ioctl_map_ctl *);
int pev_map_clear(struct pev_dev *,  struct pev_ioctl_map_ctl *);
int pev_map_set_sg(struct pev_dev *, struct pev_ioctl_map_ctl *, uint);
int pev_map_clear_sg(struct pev_dev *, struct pev_ioctl_map_ctl *, uint);
void pev_sg_master_32_set(struct pev_dev *, uint, ulong,  uint);
void pev_sg_master_64_set(struct pev_dev *, uint, ulong,  uint);
void pev_sg_slave_vme_set(struct pev_dev *, uint, ulong,  uint);
void pev_sg_vme_elb_set(struct pev_dev *, uint, ulong,  uint);
void pev_i2c_dev_cmd(struct pev_dev *, struct pev_ioctl_i2c *);
void pev_i2c_dev_read(struct pev_dev *, struct pev_ioctl_i2c *);
void pev_i2c_dev_write(struct pev_dev *, struct pev_ioctl_i2c *);
void pev_i2c_dev_reset(struct pev_dev *, struct pev_ioctl_i2c *);
void pev_i2c_pex_read(struct pev_dev *, struct pev_ioctl_i2c *);
void pev_i2c_pex_write(struct pev_dev *, struct pev_ioctl_i2c *);
void pev_vme_conf_read(struct pev_dev *, struct pev_ioctl_vme_conf *);
void pev_vme_conf_write(struct pev_dev *, struct pev_ioctl_vme_conf *);
uint pev_vme_crcsr(struct pev_dev *, struct pev_ioctl_vme_crcsr *);
uint pev_vme_crcsr(struct pev_dev *, struct pev_ioctl_vme_crcsr *);
uint pev_vme_rmw( struct pev_dev *, struct pev_ioctl_vme_rmw *);
uint pev_vme_lock( struct pev_dev *, struct pev_ioctl_vme_lock *);
uint pev_vme_unlock( struct pev_dev *);
uint pev_vme_slv_init( struct pev_dev *);
void pev_vme_irq( struct pev_dev *, int, void *);
uint pev_vme_irq_alloc( struct pev_dev *, struct pev_ioctl_vme_irq *);
uint pev_vme_irq_arm( struct pev_dev *, struct pev_ioctl_vme_irq *);
uint pev_vme_irq_wait( struct pev_dev *, struct pev_ioctl_vme_irq *);
uint pev_vme_irq_clear( struct pev_dev *, struct pev_ioctl_vme_irq *);
void pev_vme_irq_init( struct pev_dev *);
void pev_timer_irq( struct pev_dev *, int, void *);
void pev_timer_init( struct pev_dev *);
void pev_timer_read( struct pev_dev *, struct pev_ioctl_timer *);
int pev_timer_start( struct pev_dev *, struct pev_ioctl_timer *);
void pev_timer_restart( struct pev_dev *);
void pev_timer_stop( struct pev_dev *);
void pev_timer_irq_ena( struct pev_dev *);
void pev_timer_irq_dis( struct pev_dev *);
void pev_fifo_irq(struct pev_dev *, int, void *);
void pev_fifo_init(struct pev_dev *);
void pev_fifo_status( struct pev_dev *, struct pev_ioctl_fifo *);
void pev_fifo_clear( struct pev_dev *, struct pev_ioctl_fifo *);
int pev_fifo_wait_ef( struct pev_dev *, struct pev_ioctl_fifo *);
int pev_fifo_wait_ff( struct pev_dev *, struct pev_ioctl_fifo *);
int  pev_fifo_read( struct pev_dev *,  struct pev_ioctl_fifo *);
int  pev_fifo_write( struct pev_dev *, struct pev_ioctl_fifo *);
void pev_dma0_irq( struct pev_dev *, int, void *);
void pev_dma1_irq( struct pev_dev *, int, void *);
void pev_dma_init( struct pev_dev *);
void pev_dma_exit( struct pev_dev *);
int pev_dma_move( struct pev_dev *,  struct pev_ioctl_dma_req *);
int pev_dma0_status( struct pev_dev *,  struct pev_ioctl_dma_sts *);
int pev_dma1_status( struct pev_dev *,  struct pev_ioctl_dma_sts *);
int pev_dma0_wait( struct pev_dev *, int);
int pev_dma1_wait( struct pev_dev *, int);
int pev_histo_read( struct pev_dev *,  struct pev_ioctl_histo *);
int pev_histo_clear( struct pev_dev *,  struct pev_ioctl_histo *);
void pev_evt_init( struct pev_dev *);
void pev_evt_alloc( struct pev_dev *, struct pev_ioctl_evt *);
int pev_evt_free( struct pev_dev *, struct pev_ioctl_evt *);
int pev_evt_register( struct pev_dev *, struct pev_ioctl_evt *);
void pev_evt_enable( struct pev_dev *, struct pev_ioctl_evt *);
void pev_evt_unmask( struct pev_dev *, struct pev_ioctl_evt *);
void pev_evt_mask( struct pev_dev *, struct pev_ioctl_evt *);
void pev_evt_clear( struct pev_dev *, struct pev_ioctl_evt *);
void pev_evt_disable( struct pev_dev *, struct pev_ioctl_evt *);
int pev_evt_unregister( struct pev_dev *, struct pev_ioctl_evt *);
int pev_evt_read( struct pev_dev *, struct pev_ioctl_evt *);
void pev_usr1_irq( struct pev_dev *, int, void *);
void pev_usr1_irq_init( struct pev_dev *);
void pev_usr2_irq( struct pev_dev *, int, void *);
void pev_usr2_irq_init( struct pev_dev *);


#endif /*  _H_PEVLIB */

/*================================< end file >================================*/
