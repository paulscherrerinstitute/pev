/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : tst_1x.c
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
 * $Log: tst_1x.c,v $
 * Revision 1.1  2012/03/15 14:50:11  kalantari
 * added exact copy of tosca-driver_4.04 from afs
 *
 * Revision 1.2  2010/06/11 11:56:01  ioxos
 * add test status report [JFG]
 *
 * Revision 1.1  2009/12/02 15:09:39  ioxos
 * first checkin [JFG]
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
#include <pevxulib.h>

#include "xprstst.h"
#include "tstlib.h"
#include "tstxlib.h"

static struct xprstst *xt;
static void *shm_cpu_base;
static void *shm_cpu_vme_base;
static void *kbuf_cpu_base;
static void *kbuf_cpu_vme_base;
static void *ref_buf_1, *ref_buf_2;
static uint kbuf_off, shm_off, size;

int
dma_shm_kbuf( struct tst_ctl *tc,
	      int mode,
	      char *tst_id)
{
  int err, data, ref;
  int crate;
  int retval;

  retval = 0;
  crate = tc->xt->pev_para.crate;

  /* fill shm with background pattern */
  tst_cpu_copy( shm_cpu_base, ref_buf_2, 0x100000, 4);

  /* move data from kbuf to shm       */
  err = tstx_dma_move_shm_pci( crate, SHM_DMA_ADDR( xt)+shm_off, KBUF_DMA_ADDR( xt)+kbuf_off, size, mode);
  if( err < 0)
  {
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:DMA\n"));
    retval = TST_STS_ERR;
    goto dma_shm_kbuf_end;
  }
  if( mode == TST_DMA_NOWAIT)
  {
    usleep( 10000);
  }

  /* check data in shm before shm_off                     */
  err = tst_cpu_cmp( shm_cpu_base, ref_buf_2, shm_off, 4);
  if( err != shm_off)
  {
    tst_get_cmp_err( &data, &ref, 4);
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:SHM compare error at offset 0x%x [0x%08x!=0x%08x]", tst_id, err, data, ref));
    retval = TST_STS_ERR;
    goto dma_shm_kbuf_end;
  }
  /* check data in shm from shm_off to shm_off+size       */
  err = tst_cpu_cmp( shm_cpu_base + shm_off, ref_buf_1 + kbuf_off, size, 4);
  if( err != size)
  {
    tst_get_cmp_err( &data, &ref, 4);
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:SHM compare error at offset 0x%x [0x%08x!=0x%08x]", tst_id, shm_off + err, data, ref));
    retval = TST_STS_ERR;
    goto dma_shm_kbuf_end;
  }
  /* check data in shm after shm_off+size                 */
  err = tst_cpu_cmp( shm_cpu_base + shm_off + size, ref_buf_2 + shm_off + size, 0x1000, 4);
  if( err != 0x1000)
  {
    tst_get_cmp_err( &data, &ref, 4);
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:SHM compare error at offset 0x%x [0x%08x!=0x%08x]", tst_id, shm_off + size + err, data, ref));
    retval = TST_STS_ERR;
    goto dma_shm_kbuf_end;
  }
  if( tst_check_cmd_tstop())
  {
    TST_LOG( tc, (logline, "->Stopped", tst_id));
    retval = TST_STS_STOPPED;
  }
dma_shm_kbuf_end:
  return( retval | TST_STS_DONE);
}

int  
tst_dma_shm_kbuf( struct tst_ctl *tc,
		  int mode,
		  char *tst_id)
{
  time_t tm;
  char *ct;
  int i;
  int retval;

  xt = tc->xt;
  shm_cpu_base = SHM_CPU_ADDR( xt);
  shm_cpu_vme_base = SHM_CPU_VME_ADDR( xt);
  kbuf_cpu_base = KBUF_CPU_ADDR( xt);
  kbuf_cpu_vme_base = KBUF_CPU_VME_ADDR( xt);

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));
  /* allocate two reference buffers   */
  ref_buf_1 = (void *)malloc( 0x100000);
  ref_buf_2 = (void *)malloc( 0x100000);
  /* fill ref_buf with test pattern   */
  tst_cpu_fill( ref_buf_1, 0x100000, 1, 0x11223344, 0x11111111);
  tst_cpu_fill( ref_buf_2, 0x100000, 1, 0xdeadface, 0);

  /* copy ref_buf_1 to kbuf     */
  tst_cpu_copy( kbuf_cpu_base, ref_buf_1, 0x100000, 4);

  for(  i = 0; i < 0x1000; i++)
  {
    kbuf_off = 0x1000 + ((i&0xf)<<3);
    shm_off = 0x1000 + ((i&0xf0)>>1);
    size = 0x10000 + ((i&0xf00)>>5);
    TST_LOG( tc, (logline, "%s->Executing:%4d %05x:%05x:%05x", tst_id, i+1, kbuf_off, shm_off, size)); 
    retval = dma_shm_kbuf( tc, mode, tst_id);
    if( retval & (TST_STS_ERR | TST_STS_STOPPED))
    {
      break;
    }
    TST_LOG( tc, (logline, "->OK\r"));
  }
  free( ref_buf_1);
  free( ref_buf_2);

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "\n%s->Exiting:%s", tst_id, ct));
  return( retval);
}

int  
tst_11( struct tst_ctl *tc)
{
  return( tst_dma_shm_kbuf( tc, TST_DMA_NOWAIT, "Tst:11"));
}

int  
tst_12( struct tst_ctl *tc)
{
  return( tst_dma_shm_kbuf( tc, TST_DMA_POLL, "Tst:12"));
}

int  
tst_13( struct tst_ctl *tc)
{
  return( tst_dma_shm_kbuf( tc, TST_DMA_INTR, "Tst:13"));
}


int 
dma_kbuf_shm( struct tst_ctl *tc,
	      int mode,
	      char *tst_id)
{
  int err, data, ref;
  int crate;
  int retval;

  retval = 0;
  crate = tc->xt->pev_para.crate;

  /* fill kbuf with background pattern */
  tst_cpu_copy( kbuf_cpu_base, ref_buf_1, 0x100000, 4);

  /* move data from kbuf to shm       */
  err = tstx_dma_move_pci_shm( crate, KBUF_DMA_ADDR( xt)+kbuf_off, SHM_DMA_ADDR( xt)+shm_off, size, mode);
  if( err < 0)
  {
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:DMA\n", tst_id));
    retval = TST_STS_ERR;
    goto dma_kbuf_shm_end;
  }
  if( mode == TST_DMA_NOWAIT)
  {
    usleep( 10000);
  }

  /* check data in kbuf before kbuf_off                     */
  err = tst_cpu_cmp( kbuf_cpu_base, ref_buf_1, kbuf_off, 4);
  if( err != kbuf_off)
  {
    tst_get_cmp_err( &data, &ref, 4);
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:KBUF compare error at offset 0x%x [0x%08x!=0x%08x]", tst_id, err, data, ref));
    retval = TST_STS_ERR;
    goto dma_kbuf_shm_end;
  }
  /* check data in kbuf from kbuf_off to kbuf_off+size       */
  err = tst_cpu_cmp( kbuf_cpu_base + kbuf_off, ref_buf_2 + shm_off, size, 4);
  if( err != size)
  {
    tst_get_cmp_err( &data, &ref, 4);
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:KBUF compare error at offset 0x%x [0x%08x!=0x%08x]", tst_id, kbuf_off + err, data, ref));
    retval = TST_STS_ERR;
    goto dma_kbuf_shm_end;
  }
  /* check data in kbuf after kbuf_off+size                 */
  err = tst_cpu_cmp( kbuf_cpu_base + kbuf_off + size, ref_buf_1 + kbuf_off + size, 0x100000 - kbuf_off - size, 4);
  if( err != 0x100000 - kbuf_off - size)
  {
    tst_get_cmp_err( &data, &ref, 4);
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:KBUF compare error at offset 0x%x [0x%08x!=0x%08x]", tst_id, kbuf_off + size + err, data, ref));
    retval = TST_STS_ERR;
    goto dma_kbuf_shm_end;
  }
  if( tst_check_cmd_tstop())
  {
    TST_LOG( tc, (logline, "->Stopped"));
    retval = TST_STS_STOPPED;
  }
dma_kbuf_shm_end:
  return( retval | TST_STS_DONE);
}

int  
tst_dma_kbuf_shm( struct tst_ctl *tc,
		  int mode,
		  char *tst_id)
{
  time_t tm;
  char *ct;
  int i;
  int retval;

  xt = tc->xt;
  shm_cpu_base = SHM_CPU_ADDR( xt);
  shm_cpu_vme_base = SHM_CPU_VME_ADDR( xt);
  kbuf_cpu_base = KBUF_CPU_ADDR( xt);
  kbuf_cpu_vme_base = KBUF_CPU_VME_ADDR( xt);

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));
 
  /* allocate two reference buffers   */
  ref_buf_1 = (void *)malloc( 0x100000);
  ref_buf_2 = (void *)malloc( 0x100000);
  /* fill ref_buf with test pattern   */
  tst_cpu_fill( ref_buf_1, 0x100000, 1, 0xdeadface, 0);
  tst_cpu_fill( ref_buf_2, 0x100000, 1, 0x11223344, 0x11111111);

  /* copy ref_buf_2 to shm     */
  tst_cpu_copy( shm_cpu_base, ref_buf_2, 0x100000, 4);

  for(  i = 0; i < 0x1000; i++)
  {
    kbuf_off = 0x1000 + ((i&0xf)<<3);
    shm_off = 0x1000 + ((i&0xf0)>>1);
    size = 0x10000 + ((i&0xf00)>>5);
    TST_LOG( tc, (logline, "%s->Executing:%4d %05x:%05x:%05x", tst_id, i++, kbuf_off, shm_off, size));
    retval = dma_kbuf_shm( tc, mode, tst_id);
    if( retval & (TST_STS_ERR | TST_STS_STOPPED))
    {
      break;
    }
    TST_LOG( tc, (logline, "->OK\r"));
  }
  free( ref_buf_1);
  free( ref_buf_2);

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "\n%s->Exiting:%s", tst_id, ct));
  return( retval);
}

int  
tst_18( struct tst_ctl *tc)
{
  return( tst_dma_kbuf_shm( tc, TST_DMA_NOWAIT, "Tst:18"));
}

int  
tst_19( struct tst_ctl *tc)
{
  return( tst_dma_kbuf_shm( tc, TST_DMA_POLL, "Tst:19"));
}

int  
tst_1a( struct tst_ctl *tc)
{
  return( tst_dma_kbuf_shm( tc, TST_DMA_INTR, "Tst:1a"));
}


