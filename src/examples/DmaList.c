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
 * $Log: DmaList.c,v $
 * Revision 1.4  2012/06/05 13:37:31  kalantari
 * linux driver ver.4.12 with intr Handling
 *
 * Revision 1.5  2012/04/18 07:44:15  ioxos
 * cosmetics [JFG]
 *
 * Revision 1.4  2010/01/13 16:53:43  ioxos
 * Cosmetics [JFG]
 *
 * Revision 1.3  2010/01/12 15:45:59  ioxos
 * scan address offset and size + data verification [JFG]
 *
 * Revision 1.2  2010/01/08 15:40:20  ioxos
 * use pev_dma_list() to execute DMA transfer [JFG]
 *
 * Revision 1.1  2010/01/08 11:23:12  ioxos
 * fist checkin [JFG]
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

static int tst_dma_read( ulong);

struct pev_node *pev;
struct pev_ioctl_vme_conf vme_conf;
struct pev_ioctl_map_pg shm_mas_map;
struct pev_ioctl_map_pg vme_mas_map;
struct pev_ioctl_map_pg vme_slv_map;
struct pev_ioctl_buf dma_buf;
struct pev_ioctl_dma_req dma_req;
struct pev_ioctl_dma_list dma_list[64];
struct pev_ioctl_dma_sts dma_sts;
struct timeval ti, to;
struct timezone tz;
void *shm_loc_addr;
void *shm_vme_addr;
void *kbuf_loc_addr;
void *ubuf_loc_addr;
struct pev_ioctl_rdwr rdwr;


#define SHM_OFFSET 0x100000

main( int argc,
      void *argv[])
{
  int i, j, data, *p;
  ulong vme_addr;
  long dt, dt1, dt2;
  float usec;
  int kmem_fd;
  int t_10000, t_20000;
  uint crate;
  int ret;

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
  printf("shared Memory is visible at VME A32 address 0x%08x\n", vme_addr);

  /* create an address translation window in the PCIe End Point */
  /* pointing to the VME address at which the Shared Memory has been mapped  */
  vme_mas_map.rem_addr = vme_addr;
  vme_mas_map.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_VME|MAP_VME_A32;
  vme_mas_map.flag = 0x0;
  vme_mas_map.sg_id = MAP_MASTER_32;
  vme_mas_map.size = 0x100000;
  pev_map_alloc( &vme_mas_map);

  printf("offset in PCI MEM window to access SHM throug VME : %p\n", vme_mas_map.loc_addr);

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

  printf("local address = %p\n", shm_mas_map.loc_addr);
  printf("offset in PCI MEM window to access SHM locally : %p\n", shm_mas_map.loc_addr);


  printf("perform the mapping in user's space : ");
  shm_loc_addr = pev_mmap( &shm_mas_map);
  printf("%p", shm_loc_addr);
  if( shm_loc_addr == MAP_FAILED)
  {
    printf(" ->Failed\n");
    goto VmeTst_exit;
  }
  printf(" -> Done\n");


  /* allocate a test buffer and initialize it */
  ubuf_loc_addr = malloc( 0x100000);

  printf("DMA transfer : VME read (63*4 KBytes)\n");
  tst_dma_read( vme_addr);


VmeTst_exit:
  pev_munmap( &shm_mas_map);
  pev_map_free( &shm_mas_map);
  pev_munmap( &vme_mas_map);
  pev_map_free( &vme_mas_map);
  pev_map_free( &vme_slv_map);
  close( kmem_fd);
#ifdef XENOMAI
  pev_rt_exit();
#endif
  pev_exit( pev);

  exit(0);
}


int 
tst_dma_read( ulong vme_addr)
{
  void *d, *s;
  int i, j, n, retval;
  struct pev_time tmi, tmo;
  int utmi, utmo;
  int size;
  void *uaddr;
  uint usec[3];
  int data;
  unsigned char *p, *q;

  size = 0x100000;

  /* fill shared memory with test pattern */
  d = shm_loc_addr;
  data = 0x11223344;
  for( i = 0; i < 0x100000; i += 4)
  {
    *(int *)(d+i) = data;
    data += 0x11111111;
  } 

  /* allocate user buffer and fill it with 0x12345678 */
  uaddr = malloc( size);
  d = uaddr;
  for( i = 0; i < size; i += 4)
  {
    *(int *)(d+i) = 0x12345678;
  } 

  /* prepare transfer list */

  for( i = 0; i < 64; i++)
  {
    dma_list[i].addr = (ulong)vme_addr + ((i&7)<<16) + ((i>>3)&7);
    dma_list[i].size = 0x1000 + (i&7);
    dma_list[i].mode = DMA_SPACE_VME|DMA_VME_BLT;
  }
  pev_timer_read( &tmi);
  retval = pev_dma_vme_list_rd(uaddr, &dma_list[0], 63);
  pev_timer_read( &tmo);
  utmi = (tmi.utime & 0x1ffff);
  utmo = (tmo.utime & 0x1ffff);
  if( retval < 0)
  {
    printf("pev_dma_move() : error %d\n", retval);
    return(-1);
  }
  retval = pev_dma_status(&dma_sts);
  if( retval < 0)
  {
    printf("pev_dma_status() : error %d\n", retval);
    return(-1);
  }
  usec[0] = (dma_sts.start.msec - tmi.time)*1000 + (((int)(dma_sts.start.usec&0x1ffff) - utmi)/100);
  printf("timing: %d", usec[0]);
  usec[1] = (dma_sts.wr.msec - tmi.time)*1000 + (((int)(dma_sts.wr.usec&0x1ffff) - utmi)/100);
  printf(" : %d [%d]", usec[1], usec[1] - usec[0]);
  usec[2] = (dma_sts.rd.msec - tmi.time)*1000 + (((int)(dma_sts.rd.usec&0x1ffff) - utmi)/100);
  printf(" : %d [%d]", usec[2], usec[2] - usec[1]);
  usec[3] = (tmo.time - tmi.time)*1000 + (utmo - utmi)/100;
  printf(" : %d [%d]\n", usec[3], usec[3] - usec[2]);

  n = 0;
  d = uaddr;
  for( j = 0; j < 3; j++)
  {
    for( i = 0; i < dma_list[j].size; i++)
    {
      p = (unsigned char *)(d+n);
      q = (unsigned char *)(shm_loc_addr + (dma_list[j].addr & 0xfffff) + i);
      if( *p != *p)
      {
	printf("transfer error %p - %x [%x - %x]\n", p, n, *p, *q);
	return( 0);
      }
      n++;
    }
  }
  return( 0);
}

