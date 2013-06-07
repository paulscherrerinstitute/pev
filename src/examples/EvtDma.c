/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : template.c
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
 * $Log: EvtDma.c,v $
 * Revision 1.1  2013/06/07 15:01:17  zimoch
 * update to latest version
 *
 * Revision 1.1  2012/10/09 14:09:28  ioxos
 * first checkin [JFG]
 *
 * Revision 1.4  2012/07/10 09:49:07  ioxos
 * check 16 sources from user area [JFG]
 *
 * Revision 1.3  2012/06/28 13:41:45  ioxos
 * use USR1 as interrupt sources [JFG]
 *
 * Revision 1.2  2012/06/01 14:00:14  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.1  2012/05/23 15:17:10  ioxos
 * first checkin [JFG]
 *
 * Revision 1.1  2009/01/08 08:19:03  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <pevioctl.h>
#include <pevulib.h>

struct pev_node *pev;
typedef void (*sighandler_t)(int);
int ivec[256];
int icnt[256];
int mtm[256];
int dma_ret[256];
struct pev_time tm;
int tmi;

int evt_sig, evt_cnt;
struct pev_ioctl_evt *evt;
struct pev_ioctl_dma_req dma_req;
struct pev_ioctl_dma_sts dma_sts;
struct pev_ioctl_buf dma_buf;
int *kbuf_loc_addr;

void
 myhandler( int sig)
{
  int cnt;
  do
  {
    //pev_evt_read( evt, 0);
    //pev_evt_read( evt, -1);
    usleep(10);
    evt->evt_cnt = 0;
    evt->src_id = 1;
    evt->vec_id = 0;
    cnt = evt->evt_cnt;
    if( evt->src_id)
    {
      //usleep(1000);
      ivec[evt_cnt&0xff] = (evt->src_id << 8) | evt->vec_id;
      icnt[evt_cnt&0xff] = cnt;
      pev_timer_read( &tm);
      mtm[evt_cnt&0xff] = tm.time - tmi;

      dma_req.src_addr = 0x10000;                   
      dma_req.des_addr = (ulong)dma_buf.b_addr;      
      dma_req.size = 0x1000 | DMA_SIZE_PKT_1K;                  
      dma_req.src_space = DMA_SPACE_USR1;
      dma_req.des_space = DMA_SPACE_PCIE;
      dma_req.src_mode = DMA_PCIE_RR2;
      dma_req.des_mode = DMA_PCIE_RR2;
      dma_req.start_mode = DMA_MODE_BLOCK;
      dma_req.end_mode = 0;
      dma_req.intr_mode = DMA_INTR_ENA;
      dma_req.wait_mode = DMA_WAIT_INTR | DMA_WAIT_1MS | (5<<4);

      dma_req.dma_status = 0;
      pev_dma_move(&dma_req);
      if( dma_req.dma_status != 0x20000115)
      {
	printf("DMA error : %08x\n", dma_req.dma_status);
	evt_sig = 0;
	break;
      }
      dma_req.dma_status = 0;
      pev_dma_move(&dma_req);
      if( dma_req.dma_status != 0x20000115)
      {
	printf("DMA error : %08x\n", dma_req.dma_status);
	evt_sig = 0;
	break;
      }
      dma_req.dma_status = 0;
      pev_dma_move(&dma_req);
      if( dma_req.dma_status != 0x20000115)
      {
	printf("DMA error : %08x\n", dma_req.dma_status);
	evt_sig = 0;
	break;
      }
      dma_ret[evt_cnt&0xff] = dma_req.dma_status;
      evt_cnt++;


      //printf("%x - %x - %d - %d - %d\n", evt->src_id, evt->vec_id, evt->evt_cnt, evt_cnt, cnt);
      pev_evt_unmask( evt, evt->src_id);
      //if( evt->vec_id == 0xff) evt_sig = 0;
      //if( evt->src_id == 0x4f) evt_sig = 0;
      //if( evt_cnt > 500) evt_sig = 0;
      if( kbuf_loc_addr[0]) evt_sig = 0;
    }
    else
    {
      printf("evt queue empty...%d - %d\n", evt_cnt, cnt);
    }
  } while( cnt);
  return;
}



int
main( int argc,
      char **argv)
{
  int i;
  int src_id;

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
  /* allocate a 1 MBytes buffer in kernel space suitable for DMA */
  dma_buf.size = 0x100000;
  pev_buf_alloc( &dma_buf);
  kbuf_loc_addr = (int *)dma_buf.u_addr;
  printf("DMA buffer kernel address = %p\n", dma_buf.k_addr);
  printf("DMA buffer bus address    = %p\n", dma_buf.b_addr);
  printf("DMA buffer usr address    = %p\n", dma_buf.u_addr);

  //evt = pev_evt_queue_alloc( SIGUSR2);
  evt = pev_evt_queue_alloc( 0);
  //src_id = EVT_SRC_VME;
  src_id = EVT_SRC_USR1;
  for( i = 0; i < 16; i++)
  {
    pev_evt_register( evt, src_id++);
  }
  evt->wait = -1;
  evt_sig = 1;
  evt_cnt = 0;
  //signal( evt->sig, myhandler);
  printf("waiting for signal %d...\n", evt->sig);
  pev_evt_queue_enable( evt);
  pev_timer_read( &tm);
  tmi = tm.time;
  while( evt_sig)
  {
    //usleep(100000);
    myhandler(0);
    printf("%d\r", evt_cnt);
  }

  for( i = 0; i < 16; i++)
  {
    printf("%3d - %02x - %2d - %d : %08x - %08x\n", i, ivec[i], icnt[i], mtm[i], dma_ret[i], kbuf_loc_addr[i]);
  }

  pev_evt_queue_disable( evt);
  pev_evt_queue_free( evt);
  printf("evt_cnt = %d\n", evt_cnt);

  pev_buf_free( &dma_buf);
  pev_exit( pev);

  exit(0);
}
