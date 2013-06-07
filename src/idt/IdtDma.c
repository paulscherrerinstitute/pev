/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : idt.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : october 10,2012
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
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
 * $Log: IdtDma.c,v $
 * Revision 1.1  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.4  2013/02/21 11:12:10  ioxos
 * cleanup [JFG]
 *
 * Revision 1.3  2013/02/19 11:16:12  ioxos
 * set src=USR1 and des=SYSMEM [JFG]
 *
 * Revision 1.2  2013/02/14 15:17:12  ioxos
 * tagging 4.26 [JFG]
 *
 * Revision 1.1  2013/02/05 10:49:55  ioxos
 * first checkin [JFG]
 *
 * Revision 1.1  2012/10/12 12:55:55  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#include <stdlib.h>
#include <stdio.h>
#include <pty.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <idtioctl.h>
#include <pevioctl.h>
#include <pevulib.h>
#include <idtulib.h>

struct idt_ioctl_dma_req dma_req;
struct idt_ioctl_mbx mbx;
char dev_name[32];
struct pev_ioctl_buf dma_buf;
int *kbuf_loc_addr;
struct pev_node *pev;
struct pev_ioctl_vme_conf vme_conf;
struct pev_ioctl_map_pg vme_slv_map;

#define SHM_OFFSET 0x100000

#define DMA_TARGET       0x1

//#define DMA_SRC_ADDR     0x0
//#define DMA_SRC_SPACE    DMA_SPACE_SHM
#define DMA_SRC_ADDR     0x0
#define DMA_SRC_SPACE    DMA_SPACE_USR1
//#define DMA_SRC_ADDR     vme_addr
//#define DMA_SRC_SPACE    (DMA_SPACE_VME | DMA_VME_2e320)
//#define DMA_SRC_ADDR     (ulong)dma_buf.b_addr
//#define DMA_SRC_SPACE    DMA_SPACE_PCIE

//#define DMA_DES_ADDR     0x0
//#define DMA_DES_SPACE    DMA_SPACE_SHM
//#define DMA_DES_ADDR     0x0
//#define DMA_DES_SPACE    DMA_SPACE_USR1
//#define DMA_DES_ADDR     vme_addr
//#define DMA_DES_SPACE    (DMA_SPACE_VME | DMA_VME_2e320)
#define DMA_DES_ADDR     (ulong)dma_buf.b_addr
#define DMA_DES_SPACE    DMA_SPACE_PCIE

#define DMA_SIZE         (0x60000 | DMA_SIZE_PKT_1K)                 
//#define DMA_START_MODE   DMA_MODE_BLOCK
//#define DMA_START_MODE   (DMA_MODE_BLOCK | DMA_START_CTLR_1)
#define DMA_START_MODE   DMA_MODE_PIPE
//#define DMA_START_MODE   (DMA_MODE_PIPE | DMA_START_CTLR_1)

#define DMA_MBX          0x12345678


int
main( int argc,
      char **argv)
{
  int i, idx;
  int size;
  int mbx_data = DMA_MBX;;
  int usec;
  int utmi, utmo;
  struct pev_time tmi, tmo;
  long vme_addr;

  printf("Entering idt DMA test\n");


  pev = pev_init( 0);
  if( !pev)
  {
    printf("Cannot allocate data structures to control PEV1100\n");
    exit( -1);
  }
  if( pev->fd < 0)
  {
    printf("Cannot find PEV1100 interface\n");
    exit( -1);
  }

  idx = DMA_TARGET;
  if( argc > 1)
  {
    idx = argv[1][0] - '0';
  }
  if( ( idx < 0) || ( idx > 4))
  {
    printf("bad device number\n");
    exit(1);
  }
  if( argc > 2)
  {
    sscanf( argv[2], "%x", &mbx_data);
  }

  if( idt_init( idx) < 0)
  {
    printf("cannot open idt device #%d\n", idx);
    exit(-1);
  }

  /* allocate a 1 MBytes buffer in kernel space suitable for DMA */
  dma_buf.size = 0x100000;
  pev_buf_alloc( &dma_buf);
  kbuf_loc_addr = (void *)dma_buf.u_addr;
  printf("DMA buffer kernel address = %p\n", dma_buf.k_addr);
  printf("DMA buffer bus address    = %p\n", dma_buf.b_addr);
  printf("DMA buffer usr address    = %p\n", dma_buf.u_addr);
  for( i = 0; i < (dma_buf.size/4); i++)
  {
    kbuf_loc_addr[i] = ( mbx_data &0xfff00000) + (i*4);
  }

  /* get VME configuration */
  pev_vme_conf_read( &vme_conf);
  printf("VME A32 base address = 0x%08x [0x%x]", vme_conf.a32_base, vme_conf.a32_size);
  if( vme_conf.mas_ena)
  {
    printf(" -> enabled\n");
  }
  else
  {
    printf(" -> disabled\n");
  }
  /* create an address translation window in the VME slave port */
  /* pointing to the PEV1100 Shared Memory                      */

  vme_slv_map.rem_addr = SHM_OFFSET; /* shared memory at offset 1MBytes */
  vme_slv_map.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_SHM;
  vme_slv_map.flag = 0x0;
  vme_slv_map.sg_id = MAP_SLAVE_VME;
  vme_slv_map.size = 0x100000;
  pev_map_alloc( &vme_slv_map);
  vme_conf.slv_ena |= VME_SLV_ENA;
  pev_vme_conf_write( &vme_conf);

  /* calculate the VME base address at which the Shared Memory has been mapped */
  vme_addr = vme_conf.a32_base + vme_slv_map.loc_addr; 
  printf("shared Memory is visible at VME A32 address 0x%08x\n", (int)vme_addr);

  size = DMA_SIZE;

  dma_req.src_addr = DMA_SRC_ADDR;  
  dma_req.des_addr = DMA_DES_ADDR;
  dma_req.size = size;         
  dma_req.src_space = DMA_SRC_SPACE;
  dma_req.des_space = DMA_DES_SPACE;
  dma_req.src_mode = DMA_PCIE_RR2;
  dma_req.des_mode = DMA_PCIE_RR2;
  dma_req.start_mode = DMA_START_MODE;
  dma_req.end_mode = 0;
  dma_req.intr_mode = DMA_INTR_ENA;
  dma_req.wait_mode = DMA_WAIT_INTR | DMA_WAIT_1S | (5<<4);
  dma_req.mbx_data = mbx_data;

  printf("Starting DMA transfer %08lx:%x %08lx:%x %08x: ", dma_req.des_addr, dma_req.des_space, dma_req.src_addr, dma_req.src_space, dma_req.size);
  pev_timer_read( &tmi);
  idt_dma_move( idx,  &dma_req);
  pev_timer_read( &tmo);
  utmi = (tmi.utime & 0x1ffff);
  utmo = (tmo.utime & 0x1ffff);

  usec = (tmo.time - tmi.time)*1000 + (utmo - utmi)/100;
  if(  dma_req.dma_status & DMA_STATUS_TMO)
  {
   printf("NOK -> timeout - status=%08x\n",  dma_req.dma_status);
  }
  else if(  dma_req.dma_status & DMA_STATUS_ERR)
  {
    printf("NOK -> transfer error - status=%08x\n", dma_req.dma_status);
  }
  else
  {
    printf("OK -> %d usec - %f MBytes/sec - status=%08x\n", usec, (float)(dma_req.size & 0xffffff)/(float)usec, dma_req.dma_status);
  }

  printf("dma mailbox: %08x\n", dma_req.mbx_data);
  for( i = 0; i < 0x10; i++)
  {
    printf("%08x ", kbuf_loc_addr[i]);
    if( (i&3) == 3) printf("\n");
  }

  pev_buf_free( &dma_buf);
  pev_map_free( &vme_slv_map);
  idt_exit();
  exit(0);

}
