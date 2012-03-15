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
 * $Log: BerrTst.c,v $
 * Revision 1.2  2012/03/15 16:15:37  kalantari
 * added tosca-driver_4.05
 *
 * Revision 1.3  2010/08/26 14:29:24  ioxos
 * cleanup void pointers and char * [JFG]
 *
 * Revision 1.2  2009/05/20 15:15:49  ioxos
 * support for multicrate [JFG]
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

typedef unsigned int u32;

#include <pevioctl.h>
#include <pevulib.h>

struct pev_node *pev;
struct pev_ioctl_map_pg vme_mas_map;
struct pev_reg *pev_reg = 0;


main( int argc,
      char **argv)
{
  char yn;
  volatile void *my_addr;
  int data;
  volatile uint sts, addr, addr_err;
  uint crate;

  crate = 0;
  if( argc > 1)
  {
    sscanf(argv[1], "%d", &crate);
  }
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

  printf("entering XVME542 control program\n");

  vme_mas_map.rem_addr = 0x000000;
  vme_mas_map.mode = 0x1203;
  vme_mas_map.flag = 0x0;
  vme_mas_map.sg_id = MAP_MASTER_32;
  vme_mas_map.size = 0x1000000;
  pev_map_alloc( &vme_mas_map);

  printf("local address = %p\n", vme_mas_map.loc_addr);

  printf("perform memory mapping...");
  my_addr = mmap( NULL, 0x1000000, PROT_READ|PROT_WRITE, MAP_SHARED, pev->fd, vme_mas_map.loc_addr);
  printf("%p\n", my_addr);
  if( my_addr == MAP_FAILED)
  {
    printf("Failed\n");
    goto xvme542_exit;
  }
  printf("Done\n");

 /* make sur berr is not forwarded to host */
  sts = pev_csr_rd(0x404);
  sts |= 0x80;
  pev_csr_wr( 0x404, sts);


  /* scan short io */
  for( addr = 0x0; addr < 0x1000000; addr += 0x80000)
  {
    /* make sure BERR status is cleared */
    sts = pev_csr_rd(0x41c);

    /* check address */
    data = *(short *)((char *)my_addr + addr);

    /* check bus error status */
    sts = pev_csr_rd( 0x41c);
    if( sts & 0x80000000)
    {
      addr_err = pev_csr_rd( 0x418);
      printf("VME BERR detected at address %x [%x]\n", addr_err, sts);
    }
    else
    {
      printf("Board detected at address %x [%04x]\n", addr, data);
    }
  }

  pev_map_free( &vme_mas_map);

xvme542_exit:
  pev_exit( pev);
  exit(0);
}
