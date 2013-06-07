/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : tst_0x.c
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
 * $Log: tst_0x.c,v $
 * Revision 1.12  2013/06/07 14:59:54  zimoch
 * update to latest version
 *
 * Revision 1.14  2012/06/11 07:57:59  ioxos
 * use pevx_get_ instead of pev_get_ [JFG]
 *
 * Revision 1.13  2012/06/07 08:29:05  ioxos
 * cast addr range to uint [JFG]
 *
 * Revision 1.12  2012/06/07 07:42:50  ioxos
 * enable compilation warnings [JFG]
 *
 * Revision 1.11  2012/06/01 14:00:06  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.10  2012/03/21 10:55:09  ioxos
 * cleanup & cosmetics [JFG]
 *
 * Revision 1.9  2012/03/21 10:15:24  ioxos
 * support for fast execution mode [JFG]
 *
 * Revision 1.8  2012/03/19 10:04:10  ioxos
 * test 1 check software configuration [JFG]
 *
 * Revision 1.7  2012/03/05 09:31:43  ioxos
 * don't swap IACK read on PPC [JFG]
 *
 * Revision 1.6  2011/10/03 10:07:18  ioxos
 * add test for 4 MB boundary [JFG]
 *
 * Revision 1.5  2010/06/11 11:56:26  ioxos
 * add test status report [JFG]
 *
 * Revision 1.4  2009/12/15 17:18:07  ioxos
 * modification for short io window + tst_09 [JFG]
 *
 * Revision 1.3  2009/12/02 15:11:52  ioxos
 * move dma test to tst_1x.c [JFG]
 *
 * Revision 1.2  2009/08/18 14:13:57  ioxos
 * enhance print out [JFG]
 *
 * Revision 1.1  2009/08/18 13:21:49  ioxos
 * first cvs checkin [JFG]
 *
 *
 *=============================< end file header >============================*/
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <pty.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <aio.h>
#include <errno.h>
#include <sys/mman.h>
#include <time.h>
#include <signal.h>

typedef unsigned int u32;
#include <pevioctl.h>
#include <pevulib.h>
#include <pevxulib.h>

#include "xprstst.h"
#include "tstlib.h"
#include "tstxlib.h"

extern int tst_check_cmd_tstop(void);
extern struct pev_reg_remap *reg_remap;
char *ident="        ";

int  
mytst_xxx( struct tst_ctl *tc,
	 int mode,
	 char *tst_id)
{
  time_t tm;
  char *ct;
  int i;
  char **para_p;

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));

  /*---->
  insert your test here...
  <----*/
  TST_LOG( tc, (logline, "%s->Executing mytest_xxx", tst_id));

  /*---->
  print parameters list...
  <----*/
  if( tc->para_cnt)
  {
    printf("\n");
    para_p = tc->para_p;
    for( i = 0; i < tc->para_cnt; i++)
    {
      printf("%s\n", para_p[i]);
    }
  }

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "\n%s->Exiting:%s", tst_id, ct));
  return( TST_STS_DONE);
}

int  
tst_config( struct tst_ctl *tc,
	 int mode,
	 char *tst_id)
{
  time_t tm;
  char *ct;
  int retval;

  retval = 0;
  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "\n%s->Entering:%s", tst_id, ct));

  TST_LOG( tc, (logline, "%s->Checking %s configuration", tst_id, pevx_board_name()));
  TST_LOG( tc, (logline, "\n%sDriver         = %s                           -> OK", ident,  pevx_get_driver_version()));
  TST_LOG( tc, (logline, "\n%sLibrary        = %s                           -> OK", ident,  pevx_get_lib_version()));

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "\n%s->Exiting:%s", tst_id, ct));
  return( retval | TST_STS_DONE);
}

int  
tst_01( struct tst_ctl *tc)
{
  return( tst_config( tc, 0, "Tst:01"));
}

static uint shm_off, size;

int  
tst_shm( struct tst_ctl *tc,
	 int mode,
	 char *tst_id)
{
  time_t tm;
  char *ct;
  struct pev_ioctl_map_pg ms;
  void *usr_addr, *err_addr;
  int i;
  int crate;
  int retval;

  retval = 0;
  crate = tc->xt->pev_para.crate;

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));

  /* create an address translation window in the PCIe End Point */
  /* pointing at offset in the PEV1100 Shared Memory            */
  ms.rem_addr = 0;
  ms.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_SHM;
  ms.flag = 0x0;
  if( mode & 1)
  {
    ms.sg_id = MAP_MASTER_64;
  }
  else
  {
    ms.sg_id = MAP_MASTER_32;
  }

  ms.size = 0x400000;
  if( pevx_map_alloc( crate, &ms) < 0)
  {
    TST_LOG( tc, (logline, "%s->cannot allocate map for SHM", tst_id));
    retval = TST_STS_ERR;
    goto tst_shm_exit;
  }
  usr_addr = pevx_mmap(  crate, &ms);
  if( usr_addr == MAP_FAILED)
  {
    TST_LOG( tc, (logline, "%s->cannot map SHM in user space", tst_id));
    pevx_map_free(  crate, &ms);
    retval = TST_STS_ERR;
    goto tst_shm_exit;
  }
  i = 1;
  shm_off = 0x0;
  size = ms.size;
  while( shm_off < 0x8000000)
  {
    TST_LOG( tc, (logline, "%s->Executing:%4d %08x:%05x", tst_id, i++, shm_off, size)); 
    tst_cpu_fill( usr_addr, size, 1, shm_off, 4);
    err_addr = tst_cpu_check( usr_addr, size, 1, shm_off, 4);
    if( err_addr)
    {
      TST_LOG( tc, (logline, "->Error at offset %x", (uint)( err_addr - usr_addr) + shm_off));
      retval = TST_STS_ERR;
      break;
    }
    shm_off += size;
    ms.rem_addr = shm_off;
    pevx_map_modify(  crate, &ms);
    if( tst_check_cmd_tstop())
    {
      TST_LOG( tc, (logline, "                  -> Stopped"));
      retval = TST_STS_STOPPED;
      break;
    }
    else
    {
      TST_LOG( tc, (logline, "                  -> OK\r"));
    }
    if( tc->exec_mode & TST_EXEC_FAST)
    {
      if( i > 9) break;
    }
  }

  pevx_munmap( crate, &ms);
  pevx_map_free( crate, &ms);
tst_shm_exit:
  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "\n%s->Exiting:%s", tst_id, ct));
  return( retval | TST_STS_DONE);
}

int  
tst_02( struct tst_ctl *tc)
{
  return( tst_shm( tc, 0, "Tst:02"));
}
int  
tst_03( struct tst_ctl *tc)
{
  return( tst_shm( tc, 1, "Tst:03"));
}

int  
tst_vme( struct tst_ctl *tc,
	 int mode,
	 char *tst_id)
{
  time_t tm;
  char *ct;
  void *usr_addr, *err_addr;
  int i;
  int crate;
  ulong rem_addr;
  struct xprstst *xt;
  int retval;

  retval = 0;
  xt = tc->xt;
  crate = xt->pev_para.crate;
  usr_addr = SHM_CPU_VME_ADDR( xt);

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));

  i = 1;
  shm_off = 0x0;
  size = 0x100000;
  rem_addr = xt->vme_map_shm.rem_addr;
  while( shm_off < 0x8000000)
  {
    TST_LOG( tc, (logline, "%s->Executing:%4d %08x:%05x", tst_id, i++, shm_off, size)); 
    xt->vme_map_shm.rem_addr = shm_off;
    pevx_map_modify(  crate, &xt->vme_map_shm);
    tst_cpu_fill( usr_addr, size, 1, shm_off, 4);
    err_addr = tst_cpu_check( usr_addr, size, 1, shm_off, 4);
    if( err_addr)
    {
      TST_LOG( tc, (logline, "->Error at offset %x", (uint)( err_addr - usr_addr) + shm_off));
      retval = TST_STS_ERR;
      break;
    }
    shm_off += size;
    if( tst_check_cmd_tstop())
    {
      TST_LOG( tc, (logline, "                  ->Stopped"));
      retval = TST_STS_STOPPED;
      break;
    }
    else
    {
      TST_LOG( tc, (logline, "                  ->OK\r"));
    }
    if( tc->exec_mode & TST_EXEC_FAST)
    {
      if( i > 9) break;
    }
  }
  xt->vme_map_shm.rem_addr = rem_addr;
  pevx_map_modify(  crate, &xt->vme_map_shm);

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "\n%s->Exiting:%s", tst_id, ct));
  return( retval  | TST_STS_DONE);
}

int  
tst_04( struct tst_ctl *tc)
{
  return( tst_vme( tc, 0, "Tst:04"));
}


int  
tst_08( struct tst_ctl *tc)
{
  char *tst_id = "Tst:08";
  time_t tm;
  char *ct;
  int i;
  int vect, iack;
  int crate;
  int retval;

  crate = tc->xt->pev_para.crate;
  retval = 0;

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));
  /* clear any pending VME interrupts */
  pevx_csr_wr(  crate, reg_remap->vme_base + 0xc, 0x2000);
  /* clear and enable VME global interrupts */
  pevx_csr_wr(  crate, reg_remap->vme_itc + 0x4, 0x7);
  for( i = 1; i < 8; i++)
  {
    TST_LOG( tc, (logline, "%s->Checking VME IRQ%d ", tst_id, i)); 
    /* unmask IRQ level i */
    pevx_csr_wr(  crate, reg_remap->vme_itc + 0x8, 1<<i);
    usleep(100);
    /* set IRQ level i with vector ii */
    vect = (i<<8) | ((i<<4) | i);
    pevx_csr_wr(  crate, reg_remap->vme_base + 0xc, 0x1000 | vect);
    /* wait 100 usec */
    usleep(100);
    /* get vector fron IACK */
    iack = pevx_csr_rd(  crate, reg_remap->vme_itc);
    if( iack == (0x9000 | vect))
    {
      TST_LOG( tc, (logline, "                              ->OK\n"));
    }
    else
    {
      TST_LOG( tc, (logline, "                              ->NOK: %08x\n", iack));
      retval = TST_STS_ERR;
    }
  }
  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Exiting:%s", tst_id, ct));

  return( retval | TST_STS_DONE);
}

int  
tst_09( struct tst_ctl *tc)
{
  char *tst_id = "Tst:09";
  time_t tm;
  char *ct;
  int i;
  int vect, iack;
  int crate;
  ulong rem_addr;
  ushort mode;
  void *iack_addr;
  struct xprstst *xt;
  int retval;

  retval = 0;
  xt = tc->xt;
  crate = xt->pev_para.crate;

  rem_addr = xt->cpu_map_vme_shm.rem_addr;
  mode = xt->cpu_map_vme_shm.mode;
  iack_addr = SHM_CPU_VME_ADDR( xt);

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));
  /* clear any pending VME interrupts */
  pevx_csr_wr(  crate, reg_remap->vme_base + 0xc, 0x2000);
  /* clear and enable VME global interrupts */
  pevx_csr_wr(  crate, reg_remap->vme_itc + 0x4, 0x3);
  xt->cpu_map_vme_shm.mode = 0x1f03; /* IACK mode */
  xt->cpu_map_vme_shm.rem_addr = 0; /* IACK address */
  for( i = 1; i < 8; i++)
  {
    pevx_map_modify(  crate, &xt->cpu_map_vme_shm);
    TST_LOG( tc, (logline, "%s->Checking VME IRQ%d ", tst_id, i)); 
    /* unmask IRQ level i */
    pevx_csr_wr(  crate, reg_remap->vme_itc + 0x8, 1<<i);
    usleep(100);
    /* set IRQ level i with vector ii */
    vect = (i<<8) | ((i<<4) | i);
    pevx_csr_wr(  crate, reg_remap->vme_base + 0xc, 0x1000 | vect);
    /* wait 100 usec */
    usleep(100);
    /* get level from IACK */
    iack = pevx_csr_rd(  crate, reg_remap->vme_itc);
    /* perform IACK cycle manually */
#ifdef PPC
    iack |= *(short *)(iack_addr+(i<<1)) & 0xff;
#else
    iack |= (*(short *)(iack_addr+(i<<1)) >> 8) & 0xff;
#endif
    if( iack == (0x9000 | vect))
    {
      TST_LOG( tc, (logline, "                              ->OK\n"));
    }
    else
    {
      TST_LOG( tc, (logline, "                              ->NOK: IACK=%08x - CSR=%08x\n", 
		    iack, pevx_csr_rd(  crate, reg_remap->vme_itc + 0x4)));
      retval = TST_STS_ERR;
    }
  }
  xt->cpu_map_vme_shm.rem_addr = rem_addr;
  xt->cpu_map_vme_shm.mode = mode;
  pevx_map_modify(  crate, &xt->cpu_map_vme_shm);

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Exiting:%s", tst_id, ct));
  return( retval | TST_STS_DONE);
}



