/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : vme.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to control the
 *     the PVME64 interface.
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
 * $Log: vme.c,v $
 * Revision 1.1  2012/03/15 14:50:11  kalantari
 * added exact copy of tosca-driver_4.04 from afs
 *
 * Revision 1.5  2011/12/06 13:17:31  ioxos
 * support for multi task VME IRQ [JFG]
 *
 * Revision 1.4  2011/10/19 13:35:34  ioxos
 * vme interrupt handling [JFG]
 *
 * Revision 1.3  2011/03/03 15:42:38  ioxos
 * support for 1MBytes VME slave granularity [JFG]
 *
 * Revision 1.2  2010/06/11 11:51:41  ioxos
 * add VME sys reset operation [JFG]
 *
 * Revision 1.1  2008/11/12 12:25:08  ioxos
 * first cvs checkin [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: vme.c,v 1.1 2012/03/15 14:50:11 kalantari Exp $";
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

struct  pev_ioctl_vme_conf vme_conf;
struct cli_cmd_history vme_history;
struct pev_ioctl_vme_irq *vme_irq = NULL;
int 
vme_init( void)
{
  cli_history_init( &vme_history);
  pev_vme_conf_read( &vme_conf);
  return;
}

int
set_para_hex( char *prompt,
	      uint *data)
{
  char *para, *p;

  para = cli_get_cmd( &vme_history, prompt);
  para = strtok(para,"\n\r");
  if( para)
  {
    *data = strtoul( para, &p, 16);
    if( p == para)
    {
      printf("%s : bad parameter value\n", para);
      return(-1);
    }
  }
  return( 0);
}

set_para_dec( char *prompt,
	      uint *data)
{
  char *para, *p;

  para = cli_get_cmd( &vme_history, prompt);
  para = strtok(para,"\n\r");
  if( para)
  {
    *data = strtoul( para, &p, 10);
    if( p == para)
    {
      printf("%s : bad parameter value\n", para);
      return(-1);
    }
  }
  return( 0);
}

int 
xprs_vme( struct cli_cmd_para *c)
{

  int retval;
  int cnt, i;
  char *p;

  retval = -1;
  cnt = c->cnt;
  i = 0;

  if( !cnt)
  {
    printf("vme command needs arguments\n");
    return(-1);
  }

  if( cnt--)
  {
    if( !strcmp( "conf", c->para[i]))
    {
      struct pev_ioctl_vme_conf *vc;
      char prompt[32], *para, *p;
      uint size;

      vc = &vme_conf;

      printf("VME configuration\n");

      pev_vme_conf_read( vc);

      sprintf(prompt, "a32_base [%08x] -> ", vc->a32_base);
      if( set_para_hex( prompt, &vc->a32_base) < 0) return( -1);

      sprintf(prompt, "a32_size [%08x] -> ", vc->a32_size);
      size = 0;
      if( set_para_hex( prompt, &size) < 0) return( -1);
      if(  size)
      {
	vc->a32_size = size & 0xfff00000;
        if(  size & 1)
        { 
  	  vc->slv_ena |= VME_SLV_1MB;
        }
	else
        { 
  	  vc->slv_ena &= ~VME_SLV_1MB;
        }
      }
      pev_vme_conf_write( vc);
      return(0); 
    }
    if( !strcmp( "sysreset", c->para[i]))
    {
      printf("VME SYSREST...\n");
      pev_vme_sysreset( 200);
      return(0); 
    }
    if( !strcmp( "init", c->para[i]))
    {
      printf("VME INIT...\n");
      pev_vme_init();
      return(0); 
    }
    if( !strcmp( "irq", c->para[i]))
    {
      uint vector;
      uint irq, tmo;
      char *p;

      if( cnt --)
      {
        if( !strcmp( "alloc", c->para[i+1]))
        {
	  if( vme_irq)
	  {
	    pev_vme_irq_free( vme_irq);
	    vme_irq = NULL;
	  }
          irq = 0;
          if( cnt--)
          {
            irq = strtoul( c->para[i+2], &p, 16);
            if( p ==  c->para[i+2])
            {
              printf("%s : bad IRQ set\n", c->para[i+2]);
	      return(-1);
   	    }
  	  }
          if( !irq)
	  {
	    printf("usage: vme irq alloc <irq_set>\n");
	  }
          printf("Allocating VME IRQ set...\n");
          vme_irq = pev_vme_irq_alloc( irq);
          if( !vme_irq)
	  {
	    printf("Cannot allocate that VME IRQ set %08x\n", irq);
	    return( -1);
	  }
	  printf("%08x\n", vme_irq->irq); 
	  return(0);
	}
        if( !strcmp( "wait", c->para[i+1]))
        {
	  if( !vme_irq)
	  {
	    printf("VME IRQ set not allocated\n");;
	    return( -1);
	  }
	  printf("waiting for VME IRQ from set %08x...\n", vme_irq->irq);
          tmo = 0;
          if( cnt --)
          {
            tmo = strtoul( c->para[i+2], &p, 16);
            if( p ==  c->para[i+2])
            {
              printf("%s : bad timeout value\n", c->para[i+2]);
	      return(-1);
   	    }
  	  }
          vector = 0;
          printf("VME WAIT IRQ...0x%08x - %d msec\n", vme_irq->irq, tmo);
          pev_vme_irq_armwait( vme_irq, tmo, &vector);
          printf("vector = %x\n", vector);
	}
        if( !strcmp( "mask", c->para[i+1]))
        {
          irq = 0;
          if( cnt --)
          {
            irq = strtoul( c->para[i+2], &p, 16);
            if( p ==  c->para[i+2])
            {
              printf("%s : bad IRQ set\n", c->para[i+2]);
	      return(-1);
   	    }
	  }
          pev_vme_irq_mask( irq);
        }
        if( !strcmp( "unmask", c->para[i+1]))
        {
          irq = 0;
          if( cnt --)
          {
            irq = strtoul( c->para[i+2], &p, 16);
            if( p ==  c->para[i+2])
            {
              printf("%s : bad IRQ set\n", c->para[i+2]);
	      return(-1);
   	    }
	  }
          pev_vme_irq_unmask( irq);
        }
        if( !strcmp( "clear", c->para[i+1]))
        {
	  if( !vme_irq)
	  {
	    printf("VME IRQ set not allocated\n");;
	    return( -1);
	  }
	  printf("clearing VME IRQ set set %08x...\n", vme_irq->irq);
          pev_vme_irq_clear( vme_irq);
        }
        if( !strcmp( "init", c->para[i+1]))
        {
	  printf("initializing VME IRQs...\n");
          pev_vme_irq_init();
        }
      }

      return(0); 
    }
  }

  return(0);
}

