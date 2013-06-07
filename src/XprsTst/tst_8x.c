/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : tst_8x.c
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

extern int tst_check_cmd_tstop(void);

extern struct pev_reg_remap *reg_remap;
static char *ident="        ";

struct pev_i2c_devices i2c_devices_ifc[] =
{
  { "max5970",  0x45000030},
  { "bmr463_0", 0x45000053},
  { "bmr463_1", 0x4500005b},
  { "bmr463_2", 0x45000063},
  { "bmr463_3", 0x45000024},
  { "lm95255_1",0x0400004c},
  { "lm95255_2",0x0400001c},
  { "idt8n4q01",0xe500006e},
  { "pes32nt",  0xc5000075},
  { "plx8624",  0x010f0069},
  { "vmep0",    0x64000000},
  { "fmc1",     0x84000000},
  { "fmc2",     0xa4000000},
  { NULL,       0x00000000}
};

#define MAX5970    0x45000030;
#define BMR463_0   0x45000053
#define BMR463_1   0x4500005b
#define BMR463_2   0x45000063
#define BMR463_3   0x45000024
#define LM95255_1  0x0400004c
#define LM95255_2  0x0400001c
#define IDT8N4Q01  0xe500006e
#define PES32NT    0xc5000075
#define VMEP0      0x64000000
#define FMC1       0x84000000
#define FMC2       0xa4000000
#define I2C_ELB    0x00000080


int  
mytst_yyy( struct tst_ctl *tc,
	 int mode,
	 char *tst_id)
{
  int retval = 0;
  time_t tm;
  char *ct;
  int i;
  char **para_p;
  int board_id;

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));

  /*---->
  insert your test here...

  <----*/
  if( ( board_id = pevx_board()) != PEV_BOARD_IFC1210)
  {
    TST_LOG( tc, (logline, "%s-> wrong board type : %08x expected %08x               -> NOK", tst_id, board_id, PEV_BOARD_IFC1210));
    goto mytst_xxx_exit;
  }

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

mytst_xxx_exit:
  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "\n%s->Exiting:%s", tst_id, ct));
  return( retval | TST_STS_DONE);
}


int  
tst_80( struct tst_ctl *tc)
{
  return( mytst_yyy( tc, 0, "Tst:80"));
}

int  
tst_lm95255( struct tst_ctl *tc,
	     int mode,
	     char *tst_id)
{
  int retval = 0;
  time_t tm;
  char *ct;
  int board_id;
  struct pev_ioctl_i2c i2c;
  int crate;

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));

  /*---->
  insert your test here...

  <----*/
  if( ( board_id = pevx_board()) != PEV_BOARD_IFC1210)
  {
    TST_LOG( tc, (logline, "%s-> wrong board type : %08x expected %08x               -> NOK", tst_id, board_id, PEV_BOARD_IFC1210));
    goto tst_lm95255_exit;
  }

  TST_LOG( tc, (logline, "%s->Checking On Board Thermometers (I2C Serial Bus)", tst_id));

  crate = tc->xt->pev_para.crate;

  /* select sLM95255#1 */
  i2c.device = LM95255_1 | I2C_ELB;
  i2c.cmd = 0xfe;
  i2c.status = pevx_i2c_read( crate, i2c.device, i2c.cmd, &i2c.data);
  usleep(10000);
  TST_LOG( tc, (logline, "\n%sLM95255#1 Manufacter = %2x                    -> ", ident, i2c.data));
  if( i2c.data != 0x01)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
    goto tst_lm95255_exit;
  }
  TST_LOG( tc, (logline, "OK"));
  i2c.cmd = 0xff;
  i2c.status = pevx_i2c_read( crate, i2c.device, i2c.cmd, &i2c.data);
  usleep(10000);
  TST_LOG( tc, (logline, "\n%sLM95255#1 Revision = %2x                      -> ", ident, i2c.data));
  TST_LOG( tc, (logline, "DONE"));
  i2c.cmd = 0x0;
  i2c.status = pevx_i2c_read( crate, i2c.device, i2c.cmd, &i2c.data);
  usleep(10000);
  TST_LOG( tc, (logline, "\n%sLM95255#1 current temperature = %3d°         -> ", ident, i2c.data));
  TST_LOG( tc, (logline, "DONE"));

  /* select sLM95255#2 */
  i2c.device = LM95255_2 | I2C_ELB;
  i2c.cmd = 0xfe;
  i2c.status = pevx_i2c_read( crate, i2c.device, i2c.cmd, &i2c.data);
  usleep(10000);
  TST_LOG( tc, (logline, "\n%sLM95255#1 Manufacter = %2x                    -> ", ident, i2c.data));
  if( i2c.data != 0x01)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
    goto tst_lm95255_exit;
  }
  TST_LOG( tc, (logline, "OK"));
  i2c.cmd = 0xff;
  i2c.status = pevx_i2c_read( crate, i2c.device, i2c.cmd, &i2c.data);
  usleep(10000);
  TST_LOG( tc, (logline, "\n%sLM95255#1 Revision = %2x                      -> ", ident, i2c.data));
  TST_LOG( tc, (logline, "DONE"));
  i2c.cmd = 0x0;
  i2c.status = pevx_i2c_read( crate, i2c.device, i2c.cmd, &i2c.data);
  usleep(10000);
  TST_LOG( tc, (logline, "\n%sLM95255#1 current temperature = %3d°         -> ", ident, i2c.data));
  TST_LOG( tc, (logline, "DONE"));

tst_lm95255_exit:
  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "\n%s->Exiting:%s", tst_id, ct));
  return( retval | TST_STS_DONE);
}

int  
tst_81( struct tst_ctl *tc)
{
  return( tst_lm95255( tc, 0, "Tst:81"));
}

int  
tst_82( struct tst_ctl *tc)
{
  return( tst_lm95255( tc, 0, "Tst:82"));
}

int  
tst_83( struct tst_ctl *tc)
{
  return( mytst_yyy( tc, 0, "Tst:83"));
}

int  
tst_84( struct tst_ctl *tc)
{
  return( mytst_yyy( tc, 0, "Tst:84"));
}
int  
tst_pmc_jn14( struct tst_ctl *tc,
	      int mode,
	      char *tst_id)
{
  int retval;
  time_t tm;
  char *ct;
  int board_id, crate, data;

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));

  /*---->
  insert your test here...

  <----*/
  if( ( board_id = pevx_board()) != PEV_BOARD_IFC1210)
  {
    TST_LOG( tc, (logline, "%s-> wrong board type : %08x expected %08x               -> NOK", tst_id, board_id, PEV_BOARD_IFC1210));
    goto tst_pmc_jn14_exit;
  }

  TST_LOG( tc, (logline, "%s->Checking PMC JN14 Interface", tst_id));

  crate = tc->xt->pev_para.crate;

  /* Enable V6-S6 Interface */

  data = pevx_csr_rd( crate, 0x80001000);
  TST_LOG( tc, (logline, "\n%sUSER register = %08x                   -> ", ident, data));
  if( data != 0x12340002)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
    goto tst_pmc_jn14_exit;
  }
  TST_LOG( tc, (logline, "OK"));
  pevx_csr_wr( crate, 0x80001018, 0);
  pevx_csr_wr( crate, 0x80001014, 0);
  pevx_csr_wr( crate, 0x80001024, 0);
  pevx_csr_wr( crate, 0x80001020, 0);
  usleep( 10000);
  pevx_csr_wr( crate, 0x80001090, 0x8);
  usleep( 10000);
  pevx_csr_wr( crate, 0x80001090, 0x9);
  usleep( 10000);
  pevx_csr_wr( crate, 0x80001090, 0xa);
  usleep( 10000);
  data = pevx_csr_rd( crate, 0x80001090);
  TST_LOG( tc, (logline, "\n%sV6-S6 PLL Enable = %08x                -> ", ident, data));
  if( (data & 0xf000) != 0x4000)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
    goto tst_pmc_jn14_exit;
  }
  TST_LOG( tc, (logline, "OK"));
  pevx_csr_wr( crate, 0x80001090, 0xb);
  usleep( 10000);
  data = pevx_csr_rd( crate, 0x80001090);
  TST_LOG( tc, (logline, "\n%sV6-S6 Interface Enable = %08x          -> ", ident, data));
  if( (data & 0xf000) != 0x8000)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
    goto tst_pmc_jn14_exit;
  }
  TST_LOG( tc, (logline, "OK"));

  /* Checkin PMC JN14 */
  pevx_csr_wr( crate, 0x80001018, 0x55555555);
  pevx_csr_wr( crate, 0x80001014, 0x00000000);
  data = pevx_csr_rd( crate, 0x80001010);
  TST_LOG( tc, (logline, "\n%sPMC JN14 1-32 ODD: OFF = %08x          -> ", ident, data));
  if( data)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  pevx_csr_wr( crate, 0x80001014, 0x55555555);
  data = pevx_csr_rd( crate, 0x80001010);
  TST_LOG( tc, (logline, "\n%sPMC JN14 1-32 ODD: ON = %08x           -> ", ident, data));
  if( data != 0xffffffff)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  pevx_csr_wr( crate, 0x80001018, 0xaaaaaaaa);
  pevx_csr_wr( crate, 0x80001014, 0x00000000);
  data = pevx_csr_rd( crate, 0x80001010);
  TST_LOG( tc, (logline, "\n%sPMC JN14 1-32 EVEN: OFF = %08x         -> ", ident, data));
  if( data)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  pevx_csr_wr( crate, 0x80001014, 0xaaaaaaaa);
  data = pevx_csr_rd( crate, 0x80001010);
  TST_LOG( tc, (logline, "\n%sPMC JN14 1-32 EVEN: ON = %08x          -> ", ident, data));
  if( data != 0xffffffff)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  /* Checkin PMC JN14 */
  pevx_csr_wr( crate, 0x80001024, 0x55555555);
  pevx_csr_wr( crate, 0x80001020, 0x00000000);
  data = pevx_csr_rd( crate, 0x8000101c);
  TST_LOG( tc, (logline, "\n%sPMC JN14 33-64 ODD: OFF = %08x         -> ", ident, data));
  if( data)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  pevx_csr_wr( crate, 0x80001020, 0x55555555);
  data = pevx_csr_rd( crate, 0x8000101c);
  TST_LOG( tc, (logline, "\n%sPMC JN14 33-64 ODD: ON = %08x          -> ", ident, data));
  if( data != 0xffffffff)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }
  pevx_csr_wr( crate, 0x80001024, 0xaaaaaaaa);
  pevx_csr_wr( crate, 0x80001020, 0x00000000);
  data = pevx_csr_rd( crate, 0x8000101c);
  TST_LOG( tc, (logline, "\n%sPMC JN14 33-64 EVEN: OFF = %08x        -> ", ident, data));
  if( data)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  pevx_csr_wr( crate, 0x80001020, 0xaaaaaaaa);
  data = pevx_csr_rd( crate, 0x8000101c);
  TST_LOG( tc, (logline, "\n%sPMC JN14 33-64 EVEN: ON = %08x         -> ", ident, data));
  if( data != 0xffffffff)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

tst_pmc_jn14_exit:
  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "\n%s->Exiting:%s", tst_id, ct));
  return( retval | TST_STS_DONE);
}

int  
tst_85( struct tst_ctl *tc)
{
  return( tst_pmc_jn14( tc, 0, "Tst:85"));
}

int  
tst_fmc( struct tst_ctl *tc,
	 int mode,
	 char *tst_id)
{
  int retval;
  time_t tm;
  char *ct;
  int i, iex;
  int board_id, crate, data, res;

  retval = 0;
  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));

  /*---->
  insert your test here...

  <----*/
  if( ( board_id = pevx_board()) != PEV_BOARD_IFC1210)
  {
    TST_LOG( tc, (logline, "%s-> wrong board type : %08x expected %08x               -> NOK", tst_id, board_id, PEV_BOARD_IFC1210));
    retval = TST_STS_ERR;
    goto tst_fmc_exit;
  }

  crate = tc->xt->pev_para.crate;

  /* Check XMC LEDs */
  TST_LOG( tc, (logline, "%s->Checking XMC LEDs\n", tst_id));
  pevx_csr_wr( crate, 0x80001060, 0x100000);
  pevx_csr_wr( crate, 0x80001460, 0x100000);
  TST_LOG( tc, (logline, "%s  OFF                                    -> ", tst_id));
  sleep(1);
  TST_LOG( tc, (logline, "DONE\n%s  RED                                    -> ", tst_id));
  pevx_csr_wr( crate, 0x80001060, 0x110000);
  pevx_csr_wr( crate, 0x80001460, 0x110000);
  sleep(1);
  TST_LOG( tc, (logline, "DONE\n%s  GREEN                                  -> ", tst_id));
  pevx_csr_wr( crate, 0x80001060, 0x120000);
  pevx_csr_wr( crate, 0x80001460, 0x120000);
  sleep(1);
  TST_LOG( tc, (logline, "DONE\n%s  ORANGE                                 -> ", tst_id));
  pevx_csr_wr( crate, 0x80001060, 0x130000);
  pevx_csr_wr( crate, 0x80001460, 0x130000);
  sleep(1);
  pevx_csr_wr( crate, 0x80001460, 0x100000);
  pevx_csr_wr( crate, 0x80001060, 0x100000);
  TST_LOG( tc, (logline, "DONE\n"));

  /* Enable FMC Power Supply */

  data = pevx_elb_rd( crate, 0xc);
  data &= 0x3fffffff;
  TST_LOG( tc, (logline, "%s->Reload FPGA -> takes 10 seconds ", tst_id));
  pevx_elb_wr( crate, 0xc, 0x80000000 | data);
  usleep( 200000);
  pevx_elb_wr( crate, 0xc, 0xc0000000 | data);
  for( i = 1; i < 10; i++)
  {
    sleep( 1);
    printf("%d\b",i); fflush(stdout);
  }
  printf("%d",i);
  TST_LOG( tc, (logline, "     -> DONE\n "));

  TST_LOG( tc, (logline, "%s->Checking FMC LEDs\n", tst_id));

  /* Configure FMC test lines */
  pevx_csr_wr( crate, 0x80001058, 0xff000000); /* set LA[31:24]=out LA[23:0]=in              */
  pevx_csr_wr( crate, 0x80001458, 0xff000000); /* set LA[31:24]=out LA[23:0]=in              */
  pevx_csr_wr( crate, 0x8000105c, 0xffffffff); /* set LA[33:32]=out HA[23:0]=out HB[5:0]=out */
  pevx_csr_wr( crate, 0x8000145c, 0xffffffff); /* set LA[33:32]=out HA[23:0]=out HB[5:0]=out */
  pevx_csr_wr( crate, 0x80001060, 0x10ffff);   /* set HB[21:6]=out enable FMC CLK1           */
  pevx_csr_wr( crate, 0x80001460, 0x10ffff);   /* set HB[21:6]=out enable FMC CLK1           */
  usleep( 10000);


  /* Check FMC LEDs */
  pevx_csr_wr( crate, 0x80001040, 0);
  pevx_csr_wr( crate, 0x80001440, 0);
  TST_LOG( tc, (logline, "%s  OFF                                    -> ", tst_id));
  sleep(1);
  TST_LOG( tc, (logline, "DONE\n%s  RED                                    -> ", tst_id));
  pevx_csr_wr( crate, 0x80001040, 0x14000000);
  pevx_csr_wr( crate, 0x80001440, 0x14000000);
  sleep(1);
  TST_LOG( tc, (logline, "DONE\n%s  GREEN                                  -> ", tst_id));
  pevx_csr_wr( crate, 0x80001040, 0x28000000);
  pevx_csr_wr( crate, 0x80001440, 0x28000000);
  sleep(1);
  TST_LOG( tc, (logline, "DONE\n%s  ORANGE                                 -> ", tst_id));
  pevx_csr_wr( crate, 0x80001040, 0x3c000000);
  pevx_csr_wr( crate, 0x80001440, 0x3c000000);
  sleep(1);
  TST_LOG( tc, (logline, "DONE\n"));
  pevx_csr_wr( crate, 0x80001040, 0);
  pevx_csr_wr( crate, 0x80001440, 0);

  /* Check FMC#1 HA */
  TST_LOG( tc, (logline, "%s->Checking FMC#1 Lines HA[23:0]", tst_id));
  pevx_csr_wr( crate, 0x80001040, 0); /* set mode: LA[23:0] = HA[23:0] */
  iex = 0;
  for( i = 0; i < 24; i++)
  {
    data = 1 << i;
    pevx_csr_wr( crate, 0x80001044, (data << 2) | 0x2);  /* set HA[23:0] + mode: LA[23:0] = HA[23:0] */
    usleep( 10);
    res = pevx_csr_rd( crate, 0x8000104c);               /* read LA[23:0] */
    if( data != (res & 0xffffff))
    {
      TST_LOG( tc, (logline, "          -> NOK: expected=%08x - found=%08x\n", data, res));
      retval = TST_STS_ERR;
      iex = 1;
      break;
    }
  }
  if( !iex)
  {
    TST_LOG( tc, (logline, "          -> OK\n"));
  }

  /* Check FMC#1 HB */
  TST_LOG( tc, (logline, "%s->Checking FMC#1 Lines HB[21:0]", tst_id));
  pevx_csr_wr( crate, 0x80001040, 0x40000000); /* set mode: LA[23:0] = HB[23:0] */
  iex = 0;
  for( i = 0; i < 22; i++)
  {
    int hb1, hb2;
    data = 1 << i;
    hb1 = data << 26;
    hb2 = data >> 6;
    pevx_csr_wr( crate, 0x80001044, hb1 | 0x2);  /* set HB[5:0] + mode: LA[23:0] = HB[23:0] */
    pevx_csr_wr( crate, 0x80001048, hb2 );       /* set HB[21:6]                             */
    usleep( 10);
    res = pevx_csr_rd( crate, 0x8000104c);               /* read LA[23:0] */
    if( data != (res & 0x3fffff))
    {
      TST_LOG( tc, (logline, "          -> NOK: expected=%08x - found=%08x\n", data, res));
      retval = TST_STS_ERR;
      iex = 1;
      break;
    }
  }
  if( !iex)
  {
    TST_LOG( tc, (logline, "          -> OK\n"));
  }

  /* Check FMC HA+HB */
  TST_LOG( tc, (logline, "%s->Checking FMC#1 HA[23:0]+HB[21:0]", tst_id));
  pevx_csr_wr( crate, 0x80001040, 0x80000000); /* set mode: LA[23:0] = HA[23:0]  HB[23:0] */
  iex = 0;
  for( i = 0; i < 22; i++)
  {
    int hb1, hb2, ha;
    data = 1 << i;
    //ha = data << 2;
    ha = (0xffffff & ~(data)) << 2;
    hb1 = data << 26;
    hb2 = data >> 6;
    pevx_csr_wr( crate, 0x80001044, ha | hb1 | 0x2);  /* set HB[5:0] + mode: LA[23:0] = HA[23:0] +  HB[21:0] */
    pevx_csr_wr( crate, 0x80001048, hb2 );            /* set HB[21:6]                             */
    usleep( 10);
    res = pevx_csr_rd( crate, 0x8000104c);               /* read LA[23:0] */
    if( (res&0x3fffff) != 0x3fffff)
    {
      TST_LOG( tc, (logline, "       -> NOK: expected=3fffff - found=%08x\n", res));
      retval = TST_STS_ERR;
      iex = 1;
      break;
    }
  }
  if( !iex)
  {
    TST_LOG( tc, (logline, "       -> OK\n"));
  }


  /* Check FMC#2 HA */
  TST_LOG( tc, (logline, "%s->Checking FMC#2 Lines HA[23:0]", tst_id));
  pevx_csr_wr( crate, 0x80001440, 0); /* set mode: LA[23:0] = HA[23:0] */
  iex = 0;
  for( i = 0; i < 24; i++)
  {
    data = 1 << i;
    pevx_csr_wr( crate, 0x80001444, (data << 2) | 0x2);  /* set HA[23:0] + mode: LA[23:0] = HA[23:0] */
    usleep( 10);
    res = pevx_csr_rd( crate, 0x8000144c);               /* read LA[23:0] */
    if( data != (res & 0xffffff))
    {
      TST_LOG( tc, (logline, "          -> NOK: expected=%08x - found=%08x\n", data, res));
      retval = TST_STS_ERR;
      iex = 1;
      break;
    }
  }
  if( !iex)
  {
    TST_LOG( tc, (logline, "          -> OK\n"));
  }

  /* Check FMC#2 HB */
  TST_LOG( tc, (logline, "%s->Checking FMC#2 Lines HB[21:0]", tst_id));
  pevx_csr_wr( crate, 0x80001440, 0x40000000); /* set mode: LA[23:0] = HB[23:0] */
  iex = 0;
  for( i = 0; i < 22; i++)
  {
    int hb1, hb2;
    data = 1 << i;
    hb1 = data << 26;
    hb2 = data >> 6;
    pevx_csr_wr( crate, 0x80001444, hb1 | 0x2);  /* set HB[5:0] + mode: LA[23:0] = HB[23:0] */
    pevx_csr_wr( crate, 0x80001448, hb2 );       /* set HB[21:6]                             */
    usleep( 10);
    res = pevx_csr_rd( crate, 0x8000144c);               /* read LA[23:0] */
    if( data != (res & 0x3fffff))
    {
      TST_LOG( tc, (logline, "          -> NOK: expected=%08x - found=%08x\n", data, res));
      retval = TST_STS_ERR;
      iex = 1;
      break;
    }
  }
  if( !iex)
  {
    TST_LOG( tc, (logline, "          -> OK\n"));
  }
  /* Check FMC#2 HA+HB */
  TST_LOG( tc, (logline, "%s->Checking FMC#2 HA[23:0]+HB[21:0]", tst_id));
  pevx_csr_wr( crate, 0x80001440, 0x80000000); /* set mode: LA[23:0] = HA[23:0]  HB[23:0] */
  iex = 0;
  for( i = 0; i < 22; i++)
  {
    int hb1, hb2, ha;
    data = 1 << i;
    //ha = data << 2;
    ha = (0xffffff & ~(data)) << 2;
    hb1 = data << 26;
    hb2 = data >> 6;
    pevx_csr_wr( crate, 0x80001444, ha | hb1 | 0x2);  /* set HB[5:0] + mode: LA[23:0] = HA[23:0] +  HB[21:0] */
    pevx_csr_wr( crate, 0x80001448, hb2 );            /* set HB[21:6]                             */
    usleep( 10);
    res = pevx_csr_rd( crate, 0x8000144c);               /* read LA[23:0] */
    if( (res&0x3fffff) != 0x3fffff)
    {
      TST_LOG( tc, (logline, "       -> NOK: expected=3fffff - found=%08x\n", res));
      retval = TST_STS_ERR;
      iex = 1;
      break;
    }
  }
  if( !iex)
  {
    TST_LOG( tc, (logline, "       -> OK\n"));
  }

tst_fmc_exit:
  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "\n%s->Exiting:%s", tst_id, ct));
  return( retval | TST_STS_DONE);
}


int  
tst_86( struct tst_ctl *tc)
{
  return( tst_fmc( tc, 0, "Tst:86"));
}

int  
tst_vme_p2( struct tst_ctl *tc,
	 int mode,
	 char *tst_id)
{
  int retval;
  time_t tm;
  char *ct;
  int board_id, crate, data;

  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "%s->Entering:%s", tst_id, ct));

  /*---->
  insert your test here...

  <----*/
  if( ( board_id = pevx_board()) != PEV_BOARD_IFC1210)
  {
    TST_LOG( tc, (logline, "%s-> wrong board type : %08x expected %08x               -> NOK", tst_id, board_id, PEV_BOARD_IFC1210));
    goto tst_vme_p2_exit;
  }

  TST_LOG( tc, (logline, "%s->Checking VME P2 Interface", tst_id));

  crate = tc->xt->pev_para.crate;

  /* Enable V6-S6 Interface */

  data = pevx_csr_rd( crate, 0x80001000);
  TST_LOG( tc, (logline, "\n%sUSER register = %08x                   -> ", ident, data));
  if( data != 0x12340002)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
    goto tst_vme_p2_exit;
  }
  TST_LOG( tc, (logline, "OK"));
  pevx_csr_wr( crate, 0x80001018, 0);
  pevx_csr_wr( crate, 0x80001014, 0);
  pevx_csr_wr( crate, 0x80001024, 0);
  pevx_csr_wr( crate, 0x80001020, 0);
  pevx_csr_wr( crate, 0x80001030, 0);
  pevx_csr_wr( crate, 0x8000102c, 0);
  pevx_csr_wr( crate, 0x8000103c, 0);
  pevx_csr_wr( crate, 0x80001038, 0);
  usleep( 10000);
  pevx_csr_wr( crate, 0x80001090, 0);
  usleep( 10000);
  pevx_csr_wr( crate, 0x80001090, 1);
  usleep( 10000);
  pevx_csr_wr( crate, 0x80001090, 2);
  usleep( 10000);
  data = pevx_csr_rd( crate, 0x80001090);
  TST_LOG( tc, (logline, "\n%sV6-S6 PLL Enable = %08x                -> ", ident, data));
  if( (data & 0xf000) != 0x4000)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
    goto tst_vme_p2_exit;
  }
  TST_LOG( tc, (logline, "OK"));
  pevx_csr_wr( crate, 0x80001090, 3);
  usleep( 10000);
  data = pevx_csr_rd( crate, 0x80001090);
  TST_LOG( tc, (logline, "\n%sV6-S6 Interface Enable = %08x          -> ", ident, data));
  if( (data & 0xf000) != 0x8000)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
    goto tst_vme_p2_exit;
  }
  TST_LOG( tc, (logline, "OK"));

  /* Checkin VME P2 row A */
  pevx_csr_wr( crate, 0x80001018, 0x55555555);
  pevx_csr_wr( crate, 0x80001014, 0x00000000);
  data = pevx_csr_rd( crate, 0x80001010);
  TST_LOG( tc, (logline, "\n%sVME P2 Row A OFF = %08x                -> ", ident, data));
  if( data)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  pevx_csr_wr( crate, 0x80001014, 0x55555555);
  data = pevx_csr_rd( crate, 0x80001010);
  TST_LOG( tc, (logline, "\n%sVME P2 Row A ON = %08x                 -> ", ident, data));
  if( data != 0xffffffff)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  /* Checkin VME P2 row C */
  pevx_csr_wr( crate, 0x80001024, 0x55555555);
  pevx_csr_wr( crate, 0x80001020, 0x00000000);
  data = pevx_csr_rd( crate, 0x8000101c);
  TST_LOG( tc, (logline, "\n%sVME P2 Row C OFF = %08x                -> ", ident, data));
  if( data)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  pevx_csr_wr( crate, 0x80001020, 0x55555555);
  data = pevx_csr_rd( crate, 0x8000101c);
  TST_LOG( tc, (logline, "\n%sVME P2 Row C ON = %08x                 -> ", ident, data));
  if( data != 0xffffffff)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  /* Checkin VME P2 row D */
  pevx_csr_wr( crate, 0x80001030, 0x15555555);
  pevx_csr_wr( crate, 0x8000102c, 0x00000000);
  data = pevx_csr_rd( crate, 0x80001028);
  TST_LOG( tc, (logline, "\n%sVME P2 Row D OFF = %08x                -> ", ident, data));
  if( data)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  pevx_csr_wr( crate, 0x8000102c, 0x15555555);
  data = pevx_csr_rd( crate, 0x80001028);
  TST_LOG( tc, (logline, "\n%sVME P2 Row D ON = %08x                 -> ", ident, data));
  if( data != 0x3fffffff)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  /* Checkin VME P2 row Z */
  pevx_csr_wr( crate, 0x8000103c, 0x5555);
  pevx_csr_wr( crate, 0x80001038, 0x0000);
  data = pevx_csr_rd( crate, 0x80001034);
  TST_LOG( tc, (logline, "\n%sVME P2 Row Z OFF = %08x                -> ", ident, data));
  if( data)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }

  pevx_csr_wr( crate, 0x80001038, 0x5555);
  data = pevx_csr_rd( crate, 0x80001034);
  TST_LOG( tc, (logline, "\n%sVME P2 Row Z ON = %08x                 -> ", ident, data));
  if( data != 0xffff)
  {
    TST_LOG( tc, (logline, "NOK"));
    retval = TST_STS_ERR;
  }
  else
  {
    TST_LOG( tc, (logline, "OK"));
  }


tst_vme_p2_exit:
  tm = time(0);
  ct = ctime(&tm);
  TST_LOG( tc, (logline, "\n%s->Exiting:%s", tst_id, ct));
  return( retval | TST_STS_DONE);
}

int  
tst_87( struct tst_ctl *tc)
{
  return( tst_vme_p2( tc, 0, "Tst:87"));
}




