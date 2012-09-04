/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pev.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations and definition needed by the PEV1100
 *    device driver.
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
 * $Log: pevdrvr.h,v $
 * Revision 1.11  2012/09/04 07:34:33  kalantari
 * added tosca driver 4.18 from ioxos
 *
 * Revision 1.17  2012/08/27 08:41:45  ioxos
 * support for VME fast single cycles through ELB bus [JFG]
 *
 * Revision 1.16  2012/08/13 15:31:39  ioxos
 * support for timeout while waiting for DMA interrupts [JFG]
 *
 * Revision 1.15  2012/06/28 12:22:57  ioxos
 * support for register access through PCI MEM + IRQ from usr1 and usr2 [JFG]
 *
 * Revision 1.14  2012/04/05 13:43:38  ioxos
 * dynamic io_remap + I2C & SPI locking + CSR window if supported by fpga [JFG]
 *
 * Revision 1.13  2012/03/27 09:17:40  ioxos
 * add support for FIFOs [JFG]
 *
 * Revision 1.12  2012/01/26 16:17:17  ioxos
 * prepare for IFC1210 support [JFG]
 *
 * Revision 1.11  2011/12/06 13:15:18  ioxos
 * support for multi task VME IRQ [JFG]
 *
 * Revision 1.10  2011/10/19 14:06:06  ioxos
 * support for powerpc + release 3.11 [JFG]
 *
 * Revision 1.9  2009/12/15 17:13:24  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.8  2009/05/20 08:27:13  ioxos
 * add XENOMAI synchronization for dma functions [JFG]
 *
 * Revision 1.7  2009/04/06 12:18:47  ioxos
 * new pev_drv struct for multicrate + xenomai support [JFG]
 *
 * Revision 1.6  2009/02/09 13:56:29  ioxos
 * enable DMA irq according to engine actually started [JFG]
 *
 * Revision 1.5  2008/12/12 13:23:50  jfg
 * support forIRQ table and DMA
 *
 * Revision 1.4  2008/08/08 08:57:14  ioxos
 * add pex and plx parameters in  struct pev_dev{} [JFG]
 *
 * Revision 1.3  2008/07/18 14:48:21  ioxos
 * in strucr pev replace irq by msi [JFG]
 *
 * Revision 1.2  2008/07/04 07:40:12  ioxos
 * update address mapping functions [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:06  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *  
 *=============================< end file header >============================*/


#ifndef _H_PEVDRVR
#define _H_PEVDRVR

// Number of pev devices
#ifndef PEV_COUNT
#define PEV_COUNT 16
#endif

// Name of the pev driver
#ifndef PEV_NAME
#define PEV_NAME "pev"
#endif

// First minor number
#ifndef PEV_MINOR_START
#define PEV_MINOR_START 0
#endif


/*---------------------------------------------------------
 * The following data structure holds ...
 *--------------------------------------------------------*/

/* PEV1100 registers */
struct pev_reg
{
  struct pev_reg_pcie
  {
    u32 options;      /* 0x000  Static Options                       */
    u32 cnx_host;     /* 0x004  Host Connector                       */
    u32 cnx_aux;      /* 0x008  Auxilliary Connector                 */
    u32 ponfsm;       /* 0x00C  Power On sequencer status            */
    u32 spi;          /* 0x010  FPROM programming                    */
    u32 pcie_sw;      /* 0x014  PCI Express switch status            */
    u32 v5usr;        /* 0x018  Configuration                        */
    u32 genctl;       /* 0x01C  General control                      */
    u32 mmu_addr;     /* 0x020  MMU address pointer                  */
    u32 mmu_data;     /* 0x024  MMU data                             */
    u32 dum028[6];    /* 0x028  reserved   -> 0x40                   */
    u32 smon_addr;    /* 0x040  FPGA System monitor address pointer  */
    u32 smon_data;    /* 0x044  FPGA System monitor data             */
    u32 smon_sts;     /* 0x048  FPGA System monitor status           */
    u32 dum04C[13];   /* 0x04C  reserved   -> 0x80                   */
    u32 itc_iack;     /* 0x080  Interrupt Acknowledge                */
    u32 itc_csr;      /* 0x084  Interrupt Control and Status         */
    u32 itc_imc;      /* 0x088  Interrupt Mask Clear                 */
    u32 itc_ims;      /* 0x08C  Interrupt Mask Set                   */
    u32 dum090[12];   /* 0x090  reserved  -> 0xC0                    */
    u32 pmc_xmc;      /* 0x0C0  PMC/XMC sideband signaling           */
    u32 dum0C4[15];   /* 0x0C4  reserved  -> 0x100                   */
    u32 i2c_ctl;      /* 0x100  I2C control                          */
    u32 i2c_cmd;      /* 0x104  I2C Command                          */
    u32 i2c_wr;       /* 0x108  I2C data write                       */
    u32 i2c_rd;       /* 0x10C  I2C data read                        */
    u32 dum110[60];   /* 0x110 reserved  -> 0x200                    */
    u32 ep_addr;      /* 0x200  FPGA PCIE End Point address pointer  */
    u32 ep_data;      /* 0x204  FPGA PCIE End Point data             */
    u32 ep_sel;       /* 0x208  FPGA PCIE End Point sel              */
    u32 ep_rslt;      /* 0x20C  FPGA PCIE End Point sel              */
    u32 dum210[92];   /* 0x210 reserved -> 0x380                     */
    u32 rom[32];      /* 0x380 ROM sgnature                          */
  } pcie;
  struct pev_reg_vme
  {
    u32 slot1_csr;    /* 0x400 VME system controller CSR             */
    u32 mas_csr;      /* 0x404 VNE Master port CSR                   */
    u32 slv_csr;      /* 0x408 VME Slave port CSR                    */
    u32 intr_gen;     /* 0x40C VME Interrupt Generator               */
    u32 mmu_addr;     /* 0x410 MMU address pointer                   */
    u32 mmu_data;     /* 0x414 MMU data                              */
    u32 dum418[26];   /* 0x418 reserved -> 0x480                     */
    u32 itc_iack;     /* 0x480 Interrupt Acknowledge                 */
    u32 itc_csr;      /* 0x484 Interrupt Control and Status          */
    u32 itc_imc;      /* 0x488 Interrupt Mask Clear                  */
    u32 itc_ims;      /* 0x48C Interrupt Mask Set                    */
    u32 dum490[52];   /* 0x490 reserved -> 0x560                     */
    u32 CSR_ADER0[4]; /* 0x560 -> 0x56C VME CSR Address Decoders     */
    u32 dum570[33] ;  /* 0x570 reserved -> 0x5F4                     */
    u32 CSR_BCR;      /* 0x5F4 Bit Clear                             */
    u32 CSR_BSR;      /* 0x5F8 Bit Set                               */
    u32 CSR_BCA;      /* 0x5FC Base Address                          */
    u32 dum600[96] ;  /* 0x600 reserved -> 0x780                     */
    u32 rom[32];      /* 0x780 ROM sgnature                          */
  } vme;
  struct pev_reg_shm
  {
    u32 dum800[32];   /* 0x800 reserved -> 0x880                     */
    u32 itc_iack;     /* 0x880 Interrupt Acknowledge                 */
    u32 itc_csr;      /* 0x884 Interrupt Control and Status          */
    u32 itc_imc;      /* 0x888 Interrupt Mask Clear                  */
    u32 itc_ims;      /* 0x88C Interrupt Mask Set                    */
    u32 dum890[188];  /* 0x890 reserved -> 0x780                     */
    u32 rom[32];      /* 0xB80 ROM sgnature                          */
  } shm;
  struct pev_reg_usr
  {
    struct pev_reg_gpio
    {
      u32 out;   /* 0x0 GPIO output */
      u32 dir;   /* 0x4 GPIO direction */
      u32 in;    /* 0x8 GPIO intput */
      u32 rsv;   /* 0xC  not used */
    }gpio[2];         /* 0xC00 -> 0xC20 GPIO                         */
    u32 ram[24];      /* 0xC20 -> 0xC80 Test RAM                     */
    u32 itc_iack;     /* 0xC80 Interrupt Acknowledge                 */
    u32 itc_csr;      /* 0xC84 Interrupt Control and Status          */
    u32 itc_imc;      /* 0xC88 Interrupt Mask Clear                  */
    u32 itc_ims;      /* 0xC8C Interrupt Mask Set                    */
    u32 dumC90[188];  /* 0xC90 reserved -> 0x780                     */
    u32 rom[32];      /* 0xF80 ROM sgnature                          */
  } usr;
};




struct pev_dev
{
  struct pci_dev *dev; 
  u32 board;
  u32 fpga;
#ifdef PPC
  u32 pmem_base;
#else
  u64 pmem_base;
#endif
  u32 pmem_len;
  u32 io_base;
  u32 io_len;
  u32 mem_base;
  u32 mem_len;
  u32 csr_base;
  u32 csr_len;
  u32 elb_base;
  u32 elb_len;
  void *csr_ptr;
  void *elb_ptr;
  u32 crate;
  u32 msi;
  u32 irq_pending;
  u32 irq_mask;
  u32 irq_cnt;
  struct pev_irq_handler *irq_tbl;
  struct semaphore sem_remap;
  dma_addr_t dma_baddr;
  void *dma_kaddr;
  u32 dma_status;
  struct pev_ioctl_map_ctl map_mas32;   /*      */
  struct pev_ioctl_map_ctl map_mas64;   /*      */
  struct pev_ioctl_map_ctl map_slave;   /*      */
  struct pev_ioctl_map_ctl map_elb;     /*      */
  struct pci_dev *pex;                  /* pointer to the PEX8624 device structure      */
#ifdef PPC
  u32 pex_base;                         /* PEX8624 PCI MEM base address                 */
#else
  u64 pex_base;                         /* PEX8624 PCI MEM base address                 */
#endif
  u32 pex_len;                          /* PEX8624 PCI MEM window size                  */
  void *pex_ptr;                        /* PEX8624 PCI MEM kernel pointer               */
  struct pci_dev *plx;                  /* pointer to the PLX8112 device structure      */
#ifdef PPC
  u32 plx_base;                         /* PEX8624 PCI MEM base address                 */
#else
  u64 plx_base;                         /* PEX8624 PCI MEM base address                 */
#endif
  u32 plx_len;                          /* PEX8624 PCI MEM window size                  */
  void *plx_ptr;                        /* PEX8624 PCI MEM kernel pointer               */
  u32 shm_len;                          /* size of on board shared memory (SHM)         */
#ifdef PPC
  u32 dma_shm_base;                     /* base address of SHM buffer reserved for DMA  */
#else
  u64 dma_shm_base;                     /* base address of SHM buffer reserved for DMA  */
#endif
  u32 dma_shm_len;                      /* size of SHM buffer reserved for DMA          */
  void *dma_shm_ptr;                    /* pointer to SHM buffer reserved for DMA       */
#ifdef XENOMAI
  rtdm_user_info_t *user_info;
  rtdm_irq_t rtdm_irq;                  /* xenomai irq handle                           */
  xnarch_cpumask_t affinity;
  rtdm_sem_t dma_done;
  rtdm_lock_t dma_lock;
#else
  struct semaphore dma_sem;             /* semaphore to synchronize with DMA interrput  */
  struct semaphore dma_lock;            /* mutex to lock DMA access                     */
#endif
  //struct pev_reg_remap io_remap;
  struct pev_reg_remap io_remap[2];
  int csr_remap;
  struct semaphore vme_sem;             /* semaphore to synchronize with VME interrput  */
  u32 vme_status;
  u32 fpga_status;
  u32 vme_irq_set;
  struct vme_irq_ctl
  {
    uint set;
    uint vector;
    struct semaphore sem;             /* semaphore to synchronize with VME interrput  */
  } vme_irq_ctl[16];
  struct semaphore fifo_sem[8];
  struct semaphore fifo_lock[4];
  struct semaphore i2c_lock;            /* mutex to lock I2C access                     */
  struct semaphore spi_lock;            /* mutex to lock SPI access                     */
};

#define ILOC_STATIC_OFFSET    (pev->io_remap.iloc_base + 0x00)
#define ILOC_CTL_OFFSET       (pev->io_remap.iloc_base + 0x1c)

struct pev_drv
{
  struct cdev cdev;
  dev_t dev_id;
  struct pev_dev *pev[16];
};

struct pev_irq_handler
{
  void (* func)( struct pev_dev*, int, void *);
  void *arg;
};


struct pev_drv *pev_register( void);

#endif /*  _H_PEVDRVR */

/*================================< end file >================================*/
