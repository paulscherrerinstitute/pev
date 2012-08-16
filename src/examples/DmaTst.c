/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : DmaTst.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : october 10,2008
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
 * $Log: DmaTst.c,v $
 * Revision 1.8  2012/08/16 09:11:39  kalantari
 * added version 4.16 of tosca driver
 *
 * Revision 1.13  2012/08/15 06:50:57  ioxos
 * wait for end of DMA with timeout protection [JFG]
 *
 * Revision 1.12  2012/08/08 09:27:29  ioxos
 * optimize transfer mode (1kbyte + RR2) [JFG]
 *
 * Revision 1.11  2012/06/01 14:00:14  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.10  2012/04/18 07:44:15  ioxos
 * cosmetics [JFG]
 *
 * Revision 1.9  2010/08/26 14:29:24  ioxos
 * cleanup void pointers and char * [JFG]
 *
 * Revision 1.8  2010/06/16 13:00:05  ioxos
 * bug correction:assign kbuf_loc_addr [JFG]
 *
 * Revision 1.7  2010/06/11 12:03:23  ioxos
 * use pev mmap() for kernel memory mapping [JFG]
 *
 * Revision 1.6  2009/08/18 14:29:11  ioxos
 * check returned value tst_dma_read() [JFG]
 *
 * Revision 1.5  2009/06/03 12:28:19  ioxos
 * use buf_alloc instead of dma_alloc [JFG]
 *
 * Revision 1.4  2009/05/25 12:05:26  ioxos
 * ajust time to 100 MHz clock + support for xenomai [JFG]
 *
 * Revision 1.3  2009/01/27 14:43:21  ioxos
 * perform transfer for all mode/size + check data integrity [JFG]
 *
 * Revision 1.2  2009/01/09 13:16:00  ioxos
 * get DMA status and print timing [JFG]
 *
 * Revision 1.1  2009/01/08 08:19:03  ioxos
 * first checkin [JFG]
 *
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
#include <sys/time.h>

typedef unsigned int u32;
#include <pevioctl.h>
#include <pevulib.h>

#ifdef XENOMAI
#include <pevrtlib.h>
#include <rtdm/rtdm.h>
#include <native/task.h>

#define DEVICE_NAME		"pev_rt"
int rt_fd;
RT_TASK rt_task_desc;

#endif

static int tst_dma_read( long, uint, int, int *);
static int tst_dma_write( long, uint, int, int *);
static void print_res_read( void);
static void print_res_write( void);

struct pev_node *pev;
struct pev_ioctl_vme_conf vme_conf;
struct pev_ioctl_map_pg shm_mas_map;
struct pev_ioctl_map_pg vme_mas_map;
struct pev_ioctl_map_pg vme_slv_map;
struct pev_ioctl_buf dma_buf;
struct pev_ioctl_dma_req dma_req;
struct pev_ioctl_dma_sts dma_sts;
struct timeval ti, to;
struct timezone tz;
void *shm_loc_addr;
void *shm_vme_addr;
void *kbuf_loc_addr;
void *ubuf_loc_addr;
struct pev_ioctl_rdwr rdwr;

int pev_time[8][16][4];
int t_size[16] = {                      0x40,   0x60,   0x80,
                     0x100,   0x200,   0x400,  0x600,  0x800,
                    0x1000,  0x2000,  0x4000, 0x6000, 0x8000,
		   0x10000, 0x20000, 0x40000
                 };
int t_res[16];
const char *t_mode[8] = { "   SLT", "   BLT", "  MBLT", " 2eVME", "2eFAST", " 2e160", " 2e233", " 2e320"};

#define SHM_OFFSET 0x100000

int
main( int argc,
      char **argv)
{
  int i, j;
  long vme_addr;
  uint crate;

  crate = 0;
  if( argc > 1)
  {
    sscanf(argv[1], "%d", &crate);
  }


  printf("Entering DMA test program for crate %d\n", crate);

  pev = pev_init( crate);
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

#ifdef XENOMAI
  rt_fd = pev_rt_init( crate);
  if( rt_fd < 0)
  {
    printf("Cannot find real time PEV1100 interface\n");
    exit( -1);
  }
#endif
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

  /* calculate the VME base address at which the Shared Memory has been mapped */
  vme_addr = vme_conf.a32_base + vme_slv_map.loc_addr; 
  printf("shared Memory is visible at VME A32 address 0x%08x\n", (int)vme_addr);

  /* create an address translation window in the PCIe End Point */
  /* pointing to the VME address at which the Shared Memory has been mapped  */
  vme_mas_map.rem_addr = vme_addr;
  vme_mas_map.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A32;
  vme_mas_map.flag = 0x0;
  vme_mas_map.sg_id = MAP_MASTER_32;
  vme_mas_map.size = 0x100000;
  pev_map_alloc( &vme_mas_map);

  printf("offset in PCI MEM window to access SHM throug VME : %lx\n", vme_mas_map.loc_addr);

  printf("perform the mapping in user's space");
  shm_vme_addr = pev_mmap( &vme_mas_map);
  printf("%p\n", shm_vme_addr);
  if( shm_vme_addr == MAP_FAILED)
  {
    printf("Failed\n");
    goto VmeTst_exit;
  }
  printf("Done\n");

  /* create an address translation window in the PCIe End Point */
  /* pointing to the PEV1100 local address of the Shared Memory */
  shm_mas_map.rem_addr = SHM_OFFSET; /* shared memory at offset 1 MBytes */
  shm_mas_map.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_SHM;
  shm_mas_map.flag = 0x0;
  shm_mas_map.sg_id = MAP_MASTER_32;
  shm_mas_map.size = 0x100000;
  pev_map_alloc( &shm_mas_map);

  printf("local address = %lx\n", shm_mas_map.loc_addr);
  printf("offset in PCI MEM window to access SHM locally : %lx\n", shm_mas_map.loc_addr);


  printf("perform the mapping in user's space : ");
  shm_loc_addr = pev_mmap( &shm_mas_map);
  printf("%p", shm_loc_addr);
  if( shm_loc_addr == MAP_FAILED)
  {
    printf(" ->Failed\n");
    goto VmeTst_exit;
  }
  printf(" -> Done\n");

  /* allocate a 1 MBytes buffer in kernel space suitable for DMA */
  dma_buf.size = 0x100000;
  pev_buf_alloc( &dma_buf);
  kbuf_loc_addr = (void *)dma_buf.u_addr;
  printf("DMA buffer kernel address = %p\n", dma_buf.k_addr);
  printf("DMA buffer bus address    = %p\n", dma_buf.b_addr);
  printf("DMA buffer usr address    = %p\n", dma_buf.u_addr);


  /* allocate a test buffer and initialize it */
  ubuf_loc_addr = malloc( 0x100000);


  printf("DMA transfer : VME read (128 KByte)\n");
  for( i = 0x30; i <= 0xa0; i += 0x10)
    //for( i = 0x30; i <= 0x30; i += 0x10)
  {
    int idx;

    idx = (i>>4)-3;
    for( j = 0; j < 16; j++)
      //for( j = 0; j < 1; j++)
    {
      t_res[j] = tst_dma_read( vme_addr, t_size[j], i, &pev_time[idx][j][0]);
    }
  }

  print_res_read();

  for( i = 0x30; i <= 0xa0; i += 0x10)
  //for( i = 0x30; i <= 0x30; i += 0x10)
  {
    int idx;

    idx = (i>>4)-3;
    for( j = 0; j < 16; j++)
    //for( j = 0; j < 1; j++)
    {
      t_res[j] = tst_dma_write( vme_addr, t_size[j], i, &pev_time[idx][j][0]);
    }
  }
  print_res_write();

VmeTst_exit:
  pev_buf_free( &dma_buf);
  pev_munmap( &shm_mas_map);
  pev_map_free( &shm_mas_map);
  pev_munmap( &vme_mas_map);
  pev_map_free( &vme_mas_map);
  pev_map_free( &vme_slv_map);
#ifdef XENOMAI
  pev_rt_exit();
#endif
  pev_exit( pev);

  exit(0);
}

void
print_res_read( void)
{
  int i, j;

  printf("DMA transfer : Hardware Timing\n");
  printf("+--------+-----------------------------------------------------------------------++--------+\n");  
  printf("|  size  |                                 VME read                              ||  PCIe  |\n");  
  printf("|  bytes ");  
  for( i = 0; i < 8; i++)
  {
    printf("| %s ",  t_mode[i]);
  }
  printf("||  write |\n");  
  printf("+--------+--------+--------+--------+--------+--------+--------+--------+--------++--------+\n");  
  for( i = 0; i < 16; i++)
  {
    printf("| %6d ",  t_size[i]);
    for( j = 0; j < 8; j++)
    {
      printf("| %6d ",  pev_time[j][i][1] - pev_time[j][i][0]);
    }
    printf("|| %6d |\n", pev_time[0][i][2] - pev_time[0][i][1]);  

  }
  printf("+--------+--------+--------+--------+--------+--------+--------+--------+--------++--------+\n");  
  printf("|  rate  ");  
  for( i = 0; i < 8; i++)
  {
    printf("| %6.2f ",  (float)(t_size[15]-t_size[14])/(float)(pev_time[i][15][1] - pev_time[i][14][1]));
  }
  printf("|| %6.2f |\n", (float)(t_size[15]-t_size[14])/(float)((pev_time[0][15][2] - pev_time[0][15][1])-(pev_time[0][14][2] - pev_time[0][14][1])));  
  printf("+--------+--------+--------+--------+--------+--------+--------+--------+--------++--------+\n");  
  printf("\n\n");  


  printf("DMA transfer : Software Timing\n");
  printf("+--------+-----------------------------------------------------------------------++--------+\n");  
  printf("|  size  |                                 VME read                              ||  over  |\n");  
  printf("|  bytes ");  
  for( i = 0; i < 8; i++)
  {
    printf("| %s ",  t_mode[i]);
  }
  printf("||  head  |\n");  
  printf("+--------+--------+--------+--------+--------+--------+--------+--------+--------++--------+\n");  
  for( i = 0; i < 16; i++)
  {
    printf("| %6d ",  t_size[i]);
    for( j = 0; j < 8; j++)
    {
      printf("| %6d ",  pev_time[j][i][3]);
    }
    printf("|| %2d+%3d |\n", pev_time[0][i][0], pev_time[0][i][3] - pev_time[0][i][2]);  

  }
  printf("+--------+--------+--------+--------+--------+--------+--------+--------+--------++--------+\n");  
  printf("|  rate  ");  
  for( i = 0; i < 8; i++)
  {
    printf("| %6.2f ",  (float)(t_size[15]-t_size[14])/(float)(pev_time[i][15][3] - pev_time[i][14][3]));
  }
  printf("||        |\n");  
  printf("+--------+--------+--------+--------+--------+--------+--------+--------+--------++--------+\n");  
  printf("\n");  

  return;
}

void
print_res_write( void)
{
  int i, j;

  printf("DMA transfer : Hardware Timing\n");
  printf("+--------+-----------------------------------------------------------------------++--------+\n");  
  printf("|  size  |                                 VME write                             ||  PCIe  |\n");  
  printf("|  bytes ");  
  for( i = 0; i < 8; i++)
  {
    printf("| %s ",  t_mode[i]);
  }
  printf("||  read  |\n");  
  printf("+--------+--------+--------+--------+--------+--------+--------+--------+--------++--------+\n");  
  for( i = 0; i < 16; i++)
  {
    printf("| %6d ",  t_size[i]);
    for( j = 0; j < 8; j++)
    {
      printf("| %6d ",  pev_time[j][i][2] - pev_time[j][i][1]);
    }
    printf("|| %6d |\n", pev_time[0][i][1] - pev_time[0][i][0]);  

  }
  printf("+--------+--------+--------+--------+--------+--------+--------+--------+--------++--------+\n");  
  printf("|  rate  ");  
  for( i = 0; i < 8; i++)
  {
    printf("| %6.2f ",  (float)(t_size[15]-t_size[14])/(float)((pev_time[i][15][2] - pev_time[i][15][1])-(pev_time[i][14][2] - pev_time[i][14][1])));
  }
  printf("|| %6.2f |\n", (float)(t_size[15]-t_size[14])/(float)(pev_time[0][15][1] - pev_time[0][14][1]));  
  printf("+--------+--------+--------+--------+--------+--------+--------+--------+--------++--------+\n");  
  printf("\n\n");  


  printf("DMA transfer : Software Timing\n");
  printf("+--------+-----------------------------------------------------------------------++--------+\n");  
  printf("|  size  |                                 VME write                             ||  over  |\n");  
  printf("|  bytes ");  
  for( i = 0; i < 8; i++)
  {
    printf("| %s ",  t_mode[i]);
  }
  printf("||  head  |\n");  
  printf("+--------+--------+--------+--------+--------+--------+--------+--------+--------++--------+\n");  
  for( i = 0; i < 16; i++)
  {
    printf("| %6d ",  t_size[i]);
    for( j = 0; j < 8; j++)
    {
      printf("| %6d ",  pev_time[j][i][3]);
    }
    printf("|| %2d+%3d |\n", pev_time[0][i][0], pev_time[0][i][3] - pev_time[0][i][2]);  

  }
  printf("+--------+--------+--------+--------+--------+--------+--------+--------+--------++--------+\n");  
  printf("|  rate  ");  
  for( i = 0; i < 8; i++)
  {
    printf("| %6.2f ",  (float)(t_size[15]-t_size[14])/(float)(pev_time[i][15][3] - pev_time[i][14][3]));
  }
  printf("||        |\n");  
  printf("+--------+--------+--------+--------+--------+--------+--------+--------+--------++--------+\n");  
  printf("\n");  

  return;
}

int 
tst_dma_read( long vme_addr, 
              uint size,
              int mode,
	      int *usec)
{
  char *d, *s;
  int i, retval;
  struct pev_time tmi, tmo;
  int utmi, utmo;

  /* fill shared memory with test pattern */
  d = (char *)shm_loc_addr;
  for( i = 0; i < size; i += 4)
  {
    *(int *)(d+i) = i;
  } 

  /* fill kernel buffer with 0x12345678 */
  d = (char *)kbuf_loc_addr;
  for( i = 0; i < size; i += 4)
  {
    *(int *)(d+i) = 0x12345678;
  } 

  /* copy test buffer from shared memory to system memory */
  //printf("Start DMA transfer\n");

  dma_req.src_addr = vme_addr;                    /* source is VME address of SHM */
  dma_req.des_addr = (ulong)dma_buf.b_addr;       /* destination is DMA buffer    */
  dma_req.size = size| 0xe0000000;                  
  dma_req.src_space = DMA_SPACE_VME|mode;
  dma_req.des_space = DMA_SPACE_PCIE;
  dma_req.src_mode = DMA_PCIE_RR2;
  dma_req.des_mode = DMA_PCIE_RR2;
  dma_req.start_mode = DMA_MODE_BLOCK;
  dma_req.end_mode = 0;
  dma_req.intr_mode = DMA_INTR_ENA;
  dma_req.wait_mode = DMA_WAIT_INTR | DMA_WAIT_1S | (5<<4);


  retval = pev_dma_move(&dma_req);
  pev_timer_read( &tmi);
  retval = pev_dma_move(&dma_req);
  pev_timer_read( &tmo);
  utmi = (tmi.utime & 0x1ffff);
  utmo = (tmo.utime & 0x1ffff);
  if( retval < 0)
  {
    printf("pev_dma_move() : error %d\n", retval);
  }
  retval = pev_dma_status(&dma_sts);
  if( retval < 0)
  {
    printf("pev_dma_status() : error %d\n", retval);
  }

  usec[0] = (dma_sts.start.msec - tmi.time)*1000 + (((int)(dma_sts.start.usec&0x1ffff) - utmi)/100);
  //printf("timing: %d", usec[0]);
  usec[1] = (dma_sts.wr.msec - tmi.time)*1000 + (((int)(dma_sts.wr.usec&0x1ffff) - utmi)/100);
  //printf(" : %d [%d]", usec[1], usec[1] - usec[0]);
  usec[2] = (dma_sts.rd.msec - tmi.time)*1000 + (((int)(dma_sts.rd.usec&0x1ffff) - utmi)/100);
  //printf(" : %d [%d]", usec[2], usec[2] - usec[1]);
  usec[3] = (tmo.time - tmi.time)*1000 + (utmo - utmi)/100;
  //printf(" : %d [%d]\n", usec[3], usec[3] - usec[0]);


  /* compare shared memory with system memory (kernel buffer) */
  d = (char *)shm_loc_addr;
  s = (char *)kbuf_loc_addr;
  for( i = 0; i < size; i += 4)
  {
    if( *(int *)(d+i) != *(int *)(s+i))
    {
      printf("compare error at offset : %06x - %08x : %08x [%02x, %06x]\n", i, *(int *)(d+i), *(int *)(s+i), mode, size);
      break;
    }
  } 

  return( usec[3]);
}

int 
tst_dma_write( long vme_addr, 
               uint size,
               int mode,
	       int *usec)
{
  char *d, *s;
  int i, retval;
  struct pev_time tmi, tmo;
  int utmi, utmo;

  /* fill kernel buffer with test pattern */
  d = (char *)kbuf_loc_addr;
  for( i = 0; i < size; i += 4)
  {
    *(long *)(d+i) = i;
  } 

  /* fill shared memory with 0x12345678 */
  d = (char *)shm_loc_addr;
  for( i = 0; i < size; i += 4)
  {
    *(long *)(d+i) = 0x12345678;
  } 

  /* copy test buffer from shared memory to system memory */
  //printf("Start DMA transfer\n");

  dma_req.des_addr = vme_addr;                    /* destination is VME address of SHM */
  dma_req.src_addr = (ulong)dma_buf.b_addr;       /* source is DMA buffer    */
  dma_req.size = size | 0xe0000000;                  
  dma_req.des_space = DMA_SPACE_VME|mode;
  dma_req.src_space = DMA_SPACE_PCIE;
  dma_req.src_mode = DMA_PCIE_RR2;
  dma_req.des_mode = DMA_PCIE_RR2;
  dma_req.start_mode = DMA_MODE_BLOCK;
  dma_req.end_mode = 0;
  dma_req.intr_mode = DMA_INTR_ENA;
  dma_req.wait_mode = DMA_WAIT_INTR | DMA_WAIT_1S | (5<<4);

  retval = pev_dma_move(&dma_req);
  pev_timer_read( &tmi);
  retval = pev_dma_move(&dma_req);
  pev_timer_read( &tmo);
  utmi = (tmi.utime & 0x1ffff);
  utmo = (tmo.utime & 0x1ffff);
  if( retval < 0)
  {
    printf("pev_dma_move() : error %d\n", retval);
  }
  retval = pev_dma_status(&dma_sts);
  if( retval < 0)
  {
    printf("pev_dma_status() : error %d\n", retval);
  }
  //printf("timing: %d", usec[0]);
  usec[1] = (dma_sts.wr.msec - tmi.time)*1000 + (((int)(dma_sts.wr.usec&0x1ffff) - utmi)/100);
  //printf(" : %d [%d]", usec[1], usec[1] - usec[0]);
  usec[2] = (dma_sts.rd.msec - tmi.time)*1000 + (((int)(dma_sts.rd.usec&0x1ffff) - utmi)/100);
  //printf(" : %d [%d]", usec[2], usec[2] - usec[1]);
  usec[3] = (tmo.time - tmi.time)*1000 + (utmo - utmi)/100;
  //printf(" : %d [%d]\n", usec[3], usec[3] - usec[0]);


  /* compare shared memory with system memory (kernel buffer) */
  d = (char *)kbuf_loc_addr;
  s = (char *)shm_loc_addr;
  for( i = 0; i < size; i += 4)
  {
    if( *(int *)(d+i) != *(int *)(s+i))
    {
      printf("compare error at offset : %06x - %08x : %08x [%02x, %06x]\n", i, *(int *)(d+i), *(int *)(s+i), mode, size);
      break;
    }
  } 

  return( usec[3]);
}
