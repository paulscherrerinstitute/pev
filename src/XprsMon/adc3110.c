/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : adc3110.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : november 14,2011
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to perform read
 *     or write cycles through the I"C interface.
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
 * $Log: adc3110.c,v $
 * Revision 1.1  2013/06/07 15:01:02  zimoch
 * update to latest version
 *
 * Revision 1.2  2013/05/14 06:29:17  ioxos
 * new register mapping + signature + acq fifo + temperature control [JFG]
 *
 * Revision 1.1  2013/04/15 13:55:14  ioxos
 * first checkin [JFG]
 *
 * Revision 1.7  2012/09/03 13:19:05  ioxos
 * adapt pec_adc3110_xx(), pev_pex_xx() and pev_bmr_xx() to new FPGA and library [JFG]
 *
 * Revision 1.6  2012/08/28 13:40:46  ioxos
 * cleanup + update adc3110 status + reset [JFG]
 *
 * Revision 1.5  2012/06/01 13:59:44  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.4  2012/02/14 16:06:43  ioxos
 * add support for FMC [JFG]
 *
 * Revision 1.3  2012/02/03 16:27:37  ioxos
 * dynamic use of elbc for adc3110 [JFG]
 *
 * Revision 1.2  2012/01/27 15:55:44  ioxos
 * prepare release 4.01 supporting x86 & ppc [JFG]
 *
 * Revision 1.1  2012/01/27 13:39:15  ioxos
 * first checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: adc3110.c,v 1.1 2013/06/07 15:01:02 zimoch Exp $";
#endif

#define DEBUGno
#include <debug.h>

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cli.h>
#include <unistd.h>
#include <pevioctl.h>
#include <pevulib.h>

#define BUS_SPI 1
#define BUS_I2C 2
#define BUS_SBC 3

#define MAP_OLDno

#ifdef MAP_OLD
#define ADC_BASE_SERIAL_A     0x80001048
#define ADC_BASE_SERIAL_B     0x80001448
#define ADC_BASE_BMOV_A       0x80001058
#define ADC_BASE_BMOV_B       0x80001458
#else
#define ADC_BASE_SERIAL_A     0x8000120c
#define ADC_BASE_SERIAL_B     0x8000130c
#define ADC_BASE_BMOV_A       0x80001100
#define ADC_BASE_BMOV_B       0x80001110
#endif

struct pev_adc3110_devices
{
  char *name;
  uint cmd;
  int idx;
  int bus;
}
adc3110_devices[] =
{
  { "lmk",  0x02000000, -1, BUS_SBC},
  { "ads01", 0x01000000, 0, BUS_SBC},
  { "ads23", 0x01010000, 1, BUS_SBC},
  { "ads45", 0x01020000, 2, BUS_SBC},
  { "ads67", 0x01030000, 3, BUS_SBC},
  { "tmp102",0x01040048, 0, BUS_I2C},
  { "eeprom",0x01010051, 1, BUS_I2C},
  { NULL, 0}
};

int adc3110_init_flag = 0;
struct cli_cmd_history adc3110_history;
char adc3110_prompt[32];
uint adc3110_histo[2][0x10000];
struct adc3110_calib_res
{
  uint tot[2];
  uint min[2];
  uint max[2];
  float mean[2];
  float sig[2];
};

struct adc3110_sign
{
  char board_name[8];
  char serial[4];
  char version[8];
  char revision[2];
  char rsv[6];
  char test_date[8];
  char calib_date[8];
  char offset_adc[8][8];
  char pad[176];
  int cks;
} adc3110_sign;

char *
adc3110_rcsid()
{
  return( rcsid);
}

char filename[0x100];
struct cli_cmd_history adc3110_history;

void 
adc3110_init()
{
  if( !adc3110_init_flag)
  {
    cli_history_init( &adc3110_history);
    adc3110_init_flag = 1;
  }
  return;
}

int 
adc3110_calib_res( struct adc3110_calib_res *r)
{
  int i;

  r->tot[0] = 0;
  r->mean[0] = 0;
  r->sig[0] = 0;
  r->min[0] = 0xffff;
  r->max[0] = 0x0;
  r->tot[1] = 0;
  r->sig[1] = 0;
  r->min[1] = 0xffff;
  r->max[1] = 0x0;
  for( i = 0; i < 0x10000; i++)
  {
    if(adc3110_histo[0][i])
    {
      if( i < r->min[0])r->min[0] = i;
      if( i > r->max[0])r->max[0] = i;
    }
    r->tot[0] += adc3110_histo[0][i];
    r->mean[0] += (float)(i*adc3110_histo[0][i]);
    if(adc3110_histo[1][i])
    {
      if( i < r->min[1])r->min[1] = i;
      if( i > r->max[1])r->max[1] = i;
    }
    r->tot[1] += adc3110_histo[1][i];
    r->mean[1] += (float)(i*adc3110_histo[1][i]);
  }
  r->mean[0] = r->mean[0]/r->tot[0];
  r->mean[1] = r->mean[1]/r->tot[1];

  for( i = 0; i < 0x10000; i++)
  {
    r->sig[0] += (i - r->mean[0])*(i - r->mean[0])*adc3110_histo[0][i];
    r->sig[1] += (i - r->mean[1])*(i - r->mean[1])*adc3110_histo[1][i];
  }
  r->sig[0] = sqrt(r->sig[0]/r->tot[0]);
  r->sig[1] = sqrt(r->sig[1]/r->tot[1]);

  return(0);
}

int 
xprs_adc3110( struct cli_cmd_para *c)
{
  struct pev_adc3110_devices *add;
  uint cmd, data, reg, fmc, tmo;
  char *p;

  adc3110_init();

  if( c->cnt < 2)
  {
    printf("adc3110 command needs more arguments\n");
    printf("usage: adc3110.<fmc> <dev> <op> <reg> [<data>]\n");
    printf("adc3110 device list:\n");
    add = &adc3110_devices[0];
    while( add->name)
    {
      printf("   - %s\n", add->name);
      add++;
    }
    return(-1);
  }

  if( !strcmp( "save", c->para[1]))
  {
    int i, offset, size, data, chan;
    char *acq_name_1, *acq_name_2;
    FILE *acq_file_1, *acq_file_2;
    char *acq_buf;
    struct pev_ioctl_map_pg shm_mas_map;
    struct adc3110_calib_res res;

    if( c->cnt < 4)
    {
      printf("adc3110 acq command needs more arguments\n");
      printf("usage: adc3110.<fmc> <dev> acq <offset> <size>\n");
      return(-1);
    }
    if( sscanf( c->para[2],"%x", &offset) != 1)
    {
      printf("wrong offset value\n");
      printf("usage: adc3110.<fmc> <dev> acq <offset> <size>\n");
      return(-1);
    }
    if( sscanf( c->para[3],"%x", &size) != 1)
    {
      printf("wrong size value\n");
      printf("usage: adc3110.<fmc> <dev> acq <offset> <size>\n");
      return(-1);
    }

    if(  c->para[0][0] != '-')
    {
      if(  c->para[0][0] == '?')
      {
        strcpy( adc3110_prompt, "enter filename: "); 
        c->para[0] = cli_get_cmd( &adc3110_history, adc3110_prompt);
      }
      acq_name_1 = (char *)malloc( strlen( c->para[0]) + 8);
      strcpy( acq_name_1, c->para[0]);
      strcat( acq_name_1, "_1.his");
      acq_name_2 = (char *)malloc( strlen( c->para[0]) + 8);
      strcpy( acq_name_2, c->para[0]);
      strcat( acq_name_2, "_2.his");

      printf("save data acquision data from offset %x [%x] to file %s and %s\n", offset, size, acq_name_1, acq_name_2);
      acq_file_1 = fopen( acq_name_1, "w");
      if( !acq_file_1)
      {
        printf("cannot create acquisition file %s\n", acq_name_1);
        return( -1);
      }
      acq_file_2 = fopen( acq_name_2, "w");
      if( !acq_file_2)
      {
        printf("cannot create acquisition file %s\n", acq_name_2);
        fclose( acq_file_1);
        return( -1);
      }
    }
    acq_buf = NULL;
    if( size > 0)
    {
      shm_mas_map.rem_addr = offset;
      shm_mas_map.mode = MAP_ENABLE|MAP_ENABLE_WR|MAP_SPACE_SHM;
      shm_mas_map.flag = 0x0;
      shm_mas_map.sg_id = MAP_MASTER_32;
      shm_mas_map.size = size;
      pev_map_alloc( &shm_mas_map);
      acq_buf = pev_mmap( &shm_mas_map);
    }
    if( !acq_buf || ( size <= 0))
    {
      printf("cannot allocate acquisition buffer\n");
      return( -1);
    }
    for( i = 0; i < 0x10000; i++)
    {
      adc3110_histo[0][i] = 0;
      adc3110_histo[1][i] = 0;
    }
    chan = 0;
    for( i = 0; i < size; i += 16)
    {
      data = *(unsigned char *)&acq_buf[i] |  (*(unsigned char *)&acq_buf[i+1] << 8);
      adc3110_histo[0][data & 0xffff] += 1;
      if(  c->para[0][0] != '-') fprintf( acq_file_1, "%d %d\n", chan, data);
      data = *(unsigned char *)&acq_buf[i+2] |  (*(unsigned char *)&acq_buf[i+3] << 8);
      adc3110_histo[0][data & 0xffff] += 1;
      if(  c->para[0][0] != '-') fprintf( acq_file_1, "%d %d\n", chan+1, data);
      data = *(unsigned char *)&acq_buf[i+4] |  (*(unsigned char *)&acq_buf[i+5] << 8);
      adc3110_histo[0][data & 0xffff] += 1;
      if(  c->para[0][0] != '-') fprintf( acq_file_1, "%d %d\n", chan+2, data);
      data = *(unsigned char *)&acq_buf[i+6] |  (*(unsigned char *)&acq_buf[i+7] << 8);
      adc3110_histo[0][data & 0xffff] += 1;
      if(  c->para[0][0] != '-') fprintf( acq_file_1, "%d %d\n", chan+3, data);
      data = *(unsigned char *)&acq_buf[i+8] |  (*(unsigned char *)&acq_buf[i+9] << 8);
      adc3110_histo[1][data & 0xffff] += 1;
      if(  c->para[0][0] != '-') fprintf( acq_file_2, "%d %d\n", chan,  data);
      data = *(unsigned char *)&acq_buf[i+10] |  (*(unsigned char *)&acq_buf[i+11] << 8);
      adc3110_histo[1][data & 0xffff] += 1;
      if(  c->para[0][0] != '-') fprintf( acq_file_2, "%d %d\n", chan+1, data);
      data = *(unsigned char *)&acq_buf[i+12] |  (*(unsigned char *)&acq_buf[i+13] << 8);
      adc3110_histo[1][data & 0xffff] += 1;
      if(  c->para[0][0] != '-') fprintf( acq_file_2, "%d %d\n", chan+2, data);
      data = *(unsigned char *)&acq_buf[i+14] |  (*(unsigned char *)&acq_buf[i+15] << 8);
      adc3110_histo[1][data & 0xffff] += 1;
      if(  c->para[0][0] != '-') fprintf( acq_file_2, "%d %d\n", chan+3, data);
      chan += 4;
    }
    adc3110_calib_res( &res);
    printf("tot_A: %d - mean_A: %f - sig_A: %f [%d %d]\n", res.tot[0], res.mean[0], res.sig[0], res.min[0], res.max[0]);
    printf("tot_B: %d - mean_B: %f - sig_B: %f [%d %d]\n", res.tot[1], res.mean[1], res.sig[1], res.min[1], res.max[1]);

    if(  c->para[0][0] != '-')
    {
      fclose( acq_file_1);
      fclose( acq_file_2);
    }
    pev_munmap( &shm_mas_map);
    pev_map_free( &shm_mas_map);

    return( 0);
  }

  if( !c->ext) 
  {
    printf("you must specify fmc [1 or 2]\n");
    printf("usage: adc3110.<fmc> <dev> <op> <reg> [<data>]\n");
    return(-1);
  }
  else
  {
    fmc = strtoul( c->ext, &p, 16);
    if(( fmc < 1) || ( fmc > 2))
    {
      printf("bad FMC index : %d\n", fmc);
      return( -1);
    }
  }
  add = &adc3110_devices[0];
  while( add->name)
  {
    if( !strcmp(  c->para[0], add->name))
    {
      break;
    }
    add++;
  }
  if( !add->name)
  {
    printf("wrong device name\n");
    printf("usage: adc3110.<fmc> <dev> <op> <reg> [<data>]\n");
    printf("adc3110 device list:\n");
    add = &adc3110_devices[0];
    while( add->name)
    {
      printf("   - %s\n", add->name);
      add++;
    }
    return(-1);
  }
  if( !strcmp( "read", c->para[1]))
  {
    if( c->cnt < 3)
    {
      printf("adc3110 read command needs more arguments\n");
      printf("usage: adc3110.<fmc> <dev> read <reg>\n");
      return(-1);
    }
    printf("%s.%s %s %s %s\n", c->cmd, c->ext, c->para[0], c->para[1], c->para[2]);
    if( sscanf( c->para[2],"%x", &reg) != 1)
    {
      printf("wrong register number\n");
      printf("usage: adc3110.<fmc> <dev> <op> <reg> [<data>]\n");
      return(-1);
    }
    if( add->bus == BUS_SBC)
    {
      cmd =  0x80000000 | add->cmd | reg;
      tmo = 1000;
      if( fmc == 2)
      {
        pev_csr_wr( ADC_BASE_SERIAL_B, cmd);
        while( --tmo)
        {
	  if( !(pev_csr_rd( ADC_BASE_SERIAL_B) & 0x80000000)) break;
        }
      data = pev_csr_rd( ADC_BASE_SERIAL_B + 4);
      }
      else
      {
        pev_csr_wr( ADC_BASE_SERIAL_A, cmd);
        while( --tmo)
        {
	  if( !(pev_csr_rd( ADC_BASE_SERIAL_A) & 0x80000000)) break;
        }
        data = pev_csr_rd( ADC_BASE_SERIAL_A + 4);
      }
      //if( cmd & 0x02) data = data >> 5; /* LMK */
      printf("cmd = %08x - data = %08x\n", cmd, data);
    }
    if( add->bus == BUS_I2C)
    {
      int status;

      cmd = add->cmd;
      if( fmc == 2)
      {
        cmd |= 0xa0000000;
      }
      else
      {
        cmd |= 0x80000000;
	if( add->idx == 1) cmd += 1;
      }
      status = pev_i2c_read( cmd, reg, &data);
      if( status & I2C_CTL_ERR)
      {
        printf("%s: reg=%x -> error = %08x\n", add->name, reg, status);
      } 
      else
      {
	printf("cmd = %08x - data = %08x\n", cmd, data);
      }
    }
  }
  else if( !strcmp( "write", c->para[1]))
  {
    if( c->cnt < 4)
    {
      printf("adc3110 write command needs more arguments\n");
      printf("usage: adc3110.<fmc> <dev> write <reg> <data>\n");
      return(-1);
    }
    if( sscanf( c->para[2],"%x", &reg) != 1)
    {
      printf("wrong register number\n");
      printf("usage: adc3110.<fmc> <dev> <op> <reg> [<data>]\n");
      return(-1);
    }
    if( sscanf( c->para[3],"%x", &data) != 1)
    {
      printf("wrong data value\n");
      return(-1);
    }
    printf("%s.%s %s %s %s %s \n", c->cmd, c->ext, c->para[0], c->para[1], c->para[2], c->para[3]);
    if( add->bus == BUS_SBC)
    {
      cmd =  0xc0000000 | add->cmd | reg;
      printf("cmd = %08x - data = %08x\n", cmd, data);
      if( fmc == 2)
      {
        pev_csr_wr( ADC_BASE_SERIAL_B + 4, data);
        pev_csr_wr( ADC_BASE_SERIAL_B, cmd);
      }
      else 
      {
        pev_csr_wr( ADC_BASE_SERIAL_A + 4, data);
        pev_csr_wr( ADC_BASE_SERIAL_A, cmd);
      }
    }
    if( add->bus == BUS_I2C)
    {
      int status;

      cmd = add->cmd;
      if( fmc == 2)
      {
        cmd |= 0xa0000000;
      }
      else
      {
        cmd |= 0x80000000;
	if( add->idx == 1) cmd += 1;
      }
      printf("cmd = %08x - data = %08x\n", cmd, data);
      status = pev_i2c_write( cmd, reg, data);
      if( status & I2C_CTL_ERR)
      {
        printf("%s: reg=%x -> error = %08x\n", add->name, reg, status);
      } 
    }
  }
  else if( !strcmp( "acqfif", c->para[1]))
  {
    int offset, size, tmo;

    if( (add->idx < 0) || (add->bus != BUS_SBC))
    {
      printf("wrong device name\n");
      printf("usage: adc3110.<fmc> ads<ij> acq <offset> [<size>]\n");
      return(-1);
    }
    if( c->cnt < 4)
    {
      printf("adc3110 acq command needs more arguments\n");
      printf("usage: adc3110.<fmc> <dev> acq <offset> <size>\n");
      return(-1);
    }
    if( sscanf( c->para[2],"%x", &offset) != 1)
    {
      printf("wrong offset value\n");
      printf("usage: adc3110.<fmc> <dev> acq <offset> <size>\n");
      return(-1);
    }
    if( sscanf( c->para[3],"%x", &size) != 1)
    {
      printf("wrong size value\n");
      printf("usage: adc3110.<fmc> <dev> acq <offset> <size>\n");
      return(-1);
    }
    printf("start data acquision on device  %s [%d] at offset %x [%x]\n",  c->para[0], add->idx, offset, size);
    if( fmc == 2)
    {
      int cmd_sav;

      cmd_sav = pev_csr_rd( 0x80001184);
      cmd_sav &= ~( 0xff << (8*add->idx));

      cmd = cmd_sav | (1 << (8*add->idx));
      pev_csr_wr( 0x80001188, cmd);
      pev_csr_rd( 0x80001188);

      cmd = cmd_sav;
      pev_csr_wr( 0x80001188, cmd);
      pev_csr_rd( 0x80001188);

      pev_csr_wr( ADC_BASE_BMOV_B, offset);
      cmd = 0x90000000 | ( add->idx << 26) | (size & 0x3fffe00);
      pev_csr_wr( ADC_BASE_BMOV_B + 4, cmd);
      pev_csr_rd( ADC_BASE_BMOV_B + 4);

      cmd = cmd_sav | (2 << (8*add->idx));
      pev_csr_wr( 0x80001188, cmd);
      pev_csr_rd( 0x80001188);

      tmo = 100;
      while( --tmo)
      {
        usleep(2000);
        if( pev_csr_rd( ADC_BASE_BMOV_B + 4) & 0x80000000) break;
      }
      printf("acquisition status : %08x - %08x\n", pev_csr_rd( ADC_BASE_BMOV_B),  pev_csr_rd( ADC_BASE_BMOV_B + 4));
    }
    else
    {
      int cmd_sav;

      cmd_sav = pev_csr_rd( 0x80001184);
      cmd_sav &= ~( 0xff << (8*add->idx));

      cmd = cmd_sav | (1 << (8*add->idx));
      pev_csr_wr( 0x80001184, cmd);
       pev_csr_rd( 0x80001184);

      cmd = cmd_sav;;
      pev_csr_wr( 0x80001184, cmd);
      pev_csr_rd( 0x80001184);

      pev_csr_wr( ADC_BASE_BMOV_A, offset);
      cmd = 0x90000000 | ( add->idx << 26) | (size & 0x3fffe00);
      pev_csr_wr( ADC_BASE_BMOV_A + 4, cmd);
      pev_csr_rd( ADC_BASE_BMOV_A+4);

      cmd = cmd_sav | (2 << (8*add->idx));
      pev_csr_wr( 0x80001184, cmd);
      pev_csr_rd( 0x80001184);

      tmo = 100;
      while( --tmo)
      {
        usleep(2000);
        if( pev_csr_rd( ADC_BASE_BMOV_A + 4) & 0x80000000) break;
      }
      printf("acquisition status : %08x - %08x\n", pev_csr_rd( ADC_BASE_BMOV_A),  pev_csr_rd( ADC_BASE_BMOV_A + 4));
    }
  }
  else if( !strcmp( "acqdpr", c->para[1]))
  {
  }
  else if( !strcmp( "show", c->para[1]))
  {
    int status;
    int device;
    uint temp, ctl, lo, hi;

    device = add->cmd;
    if( device != 0x01040048)
    {
      printf(" show command not supported for that device\n");
      return(-1);
    }
    if( fmc == 2)
    {
      device |= 0xa0000000;
    }
    else
    {
      device |= 0x80000000;
    }
    status = pev_i2c_read( device, 1, &ctl);
    if( status & I2C_CTL_ERR)
    {
      printf("%s: reg=%x -> error = %08x\n", add->name, reg, status);
    }
    else
    {
      pev_i2c_read( device, 0, &temp);
      pev_i2c_read( device, 2, &lo);
      pev_i2c_read( device, 3, &hi);
      if( temp & 0x100)
      {
	temp = ((temp & 0xff) << 5) + ((temp & 0xf8) >> 3);
	lo = ((lo & 0xff) << 5) + ((lo & 0xf8) >> 3);
	hi = ((hi & 0xff) << 5) + ((hi & 0xf8) >> 3);
      }
      else
      {
	temp = ((temp & 0xff) << 4) + ((temp & 0xf0) >> 4);
	lo = ((lo & 0xff) << 4) + ((lo & 0xf0) >> 4);
	hi = ((hi & 0xff) << 4) + ((hi & 0xf0) >> 4);
      }
      printf("current temperature: %.2f [%.2f - %.2f]\n", (float)temp/16, (float)lo/16, (float)hi/16);
    }
  }
  else if( !strcmp( "set", c->para[1]))
  {
    int status;
    int device;
    uint temp, ctl, lo, hi;
    float flo, fhi;

    device = add->cmd;
    if( device != 0x01040048)
    {
      printf(" set command not supported for that device\n");
      return(-1);
    }
    if( fmc == 2)
    {
      device |= 0xa0000000;
    }
    else
    {
      device |= 0x80000000;
    }
    if( c->cnt < 4)
    {
      printf("adc3110 set command needs more arguments\n");
      printf("usage: adc3110.<fmc> tmp109 set <lo> <hi>\n");
      return(-1);
    }
    if( sscanf( c->para[2],"%f", &flo) != 1)
    {
      printf("wrong lo value\n");
      printf("usage: adc3110.<fmc> tmp109 set <lo> <hi>\n");
      return(-1);
    }
    if( sscanf( c->para[3],"%f", &fhi) != 1)
    {
      printf("wrong hi value\n");
      printf("usage: adc3110.<fmc> tmp109 set <lo> <hi>\n");
      return(-1);
    }
    status = pev_i2c_read( device, 1, &ctl);
    if( status & I2C_CTL_ERR)
    {
      printf("%s: reg=%x -> error = %08x\n", add->name, reg, status);
    }
    else
    {
      pev_i2c_read( device, 0, &temp);
      lo = (int)(flo*16);
      hi = (int)(fhi*16);
      if( temp & 0x100)
      {
	temp = ((temp & 0xff) << 5) + ((temp & 0xf8) >> 3);
	lo = ((lo & 0x1fe0) >> 5) | ((lo & 0x1f) << 11);
	hi = ((hi & 0x1fe0) >> 5) | ((hi & 0x1f) << 11);
      }
      else
      {
	temp = ((temp & 0xff) << 4) + ((temp & 0xf0) >> 4);
	lo = ((lo & 0xff0) >> 4) | ((lo & 0xf) << 12);
	hi = ((hi & 0xff0) >> 4) | ((hi & 0xf) << 12);
      }
      pev_i2c_write( device, 2, lo);
      usleep(10000);
      pev_i2c_write( device, 3, hi);
      usleep(10000);
      printf("current temperature: %.2f [%.2f - %.2f]\n", (float)temp/16, flo, fhi);
    }
  }
  else if( !strcmp( "sign", c->para[1]))
  {
    int device, i;
    unsigned char *p;
    int op;
    char *para_p;

    device = add->cmd;
    if( device != 0x01010051)
    {
      printf(" sign command not supported for that device\n");
      return(-1);
    }
    if( fmc == 2)
    {
      device |= 0xa0000000;
    }
    else
    {
      device |= 0x80000000;
      if( add->idx == 1) device += 1;
    }
    p = (unsigned char *)&adc3110_sign;
    for( i = 0x0; i < 0x100; i++)
    {
      pev_i2c_read( device, pev_swap_16( 0x7000 + i), &data);
      p[i] = (unsigned char)data;
    }
    op = 0;
    if( c->cnt > 2)
    {
      if( !strcmp( "set", c->para[2]))
      {
	op = 1;
      }
    }

    if( op)
    {
      char prompt[64];

      bzero( &adc3110_history, sizeof( struct cli_cmd_history));
      cli_history_init( &adc3110_history);
      printf("setting ADC3110 signature\n");
      para_p = cli_get_cmd( &adc3110_history, "Enter password ->  ");
      if( strcmp(  para_p, "goldorak"))
      {
	printf("wrong password\n");
	return(-1);
      }

      strcpy( &prompt[0], "Board Name [");
      strncat( &prompt[0], &adc3110_sign.board_name[0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.board_name[0], para_p, 8);


      strcpy( &prompt[0], "Serial Number [");
      strncat( &prompt[0], &adc3110_sign.serial[0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.serial[0], para_p, 4);

      strcpy( &prompt[0], "PCB Version :  [");
      strncat( &prompt[0], &adc3110_sign.version[0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.version[0], para_p, 8);

      strcpy( &prompt[0], "Hardware Revision :  [");
      strncat( &prompt[0], &adc3110_sign.revision[0], 2);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.revision[0], para_p, 2);

      strcpy( &prompt[0], "Test Date :  [");
      strncat( &prompt[0], &adc3110_sign.test_date[0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.test_date[0], para_p, 8);

      strcpy( &prompt[0], "Calibration Date :  [");
      strncat( &prompt[0], &adc3110_sign.calib_date[0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.calib_date[0], para_p, 8);

      strcpy( &prompt[0], "Offset Compensation Chan0 :  [");
      strncat( &prompt[0], &adc3110_sign.offset_adc[0][0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.offset_adc[0][0], para_p, 8);

      strcpy( &prompt[0], "Offset Compensation Chan1 :  [");
      strncat( &prompt[0], &adc3110_sign.offset_adc[1][0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.offset_adc[1][0], para_p, 8);

      strcpy( &prompt[0], "Offset Compensation Chan2 :  [");
      strncat( &prompt[0], &adc3110_sign.offset_adc[2][0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.offset_adc[2][0], para_p, 8);

      strcpy( &prompt[0], "Offset Compensation Chan3 :  [");
      strncat( &prompt[0], &adc3110_sign.offset_adc[3][0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.offset_adc[3][0], para_p, 8);

      strcpy( &prompt[0], "Offset Compensation Chan4 :  [");
      strncat( &prompt[0], &adc3110_sign.offset_adc[4][0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.offset_adc[4][0], para_p, 8);

      strcpy( &prompt[0], "Offset Compensation Chan5 :  [");
      strncat( &prompt[0], &adc3110_sign.offset_adc[5][0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.offset_adc[5][0], para_p, 8);

      strcpy( &prompt[0], "Offset Compensation Chan6 :  [");
      strncat( &prompt[0], &adc3110_sign.offset_adc[6][0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.offset_adc[6][0], para_p, 8);

      strcpy( &prompt[0], "Offset Compensation Chan7 :  [");
      strncat( &prompt[0], &adc3110_sign.offset_adc[7][0], 8);
      strcat( &prompt[0], "] : "); 
      para_p = cli_get_cmd( &adc3110_history, prompt);
      if( para_p[0] == 'q') return(-1);
      if( para_p[0]) strncpy( &adc3110_sign.offset_adc[7][0], para_p, 8);

    }
    printf("ADC3110 signature\n");
    p = (unsigned char *)&adc3110_sign.board_name[0];
    printf("Board Name :  %c%c%c%c%c%c%c%c\n", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    p = (unsigned char *)&adc3110_sign.serial[0];
    printf("Serial Number : %c%c%c%c\n", p[0],p[1],p[2],p[3]);
    p = (unsigned char *)&adc3110_sign.version[0];
    printf("PCB Version : %c%c%c%c%c%c%c%c\n", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    p = (unsigned char *)&adc3110_sign.revision[0];
    printf("Hardware Revision : %c%c\n", p[0],p[1]);
    p = (unsigned char *)&adc3110_sign.test_date[0];
    printf("Test Date :  %c%c%c%c%c%c%c%c\n", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    p = (unsigned char *)&adc3110_sign.calib_date[0];
    printf("Calibration Date :  %c%c%c%c%c%c%c%c\n", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    p = (unsigned char *)&adc3110_sign.offset_adc[0][0];
    printf("Offset Compensation Chan#0 : %c%c%c%c%c%c%c%c\n", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    p = (unsigned char *)&adc3110_sign.offset_adc[1][0];
    printf("Offset Compensation Chan#1 : %c%c%c%c%c%c%c%c\n", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    p = (unsigned char *)&adc3110_sign.offset_adc[2][0];
    printf("Offset Compensation Chan#2 : %c%c%c%c%c%c%c%c\n", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    p = (unsigned char *)&adc3110_sign.offset_adc[3][0];
    printf("Offset Compensation Chan#3 : %c%c%c%c%c%c%c%c\n", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    p = (unsigned char *)&adc3110_sign.offset_adc[4][0];
    printf("Offset Compensation Chan#4 : %c%c%c%c%c%c%c%c\n", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    p = (unsigned char *)&adc3110_sign.offset_adc[5][0];
    printf("Offset Compensation Chan#5 : %c%c%c%c%c%c%c%c\n", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    p = (unsigned char *)&adc3110_sign.offset_adc[6][0];
    printf("Offset Compensation Chan#6 : %c%c%c%c%c%c%c%c\n", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    p = (unsigned char *)&adc3110_sign.offset_adc[7][0];
    printf("Offset Compensation Chan#7 : %c%c%c%c%c%c%c%c\n", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    printf("\n");

    if( op)
    {
      para_p = cli_get_cmd( &adc3110_history, "Overwrite ADC3110 signature ? [y/n] ");
      if( para_p[0] != 'y')
      {
	printf("EEPROM signature update aborted\n");
	return(-1);
      }
      p = (unsigned char *)&adc3110_sign;
      for( i = 0x0; i < 0x100; i++)
      {
	data = p[i];
        pev_i2c_write( device, pev_swap_16(0x7000+i), data);
	usleep(2000);
      }
      printf("EEPROM signature update done\n");
    }
  }
  else if( !strcmp( "dump", c->para[1]))
  {
    int device, i, j, off, size;
    unsigned char *p, *buf;

    device = add->cmd;
    if( device != 0x01010051)
    {
      printf(" sign command not supported for that device\n");
      return(-1);
    }
    if( c->cnt < 4)
    {
      printf("adc3110 eeprom dump command needs more arguments\n");
      printf("usage: adc3110.<fmc> eeprom dump <offset> <size>\n");
      return(-1);
    }
    if( fmc == 2)
    {
      device |= 0xa0000000;
    }
    else
    {
      device |= 0x80000000;
      if( add->idx == 1) device += 1;
    }
    if( sscanf( c->para[2],"%x", &off) != 1)
    {
      printf("bad offset\n");
      printf("usage: adc3110.<fmc> eeprom dump <offset> <size>\n");
      return(-1);
    }
    if( sscanf( c->para[3],"%x", &size) != 1)
    {
      printf("bad size\n");
      printf("usage: adc3110.<fmc> eeprom dump <offset> <size>\n");
      return(-1);
    }
    printf("Displaying EEPROM from %x to %x\n", off, off+size);
    buf = (unsigned char *)malloc(size + 0x10);
    p = &buf[0];
    for( i = 0; i < size; i++)
    {
      pev_i2c_read( device, pev_swap_16(off+i), &data);
      p[i] = (unsigned char)data;
    }
    p = (unsigned char *)&buf[0];
    for( i = 0; i < size; i += 0x10)
    {
      for( j = 0; j < 0x10; j++)
      {
        printf("%02x ", p[i+j]);
      }
      printf("\n");
    }
  }
  else 
  {
    printf("bad operation : %s\n",  c->para[1]);
    printf("usage: adc3110.<fmc> <dev> read <reg>\n");
    printf("       adc3110.<fmc> <dev> write <reg> <data>\n");
    printf("       adc3110.<fmc> <dev> acq <off> <size>\n");
    printf("       adc3110.<fmc> <file> save <off> <size>\n");
    printf("       adc3110.<fmc> <dev> show\n");
    return(-1);
  }
  return(0);

}
