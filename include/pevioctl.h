/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : pevioctl.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations and definitions needed to perform
 *    ioctl() operation on the PEV1100 device driver.
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
 * $Log: pevioctl.h,v $
 * Revision 1.12  2012/10/01 14:56:49  kalantari
 * added verion 4.20 of tosca-driver from IoxoS
 *
 * Revision 1.41  2012/09/27 09:54:13  ioxos
 * define DMA_STATUS_ERR [JFG]
 *
 * Revision 1.40  2012/09/04 13:18:59  ioxos
 * new function to map system memory statically allocated [JFG]
 *
 * Revision 1.39  2012/08/28 13:53:43  ioxos
 * cleanup + update i2c status + reset [JFG]
 *
 * Revision 1.38  2012/08/27 08:43:53  ioxos
 * support for VME fast single cycles through ELB bus [JFG]
 *
 * Revision 1.37  2012/08/13 15:32:26  ioxos
 * support for timeout while waiting for DMA interrupts [JFG]
 *
 * Revision 1.36  2012/07/10 09:46:26  ioxos
 * rel 4.15 + support for ADNESC [JFG]
 *
 * Revision 1.35  2012/06/28 12:36:13  ioxos
 * support for IRQ from usr1 and usr2 [JFG]
 *
 * Revision 1.34  2012/05/23 08:14:39  ioxos
 * add support for event queues [JFG]
 *
 * Revision 1.33  2012/04/12 13:31:58  ioxos
 * support for dma swapping [JFG]
 *
 * Revision 1.32  2012/03/27 09:17:40  ioxos
 * add support for FIFOs [JFG]
 *
 * Revision 1.31  2012/02/14 16:09:49  ioxos
 * add support DMA byte swapping [JFG]
 *
 * Revision 1.30  2012/02/03 16:29:24  ioxos
 * address space for SHM1,2 and USR1,2 [JFG]
 *
 * Revision 1.29  2012/01/30 11:18:05  ioxos
 * add support for board name [JFG]
 *
 * Revision 1.28  2012/01/27 13:16:06  ioxos
 * prepare release 4.01 supporting x86 & ppc [JFG]
 *
 * Revision 1.27  2012/01/25 15:59:02  ioxos
 * prepare for IFC1210 support [JFG]
 *
 * Revision 1.26  2011/12/06 13:17:04  ioxos
 * support for multi task VME IRQ [JFG]
 *
 * Revision 1.25  2011/10/19 15:13:53  ioxos
 * support for VME irq [JFG]
 *
 * Revision 1.24  2011/03/03 15:42:47  ioxos
 * support for 1MBytes VME slave granularity [JFG]
 *
 * Revision 1.23  2011/01/25 13:40:56  ioxos
 * support for VME RMW [JFG]
 *
 * Revision 1.22  2010/09/14 07:30:59  ioxos
 * add VME_CR space and keep PCI address in map structure [JFG]
 *
 * Revision 1.21  2010/01/12 15:43:49  ioxos
 * cleanup [JFG]
 *
 * Revision 1.20  2010/01/08 11:21:57  ioxos
 * define for auto swap [JFG]
 *
 * Revision 1.19  2010/01/04 13:27:26  ioxos
 * add define for swapping and timer control [JFG]
 *
 * Revision 1.18  2009/12/15 17:14:39  ioxos
 * modification for short io window [JFG]
 *
 * Revision 1.17  2009/11/10 09:08:26  ioxos
 * add support for PCIe traffic class and outstanding read [JFG]
 *
 * Revision 1.16  2009/09/29 12:44:06  ioxos
 * support to read/write sflash status [JFG]
 *
 * Revision 1.15  2009/08/17 09:35:54  ioxos
 * add command to access SHM buffer reserved for DMA [JFG]
 *
 * Revision 1.14  2009/06/03 12:38:01  ioxos
 * use buf_alloc instead of dma_alloc [JFG]
 *
 * Revision 1.13  2009/06/02 11:46:32  ioxos
 * add driver identification [jfg]
 *
 * Revision 1.12  2009/05/20 09:12:02  ioxos
 * add DE transfer mode DMA_MODE_PIPE/DMA_MODE_BLOCK [JFG]
 *
 * Revision 1.11  2009/04/06 12:24:05  ioxos
 * define command for histogram [JFG]
 *
 * Revision 1.10  2009/01/27 14:45:11  ioxos
 * define all VME transfer mode [JFG]
 *
 * Revision 1.9  2009/01/09 13:11:29  ioxos
 * add support for DMA status [JFG]
 *
 * Revision 1.8  2009/01/06 14:44:39  ioxos
 * re-define pev_rdwr_mode [JFG]
 *
 * Revision 1.7  2009/01/06 13:29:31  ioxos
 * cleanup [JFG]
 *
 * Revision 1.6  2008/12/12 13:07:46  jfg
 * add define for MAP and DMA + modif dma data structures [JFG]
 *
 * Revision 1.5  2008/11/12 09:02:33  ioxos
 * add ioctl commands for timer, mapping and bit setting, declare data structures for dma and timer [JFG]
 *
 * Revision 1.4  2008/09/17 12:49:02  ioxos
 * add control structures and command for VME config and CRCSR [JFG]
 *
 * Revision 1.3  2008/08/08 09:00:49  ioxos
 * allow access to PEX86xx and PLX8112 registers and DMA buffers [JFG]
 *
 * Revision 1.2  2008/07/04 07:40:59  ioxos
 * update address mapping functions [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_PEVIOCTL
#define _H_PEVIOCTL

typedef unsigned char uchar;

#define PEV_SCSR_ILOC_BASE  0x00
#define PEV_SCSR_ILOC_SIGN  0x04
#define PEV_SCSR_ILOC_CTL   0x08
#define PEV_SCSR_PCIE_MMU   0x0c
#define PEV_SCSR_ILOC_SMON  0x14
#define PEV_SCSR_VME_BASE   0x20
#define PEV_SCSR_SHM_BASE   0x40
#define PEV_SCSR_DMA_RD     0x50
#define PEV_SCSR_DMA_WR     0x60
#define PEV_SCSR_VME_TIMER   0x70
#define PEV_SCSR_ITC        0x80
#define PEV_SCSR_ITC_ILOC   PEV_SCSR_ITC
#define PEV_SCSR_ITC_VME    0x90
#define PEV_SCSR_ITC_DMA    0xa0
#define PEV_SCSR_ITC_USR    0xb0
#define PEV_SCSR_ITC_USR1   0xb0
#define PEV_SCSR_ITC_USR2   0xb0
#define PEV_SCSR_VME_ADER   0xc0
#define PEV_SCSR_VME_CSR    0xd0
#define PEV_SCSR_VME_ELB    0xf0
#define PEV_SCSR_ILOC_SPI   0xd0
#define PEV_SCSR_ILOC_I2C   0xf0
#define PEV_SCSR_USR_FIFO   0xe0
#define PEV_SCSR_ITC_SHIFT  8
#define PEV_SCSR_ITC_MASK   0x30
#define PEV_SCSR_SEL_ILOC   0x00000000
#define PEV_SCSR_SEL_VME    0x01000000
#define PEV_SCSR_SEL_DMA    0x02000000
#define PEV_SCSR_SEL_USR    0x03000000

#define PEV_CSR_ILOC_BASE 0x00
#define PEV_CSR_ILOC_SPI  0x10
#define PEV_CSR_ILOC_SIGN 0x18
#define PEV_CSR_ILOC_CTL  0x1c
#define PEV_CSR_PCIE_MMU  0x20
#define PEV_CSR_ILOC_SMON 0x40
#define PEV_CSR_ILOC_I2C  0x100
#define PEV_CSR_VME_BASE  0x400
#define PEV_CSR_VME_TIMER 0x440
#define PEV_CSR_SHM_BASE  0x840
#define PEV_CSR_DMA_RD    0x900
#define PEV_CSR_DMA_WR    0xa00
#define PEV_CSR_USR_BASE  0xc00
#define PEV_CSR_USR_FIFO  0xc00
#define PEV_CSR_ITC       0x80
#define PEV_CSR_ITC_ILOC  0x80
#define PEV_CSR_ITC_VME   0x480
#define PEV_CSR_ITC_DMA   0x880
#define PEV_CSR_ITC_USR   0xc80
#define PEV_CSR_ITC_USR1  0x1080
#define PEV_CSR_ITC_USR2  0x1480
#define PEV_CSR_VME_ADER  0x560
#define PEV_CSR_VME_CSR   0x5f0
#define PEV_CSR_VME_ELB   0x438
#define PEV_CSR_ITC_SHIFT 2
#define PEV_CSR_ITC_MASK  0x1c00

#define PEV_CSR_ELB_BASE  0xffb00000
#define PEV_CSR_ELB_SIZE  0x10000
#define PEV_VME_ELB_BASE  0xc0000000
#define PEV_VME_ELB_SIZE  0x10000000

struct pev_reg_remap
{
  uint short_io;
  uint iloc_base;
  uint iloc_spi;
  uint iloc_sign;
  uint iloc_ctl;
  uint iloc_itc;
  uint pcie_mmu;
  uint iloc_smon;
  uint iloc_i2c;
  uint vme_base;
  uint vme_mmu;
  uint vme_timer;
  uint vme_ader;
  uint vme_csr;
  uint vme_itc;
  uint vme_elb;
  uint shm_base;
  uint dma_rd;
  uint dma_wr;
  uint dma_itc;
  uint usr_itc;
  uint usr_fifo;
  uint usr1_itc;
  uint usr2_itc;
};

#define PEV_BOARD_PEV1100 0x73571100
#define PEV_BOARD_IPV1102 0x73571102
#define PEV_BOARD_VCC1104 0x73571104
#define PEV_BOARD_VCC1105 0x73571105
#define PEV_BOARD_IFC1210 0x73571210
#define PEV_BOARD_MPC1200 0x73571200
#define PEV_BOARD_ADN4001 0x73574001

#define PEV_IOCTL_OP_MASK    0x0FFF0000

#define PEV_IOCTL_ID         0x00000100
#define PEV_IOCTL_VERSION    0x00000101
#define PEV_IOCTL_BOARD      0x00000102

#define PEV_IOCTL_RW         0x00010000
#define PEV_IOCTL_RD_IO_8    ( PEV_IOCTL_RW | 0x0111)
#define PEV_IOCTL_RD_IO_16   ( PEV_IOCTL_RW | 0x0112)
#define PEV_IOCTL_RD_IO_32   ( PEV_IOCTL_RW | 0x0114)
#define PEV_IOCTL_RD_CSR_8   ( PEV_IOCTL_RW | 0x0151)
#define PEV_IOCTL_RD_CSR_16  ( PEV_IOCTL_RW | 0x0152)
#define PEV_IOCTL_RD_CSR_32  ( PEV_IOCTL_RW | 0x0154)
#define PEV_IOCTL_RD_PMEM_8  ( PEV_IOCTL_RW | 0x0121)
#define PEV_IOCTL_RD_PMEM_16 ( PEV_IOCTL_RW | 0x0122)
#define PEV_IOCTL_RD_PMEM_32 ( PEV_IOCTL_RW | 0x0124)
#define PEV_IOCTL_RD_MEM_8   ( PEV_IOCTL_RW | 0x0131)
#define PEV_IOCTL_RD_MEM_16  ( PEV_IOCTL_RW | 0x0132)
#define PEV_IOCTL_RD_MEM_32  ( PEV_IOCTL_RW | 0x0134)
#define PEV_IOCTL_RD_CFG_8   ( PEV_IOCTL_RW | 0x0141)
#define PEV_IOCTL_RD_CFG_16  ( PEV_IOCTL_RW | 0x0142)
#define PEV_IOCTL_RD_CFG_32  ( PEV_IOCTL_RW | 0x0144)
#define PEV_IOCTL_WR_IO_8    ( PEV_IOCTL_RW | 0x0211)
#define PEV_IOCTL_WR_IO_16   ( PEV_IOCTL_RW | 0x0212)
#define PEV_IOCTL_WR_IO_32   ( PEV_IOCTL_RW | 0x0214)
#define PEV_IOCTL_WR_CSR_8   ( PEV_IOCTL_RW | 0x0251)
#define PEV_IOCTL_WR_CSR_16  ( PEV_IOCTL_RW | 0x0252)
#define PEV_IOCTL_WR_CSR_32  ( PEV_IOCTL_RW | 0x0254)
#define PEV_IOCTL_WR_PMEM_8  ( PEV_IOCTL_RW | 0x0221)
#define PEV_IOCTL_WR_PMEM_16 ( PEV_IOCTL_RW | 0x0222)
#define PEV_IOCTL_WR_PMEM_32 ( PEV_IOCTL_RW | 0x0224)
#define PEV_IOCTL_WR_MEM_8   ( PEV_IOCTL_RW | 0x0231)
#define PEV_IOCTL_WR_MEM_16  ( PEV_IOCTL_RW | 0x0232)
#define PEV_IOCTL_WR_MEM_32  ( PEV_IOCTL_RW | 0x0234)
#define PEV_IOCTL_WR_CFG_8       ( PEV_IOCTL_RW | 0x0241)
#define PEV_IOCTL_WR_CFG_16      ( PEV_IOCTL_RW | 0x0242)
#define PEV_IOCTL_WR_CFG_32      ( PEV_IOCTL_RW | 0x0244)
#define PEV_IOCTL_SET_IO_8       ( PEV_IOCTL_RW | 0x0311)
#define PEV_IOCTL_SET_IO_16      ( PEV_IOCTL_RW | 0x0312)
#define PEV_IOCTL_SET_IO_32      ( PEV_IOCTL_RW | 0x0314)
#define PEV_IOCTL_SET_CSR_8      ( PEV_IOCTL_RW | 0x0351)
#define PEV_IOCTL_SET_CSR_16     ( PEV_IOCTL_RW | 0x0352)
#define PEV_IOCTL_SET_CSR_32     ( PEV_IOCTL_RW | 0x0354)
#define PEV_IOCTL_SET_PMEM_8     ( PEV_IOCTL_RW | 0x0321)
#define PEV_IOCTL_SET_PMEM_16    ( PEV_IOCTL_RW | 0x0322)
#define PEV_IOCTL_SET_PMEM_32    ( PEV_IOCTL_RW | 0x0324)
#define PEV_IOCTL_SET_MEM_8      ( PEV_IOCTL_RW | 0x0331)
#define PEV_IOCTL_SET_MEM_16     ( PEV_IOCTL_RW | 0x0332)
#define PEV_IOCTL_SET_MEM_32     ( PEV_IOCTL_RW | 0x0334)
#define PEV_IOCTL_SET_CFG_8      ( PEV_IOCTL_RW | 0x0341)
#define PEV_IOCTL_SET_CFG_16     ( PEV_IOCTL_RW | 0x0342)
#define PEV_IOCTL_SET_CFG_32     ( PEV_IOCTL_RW | 0x0344)

#define PEV_IOCTL_REG            0x00020000
#define PEV_IOCTL_RD_REG_8       ( PEV_IOCTL_REG | 0x0151)
#define PEV_IOCTL_RD_REG_16      ( PEV_IOCTL_REG | 0x0152)
#define PEV_IOCTL_RD_REG_32      ( PEV_IOCTL_REG | 0x0154)
#define PEV_IOCTL_WR_REG_8       ( PEV_IOCTL_REG | 0x0251)
#define PEV_IOCTL_WR_REG_16      ( PEV_IOCTL_REG | 0x0252)
#define PEV_IOCTL_WR_REG_32      ( PEV_IOCTL_REG | 0x0254)

#define PEV_IOCTL_RDWR            0x00030000

#define PEV_IOCTL_EVT             0x00100000
#define PEV_IOCTL_EVT_ALLOC       0x00100001
#define PEV_IOCTL_EVT_FREE        0x00100002
#define PEV_IOCTL_EVT_REGISTER    0x00100003
#define PEV_IOCTL_EVT_UNREGISTER  0x00100004
#define PEV_IOCTL_EVT_READ        0x00100005
#define PEV_IOCTL_EVT_WAIT        0x00100006
#define PEV_IOCTL_EVT_RESET       0x00100007
#define PEV_IOCTL_EVT_ENABLE      0x00100008
#define PEV_IOCTL_EVT_DISABLE     0x00100009
#define PEV_IOCTL_EVT_UNMASK      0x0010000a
#define PEV_IOCTL_EVT_MASK        0x0010000b

#define PEV_IOCTL_DMA             0x00040000
#define PEV_IOCTL_DMA_MOVE        0x00040001
#define PEV_IOCTL_DMA_STATUS      0x00040002
#define PEV_IOCTL_DMA_WAIT        0x00040003
#define PEV_IOCTL_DMA_KILL        0x00040004

#define PEV_IOCTL_SFLASH          0x00050000
#define PEV_IOCTL_SFLASH_ID       0x00050000
#define PEV_IOCTL_SFLASH1_ID      0x00050001
#define PEV_IOCTL_SFLASH2_ID      0x00050002
#define PEV_IOCTL_SFLASH3_ID      0x00050003
#define PEV_IOCTL_SFLASH_RDSR     0x00050010
#define PEV_IOCTL_SFLASH1_RDSR    0x00050011
#define PEV_IOCTL_SFLASH2_RDSR    0x00050012
#define PEV_IOCTL_SFLASH3_RDSR    0x00050013
#define PEV_IOCTL_SFLASH_WRSR     0x00050020
#define PEV_IOCTL_SFLASH1_WRSR    0x00050021
#define PEV_IOCTL_SFLASH2_WRSR    0x00050022
#define PEV_IOCTL_SFLASH3_WRSR    0x00050023
#define PEV_IOCTL_SFLASH_RD       0x00050100
#define PEV_IOCTL_SFLASH_WR       0x00050200

#define PEV_IOCTL_MAP             0x00060000
#define PEV_IOCTL_MAP_MASK        0xFFFFFFF0
#define PEV_IOCTL_MAP_PG          0x00060010
#define PEV_IOCTL_MAP_ALLOC       0x00060011
#define PEV_IOCTL_MAP_MODIFY      0x00060012
#define PEV_IOCTL_MAP_FREE        0x00060013
#define PEV_IOCTL_MAP_FIND        0x00060014
#define PEV_IOCTL_MAP_CTL         0x00060020
#define PEV_IOCTL_MAP_READ        0x00060021
#define PEV_IOCTL_MAP_CLEAR       0x00060022

#define PEV_IOCTL_I2C             0x00070000
#define PEV_IOCTL_I2C_PEX_RD      0x00070101
#define PEV_IOCTL_I2C_PEX_WR      0x00070102
#define PEV_IOCTL_I2C_DEV_RD      0x00070103
#define PEV_IOCTL_I2C_DEV_WR      0x00070104
#define PEV_IOCTL_I2C_DEV_CMD     0x00070105
#define PEV_IOCTL_I2C_DEV_RST     0x00070106

#define PEV_IOCTL_VME             0x00080000
#define PEV_IOCTL_VME_CONF_RD     0x00080001
#define PEV_IOCTL_VME_CONF_WR     0x00080002
#define PEV_IOCTL_VME_CRCSR       0x00080003
#define PEV_IOCTL_VME_RMW         0x00080004
#define PEV_IOCTL_VME_LOCK        0x00080005
#define PEV_IOCTL_VME_UNLOCK      0x00080006
#define PEV_IOCTL_VME_SLV_INIT    0x00080007
#define PEV_IOCTL_VME_IRQ_INIT    0x00080008
#define PEV_IOCTL_VME_IRQ_ALLOC   0x00080010
#define PEV_IOCTL_VME_IRQ_ARM     0x00080020
#define PEV_IOCTL_VME_IRQ_WAIT    0x00080040
#define PEV_IOCTL_VME_IRQ_CLEAR   0x00080080

#define PEV_IOCTL_BUF             0x00090000
#define PEV_IOCTL_BUF_ALLOC       0x00090011
#define PEV_IOCTL_BUF_FREE        0x00090012
#define PEV_IOCTL_BUF_MAP         0x00090013
#define PEV_IOCTL_BUF_UNMAP       0x00090014

#define PEV_IOCTL_EEPROM          0x000a0000
#define PEV_IOCTL_EEPROM_RD       0x000a0100
#define PEV_IOCTL_EEPROM_WR       0x000a0200

#define PEV_IOCTL_FPGA            0x000b0000
#define PEV_IOCTL_FPGA_LOAD       0x000b0001

#define PEV_IOCTL_FIFO            0x000c0000
#define PEV_IOCTL_FIFO_INIT       0x000c0001
#define PEV_IOCTL_FIFO_RD         0x000c0002
#define PEV_IOCTL_FIFO_WR         0x000c0303
#define PEV_IOCTL_FIFO_CLEAR      0x000c0004
#define PEV_IOCTL_FIFO_WAIT_EF    0x000c0005
#define PEV_IOCTL_FIFO_WAIT_FF    0x000c0006
#define PEV_IOCTL_FIFO_STATUS     0x000c0007

#define PEV_IOCTL_MFCC            0x00200000
#define PEV_IOCTL_MFCC_FIFO       0x00200001

#define PEV_IOCTL_TIMER           0x00900000
#define PEV_IOCTL_TIMER_RESTART   0x00900001
#define PEV_IOCTL_TIMER_STOP      0x00900002
#define PEV_IOCTL_TIMER_START     0x00900003
#define PEV_IOCTL_TIMER_READ      0x00900004
#define PEV_IOCTL_TIMER_IRQ_ENA   0x00900005
#define PEV_IOCTL_TIMER_IRQ_DIS   0x00900006

#define PEV_IOCTL_HISTO           0x00a00000
#define PEV_IOCTL_HISTO_READ      0x00a00001
#define PEV_IOCTL_HISTO_CLEAR     0x00a00008
#define PEV_IOCTL_HISTO_RESET     0x00a0000f

#define PEV_IOCTL_IO_REMAP        0x00f00000

#define PEV_IOCTL_RST             0x00800000
#define PEV_IOCTL_START           0x00800001
#define PEV_IOCTL_STOP            0x00800002

struct pev_ioctl_rw
{
  uint offset;
  uint data;
};

struct pev_ioctl_sflash_rw
{
  void *buf;
  uint offset;
  uint len;
  uint dev;
};

struct pev_ioctl_rw_reg
{
  uint addr_off;
  uint data_off;
  uint reg_idx;
  uint reg_data;
};


/* define for data size                                              */
#define RDWR_BYTE       1       /* 8 bit transfer                    */
#define RDWR_SHORT      2       /* 16 bit transfer                   */
#define RDWR_INT        4       /* 32 bit transfer                   */
#define RDWR_LONG       8       /* 64 bit transfer                   */    

/* define for swapping mode                                          */
#define RDWR_NOSWAP     0       /* don't perform byte swapping       */
#define RDWR_SWAP       1       /* 64 bit transfer                   */

/* define for transfer direction                                     */
#define RDWR_READ       0       /* from remote space to local buffer */
#define RDWR_WRITE      1       /* from local buffer to remote space */
#define RDWR_LOOP_READ  4
#define RDWR_LOOP_WRITE 5
#define RDWR_LOOP_RDWR  7
#define RDWR_LOOP_CHECK 6

/* define for remote space identifier                        */
#define RDWR_CFG         0x01       /* FPGA PCI configuration registers */
#define RDWR_PCIE_CFG    0x01       /* FPGA PCI configuration registers */
#define RDWR_PCIE_IO     0x02       /* FPGA PCI IO window */
#define RDWR_PCIE_BAR4   0x02       /* FPGA PCI IO window */
#define RDWR_IO          0x02       /* FPGA PCI IO window */
#define RDWR_PCIE_PMEM   0x03       /* FPGA PCI PMEM window */
#define RDWR_PCIE_BAR0   0x03       /* FPGA PCI PMEM window */
#define RDWR_PMEM        0x03       /* FPGA PCI pMEM window */
#define RDWR_PCIE_MEM    0x04       /* FPGA PCI MEM window */
#define RDWR_PCIE_BAR2   0x04       /* FPGA PCI MEM window */
#define RDWR_MEM         0x04       /* FPGA PCI MEM window */
#define RDWR_DMA_SHM     0x05       /* SHM reserved for DMA operations */          
#define RDWR_USR         0x06          
#define RDWR_PCIE_BAR3   0x07       /* FPGA PCI BAR3 window (8k in MEM space for CSR) */
#define RDWR_CSR         0x07       /* FPGA PCI BAR3 window (8k in MEM space for CSR) */
#define RDWR_PEX         0x14
#define RDWR_PEX_MEM     0x14       /* PEX8624 PCI MEM window */
#define RDWR_ELB         0x18       /* P2020 ELB bus */
#define RDWR_KMEM        0x20


struct pev_ioctl_rdwr
{
  void *buf;            /* data buffer pointer in user space */
  uint offset;          /* address offset in remote space    */
  uint len;             /* data transfer size in byte        */
  struct pev_rdwr_mode
  {
    char ds;            /* data size                         */
    char swap;          /* swap mode                         */
    char as;            /* address size                      */
    char dir;           /* transfer direction                */
    char crate;         /* remote crate identifier           */
    char am;            /* address modifier                  */
    char space;         /* remote space identifier           */
    char rsv3;
  } mode;
  void *k_addr;
};

#define MAP_INVALID       0
#define MAP_MASTER_32     1
#define MAP_PCIE_MEM      1
#define MAP_MASTER_64     2
#define MAP_PCIE_PMEM     2
#define MAP_SLAVE_VME     4
#define MAP_VME_SLAVE     4
#define MAP_VME_ELB       8

#define MAP_SPACE_PCIE    0x0000
#define MAP_SPACE_VME     0x1000
#define MAP_SPACE_SHM     0x2000
#define MAP_SPACE_SHM1    0x2000
#define MAP_SPACE_SHM2    0x3000
#define MAP_SPACE_USR     0x3000
#define MAP_SPACE_USR1    0x4000
#define MAP_SPACE_USR2    0x5000

#define MAP_ENABLE        0x0001
#define MAP_ENABLE_WR     0x0002

#define MAP_SWAP_NO       0x0000
#define MAP_SWAP_AUTO     0x0040

#define MAP_VME_SP        0x0020
#define MAP_VME_CR        0x0000
#define MAP_VME_A16       0x0100
#define MAP_VME_A24       0x0200
#define MAP_VME_A32       0x0300
#define MAP_VME_BLT       0x0400
#define MAP_VME_MBLT      0x0500
#define MAP_VME_IACK      0x0f00

#define MAP_PCIE_TC0      0x0000
#define MAP_PCIE_TC1      0x0004
#define MAP_PCIE_TC2      0x0008
#define MAP_PCIE_TC3      0x000c
#define MAP_PCIE_TC4      0x0010
#define MAP_PCIE_TC5      0x0014
#define MAP_PCIE_TC6      0x0018
#define MAP_PCIE_TC7      0x001c
#define MAP_PCIE_SNOOP    0x0400
#define MAP_PCIE_RLXO     0x0800

#define MAP_FLAG_FREE           0
#define MAP_FLAG_BUSY           1
#define MAP_FLAG_PRIVATE        2
#define MAP_MODE_MASK           0xffff

struct pev_map_blk
{
  char flag; char usr; short npg;
  uint mode;
  ulong rem_addr;
};

 
struct pev_ioctl_map_pg
{
  uint size;                            /* mapping size required by user            */
  char flag; char sg_id; ushort mode;   /* mapping mode                             */
  ulong rem_addr;                       /* remote address to be mapped              */
  ulong loc_addr;                       /* local address returned by mapper         */
  uint offset;                          /* offset of page containing local address  */
  uint win_size;                        /* size actually mapped                     */
  ulong rem_base;                       /* remote address of window actually mapped */
  ulong loc_base;                       /* local address of window actually mapped  */
  void *usr_addr;                       /* user address pointing to local address   */
  ulong pci_base;                       /* pci base address of SG window            */
};


struct pev_ioctl_map_ctl
{
  struct pev_map_blk *map_p;
  char rsv; char sg_id; short pg_num;
  int pg_size;
  ulong loc_base;
};


#define I2C_DEV_PEX    0x0
#define I2C_DEV_LM86_1 0x2
#define I2C_DEV_LM86_2 0x3
#define I2C_DEV_XMC_1  0x4
#define I2C_DEV_XMC_2  0x5
#define I2C_CTL_ERR  0x100000
#define I2C_CTL_DONE   0x200000

struct pev_i2c_devices
{
  char *name;
  uint id;
};

struct pev_ioctl_i2c
{
  uint device;
  uint cmd;
  uint data;
  uint status;
};

struct pev_ioctl_vme_conf
{
  uint a24_base;
  uint a24_size;
  uint a32_base;
  uint a32_size;
  char x64; char slot1; char sysrst; char rto;
  char arb; char bto; char req; char level;
  char mas_ena; char slv_ena; char slv_retry; char burst;
};

#define VME_CRCSR_GET   0x1
#define VME_CRCSR_SET   0x2
#define VME_CRCSR_CLEAR 0x4
#define VME_SLV_ENA     0x1
#define VME_SLV_1MB     0x8

#define VME_IRQ_STARTED          0x01
#define VME_IRQ_WAITING          0x02
#define VME_IRQ_RECEIVED         0x04
#define VME_IRQ_ENDED            0x10
#define VME_IRQ_TMO              0x80

#define VME_IRQ_SYSFAIL          (1<<0)
#define VME_IRQ_1                (1<<1)
#define VME_IRQ_2                (1<<2)
#define VME_IRQ_3                (1<<3)
#define VME_IRQ_4                (1<<4)
#define VME_IRQ_5                (1<<5)
#define VME_IRQ_6                (1<<6)
#define VME_IRQ_7                (1<<7)
#define VME_IRQ_ACFAIL           (1<<8)
#define VME_IRQ_ERROR            (1<<9)
#define VME_IRQ_TIMER_TIC        (1<<10)
#define VME_IRQ_TIMER_ERROR      (1<<11)
#define VME_IRQ_LOCMON_0         (1<<12)
#define VME_IRQ_LOCMON_1         (1<<13)
#define VME_IRQ_LOCMON_2         (1<<14)
#define VME_IRQ_LOCMON_3         (1<<15)

struct pev_ioctl_vme_crcsr
{
  uint get;
  uint set;
  uint clear;
  uint operation;
};

struct pev_ioctl_vme_rmw
{
  uint status;
  uint addr;
  uint cmp;
  uint up;
  uint ds;
  uint am;
};

struct pev_ioctl_vme_lock
{
  uint status;
  uint addr;
  uint mode;
};

struct pev_ioctl_vme_irq
{
  uint status;
  uint irq;
  uint vector;
  uint tmo;
  uint umask;
};

struct pev_ioctl_vme_map
{
  uint status;
  uint addr;
  uint size;
  uint mode;
};

struct pev_ioctl_dma
{
  uint status;
  uint size;
  void *b_addr;
  void *u_addr;
  void *k_addr;
};

struct pev_ioctl_buf
{
  int kmem_fd;
  uint size;
  void *b_addr;
  void *u_addr;
  void *k_addr;
};

#define DMA_SPACE_PCIE    0x00
#define DMA_PCIE_BADDR    0x00
#define DMA_PCIE_KADDR    0x10
#define DMA_PCIE_UADDR    0x20
#define DMA_PCIE_BUF      0x08
#define DMA_SPACE_VME     0x01
#define DMA_SPACE_SHM     0x02
#define DMA_SPACE_SHM1    0x02
#define DMA_SPACE_SHM2    0x03
#define DMA_SPACE_USR     0x03
#define DMA_SPACE_USR1    0x04
#define DMA_SPACE_USR2    0x05
#define DMA_SPACE_MASK    0x07
#define DMA_VME_A16       0x10
#define DMA_VME_A24       0x20
#define DMA_VME_A32       0x30
#define DMA_VME_SGL       0x30
#define DMA_VME_BLT       0x40
#define DMA_VME_MBLT      0x50
#define DMA_VME_2eVME     0x60
#define DMA_VME_2eFAST    0x70
#define DMA_VME_2e160     0x80
#define DMA_VME_2e233     0x90
#define DMA_VME_2e320     0xa0
#define DMA_INTR_ENA      0x01
#define DMA_WAIT_INTR     0x01
#define DMA_WAIT_1MS      0x02
#define DMA_WAIT_10MS     0x04
#define DMA_WAIT_100MS    0x06
#define DMA_WAIT_1S       0x08
#define DMA_WAIT_10S      0x0a
#define DMA_WAIT_100S     0x0c
#define DMA_MODE_BLOCK    0x00  /* move one block from src to des */
#define DMA_MODE_PIPE     0x01
#define DMA_MODE_LIST_RD  0x02  /* source is a chain descriptor      */
#define DMA_MODE_LIST_WR  0x03  /* destination is a chain descriptor */
#define DMA_START_BLOCK   0x00  /* move one block from src to des */
#define DMA_START_PIPE    0x01
#define DMA_SIZE_MAX_BLK  0xff800  
#define DMA_SIZE_MAX_LIST 0x3f  
#define DMA_SIZE_PKT_128  0x00000000 
#define DMA_SIZE_PKT_256  0x40000000 
#define DMA_SIZE_PKT_512  0x80000000 
#define DMA_SIZE_PKT_1K   0xc0000000 

#define DMA_PCIE_TC0      0x00  /* Traffic Class 0 */
#define DMA_PCIE_TC1      0x01  /* Traffic Class 1 */
#define DMA_PCIE_TC2      0x02  /* Traffic Class 2 */
#define DMA_PCIE_TC3      0x03  /* Traffic Class 3 */
#define DMA_PCIE_TC4      0x04  /* Traffic Class 4 */
#define DMA_PCIE_TC5      0x05  /* Traffic Class 5 */
#define DMA_PCIE_TC6      0x06  /* Traffic Class 6 */
#define DMA_PCIE_TC7      0x07  /* Traffic Class 7 */
#define DMA_PCIE_RR1      0x00  /* 1 outstanding read request */
#define DMA_PCIE_RR2      0x10  /* 2 outstanding read request */
#define DMA_PCIE_RR3      0x20  /* 3 outstanding read request */

#define DMA_SWAP          0x40  /* automatic byte swapping */  
#define DMA_VME_SWAP      0x40  /* automatic VME byte swapping */  
#define DMA_SPACE_WS      0x10
#define DMA_SPACE_DS      0x20
#define DMA_SPACE_QS      0x30

#define DMA_STATUS_RUN_RD0             0x01
#define DMA_STATUS_RUN_RD1             0x02
#define DMA_STATUS_RUN_WR0             0x04
#define DMA_STATUS_RUN_WR1             0x08
#define DMA_STATUS_DONE                0x10
#define DMA_STATUS_WAITING             0x000
#define DMA_STATUS_ENDED               0x100
#define DMA_STATUS_TMO                 0x80
#define DMA_STATUS_ERR                 0x40

struct pev_ioctl_dma_req
{
  ulong src_addr;
  ulong des_addr;
  uint size;
  uchar src_space; uchar src_mode; uchar des_space; uchar des_mode;
  uchar start_mode; uchar end_mode; uchar intr_mode; uchar wait_mode;
  uint dma_status;
};
struct pev_ioctl_dma_list
{
  ulong addr;
  uint size;
  uint mode;
};

struct pev_ioctl_dma_desc
{
  uint csr;
  uint cnt;
  uint shm_p;
  uint desc_p;
  uint addr_lp;
  uint addr_hp;
  uint usec;
  uint msec;
};

struct pev_ioctl_dma_sts
{
  uint rd_csr;
  uint rd_ndes;
  uint rd_cdes;
  uint rd_cnt;
  uint wr_csr;
  uint wr_ndes;
  uint wr_cdes;
  uint wr_cnt;
  struct pev_ioctl_dma_desc start;
  struct pev_ioctl_dma_desc wr;
  struct pev_ioctl_dma_desc rd;
};

#define TIMER_ENA          0x80000000  /* timer global enable                    */
#define TIMER_1MHZ         0x00000000  /* timer frequency 1 MHz                  */
#define TIMER_5MHZ         0x00000001  /* timer frequency 5 MHz                  */
#define TIMER_25MHZ        0x00000002  /* timer frequency 25 MHz                 */
#define TIMER_100MHZ       0x00000003  /* timer frequency 100 MHz                */
#define TIMER_BASE_1000    0x00000000  /* timer period 1000 usec                 */
#define TIMER_BASE_1024    0x00000008  /* timer period 1024 usec                 */
#define TIMER_SYNC_LOC     0x00000000  /* timer synchronization local            */
#define TIMER_SYNC_USR1    0x00000010  /* timer synchronization user signal #1   */
#define TIMER_SYNC_USR2    0x00000020  /* timer synchronization user signal #2   */
#define TIMER_SYNC_SYSFAIL 0x00000040  /* timer synchronization VME sysfail      */
#define TIMER_SYNC_IRQ1    0x00000050  /* timer synchronization VME IRQ#1        */
#define TIMER_SYNC_IRQ2    0x00000060  /* timer synchronization VME IRQ#2        */
#define TIMER_SYNC_ENA     0x00000080  /* timer synchronization enable           */
#define TIMER_OUT_SYSFAIL  0x00000100  /* issue sync signal on VME sysfail       */
#define TIMER_OUT_IRQ1     0x00000200  /* issue sync signal on VME IRQ#1         */
#define TIMER_OUT_IRQ2     0x00000300  /* issue sync signal on VME IRQ#2         */
#define TIMER_SYNC_ERR     0x00010000  /* timer synchronization error           */

struct pev_ioctl_timer
{
  uint operation;   /* operation to perform   */
  uint time;        /* tick counter (msec)    */
  uint utime;       /* usec counter           */
  uint mode;        /* operating mode         */
};

struct pev_ioctl_fifo
{
  uint idx;
  uint sts;
  uint *data;
  uint cnt;
  uint tmo;
};

struct pev_ioctl_intr
{
  long cnt;
};

struct pev_ioctl_histo
{
  int *histo;
  int idx;
  int size;
};


#define EVT_SRC_LOC     0x00  /* interrupt controler for local events          */
#define EVT_SRC_VME     0x10  /* interrupt controler for VME events          */
#define EVT_SRC_DMA     0x20  /* interrupt controler for DMA events          */
#define EVT_SRC_USR     0x30  /* interrupt controler for USR events          */
#define EVT_SRC_USR1    0x40  /* interrupt controler for USR1 events          */
#define EVT_SRC_USR2    0x50  /* interrupt controler for USR2 events          */

struct pev_ioctl_evt
{
  void *evt_queue;
  int src_id;
  int vec_id;
  int evt_cnt;
  int sig;
  int wait;
};

struct pev_ioctl_evt_xx
{
  uint offset;
  uint data;
};

#endif /*  _H_PEVIOCTL */
