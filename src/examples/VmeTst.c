/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : VmeTst.c
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
 * $Log: VmeTst.c,v $
 * Revision 1.1  2012/03/15 14:50:11  kalantari
 * added exact copy of tosca-driver_4.04 from afs
 *
 * Revision 1.3  2010/08/26 14:29:24  ioxos
 * cleanup void pointers and char * [JFG]
 *
 * Revision 1.2  2009/05/20 15:18:57  ioxos
 * support for multicrate + test using PMEM windoe [JFG]
 *
 * Revision 1.1  2009/01/08 08:19:03  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>

typedef unsigned int u32;
#include <pevioctl.h>
#include <pevulib.h>

static float tst_read( void *);
static float tst_write( void *);

struct pev_node *pev;
struct pev_ioctl_map_pg shm_mas_map;
struct pev_ioctl_map_pg shm_mas64_map;
struct pev_ioctl_map_pg vme_mas_map;
struct pev_ioctl_map_pg vme_slv_map;
struct pev_ioctl_vme_conf vme_conf;
struct timeval ti, to;
struct timezone tz;

main( int argc,
      char **argv)
{
  void *shm_loc_addr, *shm_loc64_addr, *shm_vme_addr;
  int i, data, *p;
  long vme_addr;
  long dt, dt1, dt2;
  float usec;
  char yn;
  uint crate;

  crate = 0;
  if( argc > 1)
  {
    sscanf(argv[1], "%d", &crate);
  }

  printf("Entering VME test program for crate %d\n", crate);

  /* call PEV1100 user library initialization function */
  pev = pev_init( crate);
  if( !pev)
  {
    printf("Cannot allocate data structures to control PEV1100\n");
    exit( -1);
  }
  /* verify if the PEV1100 is accessible */
  if( pev->fd < 0)
  {
    printf("Cannot find PEV1100 interface\n");
    exit( -1);
  }


  /* get the current VME configuration */
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

  vme_slv_map.rem_addr = 0x000000; /* shared memory base address */
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
  shm_mas_map.rem_addr = 0x000000; /* shared memory base address */
  shm_mas_map.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_SHM;
  shm_mas_map.flag = 0x0;
  shm_mas_map.sg_id = MAP_MASTER_32;
  shm_mas_map.size = 0x100000;
  pev_map_alloc( &shm_mas_map);

  printf("local address usinf MEM space = %p\n", shm_mas_map.loc_addr);
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

  /* create an address translation window in the PCIe End Point */
  /* pointing to the PEV1100 local address of the Shared Memory */
  shm_mas64_map.rem_addr = 0x000000; /* shared memory base address */
  shm_mas64_map.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_SHM;
  shm_mas64_map.flag = 0x0;
  shm_mas64_map.sg_id = MAP_MASTER_64;
  shm_mas64_map.size = 0x100000;
  pev_map_alloc( &shm_mas64_map);

  printf("local address using PMEM space = %p\n", shm_mas64_map.loc_addr);
  printf("offset in PCI PMEM window to access SHM locally : %p\n", shm_mas64_map.loc_addr);


  printf("perform the mapping in user's space : ");
  shm_loc64_addr = pev_mmap( &shm_mas64_map);
  printf("%p", shm_loc64_addr);
  if( shm_loc64_addr == MAP_FAILED)
  {
    printf(" ->Failed\n");
    goto VmeTst_exit;
  }
  printf(" -> Done\n");

  printf("Continue -> ");
  scanf("%c", &yn);

  usec = tst_read( shm_vme_addr);
  printf("VME read cycle %4f usec\n", usec);
  usec = tst_write( shm_vme_addr);
  printf("VME write cycle %4f usec\n", usec);
  usec = tst_read( shm_loc_addr);
  printf("SHM MEM read cycle %4f usec\n", usec);
  usec = tst_write( shm_loc_addr);
  printf("SHM MEM write cycle %4f usec\n", usec);
  usec = tst_read( shm_loc64_addr);
  printf("SHM PMEM read cycle %4f usec\n", usec);
  usec = tst_write( shm_loc64_addr);
  printf("SHM PMEM write cycle %4f usec\n", usec);


VmeTst_exit:
  pev_munmap( &shm_mas64_map);
  pev_map_free( &shm_mas64_map);
  pev_munmap( &shm_mas_map);
  pev_map_free( &shm_mas_map);
  pev_munmap( &vme_mas_map);
  pev_map_free( &vme_mas_map);
  pev_map_free( &vme_slv_map);

  pev_exit( pev);

  exit(0);
}

float
tst_read( void *addr)
{
  int i, data;
  int *s, *d;
  long dt1, dt2;

  gettimeofday( &ti, &tz);
  s = (int *)addr;
  for( i = 0; i < 0x10000; i++)
  {
    data = *s++;
  }
  gettimeofday( &to, &tz);
  dt1 = ((to.tv_sec -  ti.tv_sec) * 1000000) + ( to.tv_usec - ti.tv_usec);
  gettimeofday( &ti, &tz);
  s = (int *)addr;
  for( i = 0; i < 0x20000; i++)
  {
    data = *s++;
  }
  gettimeofday( &to, &tz);
  dt2 = ((to.tv_sec -  ti.tv_sec) * 1000000) + ( to.tv_usec - ti.tv_usec);
  return( (float)(dt2-dt1)/0x10000);
}

float
tst_write( void *addr)
{
  int i, data;
  int *s, *d;
  long dt1, dt2;

  data = 0xa5a5a5a5;
  gettimeofday( &ti, &tz);
  d = (int *)addr;
  for( i = 0; i < 0x10000; i++)
  {
    *d++ = data;
  }
  gettimeofday( &to, &tz);
  dt1 = ((to.tv_sec -  ti.tv_sec) * 1000000) + ( to.tv_usec - ti.tv_usec);
  gettimeofday( &ti, &tz);
  d = (int *)addr;
  for( i = 0; i < 0x20000; i++)
  {
    *d++ = data;
  }
  gettimeofday( &to, &tz);
  dt2 = ((to.tv_sec -  ti.tv_sec) * 1000000) + ( to.tv_usec - ti.tv_usec);
  return( (float)(dt2-dt1)/0x10000);
}

