/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : tst_2x.c
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
 * $Log: tst_2x.c,v $
 * Revision 1.1  2012/03/15 14:50:11  kalantari
 * added exact copy of tosca-driver_4.04 from afs
 *
 * Revision 1.3  2010/06/11 11:56:01  ioxos
 * add test status report [JFG]
 *
 * Revision 1.2  2009/12/15 17:18:07  ioxos
 * modification for short io window + tst_09 [JFG]
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
static void *ref_buf_1, *ref_buf_2;
static uint vme_off, shm_off, size;
static ulong shm_vme_base;

int
dma_shm_vme( struct tst_ctl *tc,
	     int mode,
	     char *tst_id)
{
  int err, data, ref;
  int crate;
  int retval;

  retval = 0;
  crate = tc->xt->pev_para.crate;
  shm_vme_base = xt->vme_map_shm.loc_addr +  (ulong)tstx_vme_conf_read( crate, &xt->vme_conf);

  /* fill shm with background pattern */
  tst_cpu_copy( shm_cpu_base, ref_buf_2, 0x80000, 4);

  /* move data from vme to shm       */
  err = tstx_dma_move_shm_vme( crate, SHM_DMA_ADDR( xt)+shm_off, shm_vme_base+vme_off, mode, size, TST_DMA_INTR);
  if( err < 0)
  {
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:DMA\n"));
    retval = TST_STS_ERR;
    goto dma_shm_vme_end;
  }

  /* check data in shm before shm_off                     */
  err = tst_cpu_cmp( shm_cpu_base, ref_buf_2, shm_off, 4);
  if( err != shm_off)
  {
    tst_get_cmp_err( &data, &ref, 4);
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:SHM compare error at offset 0x%x [0x%08x!=0x%08x]", tst_id, err, data, ref));
    retval = TST_STS_ERR;
    goto dma_shm_vme_end;
  }
  /* check data in shm from shm_off to shm_off+size       */
  err = tst_cpu_cmp( shm_cpu_base + shm_off, ref_buf_1 + vme_off - 0x80000, size, 4);
  if( err != size)
  {
    tst_get_cmp_err( &data, &ref, 4);
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:SHM compare error at offset 0x%x [0x%08x!=0x%08x]", tst_id, shm_off + err, data, ref));
    retval = TST_STS_ERR;
    goto dma_shm_vme_end;
  }
  /* check data in shm after shm_off+size                 */
  err = tst_cpu_cmp( shm_cpu_base + shm_off + size, ref_buf_2 + shm_off + size, 0x1000, 4);
  if( err != 0x1000)
  {
    tst_get_cmp_err( &data, &ref, 4);
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:SHM compare error at offset 0x%x [0x%08x!=0x%08x]", tst_id, shm_off + size + err, data, ref));
    retval = TST_STS_ERR;
    goto dma_shm_vme_end;
  }
  if( tst_check_cmd_tstop())
  {
    TST_LOG( tc, (logline, "->Stopped", tst_id));
    retval = TST_STS_STOPPED;
  }

dma_shm_vme_end:
  return( retval | TST_STS_DONE);
}

int  
tst_dma_shm_vme32( struct tst_ctl *tc,
		   int mode,
		   char *tst_id)
{
  time_t tm;
  char *ct;
  int i, retval;

  xt = tc->xt;
  shm_cpu_base = SHM_CPU_ADDR( xt);
  shm_cpu_vme_base = SHM_CPU_VME_ADDR( xt);

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));
  /* allocate two reference buffers   */
  ref_buf_1 = (void *)malloc( 0x100000);
  ref_buf_2 = (void *)malloc( 0x100000);
  /* fill ref_buf with test pattern   */
  tst_cpu_fill( ref_buf_1, 0x100000, 1, 0x11223344, 0x11111111);
  tst_cpu_fill( ref_buf_2, 0x100000, 1, 0xdeadface, 0);

  /* copy ref_buf_1 to shm     */
  tst_cpu_copy( shm_cpu_base+0x80000, ref_buf_1, 0x80000, 4);

  for(  i = 0; i < 0x1000; i++)
  {
    vme_off = 0x81000 + ((i&0xf)<<2);
    shm_off = 0x1000 + ((i&0xf0)>>2);
    size = 0x10000 + ((i&0xf00)>>6);
    TST_LOG( tc, (logline, "%s->Executing:%4d %05x:%05x:%05x", tst_id, i+1, vme_off, shm_off, size)); 
    retval = dma_shm_vme( tc, mode, tst_id);
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
tst_dma_shm_vme64( struct tst_ctl *tc,
		   int mode,
		   char *tst_id)
{
  time_t tm;
  char *ct;
  int i, retval;

  xt = tc->xt;
  shm_cpu_base = SHM_CPU_ADDR( xt);
  shm_cpu_vme_base = SHM_CPU_VME_ADDR( xt);

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));
  /* allocate two reference buffers   */
  ref_buf_1 = (void *)malloc( 0x100000);
  ref_buf_2 = (void *)malloc( 0x100000);
  /* fill ref_buf with test pattern   */
  tst_cpu_fill( ref_buf_1, 0x100000, 1, 0x11223344, 0x11111111);
  tst_cpu_fill( ref_buf_2, 0x100000, 1, 0xdeadface, 0);

  /* copy ref_buf_1 to shm     */
  tst_cpu_copy( shm_cpu_base+0x80000, ref_buf_1, 0x80000, 4);

  for(  i = 0; i < 0x1000; i++)
  {
    vme_off = 0x81000 + ((i&0xf)<<3);
    shm_off = 0x1000 + ((i&0xf0)>>1);
    size = 0x10000 + ((i&0xf00)>>5);
    TST_LOG( tc, (logline, "%s->Executing:%4d %05x:%05x:%05x", tst_id, i+1, vme_off, shm_off, size)); 
    retval = dma_shm_vme( tc, mode, tst_id);
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
tst_20( struct tst_ctl *tc)
{
  //tst_dma_shm_vme32( tc, 0x30, "Tst:20");
  tst_dma_shm_vme64( tc, 0x30, "Tst:20");
}

int  
tst_21( struct tst_ctl *tc)
{
  //tst_dma_shm_vme32( tc, 0x40, "Tst:21");
  tst_dma_shm_vme64( tc, 0x40, "Tst:21");
}

int  
tst_22( struct tst_ctl *tc)
{
  tst_dma_shm_vme64( tc, 0x50, "Tst:22");
}

int  
tst_23( struct tst_ctl *tc)
{
  tst_dma_shm_vme64( tc, 0x60, "Tst:23");
}

int  
tst_24( struct tst_ctl *tc)
{
  tst_dma_shm_vme64( tc, 0x70, "Tst:24");
}

int  
tst_25( struct tst_ctl *tc)
{
  tst_dma_shm_vme64( tc, 0x80, "Tst:25");
}

int  
tst_26( struct tst_ctl *tc)
{
  tst_dma_shm_vme64( tc, 0x90, "Tst:26");
}

int  
tst_27( struct tst_ctl *tc)
{
  tst_dma_shm_vme64( tc, 0xa0, "Tst:27");
}



int 
dma_vme_shm( struct tst_ctl *tc,
	     int mode,
	     char *tst_id)
{
  int err, data, ref;
  int crate;
  int retval;

  retval = 0;
  crate = tc->xt->pev_para.crate;
  shm_vme_base = xt->vme_map_shm.loc_addr +  (ulong)tstx_vme_conf_read( crate, &xt->vme_conf);

  /* fill vme with background pattern */
  tst_cpu_copy( shm_cpu_base+0x80000, ref_buf_1, 0x80000, 4);

  /* move data from vme to shm       */
  err = tstx_dma_move_vme_shm( crate, shm_vme_base+vme_off, SHM_DMA_ADDR( xt)+shm_off, mode, size, TST_DMA_INTR);
  if( err < 0)
  {
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:DMA\n", tst_id));
    retval = TST_STS_ERR;
    goto dma_vme_shm_end;
  }

  /* check data in VME before vme_off                     */
  err = tst_cpu_cmp( shm_cpu_base+0x80000, ref_buf_1, vme_off-0x80000, 4);
  if( err != vme_off-0x80000)
  {
    tst_get_cmp_err( &data, &ref, 4);
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:VME compare error at offset 0x%x [0x%08x!=0x%08x]", tst_id, err, data, ref));
    retval = TST_STS_ERR;
    goto dma_vme_shm_end;
  }
  /* check data in vme from vme_off to vme_off+size       */
  err = tst_cpu_cmp( shm_cpu_base + vme_off, ref_buf_2 + shm_off, size, 4);
  if( err != size)
  {
    tst_get_cmp_err( &data, &ref, 4);
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:VME compare error at offset 0x%x [0x%08x!=0x%08x]", tst_id, vme_off + err, data, ref));
    retval = TST_STS_ERR;
    goto dma_vme_shm_end;
  }
  /* check data in vme after vme_off+size                 */
  //err = tst_cpu_cmp( shm_cpu_base + vme_off + size, ref_buf_1 + vme_off + size -0x80000, 0x100000 - vme_off - size, 4);
  err = tst_cpu_cmp( shm_cpu_base + vme_off + size, ref_buf_1 + vme_off + size -0x80000, 0x1000, 4);
  //if( err != 0x100000 - vme_off - size)
  if( err != 0x1000)
  {
    tst_get_cmp_err( &data, &ref, 4);
    TST_LOG( tc, (logline, "->NOK\n%s->ERROR:VME compare error at offset 0x%x [0x%08x!=0x%08x]", tst_id, vme_off + size + err, data, ref));
    retval = TST_STS_ERR;
    goto dma_vme_shm_end;
  }
  if( tst_check_cmd_tstop())
  {
    TST_LOG( tc, (logline, "->Stopped"));
    retval = TST_STS_STOPPED;
  }
dma_vme_shm_end:
  return( retval | TST_STS_DONE);
}

int  
tst_dma_vme_shm32( struct tst_ctl *tc,
		   int mode,
		   char *tst_id)
{
  time_t tm;
  char *ct;
  int i, retval;

  xt = tc->xt;
  shm_cpu_base = SHM_CPU_ADDR( xt);
  shm_cpu_vme_base = SHM_CPU_VME_ADDR( xt);

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
  tst_cpu_copy( shm_cpu_base, ref_buf_2, 0x80000, 4);

  for(  i = 0; i < 0x1000; i++)
  {
    vme_off = 0x81000 + ((i&0xf)<<2);
    shm_off = 0x1000 + ((i&0xf0)>>2);
    size = 0x10000 + ((i&0xf00)>>6);
    TST_LOG( tc, (logline, "%s->Executing:%4d %05x:%05x:%05x", tst_id, i++, vme_off, shm_off, size));
    retval = dma_vme_shm( tc, mode, tst_id);
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
tst_dma_vme_shm64( struct tst_ctl *tc,
		   int mode,
		   char *tst_id)
{
  time_t tm;
  char *ct;
  int i, retval;

  xt = tc->xt;
  shm_cpu_base = SHM_CPU_ADDR( xt);
  shm_cpu_vme_base = SHM_CPU_VME_ADDR( xt);

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
  tst_cpu_copy( shm_cpu_base, ref_buf_2, 0x80000, 4);

  for(  i = 0; i < 0x1000; i++)
  {
    vme_off = 0x81000 + ((i&0xf)<<3);
    shm_off = 0x1000 + ((i&0xf0)>>1);
    size = 0x10000 + ((i&0xf00)>>5);
    TST_LOG( tc, (logline, "%s->Executing:%4d %05x:%05x:%05x", tst_id, i++, vme_off, shm_off, size));
    retval = dma_vme_shm( tc, mode, tst_id);
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
tst_28( struct tst_ctl *tc)
{
  //tst_dma_vme_shm32( tc, 0x30, "Tst:28");
  return( tst_dma_vme_shm64( tc, 0x30, "Tst:28"));
}

int  
tst_29( struct tst_ctl *tc)
{
  //tst_dma_vme_shm32( tc, 0x40, "Tst:29");
  return( tst_dma_vme_shm64( tc, 0x40, "Tst:29"));
}

int  
tst_2a( struct tst_ctl *tc)
{
  return( tst_dma_vme_shm64( tc, 0x50, "Tst:2a"));
}

int  
tst_2b( struct tst_ctl *tc)
{
  return( tst_dma_vme_shm64( tc, 0x60, "Tst:2b"));
}

int  
tst_2c( struct tst_ctl *tc)
{
  return( tst_dma_vme_shm64( tc, 0x70, "Tst:2c"));
}

int  
tst_2d( struct tst_ctl *tc)
{
  return( tst_dma_vme_shm64( tc, 0x80, "Tst:2d"));
}

int  
tst_2e( struct tst_ctl *tc)
{
  return( tst_dma_vme_shm64( tc, 0x90, "Tst:2e"));
}

int  
tst_2f( struct tst_ctl *tc)
{
  return( tst_dma_vme_shm64( tc, 0xa0, "Tst:2f"));
}


