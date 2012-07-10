/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : rdwr.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to perform read
 *     or write cycles through the PEV1000 interface.
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
 * $Log: rdwr.c,v $
 * Revision 1.7  2012/07/10 10:21:48  kalantari
 * added tosca driver release 4.15 from ioxos
 *
 * Revision 1.26  2012/06/01 13:59:44  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.25  2012/05/23 15:27:37  ioxos
 * remove reference to pev_evt_xx() [JFG]
 *
 * Revision 1.24  2012/04/12 13:31:07  ioxos
 * support for dma swapping [JFG]
 *
 * Revision 1.23  2012/03/27 09:17:41  ioxos
 * add support for FIFOs [JFG]
 *
 * Revision 1.22  2012/03/21 11:28:30  ioxos
 * buf for CSR access [JFG]
 *
 * Revision 1.21  2012/03/05 09:53:47  ioxos
 * adjust casting of rdwr_filename [JFG]
 *
 * Revision 1.20  2012/02/24 10:45:04  ioxos
 * bug: cp->op initialize to 0 [JFG]
 *
 * Revision 1.19  2012/02/14 16:08:03  ioxos
 * add support DMA byte swapping + bug in command pu [JFG]
 *
 * Revision 1.18  2012/02/03 16:27:37  ioxos
 * dynamic use of elbc for i2c [JFG]
 *
 * Revision 1.17  2012/01/27 15:55:44  ioxos
 * prepare release 4.01 supporting x86 & ppc [JFG]
 *
 * Revision 1.16  2011/10/03 10:07:51  ioxos
 * set read cacle as default [JFG]
 *
 * Revision 1.15  2011/03/03 15:42:38  ioxos
 * support for 1MBytes VME slave granularity [JFG]
 *
 * Revision 1.14  2011/01/25 13:48:19  ioxos
 * support for VME RMW [JFG]
 *
 * Revision 1.13  2009/10/21 07:22:04  ioxos
 * correct bug in rdwr_fill_buf() [JFG]
 *
 * Revision 1.12  2009/06/03 12:30:06  ioxos
 * use buf_alloc instead of dma_alloc [JFG]
 *
 * Revision 1.11  2009/06/02 11:59:16  ioxos
 * suppress debug message in xprs_rdwr_i2c()  [jfg]
 *
 * Revision 1.10  2009/05/20 14:58:53  ioxos
 * add support for XENOMAI real time driver (dma function) [JFG]
 *
 * Revision 1.9  2009/01/06 13:39:00  ioxos
 * define string for VME AM [JFG]
 *
 * Revision 1.8  2008/12/12 14:12:44  jfg
 * suppor for pci addresses, address extension and DMA transfer [JFG]
 *
 * Revision 1.7  2008/11/12 13:56:07  ioxos
 *  display cycle info in case of error in tm [JFG]
 *
 * Revision 1.6  2008/09/17 13:03:50  ioxos
 * cleanup tm + 64 kloop for lm + remove tst [JFG]
 *
 * Revision 1.5  2008/08/08 12:15:35  ioxos
 * support to access PEX86xx and PLX8112 registers and DMA buffer [JFG]
 *
 * Revision 1.4  2008/07/18 14:04:30  ioxos
 * debug tm+lm, add loop count, pm single read, re-organize fm [JFG]
 *
 * Revision 1.3  2008/07/04 07:49:26  ioxos
 * add address mapping functions [JFG]
 *
 * Revision 1.2  2008/07/01 10:40:12  ioxos
 * set first page of sg_mas_32 to pint to shared memory [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: rdwr.c,v 1.7 2012/07/10 10:21:48 kalantari Exp $";
#endif

#define DEBUGno
#include <debug.h>

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <cli.h>
#include <rdwr.h>
#include <pevioctl.h>
#include <pevulib.h>
#include <aio.h>
#include <errno.h>
#include <ctype.h>

#ifdef XENOMAI
#include <pevrtlib.h>
#endif

struct cli_cmd_history pm_history;

/* Holds the parameters of the last cycle executed in the config space */
struct rdwr_cycle_para last_cfg_cycle =
{
  0,
  0,
  0x0, 0, 'r', 32,
  'd', 'c', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last cycle executed in the io space */
struct rdwr_cycle_para last_io_cycle =
{
  0,
  0,
  0x0, 0, 'r', 32,
  'd', 'i', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last cycle executed in the csr space */
struct rdwr_cycle_para last_csr_cycle =
{
  0,
  0,
  0x0, 0, 'r', 32,
  'd', 'r', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last cycle executed in the elb space */
struct rdwr_cycle_para last_elb_cycle =
{
  0,
  0,
  0x0, 0, 'r', 32,
  'd', 'e', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last VME cycle executed */
struct rdwr_cycle_para last_vme_cycle =
{
  0,
  0,
  0x13, 0, 'r', 32,
  'd', 'v', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last RMW cycle executed */
struct rdwr_cycle_para last_rmw_cycle =
{
  0,
  0,
  0x9, 0, 'r', 32,
  'd', 'v', 'w', 0,
  0x0,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last cycle executed in the shared memory */
struct rdwr_cycle_para last_shm1_cycle =
{
  0,
  0,
  0x20, 0, 'r', 32,
  'd', 's', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last cycle executed in the shared memory */
struct rdwr_cycle_para last_shm2_cycle =
{
  0,
  0,
  0x30, 0, 'r', 32,
  'd', 's', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last cycle executed in the shared memory */
struct rdwr_cycle_para last_usr1_cycle =
{
  0,
  0,
  0x40, 0, 'r', 32,
  'd', 's', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last cycle executed in the shared memory */
struct rdwr_cycle_para last_usr2_cycle =
{
  0,
  0,
  0x50, 0, 'r', 32,
  'd', 's', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last cycle executed in the PEX86XX registers */
struct rdwr_cycle_para last_pex_cycle =
{
  0,
  0,
  0x20, 0, 'r', 32,
  'd', 'x', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last cycle executed in kernel space */
struct rdwr_cycle_para last_kmem_cycle =
{
  0,
  0,
  0x20, 0, 'r', 32,
  'd', 'm', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last PCI cycle executed */
struct rdwr_cycle_para last_pci_cycle =
{
  0,
  0,
  0x13, 0, 'r', 32,
  'd', 'v', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

/* Holds the parameters of the last DMA cycle executed */
struct rdwr_cycle_para last_dma_cycle =
{
  0,
  0,
  0x13, 0, 'r', 32,
  'd', 'v', 'w', 0,
  0x40,
  0x0,
  0x0,
  0x0,
  0x0
};

struct pev_ioctl_map_pg mas_map;
#define vme_mas_map mas_map
#define shm_mas_map mas_map
#ifdef JFG
struct pev_ioctl_map_pg vme_mas_map;
struct pev_ioctl_map_pg shm_mas_map;
#endif
struct pev_ioctl_map_pg shm_slv_map;
struct pev_ioctl_map_pg kmem_slv_map;
struct pev_ioctl_map_pg csr_slv_map;
extern struct aiocb aiocb;
extern char aio_buf[256];
struct pev_ioctl_buf dma_buf;
struct pev_ioctl_dma_req dma_req;
char rdwr_filename[256];
int rdwr_board = 0;

char *
rdwr_rcsid()
{
  return( rcsid);
}

int 
rdwr_init( void)
{
  cli_history_init( &pm_history);
  rdwr_board = pev_board();

  mas_map.rem_addr = 0x0;
  mas_map.mode = 0x0;
  mas_map.flag = 0x0;
  mas_map.sg_id = MAP_MASTER_32;
  mas_map.size = 0x100000;
  pev_map_alloc( &mas_map);

  if(( rdwr_board == PEV_BOARD_PEV1100) ||
     ( rdwr_board == PEV_BOARD_IPV1102) ||
     ( rdwr_board == PEV_BOARD_VCC1104)    )
  {
    last_usr1_cycle.am = 0x30;
  }

#ifdef JFG
  shm_mas_map.rem_addr = 0x0;
  shm_mas_map.mode = 0x2003;
  shm_mas_map.flag = 0x0;
  shm_mas_map.sg_id = MAP_MASTER_32;
  shm_mas_map.size = 0x100000;
  pev_map_alloc( &shm_mas_map);

  vme_mas_map.rem_addr = 0x0;
  vme_mas_map.mode = 0x1303;
  vme_mas_map.flag = 0x0;
  vme_mas_map.sg_id = MAP_MASTER_32;
  vme_mas_map.size = 0x100000;
  pev_map_alloc( &vme_mas_map);
#endif

  shm_slv_map.rem_addr = 0x0;
  shm_slv_map.mode = 0x2003;
  shm_slv_map.flag = 0x0;
  shm_slv_map.sg_id = MAP_SLAVE_VME;
  shm_slv_map.size = 0x100000;
  pev_map_alloc( &shm_slv_map);

  dma_buf.size = 0x100000;
  dma_buf.k_addr = 0;
  pev_buf_alloc( &dma_buf);
  if( dma_buf.k_addr)
  {
    kmem_slv_map.rem_addr = (ulong)dma_buf.b_addr;
    kmem_slv_map.mode = 0x3;
    kmem_slv_map.flag = 0x0;
    kmem_slv_map.sg_id = MAP_SLAVE_VME;
    kmem_slv_map.size = dma_buf.size;
    pev_map_alloc( &kmem_slv_map);
  }

  csr_slv_map.rem_addr = 0x0;
  csr_slv_map.mode = 0x3003;
  csr_slv_map.flag = 0x0;
  csr_slv_map.sg_id = MAP_SLAVE_VME;
  csr_slv_map.size = 0x100000;
  pev_map_alloc( &csr_slv_map);

  bzero( (char *)&aiocb, sizeof(struct aiocb) );
  aiocb.aio_fildes = STDIN_FILENO;
  aiocb.aio_buf = aio_buf;
  aiocb.aio_nbytes = 1;

  return(0);
}

int 
rdwr_exit( void)
{
  pev_map_free( &mas_map);
#ifdef JFG
  pev_map_free( &vme_mas_map);
  pev_map_free( &shm_mas_map);
#endif
  pev_map_free( &shm_slv_map);
  if( dma_buf.k_addr)
  {
    pev_map_free( &kmem_slv_map);
    pev_buf_free( &dma_buf);
  }
  pev_map_free( &csr_slv_map);
  return(0);
}

/* decode address parameters */
int 
rdwr_get_cycle_addr( char *addr_p,
		     struct rdwr_cycle_para *cp)
{ 
  char *p;
  unsigned long end;

  cp->addr = strtoul( addr_p, &p, 16);
  if( p == addr_p)
  {
    cp->error |= RDWR_STS_ADDR;
    return(-1);
  }
  cp->len = 0x40;
  p = strpbrk( addr_p,".:");
  if( p)
  {
    if( *p == '.')
    {
      addr_p = p + strspn(p,".");
      end =  strtoul( addr_p, &p, 16);
      if( p != addr_p)
      {
        if( end > cp->addr)
        {
          cp->len = end - cp->addr;
        }
        else
        {
          cp->error |= RDWR_STS_LEN;
        }
      }
      else
      {
        cp->error |= RDWR_STS_LEN;
	return( -1);
      }
      p = strpbrk( addr_p,":");
      if( !p)
      {
	return( 0);
      }
    }
    if( *p == ':')
    {
      addr_p = p + 1;
      cp->am =  (unsigned char)strtoul( addr_p, &p, 16);
    }
  }
  return(0);
}

/* decode data parameters */
int
rdwr_get_cycle_data( char *data_p,
		     struct rdwr_cycle_para *cp)
{
  char *p;

  cp->op = 0;
  if( data_p[0] == '?')
  {
    cp->dir = 'r';
    return(0);
  }
  if( data_p[0] == '.')
  {
    cp->dir = 'r';
    cp->op = '.';
    return(0);
  }
  if( data_p[1] == ':')
  {
    p = strpbrk( data_p,":");
    if( !p)
    {
      cp->error |= RDWR_STS_DATA;
      return(-1);
    }
    cp->op = data_p[0];
    data_p = p + strspn(p,":");
    if( cp->op == 'f') /* get data from file */
    {
      cp->dir = 'w';
      strcpy( rdwr_filename, data_p); /* save file name */
      cp->para = (ulong)rdwr_filename;   /* data parameters point to filename */
      return(0);
    }
  }  
  cp->data = strtoul( data_p, &p, 16);
  if( p == data_p)
  {
    cp->error |= RDWR_STS_DATA;
    return(-1);
  }
  cp->dir = 'w';
  p = strpbrk( data_p,".");
  if( p)
  {
    data_p = p + strspn(p,".");
    cp->para =  strtoll( data_p, &p, 16);
    if( p == data_p)
    {
      cp->error |= RDWR_STS_DATA;
    }
  }
  return(0);
}

/* decode control parameters */
char
rdwr_get_vme_am( char *name)
{
  char am;
  am = -1;

  if( !strncmp("cr", name, 2)) am = 0x10;
  if( !strncmp("a16", name, 3)) am = 0x11;
  if( !strncmp("a24", name, 3)) am = 0x12;
  if( !strncmp("a32", name, 3)) am = 0x13;
  if( !strncmp("blt", name, 3)) am = 0x14;
  if( !strncmp("mblt", name, 4)) am = 0x15;
  if( !strncmp("2evme", name, 4)) am = 0x16;
  if( !strncmp("2efast", name, 4)) am = 0x17;
  if( !strncmp("2e160", name, 4)) am = 0x18;
  if( !strncmp("2e233", name, 4)) am = 0x19;
  if( !strncmp("2e320", name, 4)) am = 0x1a;
  if( !strncmp("ao24", name, 3)) am = 0x1d;
  if( !strncmp("ao32", name, 3)) am = 0x1e;
  if( !strncmp("iack", name, 4)) am = 0x1f;

  return(am);
}


int
rdwr_get_cycle_arg( char *arg_p,
		    struct rdwr_cycle_para *cp)
{
  char *p;
  char *arg;
  long err;
  int ds;

  arg = (char *)0;
  if( arg_p[0] == 'm')
  {
    arg = &cp->am;
    err = RDWR_STS_AM;
    ds = 1;
  }
  if( arg_p[0] == 'c')
  {
    arg = &cp->crate;
    err = RDWR_STS_CRATE;
    ds = 1;
  }
  if( arg_p[0] == 'a')
  {
    arg = &cp->as;
    err = RDWR_STS_AS;
    ds = 1;
  }
  if( arg_p[0] == 'l')
  {
    arg = (char *)&cp->loop;
    err = RDWR_STS_LOOP;
    ds = 4;
  }
  if( arg_p[0] == 'f')
  {
    arg = (char *)&cp->para;
    err = RDWR_STS_FILE;
    ds = 4;
  }
  if( !arg)
  {
    return( -1);
  }
  p = strpbrk( arg_p,":");
  if( !p)
  {
    cp->error |= err;
    return(-1);
  }
  arg_p = p + strspn(p,":");
  if( err == RDWR_STS_FILE)
  {
    cp->op = 'f';
    strcpy( rdwr_filename, arg_p); /* save file name */
    cp->para = (ulong)rdwr_filename;   /* data parameters point to filename */
    return(0);
  }
  if( err == RDWR_STS_AM)
  {
    char am;

    am = rdwr_get_vme_am( arg_p);
    if( am > 0)
    {
      *arg = am;
      return(0);
    }
    *arg = (char)strtoul( arg_p, &p, 16);
  }
  else
  {
    if( ds == 1)
    {
      *arg = (char)strtoul( arg_p, &p, 0);
    }
    if( ds == 4)
    {
      *(int *)arg = (int)strtoul( arg_p, &p, 0);
    }
  }
  if( p == arg_p)
  {
    cp->error |= err;
    return(-1);
  }
  return(0);
}


/* decode cycle parameters */
int 
rdwr_get_cycle_para( struct cli_cmd_para *c,
		     struct rdwr_cycle_para *cp)
{
  long i, i0;

  cp->error = 0;
  cp->op = c->cmd[0];
  cp->space = c->cmd[1];
  cp->swap = 0;
  if( c->ext)
  {
    cp->ds = c->ext[0];
    cp->swap = c->ext[1];
  }
  cp->dir = 'r';
  i0 = 0;
  if( cp->space == 'd') i0 = 1; 
  for( i = i0; i < c->cnt; i++)
  {
    if( i == i0)
    {
      /* address + len */
      rdwr_get_cycle_addr( c->para[i], cp);
    }
    if( i == (i0+1))
    {
      if( cp->op == 'd')
      {
	rdwr_get_cycle_arg( c->para[i], cp);
      }
      else
      {
	/* address + len */
	rdwr_get_cycle_data( c->para[i], cp);
      }
    }
    if( i > (i0+1))
    {
      rdwr_get_cycle_arg( c->para[i], cp);
    }
  }

  return(0);
}

/* set cycle mode */
int 
rdwr_set_cycle_mode( struct rdwr_cycle_para *cp)
{

  cp->mode = 0;
  /* set data size */
  if( ( cp->ds == 'c') || ( cp->ds == 'b')) cp->mode = 1;
  if( ( cp->ds == 's') || ( cp->ds == 'h')) cp->mode = 2;
  if( ( cp->ds == 'i') || ( cp->ds == 'w')) cp->mode = 4;
  if( ( cp->ds == 'l') || ( cp->ds == 'd')) cp->mode = 8;

  /* set swapping */
  if( cp->swap == 's') cp->mode |= 0x10;

  /* set address size */
  cp->mode |= cp->as << 8;

  /* set address modifier */
  cp->mode |= cp->am << 16;

  return( cp->mode);
}

/* set cycle mode */
int 
rdwr_set_ioctl_arg( struct pev_ioctl_rdwr *rdwr_p,
		    struct rdwr_cycle_para *cp)
{
  struct pev_rdwr_mode *p;

  p = &rdwr_p->mode;
  p->ds = 0;
  /* set data size */
  if( ( cp->ds == 'c') || ( cp->ds == 'b')) p->ds = RDWR_BYTE;
  if( ( cp->ds == 's') || ( cp->ds == 'h')) p->ds = RDWR_SHORT;
  if( ( cp->ds == 'i') || ( cp->ds == 'w')) p->ds = RDWR_INT;
  if( ( cp->ds == 'l') || ( cp->ds == 'd')) p->ds = RDWR_LONG;

  /* set swapping */
  p->swap = 0;
  if( cp->swap == 's') p->swap = RDWR_SWAP;

  /* set address size */
  p->as = cp->as;

  /* set transfer direction */
  p->dir = RDWR_READ;
  if( cp->dir == 'w') p->dir = RDWR_WRITE;

  /* set crate number */
  p->crate = cp->crate;

  /* set address modifier */
  p->am = cp->am;

  /* set address space */
  p->space = 0;
  if( cp->space == 'c') p->space = RDWR_CFG;
  if( cp->space == 'i') p->space = RDWR_IO;
  if( cp->space == 's') p->space = RDWR_MEM;
  if( cp->space == 'u') p->space = RDWR_MEM;
  if( cp->space == 'v') p->space = RDWR_MEM;
  if( cp->space == 'p') p->space = RDWR_MEM;
  if( cp->space == 'r') p->space = RDWR_CSR;
  if( cp->space == 'e') p->space = RDWR_ELB;
  if( cp->space == 'x') p->space = RDWR_PEX;
  if( cp->space == 'm') p->space = RDWR_KMEM;

  return( cp->mode);
}

int 
rdwr_save_buf( char *buf,
	       struct rdwr_cycle_para *cp,
	       int mode)
{
  FILE *file;

  if( cp->op == 'f')
  {
    int off;

    if( mode & 1)
    {
      file = fopen( (char *)cp->para, "a");
    }
    else
    {
      file = fopen( (char *)cp->para, "w");
    }
    if( !file)
    {
       printf("\nCannot open file %s\n", (char *)cp->para);
       return( -1);
    }
    //fseek( file, 0, SEEK_END);
    off = ftell( file);
    fwrite( buf, 1, cp->len, file);
    fclose( file);
    printf("0x%x bytes written in file %s at offset 0x%x\n", cp->len, (char *)cp->para, off);
    return( 0);
  }
  else
  {
    printf("You need to provide a filename !!!\n");
    return( -1);
  }
}

int 
rdwr_fill_buf( void *buf,
	       struct rdwr_cycle_para *cp)
{
  int i, ds;
  FILE *file;

  ds = cp->mode & 0xf;
  if( cp->op == 'r') srandom( (uint)cp->data);
  if( cp->op == 'f')
  {
    int size;

    file = fopen( (char *)cp->para, "r");
    if( !file)
    {
       printf("\nFile %s doesn't exist\n", (char *)cp->para);
       return( -1);
    }
    fseek( file, 0, SEEK_END);
    size = ftell( file);
    if( size < cp->len)
    {
      printf("\nFile %s too short [%x]\n", (char *)cp->para, size);
      fclose( file);
       return( -1);
    }
    fseek( file, 0, SEEK_SET);
    fread( buf, 1, cp->len, file);
    fclose( file);
    return( 0);
  }
  switch( ds)
  {
    case 1:
    {
      char *p;
      char data;

      p = (char *)buf;
      data = (char)cp->data;
      for( i = 0; i < cp->len; i+=1)
      {
	if( cp->op == 'w')
	{ 
	  char para;
	  int shift;

	  shift = i & 0x7;
	  para = ( (char)cp->para << shift) | ( (char)cp->para >> ( 0x8 - shift));
	  data =  cp->data ^ para;
	}
        *p++ = data;
	if( cp->op == 'r') data =  (char)random();
	if( cp->op == 's') data +=  (char)cp->para;
      }
      cp->data = data;
      break;
    }
    case 2:
    {
      ushort *p;
      ushort data;

      p = (ushort *)buf;
      data = (ushort)cp->data;
      for( i = 0; i < cp->len; i+=2)
      {
	if( cp->op == 'w')
	{ 
	  ushort para;
	  int shift;

	  shift = (i/2) & 0xf;
	  para = ((ushort)cp->para << shift) | ((ushort)cp->para >> ( 0x10 - shift));
	  data =  (ushort)cp->data ^ para;
	}
        *p++ = data;
	if( cp->op == 'r') data =  (ushort)random();
	if( cp->op == 's') data +=  (ushort)cp->para;
      }
      cp->data = data;
      break;
    }
    case 4:
    {
      uint *p;
      uint data;

      p = (uint *)buf;
      data = (uint)cp->data;
      for( i = 0; i < cp->len; i+=4)
      {
	if( cp->op == 'w')
	{ 
	  uint para;
	  int shift;

	  shift = (i/4) & 0x1f;
	  para = ((uint)cp->para << shift) | ((uint)cp->para >> ( 0x20 - shift));
	  data =  (uint)cp->data ^ para;
	}
        *p++ = data;
	if( cp->op == 'r') data =  (uint)((random()<<24) + random());
	if( cp->op == 's') data +=  (uint)cp->para;
      }
      cp->data = data;
      break;
    }
#if defined(X86_64)
    case 8:
    {
      ulong *p;
      ulong data;

      p = (ulong *)buf;
      data = cp->data;
      for( i = 0; i < cp->len; i+=8)
      {
	if( cp->op == 'w')
	{ 
	  ulong para;
	  int shift;

	  shift = (i/8) & 0x3f;
	  para = (cp->para << shift) | (cp->para >> ( 0x40 - shift));
	  data =  cp->data ^ para;
	}
        *p++ = data;
	if( cp->op == 'r') data =  (random()<<48)+ (random()<<24) + random();
	if( cp->op == 's') data +=  cp->para;
      }
      cp->data = data;
      break;
    }
#endif
    default:
    {
      return(-1);
    }
  }
  return(0);
}



void
rdwr_show_addr( ulong addr,
		int mode)
{
  int as;

  as = (mode >> 8)&0xff;
  switch( as)
  {
    case 16: /* A16 */
    {
      printf("0x%04x : ", (ushort)addr);
      break;
    }
    case 24: /* A24 */
    {
      printf("0x%06x : ", (uint)addr);
      break;
    }
    case 64: /* A64 */
    {
      printf("0x%016lx : ", addr);
      break;
    }
    default: /* A32 */
    {
      printf("0x%08x : ", (uint)addr);
      break;
    }
  }
  return;
}

int 
rdwr_show_buf( ulong addr, 
	       void *buf,
	       int len,
	       int mode)
{
  unsigned char *p;
  long i, j;
  long ds;

  p = (unsigned char *)buf;
  ds = mode & 0xf;
  for( i = 0; i < len; i += 16)
  {
    rdwr_show_addr( addr, mode);
    for( j = 0; j < 16; j += ds)
    {
      if( ( i + j) >= len)
      {
	long n;
	n = ((2*ds)+1)*((16-j)/ds);
	while( n--)
	{
	  putchar(' ');
	}
	break;
      }
      switch( ds)
      {
        case 1:
	{
	  printf("%02x ", p[j]);
	  break;
	}
        case 2:
	{
	  printf("%04x ", *(ushort *)&p[j]);
	  break;
	}
        case 4:
	{
	  printf("%08x ", *(uint *)&p[j]);
	  break;
	}
        case 8:
	{
	  printf("%016lx ", *(ulong *)&p[j]);
	  break;
	}
      }
    }
    printf(" ");
    for( j = 0; j < 16; j++)
    {
      char c;
      if( ( i + j) >= len)
      {
	break;
      }

      c = p[j];
      if( !isprint(c)) c = '.';
      printf("%c", c);
    }
    printf("\n");
    p += 16;
    addr += 16;
  }
  return(0);
}

int 
rdwr_cmp_buf( void *buf1, 
	      void *buf2,
	      int len)
{
  long *p1, *p2;
  int i;

  p1 = (long *)buf1;
  p2 = (long *)buf2;
  i = 0;
  while( i < len)
  {
    if( *p1++ != *p2++)
    {
      break;
    }
    i += sizeof( long);
  }
  return( i);
}

ulong
rdwr_set_map( struct rdwr_cycle_para *cp,
	      struct pev_ioctl_map_pg *mp)
{
  ushort mm;

  mm = cp->am << 8 | 0x3;
  if( !mp)
  {
    return( cp->addr);
  }
  if( ( cp->addr >= mp->rem_base) &&
      ( ( cp->addr + cp->len) <= ( mp->rem_base + mp->win_size)) &&
      ( mm == mp->mode))
  {
    return( mp->loc_base + cp->addr - mp->rem_base);
  }
  mp->rem_addr = cp->addr;
  mp->mode = mm;
  pev_map_modify( mp);
  if(( cp->addr + cp->len) > ( mp->rem_base + mp->win_size))
  {
    return( -1);
  }
  return( mp->loc_addr);
}

int 
xprs_rdwr_dm( struct cli_cmd_para *c)
{
  struct rdwr_cycle_para *cp;
  struct pev_ioctl_rdwr rdwr;
  struct pev_ioctl_map_pg *mp;
  void *buf;
  int i;
  int ret;
  int first, last, n, append;

  mp = (struct pev_ioctl_map_pg *)0;
  if( c->cmd[1] == 'e') cp = &last_elb_cycle;
  if( c->cmd[1] == 'r') cp = &last_csr_cycle;
  if( c->cmd[1] == 'v')
  {
    cp = &last_vme_cycle;
    mp = &vme_mas_map;
  }
  if( c->cmd[1] == 's')
  {
    cp = &last_shm1_cycle;
    if( c->cmd[2] == '2')
    {
      cp = &last_shm2_cycle;
    }
    mp = &shm_mas_map;
  }
  if( c->cmd[1] == 'u')
  {
    cp = &last_usr1_cycle;
    if( c->cmd[2] == '2')
    {
      cp = &last_usr2_cycle;
    }
    mp = &shm_mas_map;
  }
  if( c->cmd[1] == 'm')
  {
    cp = &last_kmem_cycle;
    if( !dma_buf.k_addr)
    {
      printf("kernel buffer has not been allocated !!! \n");
      return(-1);
    }
    rdwr.k_addr = dma_buf.k_addr;
  }
  if( c->cmd[1] == 'p')
  {
    cp = &last_pci_cycle;
  }

  cp->op = 0;
  rdwr_get_cycle_para( c, cp);
  if( cp->crate == -1)
  {
    printf("crate number not defined !!! \n");
    return(-1);
  }

  rdwr_set_cycle_mode( cp);

  first = 0x100000 - (int)(cp->addr & 0xfffff);
  if( cp->len <= first)
  {
    first = cp->len;
    last = 0;
    n = 0;
  }
  else
  {
    last = (int)(( cp->addr + cp->len) & 0xfffff);
    n = ( cp->len - first - last) >> 20;
  }
  append = 0;
  if( first)
  {
    buf = (char *)malloc( first);
    rdwr.buf = buf;
    cp->len = first;
    rdwr.offset = rdwr_set_map( cp, mp);
    if( rdwr.offset == -1)
    {
      printf("cannot map remote address : %lx\n", cp->addr);
      goto xprs_rdwr_dm_error;
    }

    rdwr_set_ioctl_arg( &rdwr, cp);
    rdwr.len =  cp->len;
    rdwr.mode.dir = RDWR_READ;

    ret = pev_rdwr( &rdwr);
    if( ret < 0)
    {
      printf("cannot read data\n");
      goto xprs_rdwr_dm_error;
    }
    if( c->cmd[0] == 'r')
    {
      if( rdwr_save_buf( buf, cp, append) < 0)
      {
	return( -1);
      }
      append = 1;
    }
    else
    {
      rdwr_show_buf( cp->addr, buf, first, cp->mode);
    }
    cp->addr += cp->len;
    free( buf);
  }
  if( n)
  {
    buf = (char *)malloc( 0x100000);
    rdwr.buf = buf;
    for( i = 0; i < n; i++)
    {
      cp->len = 0x100000;
      rdwr.offset = rdwr_set_map( cp, mp);
      if( rdwr.offset == -1)
      {
	goto xprs_rdwr_dm_error;
        return( -1);
      }

      rdwr_set_ioctl_arg( &rdwr, cp);
      rdwr.mode.dir = RDWR_READ;
      rdwr.len =  cp->len;
      ret = pev_rdwr( &rdwr);
      if( ret < 0)
      {
        printf("cannot read data\n");
	goto xprs_rdwr_dm_error;
      }
      if( c->cmd[0] == 'r')
      {
        if( rdwr_save_buf( buf, cp, append) < 0)
        {
  	  return( -1);
        }
	append = 1;
      }
      else
      {
        rdwr_show_buf( cp->addr, buf, 0x100000, cp->mode);
      }
      cp->addr += cp->len;    
    }
    free( buf);
  }
  if( last)
  {
    buf = (char *)malloc( last);
    rdwr.buf = buf;
    cp->len = last;
    rdwr.offset = rdwr_set_map( cp, mp);
    if( rdwr.offset == -1)
    {
      goto xprs_rdwr_dm_error;
    }

    rdwr_set_ioctl_arg( &rdwr, cp);
    rdwr.mode.dir = RDWR_READ;
    rdwr.len =  cp->len;
    ret = pev_rdwr( &rdwr);
    if( ret < 0)
    {
      printf("cannot read data\n");
      goto xprs_rdwr_dm_error;
    }
    if( c->cmd[0] == 'r')
    {
      if( rdwr_save_buf( buf, cp, append) < 0)
      {
	return( -1);
      }
      append = 1;
    }
    else
    {
      rdwr_show_buf( cp->addr, buf, last, cp->mode);
    }
    cp->addr += cp->len;
    free( buf);
  }
  return(0);

xprs_rdwr_dm_error:
  free( buf);
  return( -1);
}

int 
xprs_rdwr_fm( struct cli_cmd_para *c)
{
  struct rdwr_cycle_para *cp;
  struct pev_ioctl_rdwr rdwr;
  struct pev_ioctl_map_pg *mp;
  int i;
  int ret;
  int first, last, n;

  ret = 0;

  if( c->cmd[1] == 'c') cp = &last_cfg_cycle;
  if( c->cmd[1] == 'i') cp = &last_io_cycle;
  mp = (struct pev_ioctl_map_pg *)0;
  if( c->cmd[1] == 'v')
  {
    cp = &last_vme_cycle;
    mp = &vme_mas_map;
  }
  if( c->cmd[1] == 's')
  {
    cp = &last_shm1_cycle;
    if( c->cmd[2] == '2')
    {
      cp = &last_shm2_cycle;
    }
    mp = &shm_mas_map;
  }
  if( c->cmd[1] == 'u')
  {
    cp = &last_usr1_cycle;
    if( c->cmd[2] == '2')
    {
      cp = &last_usr2_cycle;
    }
    mp = &shm_mas_map;
  }
  if( c->cmd[1] == 'm')
  {
    cp = &last_kmem_cycle;
    if( !dma_buf.k_addr)
    {
      printf("kernel buffer has not been allocated !!! \n");
      return(-1);
    }
    rdwr.k_addr = dma_buf.k_addr;
  }
  if( c->cmd[1] == 'p')
  {
    cp = &last_pci_cycle;
  }

  cp->loop = 1;
  rdwr_get_cycle_para( c, cp);
  if( cp->crate == -1)
  {
    printf("crate number not defined !!! \n");
    return(-1);
  }
  rdwr_set_cycle_mode( cp);

  first = 0x100000 - (int)(cp->addr & 0xfffff);
  if( cp->len <= first)
  {
    first = cp->len;
    last = 0;
    n = 0;
  }
  else
  {
    last = (int)(( cp->addr + cp->len) & 0xfffff);
    n = ( cp->len - first - last) >> 20;
  }

  if( first)
  {
    cp->len = first;
    rdwr.buf = (char *)malloc( cp->len);
    rdwr.offset = rdwr_set_map( cp, mp);
    if( rdwr.offset == -1)
    {
      printf("cannot map remote address : %lx\n", cp->addr);
      goto xprs_rdwr_fm_error;
    }

    rdwr_set_ioctl_arg( &rdwr, cp);
    rdwr.len =  first;
    rdwr.mode.dir = RDWR_WRITE;

    rdwr_fill_buf( rdwr.buf, cp);
    ret = pev_rdwr( &rdwr);
    if( ret < 0)
    {
      printf("cannot write data\n");
      goto xprs_rdwr_fm_error;
    }
    free( rdwr.buf);
    cp->addr += cp->len;
  }
  if( n)
  {
    cp->len = 0x100000;
    rdwr.buf = (char *)malloc( cp->len);
    for( i = 0; i < n; i++)
    {
      rdwr.offset = rdwr_set_map( cp, mp);
      if( rdwr.offset == -1)
      {
	goto xprs_rdwr_fm_error;
        return( -1);
      }

      rdwr_set_ioctl_arg( &rdwr, cp);
      rdwr.mode.dir = RDWR_WRITE;
      rdwr.len =  cp->len;
      rdwr_fill_buf( rdwr.buf, cp);
      ret = pev_rdwr( &rdwr);
      if( ret < 0)
      {
        printf("cannot write data\n");
	goto xprs_rdwr_fm_error;
      }
      cp->addr += cp->len;
    }
    free( rdwr.buf);
  }
  if( last)
  {
    cp->len = last;
    rdwr.buf = (char *)malloc( cp->len);
    rdwr.offset = rdwr_set_map( cp, mp);
    if( rdwr.offset == -1)
    {
      goto xprs_rdwr_fm_error;
    }

    rdwr_set_ioctl_arg( &rdwr, cp);
    rdwr.mode.dir = RDWR_WRITE;
    rdwr.len =  cp->len;
    rdwr_fill_buf( rdwr.buf, cp);
    ret = pev_rdwr( &rdwr);
    if( ret < 0)
    {
      printf("cannot read data\n");
      goto xprs_rdwr_fm_error;
    }
    cp->addr += cp->len;
    free( rdwr.buf);
  }
  return( 0);

xprs_rdwr_fm_error:
  free( rdwr.buf);
  return( ret);
}

int
rdwr_tm_error( void *buf_in,
	       void *buf_out,
	       uint idx,
	       int ds)
{
  printf("data error at address %x \n", idx);
  idx &= 0xffff8;
  switch( ds)
  {
    case RDWR_BYTE:
    {
      unsigned char *p;

      p = (unsigned char *)buf_in + idx;
      printf("expected pattern -> %06x : %02x %02x %02x %02x %02x %02x %02x %02x\n", 
	     idx, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
      p = (unsigned char *)buf_out + idx;
      printf("actual pattern   -> %06x : %02x %02x %02x %02x %02x %02x %02x %02x\n", 
	     idx, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
      break;
    }
    case RDWR_SHORT:
    {
      unsigned short *p;

      p = (unsigned short *)buf_in + idx/sizeof(short);
      printf("expected pattern -> %06x : %02x %02x %02x %02x\n", 
	     idx, p[0], p[1], p[2], p[3]);
      p = (unsigned short *)buf_out + idx/sizeof(short);
      printf("actual pattern   -> %06x : %02x %02x %02x %02x\n", 
	     idx, p[0], p[1], p[2], p[3]);
      break;
    }
    case RDWR_INT:
    {
      unsigned int *p;

      p = (unsigned int *)buf_in + idx/sizeof(int);
      printf("expected pattern -> %06x : %08x %08x\n", 
	     idx, p[0], p[1]);
      p = (unsigned int *)buf_out + idx/sizeof(int);
      printf("actual pattern   -> %06x : %08x %08x\n", 
	     idx, p[0], p[1]);
      break;
    }
    case RDWR_LONG:
    {
      unsigned long *p;

      p = (unsigned long *)buf_in + idx/sizeof(long);
      printf("expected pattern -> %06x : %016lx\n", idx, *p);
      p = (unsigned long *)buf_out + idx/sizeof(long);
      printf("actual pattern   -> %06x : %016lx\n", idx, *p);
      break;
    }
  }
  return( 0);
}

int 
xprs_rdwr_tm( struct cli_cmd_para *c)
{
  struct rdwr_cycle_para *cp;
  struct pev_ioctl_rdwr rdwr;
  struct pev_ioctl_map_pg *mp;
  void *buf_in, *buf_out;
  int i;
  int ret;
  uint size, cnt;
  ulong addr, data;
  int first, last, n;
  time_t tm, tm_start, tm_end;

  ret = 0;

  if( c->cmd[1] == 'c') cp = &last_cfg_cycle;
  if( c->cmd[1] == 'i') cp = &last_io_cycle;
  mp = (struct pev_ioctl_map_pg *)0;
  if( c->cmd[1] == 'v')
  {
    cp = &last_vme_cycle;
    mp = &vme_mas_map;
  }
  if( c->cmd[1] == 's')
  {
    cp = &last_shm1_cycle;
    if( c->cmd[2] == '2')
    {
      cp = &last_shm2_cycle;
    }
    mp = &shm_mas_map;
  }
  if( c->cmd[1] == 'u')
  {
    cp = &last_usr1_cycle;
    if( c->cmd[2] == '2')
    {
      cp = &last_usr2_cycle;
    }
    mp = &shm_mas_map;
  }

  rdwr_get_cycle_para( c, cp);
  if( cp->crate == -1)
  {
    printf("crate number not defined !!! \n");
    return(-1);
  }
  rdwr_set_cycle_mode( cp);

  size = cp->len;
  addr = cp->addr;
  data = cp->data;

  first = 0x100000 - (int)(addr & 0xfffff);
  if( size <= first)
  {
    first = size;
    last = 0;
    n = 0;
  }
  else
  {
    last = (int)(( addr + size) & 0xfffff);
    n = ( size - first - last) >> 20;
  }

  buf_in = (void *)malloc( 0x100000);
  buf_out = (void *)malloc( 0x100000);
  if( aio_error( &aiocb) != EINPROGRESS)
  {
    ret = aio_read( &aiocb);
    if( ret < 0)
    {
      perror("aio_read");
      goto xprs_rdwr_tm_exit;
    }
  }
  cnt = 0;
  printf("enter any key to stop memory test\n");
  printf("loop: %d\r", cnt);
  fflush(stdout);
  tm = time(0);
  tm_start = tm;
  while( (cnt < cp->loop) || !cp->loop)
  {
    /* restart test with same data unless mode is random */
    if( cp->op != 'r')
    {
      cp->data = data;
    }
    cp->addr = addr;
    if( first)
    {
      cp->len = first;
      rdwr.offset = (uint)rdwr_set_map( cp, mp);
      if( rdwr.offset == (uint)-1)
      {
	goto xprs_rdwr_tm_exit;
      }
      rdwr.buf = buf_in;
      rdwr.len = cp->len;
      rdwr_set_ioctl_arg( &rdwr, cp);
      rdwr.mode.dir = RDWR_WRITE;
      rdwr_fill_buf( buf_in, cp);
      ret = pev_rdwr( &rdwr);
      if( ret < 0)
      {
 	printf("cannot write data at address %lx\n", cp->addr);
	goto xprs_rdwr_tm_exit;
      }
      rdwr.mode.dir = RDWR_READ;
      rdwr.buf = buf_out;
      ret = pev_rdwr( &rdwr);
      if( ret < 0)
      {
	printf("cannot read data at address %lx\n", cp->addr);
	goto xprs_rdwr_tm_exit;
      }
      if( rdwr.buf == buf_out)
      {
        ret = rdwr_cmp_buf( buf_in, buf_out, cp->len);
        if( ret < cp->len)
        {
	  rdwr_tm_error( buf_in, buf_out, cp->addr + ret, rdwr.mode.ds);
	  goto xprs_rdwr_tm_exit;
        }
      }
      if( aio_error( &aiocb) != EINPROGRESS)
      {
        aio_return( &aiocb);
	goto xprs_rdwr_tm_exit;
      }
      cp->addr += first;
      cp->data += first;
      if( time(0) != tm)
      {
        tm = time(0);
        printf("loop: %d - %08x\r", cnt, (uint)cp->addr);
        fflush(stdout);
      }
    }
    for( i = 0; i < n; i++)
    {
      cp->len = 0x100000;
      rdwr.offset = (uint)rdwr_set_map( cp, mp);
      if( rdwr.offset == (uint)-1)
      {
	goto xprs_rdwr_tm_exit;
      }
      rdwr.buf = buf_in;
      rdwr.len = cp->len;
      rdwr_set_ioctl_arg( &rdwr, cp);
      rdwr.mode.dir = RDWR_WRITE;
      rdwr_fill_buf( buf_in, cp);
      ret = pev_rdwr( &rdwr);
      if( ret < 0)
      {
	printf("cannot write data at address %lx\n", cp->addr);
	goto xprs_rdwr_tm_exit;
      }
      rdwr.buf = buf_out;
      rdwr.mode.dir = RDWR_READ;
      ret = pev_rdwr( &rdwr);
      if( ret < 0)
      {
	printf("cannot read data at address %lx\n", cp->addr);
	goto xprs_rdwr_tm_exit;
      }
      ret = rdwr_cmp_buf( buf_in, buf_out, cp->len);
      if( ret < cp->len)
      {
	rdwr_tm_error( buf_in, buf_out, cp->addr + ret, rdwr.mode.ds);
	goto xprs_rdwr_tm_exit;
      }
      if( aio_error( &aiocb) != EINPROGRESS)
      {
        aio_return( &aiocb);
	goto xprs_rdwr_tm_exit;
      }
      cp->addr += 0x100000;
      cp->data += 0x100000;
      if( time(0) != tm)
      {
        tm = time(0);
        printf("loop: %d - %08x\r", cnt, (uint)cp->addr);
        fflush(stdout);
      }
    }
    if( last)
    {
      cp->len = last;
      rdwr.offset = (uint)rdwr_set_map( cp, mp);
      if( rdwr.offset == (uint)-1)
      {
	goto xprs_rdwr_tm_exit;
      }
      rdwr.buf = buf_in;
      rdwr.len = cp->len;
      rdwr_set_ioctl_arg( &rdwr, cp);
      rdwr.mode.dir = RDWR_WRITE;
      rdwr_fill_buf( buf_in, cp);
      ret = pev_rdwr( &rdwr);
      if( ret < 0)
      {
	printf("cannot write data at address %lx\n", cp->addr);
	goto xprs_rdwr_tm_exit;
	break;
      }
      rdwr.buf = buf_out;
      rdwr.mode.dir = RDWR_READ;
      ret = pev_rdwr( &rdwr);
      if( ret < 0)
      {
	printf("cannot read data at address %lx\n", cp->addr);
	goto xprs_rdwr_tm_exit;
      }
      ret = rdwr_cmp_buf( buf_in, buf_out, cp->len);
      if( ret < cp->len)
      {
	rdwr_tm_error( buf_in, buf_out, cp->addr + ret, rdwr.mode.ds);
	goto xprs_rdwr_tm_exit;
      }
      if( aio_error( &aiocb) != EINPROGRESS)
      {
        aio_return( &aiocb);
	goto xprs_rdwr_tm_exit;
      }
      cp->addr += last;
      if( time(0) != tm)
      {
        tm = time(0);
        printf("loop: %d - %08x\r", cnt, (uint)cp->addr);
        fflush(stdout);
      }
    }
    cnt += 1;
  }
  if( aio_error( &aiocb) == EINPROGRESS)
  {
    ret = aio_cancel( aiocb.aio_fildes, &aiocb);
    //printf("after io_cancel: %d\n", ret);
    //printf("after io_return: %d\n", ret);
  }

xprs_rdwr_tm_exit:
  tm_end = time(0);
  printf("loop: %d - %08x    elapsed time = %ld sec\n", cnt, (uint)cp->addr, tm_end - tm_start);
  free( buf_in);
  free( buf_out);

  return( ret);
}


char
*rdwr_set_prompt( char *prompt,
		  ulong addr,
		  int mode)
{
  int as;

  as = (mode >> 8)&0xff;
  switch( as)
  {
    case 16: /* A16 */
    {
      sprintf( prompt, "0x%04x : ", (ushort)addr);
      break;
    }
    case 24: /* A24 */
    {
      sprintf( prompt, "0x%06x : ", (uint)addr);
      break;
    }
    case 64: /* A64 */
    {
      sprintf( prompt, "0x%016lx : ", addr);
      break;
    }
    default: /* A32 */
    {
      sprintf( prompt, "0x%08x : ", (uint)addr);
      break;
    }
  }
  return( prompt);
}

char * 
rdwr_patch_addr( ulong addr, 
		 void *data_p,
		 int mode,
		 int ex)
{
  unsigned char *p;
  int ds;
  char pm_prompt[32];
  int idx;

  p = (unsigned char *)data_p;
  ds = mode & 0xf;
  rdwr_set_prompt( pm_prompt, addr, mode);
  idx = strlen( pm_prompt);
  switch( ds)
  {
    case 1:
    {
      sprintf( &pm_prompt[idx], "%02x -> ", *p);
      break;
    }
    case 2:
    {
      sprintf( &pm_prompt[idx], "%04x -> ", *(ushort *)p);
      break;
    }
    case 4:
    {
      sprintf( &pm_prompt[idx], "%08x -> ", *(uint *)p);
      break;
    }
    case 8:
    {
      sprintf( &pm_prompt[idx], "%016lx -> ", *(ulong *)p);
      break;
    }
  }
  if( ex)
  {
    printf("%s .\n", pm_prompt);
    return( 0);
  }
  return( cli_get_cmd( &pm_history, pm_prompt));
}

int 
xprs_rdwr_rmw( struct cli_cmd_para *c)
{
  struct rdwr_cycle_para *cp;
  struct pev_ioctl_vme_rmw rmw;

  cp = &last_rmw_cycle;
  rdwr_get_cycle_para( c, cp);
  rdwr_set_cycle_mode( cp);

  rmw.status = 0;
  rmw.addr = cp->addr;
  rmw.cmp  = cp->para;
  rmw.up = cp->data;
  rmw.ds = cp->mode & 0xf;
  rmw.am = cp->am;

  pev_vme_rmw( &rmw);

  if( rmw.status == -1)
  {
    printf("bad parameters\n");
    return(-1);
  }
  if( rmw.status & 0x10000000)
  {
    printf("RMW status -> OK [%08x]\n",  rmw.status);
  }
  if( rmw.status & 0x20000000)
  {
    printf("RMW status -> compare error [%08x]\n",  rmw.status);
  }
  if( rmw.status & 0x40000000)
  {
    printf("RMW status -> BERR  [%08x]\n",  rmw.status);
  }

  return(0);
}


int 
xprs_rdwr_pm( struct cli_cmd_para *c)
{
  struct rdwr_cycle_para *cp;
  struct pev_ioctl_rdwr rdwr;
  struct pev_ioctl_map_pg *mp;
  char *next, *p;
  ulong addr;
  ulong data;
  int ds;
  int iex;
  int ret;

  if( c->cmd[1] == 'c') cp = &last_cfg_cycle;
  if( c->cmd[1] == 'e') cp = &last_elb_cycle;
  if( c->cmd[1] == 'i') cp = &last_io_cycle;
  if( c->cmd[1] == 'r') cp = &last_csr_cycle;
  if( c->cmd[1] == 'x') cp = &last_pex_cycle;
  mp = (struct pev_ioctl_map_pg *)0;
  if( c->cmd[1] == 'v')
  {
    cp = &last_vme_cycle;
    mp = &vme_mas_map;
  }
  if( c->cmd[1] == 's')
  {
    cp = &last_shm1_cycle;
    if( c->cmd[2] == '2')
    {
      cp = &last_shm2_cycle;
    }
    mp = &shm_mas_map;
  }
  if( c->cmd[1] == 'u')
  {
    cp = &last_usr1_cycle;
    if( c->cmd[2] == '2')
    {
      cp = &last_usr2_cycle;
    }
    mp = &shm_mas_map;
  }
  if( c->cmd[1] == 'm')
  {
    cp = &last_kmem_cycle;
    if( !dma_buf.k_addr)
    {
      printf("kernel buffer has not been allocated !!! \n");
      return(-1);
    }
    rdwr.k_addr = dma_buf.k_addr;
  }
  if( c->cmd[1] == 'p')
  {
    cp = &last_pci_cycle;
  }

  rdwr_get_cycle_para( c, cp);
  if( cp->crate == -1)
  {
    printf("crate number not defined !!! \n");
    return(-1);
  }

  addr = cp->addr;
  data = cp->data;
  rdwr_set_cycle_mode( cp);
  ds = cp->mode & 0xf;

  rdwr.buf = (void *)&data;
  rdwr.len = 0;
  rdwr_set_ioctl_arg( &rdwr, cp);

  if( cp->dir == 'w')
  {
    /* -> shall write data */
    rdwr.offset = rdwr_set_map( cp, mp);
    rdwr.mode.dir = RDWR_WRITE;
    ret = pev_rdwr( &rdwr);
    if( ret < 0)
    {
      printf("cannot access data\n");
      return(-1);
    }
    return( 0);
  }
  iex = 1;
  while( iex)
  {
    /* -> shall read data */
    rdwr.offset = rdwr_set_map( cp, mp);
    rdwr.mode.dir = RDWR_READ;
    ret = pev_rdwr( &rdwr);
    if( ret < 0)
    {
      printf("cannot read data\n");
      return( -1);
    }
    if( cp->op == '.')
    {
      rdwr_patch_addr( addr, (void *)&data, cp->mode, 1);
      return( 0);;
    }
    next = rdwr_patch_addr( addr, (void *)&data, cp->mode, 0);
    switch( next[0])
    {
      case 0:
      {
	addr += ds;
        break;
      }
      case '.':
      {
	iex = 0;
        break;
      }
      case '-':
      {
	addr -= ds;
        break;
      }
      case '=':
      {
        break;
      }
      default:
      {
	data = strtoul( next, &p, 16);
	if( p == next)
        {
	  printf("format error\n");
	}
	else
	{
	  rdwr.mode.dir = RDWR_WRITE;
	  ret = pev_rdwr( &rdwr);
	  addr += ds;
	}
        break;
      }
    }
    cp->addr = addr;
  }
  return(0);
}

int 
xprs_rdwr_lm( struct cli_cmd_para *c)
{
  struct rdwr_cycle_para *cp;
  struct pev_ioctl_rdwr rdwr;
  struct pev_ioctl_map_pg *mp;
  ulong data;
  int ret, cnt;
  time_t tm;

  mp = (struct pev_ioctl_map_pg *)0;
  if( c->cmd[1] == 'v')
  {
    cp = &last_vme_cycle;
    mp = &vme_mas_map;
  }
  if( c->cmd[1] == 's')
  {
    cp = &last_shm1_cycle;
    if( c->cmd[2] == '2')
    {
      cp = &last_shm2_cycle;
    }
    mp = &shm_mas_map;
  }
  if( c->cmd[1] == 'u')
  {
    cp = &last_usr1_cycle;
    if( c->cmd[2] == '2')
    {
      cp = &last_usr2_cycle;
    }
    mp = &shm_mas_map;
  }

  rdwr_get_cycle_para( c, cp);
  if( cp->crate == -1)
  {
    printf("crate number not defined !!! \n");
    return(-1);
  }
  rdwr_set_cycle_mode( cp);

  /* prepare data structure to perform ioctl */
  rdwr.offset = rdwr_set_map( cp, mp);
  rdwr.buf = (void *)&cp->data;
  rdwr.len = 0x1000;
  rdwr_set_cycle_mode( cp);
  rdwr_set_ioctl_arg( &rdwr, cp);

  if( cp->dir == 'w')
  {
    rdwr.mode.dir = RDWR_LOOP_WRITE;
    if( cp->op == 'r')
    {
      rdwr.mode.dir = RDWR_LOOP_RDWR;
    }
    if( cp->op == 'x')
    {
      rdwr.mode.dir = RDWR_LOOP_CHECK;
      data = cp->data;
    }
  }
  if( cp->dir == 'r')
  {
    rdwr.mode.dir = RDWR_LOOP_READ;
  }
  ret = 0;
  if( aio_error( &aiocb) != EINPROGRESS)
  {
    ret = aio_read( &aiocb);
    if( ret < 0)
    {
      perror("aio_read");
      goto xprs_rdwr_lm_exit;
    }
  }

  cnt = 0;
  tm = time(0);
  printf("enter any key to stop looping\n");
  printf("loop: %d\r", cnt);
  fflush(stdout);
  while( ( cnt < cp->loop) || !cp->loop)
  {
    if( ( ( cp->loop - cnt) < rdwr.len) && cp->loop)
    {
      rdwr.len = cp->loop - cnt;
    }
    ret = pev_rdwr( &rdwr);
    if( ret < 0)
    {
      if( cp->op == 'x')
      {
	printf("data error : read = %08x - expected = %08x\n", *(uint *)rdwr.buf, (uint)data);
      }
      else
      {
	printf("cannot access address\n");
      }
      goto xprs_rdwr_lm_exit;
    }
    cnt += rdwr.len;
    if( time(0) != tm)
    {
      tm = time(0);
      printf("loop: %d\r", cnt);
      fflush(stdout);
    }
    if( aio_error( &aiocb) != EINPROGRESS)
    {
      ret = aio_return( &aiocb);
      goto xprs_rdwr_lm_exit;
    }
  }
  if( aio_error( &aiocb) == EINPROGRESS)
  {
    ret = aio_cancel( aiocb.aio_fildes, &aiocb);
    if( ret != AIO_CANCELED)
    {
    //printf("after io_cancel: %d - %d\n", ret, AIO_NOTCANCELED);
    }
  }

xprs_rdwr_lm_exit:
  printf("loop: %d\n", cnt);
  return( ret);
}

int 
xprs_rdwr_i2c( struct cli_cmd_para *c)
{
  int retval;
  int cnt, i;
  char *p;

  retval = -1;
  cnt = c->cnt;
  i = 0;
  if( cnt--)
  {
    if( !strcmp( "pex", c->para[i]))
    {
      i++;
      if( cnt--)
      {
	int reg;
	uint data;

	if( !strcmp( "read", c->para[i]))
        {
	  i++;
	  if( cnt--)
          {
	    reg = strtoul( c->para[i], &p, 16);
	    if( p == c->para[i])
            {
	      return(-1);
	    }
	    data = pev_pex_read( reg);
	    printf("Reading PEX8624 register %03x = %08x\n", reg, data);
	  }
	  return( -1);
	}
	if( !strcmp( "write", c->para[i]))
        {
	  i++;
	  if( cnt > 1)
          {
	    reg = strtoul( c->para[i], &p, 16);
	    if( p == c->para[i])
            {
	      return(-1);
	    }
	    data = strtoul( c->para[i+1], &p, 16);
	    if( p == c->para[i+1])
            {
	      return(-1);
	    }
	    pev_pex_write( reg, data);
	    printf("Writing PEX8624 register %03x = %08x\n", reg, data);
	  }
	  return( -1);
	}
      }
      return( -1);
    }

  }
  return( retval);
}
int 
xprs_rdwr_evt( struct cli_cmd_para *c)
{
  int retval;

  retval = 0;
  if( c->cnt > 0)
  {
    if( !strcmp( "wait", c->para[0]))
    {
      printf("wait event...");
      //retval = pev_evt_wait();
      printf("ok\n");
    }

    if( !strcmp( "set", c->para[0]))
    {
      printf("set event\n");
      //retval = pev_evt_set();
    }
  }

  return( retval);
}



int 
xprs_rdwr_dma( struct cli_cmd_para *c)
{
  int retval;
  struct pev_time tmi, tmo;
  int usec;
  int utmi, utmo;

  retval = 0;
  if( c->cnt > 0)
  {
    if( !strncmp( "start", c->para[0], 5))
    {
      uint para, npara;
      char sw;

      if( c->cnt < 4)
      {
	printf("usage : dma start <des_start>:<des_mode>[.s] <src_start>:<src_mode>[.s] <size>\n");
	return( -1);
      }
      sw = 0;
      npara = sscanf( c->para[1], "%lx:%x.%c", &dma_req.des_addr, &para, &sw);
      dma_req.des_space = (char)para;
      dma_req.des_mode = 0x0;
      if( npara == 3)
      {
        if( ( dma_req.des_space & DMA_SPACE_MASK) ==  DMA_SPACE_VME)
	{
	  if( ( sw == 's') ||( sw == 'a')) 
	  {
	    printf("set VME destination swapping\n");
	    dma_req.des_mode = DMA_VME_SWAP;
	  }
	}
	else
	{
 	  if( sw == 'w')
	  {
	    printf("set destination word swapping\n");
	    dma_req.des_space |= DMA_SPACE_WS;
	  }
 	  if( sw == 'd')
	  {
	    printf("set destination double word swapping\n");
	    dma_req.des_space |= DMA_SPACE_DS;
	  }
 	  if( sw == 'q')
	  {
	    printf("set destination quad word swapping\n");
	    dma_req.des_space |= DMA_SPACE_QS;
	  }
	}
      }
      sw = 0;
      npara = sscanf( c->para[2], "%lx:%x.%c", &dma_req.src_addr, &para, &sw);
      dma_req.src_space = (char)para;
      dma_req.src_mode = 0x0;
      if( npara == 3)
      {
        if( ( dma_req.src_space & DMA_SPACE_MASK) ==  DMA_SPACE_VME)
	{
	  if( ( sw == 's') ||( sw == 'a')) 
	  {
	    printf("set VME source auto swapping\n");
	    dma_req.src_mode = DMA_VME_SWAP;
	  }
	}
        else
	{
 	  if( sw == 'w')
	  {
	    printf("set source word swapping\n");
	    dma_req.src_space |= DMA_SPACE_WS;
	  }
 	  if( sw == 'd')
	  {
	    printf("set source double word swapping\n");
	    dma_req.src_space |= DMA_SPACE_DS;
	  }
 	  if( sw == 'q')
	  {
	    printf("set source quad word swapping\n");
	    dma_req.src_space |= DMA_SPACE_QS;
	  }
	}
      }

      sscanf( c->para[3], "%x", &dma_req.size);

      if( (dma_req.des_space & DMA_SPACE_MASK) == DMA_SPACE_PCIE)
      {
	dma_req.des_addr += (ulong)dma_buf.b_addr;       /* add buffer base address */
      }
      if( (dma_req.src_space & DMA_SPACE_MASK) == DMA_SPACE_PCIE)
      {
	dma_req.src_addr += (ulong)dma_buf.b_addr;       /* add buffer base address */
      }
      dma_req.start_mode = DMA_MODE_BLOCK;
      if( c->para[0][6] == 'p')
      {
	dma_req.start_mode = DMA_MODE_PIPE;
      }
      dma_req.end_mode = 0;
      dma_req.intr_mode = DMA_INTR_ENA;
      dma_req.wait_mode = DMA_WAIT_INTR;
      //dma_req.intr_mode = 0;
      //dma_req.wait_mode = 0;

      pev_timer_read( &tmi);
      pev_dma_move(&dma_req);
      pev_timer_read( &tmo);
      utmi = (tmi.utime & 0x1ffff);
      utmo = (tmo.utime & 0x1ffff);

      usec = (tmo.time - tmi.time)*1000 + (utmo - utmi)/100;
      printf("ok [%d usec - %f MBytes/sec]\n", usec, (float)dma_req.size/(float)usec);
    }
    if( !strcmp( "status", c->para[0]))
    {
      printf("DMA status...");
      printf("ok\n");
    }
    if( !strcmp( "kill", c->para[0]))
    {
      printf("kill dma...");
      printf("ok\n");
    }
  }

  return( retval);
}

