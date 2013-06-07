/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : idt.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : november 24,2012
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to configure the
 *     IDT PCIe switch.
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
 * $Log: idt.c,v $
 * Revision 1.1  2013/06/07 15:01:02  zimoch
 * update to latest version
 *
 * Revision 1.4  2013/02/15 11:31:48  ioxos
 * init nt_bar0 by default [JFG]
 *
 * Revision 1.3  2013/02/05 11:26:09  ioxos
 * add idt msg command [JFG]
 *
 * Revision 1.2  2012/12/18 16:18:47  ioxos
 * remove printout [JFG]
 *
 * Revision 1.1  2012/12/13 14:43:25  ioxos
 * first checkin [JFG]
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: idt.c,v 1.1 2013/06/07 15:01:02 zimoch Exp $";
#endif

#define DEBUGno
#include <debug.h>

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cli.h>
#include <pevioctl.h>
#include <pevulib.h>

int idt_part_reg[8][3];
int idt_port_reg[24][3];
int idt_port_width[24];
int idt_stkcfg[4];

int idt_stack0[4][8] =
{
  {8,4,4,2, 0,0,2,0},
  {0,0,0,2, 0,0,2,0},
  {0,4,2,2, 0,0,4,0},
  {0,0,2,2, 0,0,0,0}
};

int idt_stack2[8][32] =
{
  {8,4,4,2, 0,0,2,0, 2,1,1,2, 4,4,2,2,  1,2,1,1, 2,1,1,2, 1,1,2,1, 4,0,0,0},
  {0,0,0,0, 0,0,0,0, 0,1,1,0, 0,0,0,0,  1,0,1,1, 0,1,1,0, 1,1,0,1, 0,0,0,0},
  {0,0,0,2, 0,0,2,0, 1,1,2,1, 0,0,2,2,  1,1,1,1, 2,2,2,1, 2,2,1,1, 0,0,0,0},
  {0,0,0,0, 0,0,0,0, 1,1,0,1, 0,0,0,0,  1,1,1,1, 0,0,0,1, 0,0,1,1, 0,0,0,0},
  {0,4,2,2, 0,0,4,0, 4,4,2,2, 2,1,1,2,  2,1,1,2, 1,2,1,1, 4,1,2,1, 1,0,0,0},
  {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,1,1,0,  0,1,1,0, 1,0,1,1, 0,1,0,1, 1,0,0,0},
  {0,0,2,2, 0,0,0,0, 0,0,2,2, 1,1,2,1,  2,2,2,1, 1,1,1,1, 0,2,1,1, 2,0,0,0},
  {0,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,0,1,  0,0,0,1, 1,1,1,1, 0,0,1,1, 0,0,0,0}
};

int idt_status = 0;
int idt_nt0_baddr = 0;

char *
idt_rcsid()
{
  return( rcsid);
}

int 
idt_pex_wr( int offset,
  	    int val)
{
  struct pev_ioctl_rdwr rdwr_pex;
  int ret, data;

  rdwr_pex.mode.space = RDWR_PEX;
  rdwr_pex.mode.ds = RDWR_INT;
  rdwr_pex.mode.swap = RDWR_NOSWAP;
  rdwr_pex.mode.dir = RDWR_WRITE;
  rdwr_pex.buf = (void *)&data;;
  rdwr_pex.offset = offset;
  rdwr_pex.len = 0;
  data = val;
  ret = pev_rdwr( &rdwr_pex);
  if( ret < 0)
  {
    return( -1);
  }

  return(0);
}

int 
idt_pex_rd( int offset)
{
  struct pev_ioctl_rdwr rdwr_pex;
  int data, ret;

  rdwr_pex.mode.space = RDWR_PEX;
  rdwr_pex.mode.ds = RDWR_INT;
  rdwr_pex.mode.swap = RDWR_NOSWAP;
  rdwr_pex.mode.dir = RDWR_READ;
  rdwr_pex.buf = (void *)&data;
  rdwr_pex.offset = offset;
  rdwr_pex.len = 0;
  ret = pev_rdwr( &rdwr_pex);
  if( ret < 0)
  {
    return( -1);
  }

  return( data);
}

int
idt_conf( struct cli_cmd_para *c)
{
  int i, j, ret;
  struct pev_ioctl_rdwr rdwr;
  int width;

  rdwr.mode.space = RDWR_PEX;
  rdwr.mode.dir = RDWR_READ;
  rdwr.mode.ds = RDWR_INT;
  rdwr.mode.swap = RDWR_NOSWAP;
  printf("IDT switch configuration [%x]\n", pev_board());
  //printf("Port stacking\n");
  for( i = 0; i < 4; i++)
  {
    rdwr.buf = (void *)&idt_stkcfg[i];
    rdwr.offset = 0x3e010 + 4*i;
    rdwr.len = 0;
    ret = pev_rdwr( &rdwr);
    if( ret < 0)
      {
      printf("cannot read IDT register at offset 0x%x\n", rdwr.offset);
      return( -1);
    }
  }
  for( i = 0; i < 4; i++)
  {
    width = idt_stack0[i][idt_stkcfg[0] & 7];
    idt_port_width[i] = width;
    //if( width) printf("Port#%d x%d\n", i, width);
  }
  for( i = 0; i < 4; i++)
  {
    width = idt_stack0[i][idt_stkcfg[1] & 7];
    idt_port_width[i+4] = width;
    //if( width) printf("Port#%d x%d\n", i+4, width);
  }
  for( i = 0; i < 8; i++)
  {
    width = idt_stack2[i][idt_stkcfg[2] & 0x1f];
    idt_port_width[i+8] = width;
    //if( width) printf("Port#%d x%d\n", i+8, width);
  }
  for( i = 0; i < 8; i++)
  {
    width = idt_stack2[i][idt_stkcfg[3] & 0x1f];
    idt_port_width[i+16] = width;
    //if( width) printf("Port#%d x%d\n", i+16, width);
  }

  printf("\n+------+--------+-----------+-----------+------------+\n");
  printf("| part | offset | SWPARTCTL | SWPARTSTS | SWPARTFCTL |\n");
  printf("+------+--------+-----------+-----------+------------+\n");
  for( i = 0; i < 8; i++)
  {
    for( j = 0; j < 3; j++)
    {
      rdwr.buf = (void *)&idt_part_reg[i][j];
      rdwr.offset = 0x3e100 + 0x20*i + 4*j;
      rdwr.len = 0;
      ret = pev_rdwr( &rdwr);
      if( ret < 0)
      {
        printf("cannot read IDT register at offset 0x%x\n", rdwr.offset);
        return( -1);
      }
    }
    if( idt_part_reg[i][0] & 3)
    {
      printf("| %3d  |  %05x |  %08x |  %08x |  %08x  |\n", i,  0x3e100 + 0x20*i, idt_part_reg[i][0], idt_part_reg[i][1], idt_part_reg[i][2]);
    }
  }
  printf("+------+--------+-----------+-----------+------------+\n\n");
  printf("\n+------+--------+-----------+-----------+------------+------------+-----+------+-------+-------+\n");
  printf("| port | offset | SWPORTCTL | SWPORTSTS | SWPARTFCTL |    mode    | dev | part | state | width |\n");
  printf("+------+--------+-----------+-----------+------------+------------+-----+------+-------+-------+\n");
  for( i = 0; i < 24; i++)
  {
    for( j = 0; j < 3; j++)
    {
      rdwr.buf = (void *)&idt_port_reg[i][j];
      rdwr.offset = 0x3e200 + 0x20*i + 4*j;
      rdwr.len = 0;
      ret = pev_rdwr( &rdwr);
      if( ret < 0)
      {
        printf("cannot read IDT register at offset 0x%x\n", rdwr.offset);
        return( -1);
      }
    }
    if( (idt_port_reg[i][0] & 0xf) && idt_port_width[i])
    {
      printf("| %3d  |  %05x |  %08x |  %08x |  %08x  |", i, 0x3e200 + 0x20*i, idt_port_reg[i][0], idt_port_reg[i][1], idt_port_reg[i][2]);
      if(( idt_port_reg[i][1] & 0x3c0) == 0) printf("  Disabled  |  xx |");
      if(( idt_port_reg[i][1] & 0x3c0) == 1<<6)
      {
	printf("    Down    | %3d |", (idt_port_reg[i][1] >> 16) & 0x1f);
      }
      if(( idt_port_reg[i][1] & 0x3c0) == 2<<6) printf("     Up     |  xx |");
      if(( idt_port_reg[i][1] & 0x3c0) == 3<<6) printf("     NT     |  xx |");
      if(( idt_port_reg[i][1] & 0x3c0) == 4<<6) printf("   Up+NT    |  xx |");
      if(( idt_port_reg[i][1] & 0x3c0) == 5<<6) printf(" Unattached |  xx |");
      if(( idt_port_reg[i][1] & 0x3c0) == 6<<6) printf("   Up+DMA   |  xx |");
      if(( idt_port_reg[i][1] & 0x3c0) == 7<<6) printf("  Up+NT+DMA |  xx |");
      if(( idt_port_reg[i][1] & 0x3c0) == 8<<6) printf("   NT+DMA   |  xx |");
      printf("  %2d  |", (idt_port_reg[i][1] >> 10) & 0x7);
      if( idt_port_reg[i][1] & 0x10) printf("   UP  |");
      else  printf("       |");
      printf("   x%d  | \n", idt_port_width[i]);
    }
  }
  printf("+------+--------+-----------+-----------+------------+------------+-----+------+-------+-------+\n");
  return( 0);
}
unsigned char idt_ins[8];
char line[256];

int
idt_load( struct cli_cmd_para *c)
{
  FILE *infile;
  unsigned char *eep_buf, *eep_cmp, *p;
  uint port, reg, data;
  unsigned char cks;
  int i,n, offset, err;

  if( c->cnt < 2)
  {
    printf("prom load command needs filename\n");
    return( -1);
  }
  infile = fopen( c->para[1], "r");
  if( !infile)
  {
    printf("cannot open input file %s\n", c->para[1]);
    return( -1);
  }
  eep_buf = (unsigned char *)malloc( 0x20000);
  eep_cmp = (unsigned char *)malloc( 0x20000);
  n = 0;
  p = eep_buf;
  cks = 0;
  while( fgets( line, 256, infile))
  {
    bzero( idt_ins, 8);
    strtok( line, "\n\r#");
    if( (line[0] == 'w') || (line[0] == 'W'))
    {
      int mask;
      if( sscanf( &line[1], "%x:%x=%x:%x", &port, &reg, &data,&mask) == 4)
      {
        //printf("%02x:%03x=%08x -> ", port, reg, data);
        idt_ins[0] = 0xe0;
        idt_ins[1] = (reg>>2)&0xff;
        idt_ins[2] =  ((port << 3) | (reg >> 10))&0xff;
        idt_ins[3] = data & 0xff;
        idt_ins[4] = (data>>8) & 0xff;
        idt_ins[5] = (data>>16) & 0xff;
        idt_ins[6] = (data>>24) & 0xff;
        idt_ins[7] = mask & 0xff;
        idt_ins[8] = (mask>>8) & 0xff;
        idt_ins[9] = (mask>>16) & 0xff;
        idt_ins[10] = (mask>>24) & 0xff;
        bcopy( idt_ins, p, 7);
        for( i = 0; i < 11; i++)
        {
	  cks += p[i];
	  //printf("%02x", p[i]);
        }
        //printf("\n");
        n += 11;
        p += 11;
      }
    }
    else if( sscanf( line, "%x:%x=%x", &port, &reg, &data) == 3)
    {
      //printf("%02x:%03x=%08x -> ", port, reg, data);
      idt_ins[0] = 0;
      idt_ins[1] = (reg>>2)&0xff;
      idt_ins[2] =  ((port << 3) | (reg >> 10))&0xff;
      idt_ins[3] = data & 0xff;
      idt_ins[4] = (data>>8) & 0xff;
      idt_ins[5] = (data>>16) & 0xff;
      idt_ins[6] = (data>>24) & 0xff;
      bcopy( idt_ins, p, 7);
      for( i = 0; i < 7; i++)
      {
	cks += p[i];
	//printf("%02x", p[i]);
      }
      //printf("\n");
      n += 7;
      p += 7;
    }
  }
  *p = 0xe0;
  cks += *p;
  n++; p++;
  *p = ~cks;
  n++;
  p = eep_buf;
  cks = 0;
  for( i = 0; i < n; i++)
  {
    cks += *p++;
  }
  //printf("checksum = %02x\n", cks);
  offset = 0;
  printf("Loading EEPROM from file %s at offset %x [size %x] ...", c->para[1], offset, n);
  fflush( stdout);
  pev_eeprom_wr( offset, (char *)eep_buf, n);
  printf(" -> done\n");
  printf("Verifying EEPROM content ...");
  fflush( stdout);
  pev_eeprom_rd( offset, (char *)eep_cmp, n);
  err = 0;
  for( i = 0; i < n; i++)
  {
    if( eep_buf[i] != eep_cmp[i])
    {
      err = 1;
      break;
    }
  }
  if( err)
  {
    printf(" -> compare error at offset 0x%x\n", i);
  }
  else 
  {
    printf(" -> OK\n");
  }
  free(eep_buf);

  return(0);
}


int
idt_rid_init()
{
  int ret;
  struct pev_ioctl_rdwr rdwr_pex;
  struct pev_ioctl_rdwr rdwr_baddr;
  int data, did_vid;

  rdwr_pex.mode.space = RDWR_PEX;
  rdwr_pex.mode.ds = RDWR_INT;
  rdwr_pex.mode.swap = RDWR_NOSWAP;
  rdwr_pex.mode.dir = RDWR_READ;
  rdwr_pex.buf = (void *)&data;;
  rdwr_pex.len = 0;

  rdwr_pex.offset = 0x1000;
  ret = pev_rdwr( &rdwr_pex);
  if( ret < 0)
  {
    printf("cannot access IDT configuration registers\n");
    return( -1);
  }
  did_vid = data;
  if( ( did_vid != 0x808c111d) && ( did_vid != 0x8097111d))
  {
    printf("VID/DID of port NT#0 not recognized : %08x\n", data);
    printf("   -> verify switch static configuration (EEPROM)\n");
    return( -1);
  }
  /* read BAR0 setup register of port NT#0  */ 
  rdwr_pex.offset = 0x1470;
  ret = pev_rdwr( &rdwr_pex);
  if( (data & 0x80000400) !=  0x80000400)
  {
    printf("BAR0 of port NT#0 not enabled to access configuration registers[%08x]\n", data);
    printf("   -> verify switch static configuration (EEPROM)\n");
    return( -1);
  }

  /* read BAR0 of NT#0  */ 
  rdwr_pex.offset = 0x1010;
  ret = pev_rdwr( &rdwr_pex);
  idt_nt0_baddr = data;
  printf("NT#0 BAR0 address = %x\n", idt_nt0_baddr);

  rdwr_baddr.mode.space = RDWR_BADDR;
  rdwr_baddr.mode.ds = RDWR_INT;
  rdwr_baddr.mode.swap = RDWR_SWAP;
  rdwr_baddr.mode.dir = RDWR_READ;
  rdwr_baddr.buf = (void *)&data;;
  rdwr_baddr.offset = idt_nt0_baddr;
  rdwr_baddr.len = 0;
  ret = pev_rdwr( &rdwr_baddr);
  if( ret < 0)
  {
    printf("cannot access bus address 0x%x\n", rdwr_baddr.offset);
    return( -1);
  }
  if(( data != 0x808c111d) && (data != 0x8097111d))
  {
    printf("VID/DID inconsistent = %08x -%08x\n", data, did_vid);
    return( -1);
  }

  /* Store root complex identifier of partition 0 in NT mapping table */
  rdwr_baddr.mode.dir = RDWR_WRITE;
  data = 0;
  rdwr_baddr.offset = idt_nt0_baddr + 0x4d0;
  pev_rdwr( &rdwr_baddr);
  data = 1;
  rdwr_baddr.offset = idt_nt0_baddr + 0x4d8;
  pev_rdwr( &rdwr_baddr);
  /* Store root complex identifier of partition 1 in NT mapping table */
  data = 1;
  rdwr_baddr.offset = idt_nt0_baddr + 0x4d0;
  pev_rdwr( &rdwr_baddr);
  data = 0x20001;
  rdwr_baddr.offset = idt_nt0_baddr + 0x4d8;
  pev_rdwr( &rdwr_baddr);

  /* enable RID translation in NT0 and NT12 */
  rdwr_pex.mode.dir = RDWR_WRITE;
  data = 2;
  rdwr_pex.offset = 0x1400;
  ret = pev_rdwr( &rdwr_pex);
  rdwr_pex.offset = 0x19400;
  ret = pev_rdwr( &rdwr_pex);

  return(0);

}

int
idt_map( struct cli_cmd_para *c)
{
  int ret, data;
  struct pev_ioctl_rdwr rdwr;
  int port, bar, des_addr, loc_addr;
  int bar_ctl, bar_size, bar_type;
  int bar_min, bar_max, part;

  port = -1;
  bar = -1;
  bar_min = 0;
  bar_max = 5;
  if( c->ext)
  {
    if( sscanf( c->ext,"%d", &port) == 1)
    {
      if( ( port < 0) || ( port > 24))
      {
	port = -1;
      }
    }
  }
  if( port == -1)
  {
    printf("bad port identifier\n");
    goto idt_map_usage;
  }
  if( c->cnt > 1)
  {
    if( sscanf( c->para[1],"BAR%d", &bar) == 1)
    {
      if( ( bar < 0) || ( bar > 5))
      {
        printf("bad BAR identifier\n");
        goto idt_map_usage;
      }
      bar_min = bar;
      bar_max = bar;
    }
    else
    {
      printf("bad BAR identifier\n");
      goto idt_map_usage;
    }
  }
  if( port == -1)
  {
    printf("bad port identifier\n");
    goto idt_map_usage;
  }
  des_addr = -1;
  if( c->cnt > 2)
  {
    if( sscanf( c->para[2],"%x", &des_addr) != 1)
    {
      printf("bad destination address\n");
      goto idt_map_usage;
    }
  }
  rdwr.mode.space = RDWR_PEX;
  rdwr.mode.ds = RDWR_INT;
  rdwr.mode.swap = RDWR_NOSWAP;
  rdwr.len = 0;

  printf("+------+-----+----------+------------+----------+------+------+\n");
  printf("| Port | BAR | loc_addr |  rem_addr  |   size   | type | mode |\n");
  printf("+------+-----+----------+------------+----------+------+------+\n");
  for( bar = bar_min; bar <= bar_max; bar++)
  {
    rdwr.mode.dir = RDWR_READ;
    rdwr.buf = (void *)&bar_ctl;
    rdwr.offset = (0x1000 | (port << 13) | 0x470) + (bar << 4);
    ret = pev_rdwr( &rdwr);
    if( ret < 0)
    {
      printf("cannot access IDT register 0x%x\n", rdwr.offset);
      return( -1);
    }
    bar_size = (bar_ctl>>4) & 0x1f;
    if( bar_size)
    {
      bar_size = 1 << bar_size;
    }
    bar_type = 32;
    part = (bar_ctl>>13)&7;
    if( bar_ctl & 0x4) bar_type = 64;

    rdwr.buf = (void *)&loc_addr;
    rdwr.offset = (0x1000 | (port << 13) | 0x10) + (bar << 2);
    pev_rdwr( &rdwr);

    rdwr.buf = (void *)&des_addr;
    rdwr.offset = (0x1000 | (port << 13) | 0x470) + (bar << 4) + 8;
    if( des_addr != -1)
    {
      if( !bar && (bar_ctl & 0x400))
      {
        printf("-> cannot map BAR0 : reserverd to access CSR\n");
      }
      else
      {
        rdwr.mode.dir = RDWR_WRITE;
        pev_rdwr( &rdwr);
      }
    }
    rdwr.mode.dir = RDWR_READ;
    rdwr.buf = (void *)&data;
    pev_rdwr( &rdwr);
    if( bar_size)
    {
      if( !bar && (bar_ctl & 0x400))
      {
        printf("| %4d | %3d | %08x |     CSR    | %08x |  %2d  |", port, bar, loc_addr, bar_size, bar_type);
      }
      else
      {
        printf("| %4d | %3d | %08x | %1d:%08x | %08x |  %2d  |", port, bar, loc_addr, part, data, bar_size, bar_type);
      }
      if( bar_ctl & 0x8)
      {
        printf(" PMEM |\n");
      }
      else
      {
        printf("  MEM |\n");
      }
    }
  }
  printf("+------+-----+----------+------------+----------+------+------+\n");

  return(0);

idt_map_usage:
  printf("usage: idt.<x> map BAR<x> [<des_addr>]\n");
  return( -1);

}

#define IDT_RID_SHOW 1
#define IDT_RID_DEL  2
#define IDT_RID_ADD  3
#define IDT_RID_INIT  4

int
idt_rid( struct cli_cmd_para *c)
{
  int ret, i;
  struct pev_ioctl_rdwr rdwr_baddr;
  int data, offset;
  int op;
  int part, bus, dev, func;

  op = 0;
  if( c->cnt < 2)
  {
    op = IDT_RID_SHOW;
  }
  if( !op)
  {
    if( !strcmp( "show", c->para[1]))
    {
      op = IDT_RID_SHOW;
    }
    else if( !strcmp( "del", c->para[1]))
    {
      op = IDT_RID_DEL;
    }
    else if( !strcmp( "add", c->para[1]))
    {
      op = IDT_RID_ADD;
    }
    else if( !strcmp( "init", c->para[1]))
    {
      return( idt_rid_init());
    }
    else
    {
      printf("bad operation\n");
      goto idt_rid_usage;
    }
  }
  if( op > IDT_RID_SHOW)
  {
    if( sscanf( c->para[2],"%d.%d.%d.%d", &part, &bus, &dev, &func) != 4)
    {
      printf("bad RID identifier\n");
      goto idt_rid_usage;
    }
  }


  offset = 0;
  rdwr_baddr.mode.space = RDWR_BADDR;
  rdwr_baddr.mode.ds = RDWR_INT;
  rdwr_baddr.mode.swap = RDWR_SWAP;
  rdwr_baddr.mode.dir = RDWR_READ;
  rdwr_baddr.buf = (void *)&data;;
  rdwr_baddr.offset = idt_nt0_baddr + offset;
  rdwr_baddr.len = 0;
  ret = pev_rdwr( &rdwr_baddr);
  if( ret < 0)
  {
    printf("cannot access bus address 0x%x\n", rdwr_baddr.offset);
    return( -1);
  }
  if(( data != 0x808c111d) && (data != 0x8097111d))
  {
    printf("VID/DID inconsistent = %08x - 0x808c111d\n", data);
    return( -1);
  }
  if( op == IDT_RID_SHOW)
  {
    printf("+------------------------------+\n");
    printf("| Requester IDs Mapping Table  |\n");
    printf("+-----+------+-----+-----+-----+\n");
    printf("| idx | part | bus | dev | fun |\n");
    printf("+-----+------+-----+-----+-----+\n");
    for( i = 0; i < 64; i++)
    {
      /* Store root complex identifier of partition 0 in NT mapping table */
      rdwr_baddr.mode.dir = RDWR_WRITE;
      data = i;
      rdwr_baddr.offset = idt_nt0_baddr + 0x4d0;
      pev_rdwr( &rdwr_baddr);
      rdwr_baddr.mode.dir = RDWR_READ;
      rdwr_baddr.offset = idt_nt0_baddr + 0x4d8;
      pev_rdwr( &rdwr_baddr);
      if( data &1)
      {

	part = (data >> 17) & 7;
	bus  = (data >> 9) & 0xff;
	dev  = (data >> 4) & 0x1f;
	func = (data >> 1) & 7;
	printf("| %2d |  %3d  | %3d | %3d | %3d |\n", i, part, bus,dev,func);
      }
    }
    printf("+-----+------+-----+-----+-----+\n");
  }
  if( op == IDT_RID_DEL)
  {
    int tmp;

    for( i = 0; i < 64; i++)
    {
      /* Scan NT mapping table looking for entry to delete */
      rdwr_baddr.mode.dir = RDWR_WRITE;
      data = i;
      rdwr_baddr.offset = idt_nt0_baddr + 0x4d0;
      pev_rdwr( &rdwr_baddr);
      rdwr_baddr.mode.dir = RDWR_READ;
      rdwr_baddr.offset = idt_nt0_baddr + 0x4d8;
      pev_rdwr( &rdwr_baddr);
      tmp = ((part&7)<<17) | ((bus&0xff)<<9) | ((dev&0x1f)<<4) | ((func&7)<<1) | 1;
      if( tmp == data)
      {
         /* entry found -> remove entry from table*/
	printf("deleting RID %x from table [offset %d ]\n",data, i);
	data = 0;
	rdwr_baddr.mode.dir = RDWR_WRITE;
	pev_rdwr( &rdwr_baddr);
	break;
      }
    }
  }
  if( op == IDT_RID_ADD)
  {
    for( i = 0; i < 64; i++)
    {
      /* Scan NT mapping table for free entry */
      rdwr_baddr.mode.dir = RDWR_WRITE;
      data = i;
      rdwr_baddr.offset = idt_nt0_baddr + 0x4d0;
      pev_rdwr( &rdwr_baddr);
      rdwr_baddr.mode.dir = RDWR_READ;
      rdwr_baddr.offset = idt_nt0_baddr + 0x4d8;
      pev_rdwr( &rdwr_baddr);
      if( !(data &1))
      {
         /* free entry found -> add RID in table*/
	data = ((part&7)<<17) | ((bus&0xff)<<9) | ((dev&0x1f)<<4) | ((func&7)<<1) | 1;
	printf("adding RID %x in table at offset %d\n", data, i);
	rdwr_baddr.mode.dir = RDWR_WRITE;
	pev_rdwr( &rdwr_baddr);
	break;
      }
    }
  }

  return(0);
idt_rid_usage:
  printf("   usage: idt rid <op> <part>.<bus>.<dev>.<func>\n");
  return( -1);
}

int
idt_message( struct cli_cmd_para *c)
{
  int off;
  int port, msg, data, sts;

  port = -1;
  if( c->ext)
  {
    if( sscanf( c->ext,"%d", &port) == 1)
    {
      if( ( port < 0) || ( port > 24))
      {
	port = -1;
      }
    }
  }
  if( port == -1)
  {
    printf("bad port identifier\n");
    goto idt_msg_usage;
  }
  msg = -1;
  if( sscanf( c->para[0],"msg%d", &msg) == 1)
  {
    if( ( msg < 0) || ( msg > 3))
    {
      printf("bad message identifier\n");
      goto idt_msg_usage;
    }
  }

  if( msg == -1)
  {
    int op;

    op = 0;
    if( c->cnt < 2) op = 1;
    else
    {
      if( !strcmp( "status", c->para[1])) op = 1;
    }
    if( op)
    {
      off = (port << 13) | 0x1400;
      sts = idt_pex_rd( off + 0x60);
      printf("+-----+----------+----------+-------+\n");
      printf("| idx |  msgout  |   msgin  | state |\n");
      printf("+-----+----------+----------+-------+\n");
      for( msg = 0; msg < 4; msg++)
      {
	unsigned int msgout, msgin;

	msgout = idt_pex_rd( off | 0x30 | (msg<<2));
	msgin  = idt_pex_rd( off | 0x40 | (msg<<2));
	printf("| %3d | %08x | %08x |", msg, msgout, msgin);
        if( sts & (0x10000<<msg))
        {
	  printf("  full |\n");
	}
	else
	{
	  printf(" empty |\n");
	}
      }
      printf("+-----+----------+----------+-------+\n");
      return(0);
    }
    else
    {
      printf("bad message identifier\n");
      goto idt_msg_usage;
    }
  }
  off = (port << 13) | 0x1400;
  if( !strcmp( "post", c->para[1]))
  {
    if( c->cnt > 2)
    {
      if( sscanf( c->para[2],"%x", &data) != 1)
      {
        printf("bad message data\n");
        goto idt_msg_usage;
      }
      idt_pex_wr( off | 0x60, 0x1<<msg);
      idt_pex_wr( off | 0x30 | (msg<<2), data);
      sts = idt_pex_rd( off + 0x60);
      if( sts & (0x1<<msg))
      {
	printf("%s : post failed\n", c->para[0]);
      }
      else 
      {
	printf("%s : post OK\n", c->para[0]);
      }
    }
    else
    {
        printf("need msg data\n");
        goto idt_msg_usage;
    }
    return(0);
  }
  if( !strcmp( "get", c->para[1]))
  {
    sts = idt_pex_rd( off | 0x60);
    data = idt_pex_rd( off | 0x40 | (msg<<2));
    if( sts & (0x10000<<msg))
    {
      printf("%s : %08x [full]\n", c->para[0], data);
      idt_pex_wr( off + 0x60, 0x10000<<msg);
    }
    else
    {
      printf("%s : %08x [empty]\n", c->para[0], data);
    }
    return(0);
  }
  if( !strcmp( "status", c->para[1]))
  {
    return(0);
  }

idt_msg_usage:
  printf("   usage: idt.<port> msg<i> <op> [<data>]\n");
  return( -1);
}

int 
xprs_idt( struct cli_cmd_para *c)
{
  if( c->cnt < 1)
  {
    printf("idt command needs more arguments\n");
    return(-1);
  }
  if( pev_board() != PEV_BOARD_IFC1210)
  {
    printf("idt command not supported on %s\n", pev_board_name());
  }

  if( !idt_nt0_baddr)
  {
    idt_rid_init();
  }

  if( !strcmp( "conf", c->para[0]))
  {
    return( idt_conf( c));
  }
  if( !strcmp( "load", c->para[0]))
  {
    return( idt_load( c));
  }

  if( !strcmp( "map", c->para[0]))
  {
    return( idt_map( c));
  }

  if( !strcmp( "rid", c->para[0]))
  {
    return( idt_rid( c));
  }

  if( !strncmp( "msg", c->para[0], 3))
  {
    return( idt_message( c));
  }

  return(0);
}
