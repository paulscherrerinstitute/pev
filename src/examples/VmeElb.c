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
 * $Log: VmeElb.c,v $
 * Revision 1.1  2013/06/07 15:01:17  zimoch
 * update to latest version
 *
 * Revision 1.3  2012/09/04 13:32:42  ioxos
 * map statically allocated sysmem [JFG]
 *
 * Revision 1.2  2012/08/29 11:32:27  ioxos
 * build three mapping and check for error [JFG]
 *
 * Revision 1.1  2012/08/27 12:02:55  ioxos
 * first checkin [JFG]
 *
 * Revision 1.4  2012/06/01 14:00:14  ioxos
 * -Wall cleanup [JFG]
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
#include <string.h>

typedef unsigned int u32;
#include <pevioctl.h>
#include <pevulib.h>

static float tst_read( void *);
static float tst_write( void *);

struct pev_node *pev;
struct pev_ioctl_map_pg vme_elb_map;
struct pev_ioctl_map_pg vme_elb_map2;
struct pev_ioctl_map_pg vme_elb_map3;
struct pev_ioctl_map_ctl vme_elb_ctl;
struct pev_ioctl_buf sys_mem_buf;
struct timeval ti, to;
struct timezone tz;

int
main( int argc,
      char **argv)
{
  void *shm_elb_addr;
  void *shm_elb_addr2;
  void *shm_elb_addr3;
  void *sys_mem_addr;
  long vme_addr;
  float usec;
  char yn;
  uint crate;
  ushort mode;

  crate = 0;
  vme_addr = 0;
  mode = 0x3c;
  if( argc < 3)
  {
    printf("usage: VmeElb <vme_addr> <mode>\n");
    printf("       where  <vme_addr> = vme address in hexadecimal\n");
    printf("              <mode> = a16u, a16s, a24u, a24s, a32u, a32s or iack\n");
    exit(0);
  }

  sscanf(argv[1], "%lx", &vme_addr);
  mode = 0x38;
  if( !strncmp( argv[2], "a16u",4)) mode |= 0x0;
  if( !strncmp( argv[2], "a16s",4)) mode |= 0x1;
  if( !strncmp( argv[2], "a24u",4)) mode |= 0x2;
  if( !strncmp( argv[2], "a24s",4)) mode |= 0x3;
  if( !strncmp( argv[2], "a32u",4)) mode |= 0x4;
  if( !strncmp( argv[2], "a32s",4)) mode |= 0x5;
  if( !strncmp( argv[2], "iack",4)) mode |= 0x6;

  printf("Entering test program for VME fast cycles (using ELB bus): %lx : %x\n", vme_addr, mode);

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


  /* clear any existing mapping */
  vme_elb_ctl.sg_id = MAP_VME_ELB;
  pev_map_clear( &vme_elb_ctl);

  /* create an address translation window in the ELB address space */
  /* pointing to the VME address at which the Shared Memory has been mapped  */
  vme_elb_map.rem_addr = vme_addr;
  vme_elb_map.mode = mode;
  vme_elb_map.flag = 0x0;
  vme_elb_map.sg_id = MAP_VME_ELB;
  vme_elb_map.size = 0x100000;
  pev_map_alloc( &vme_elb_map);

  printf("offset in ELB window to access VME : %lx\n", vme_elb_map.loc_addr);

  shm_elb_addr = NULL;
  if( vme_elb_map.loc_addr == -1)
  {
    printf("Allocation Failed\n");
  }
  else
  {
    printf("perform the mapping in user's space ");
    shm_elb_addr = pev_mmap( &vme_elb_map);
    printf("%p\n", shm_elb_addr);
    if( shm_elb_addr == MAP_FAILED)
    {
      printf("Mapping Failed\n");
      goto VmeTst_exit;
    }
    printf("Done\n");
  }


  /* create an address translation window in the ELB address space */
  /* pointing to the VME address at which the Shared Memory has been mapped  */
  vme_elb_map2.rem_addr = vme_addr+0x100000; /* address within the 256 MBytes window -> mapping shoud succed */
  vme_elb_map2.mode = mode;
  vme_elb_map2.flag = 0x0;
  vme_elb_map2.sg_id = MAP_VME_ELB;
  vme_elb_map2.size = 0x100000;
  pev_map_find( &vme_elb_map2);

  printf("offset in ELB window to access VME : %lx\n", vme_elb_map2.loc_addr);

  shm_elb_addr2 = NULL;
  if( vme_elb_map2.loc_addr == -1)
  {
    printf("Allocation Failed\n");
  }
  else
  {
    printf("perform the mapping in user's space ");
    shm_elb_addr2 = pev_mmap( &vme_elb_map2);
    printf("%p\n", shm_elb_addr2);
    if( shm_elb_addr2 == MAP_FAILED)
    {
      printf("Mapping Failed\n");
      goto VmeTst_exit;
    }
    printf("Done\n");
  }



  /* create an address translation window in the ELB address space */
  /* pointing to the VME address at which the Shared Memory has been mapped  */
  vme_elb_map3.rem_addr =  vme_addr+0x30000000; /* address out of the 256 MBytes window -> mapping shoud fail */
  vme_elb_map3.mode = mode;
  vme_elb_map3.flag = 0x0;
  vme_elb_map3.sg_id = MAP_VME_ELB;
  vme_elb_map3.size = 0x100000;
  pev_map_find( &vme_elb_map3);

  printf("offset in ELB window to access VME : %lx\n", vme_elb_map3.loc_addr);

  shm_elb_addr3 = NULL;
  if( vme_elb_map3.loc_addr == -1)
  {
    printf("Allocation Failed\n");
  }
  else
  {
    printf("perform the mapping in user's space ");
    shm_elb_addr3 = pev_mmap( &vme_elb_map3);
    printf("%p\n", shm_elb_addr3);
    if( shm_elb_addr3 == MAP_FAILED)
    {
      printf("Mapping Failed\n");
      goto VmeTst_exit;
    }
    printf("Done\n");
  }
  /* create user mapping for the upper part of system memory */
  printf("perform the mapping of sysmem at offset 0x20000000 :");
  sys_mem_buf.k_addr = NULL;
  sys_mem_buf.u_addr = NULL;
  sys_mem_buf.b_addr = (void *)0x20000000;
  sys_mem_buf.size = 0x100000;
  sys_mem_addr = pev_buf_map( &sys_mem_buf);
  printf("%p - %p - %p\n", sys_mem_buf.b_addr, sys_mem_buf.k_addr, sys_mem_buf.u_addr);
  printf("%p - %x\n", sys_mem_addr, *(int *)sys_mem_addr);

  printf("Continue -> ");
  scanf("%c", &yn);

  if( yn == 'n')
  {
    goto VmeTst_exit;
  }

  if( shm_elb_addr)
  {
    usec = tst_read( shm_elb_addr);
    printf("ELB read cycle %4f usec\n", usec);
    usec = tst_write( shm_elb_addr);
    printf("ELB write cycle %4f usec\n", usec);
  }

  if( shm_elb_addr)
  {
    usec = tst_read( shm_elb_addr2);
    printf("ELB read cycle %4f usec\n", usec);
    usec = tst_write( shm_elb_addr2);
    printf("ELB write cycle %4f usec\n", usec);
  }


VmeTst_exit:
  if( vme_elb_map3.loc_addr != -1)
  {
    pev_map_free( &vme_elb_map3);
  }
  if( vme_elb_map2.loc_addr != -1)
  {
    pev_map_free( &vme_elb_map2);
  }
  if( vme_elb_map.loc_addr != -1)
  {
    pev_map_free( &vme_elb_map);
  }
  if( sys_mem_buf.u_addr)
  {
    pev_buf_unmap( &sys_mem_buf);
  }

  pev_exit( pev);

  exit(0);
}

float
tst_read( void *addr)
{
  int i;
  volatile int *s;
  long dt1, dt2;

  gettimeofday( &ti, &tz);
  s = (int *)addr;
  for( i = 0; i < 0x10000; i++)
  {
    *s++;
  }
  gettimeofday( &to, &tz);
  dt1 = ((to.tv_sec -  ti.tv_sec) * 1000000) + ( to.tv_usec - ti.tv_usec);
  gettimeofday( &ti, &tz);
  s = (int *)addr;
  for( i = 0; i < 0x20000; i++)
  {
    *s++;
  }
  gettimeofday( &to, &tz);
  dt2 = ((to.tv_sec -  ti.tv_sec) * 1000000) + ( to.tv_usec - ti.tv_usec);
  return( (float)(dt2-dt1)/0x10000);
}

float
tst_write( void *addr)
{
  int i, data;
  int *d;
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

