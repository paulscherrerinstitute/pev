/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : map.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : july 3,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to handle
 *     the PEV1000 address mapping.
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
 * $Log: map.c,v $
 * Revision 1.3  2012/04/25 13:18:28  kalantari
 * added i2c epics driver and updated linux driver to v.4.10
 *
 * Revision 1.6  2012/01/27 15:55:44  ioxos
 * prepare release 4.01 supporting x86 & ppc [JFG]
 *
 * Revision 1.5  2009/01/06 13:43:03  ioxos
 * code re-organization [JFG]
 *
 * Revision 1.4  2008/11/12 13:33:32  ioxos
 * add map clear command [JFG]
 *
 * Revision 1.3  2008/08/08 11:50:22  ioxos
 *  reorganize code (cosmetic) [JFG]
 *
 * Revision 1.2  2008/07/18 14:09:19  ioxos
 * correct bug in map size calculation [JFG]
 *
 * Revision 1.1  2008/07/04 07:38:29  ioxos
 * file creation [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: map.c,v 1.3 2012/04/25 13:18:28 kalantari Exp $";
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

int
map_get_id( char *name,
	    struct pev_ioctl_map_ctl *map)
{
  map->sg_id =  MAP_INVALID;
  if( !strcmp( "mas_32", name))
  {
    map->sg_id =  MAP_MASTER_32;
  }
  if( !strcmp( "mas_64", name))
  {
    map->sg_id =  MAP_MASTER_64;
  }
  if( !strcmp( "slv_vme", name))
  {
    map->sg_id =  MAP_SLAVE_VME;
  }
  return( map->sg_id);
}

int
map_show( char *name)
{
  struct pev_ioctl_map_ctl map_ctl;
  struct pev_map_blk *p;
  int i, j;


  if( map_get_id( name, &map_ctl) == MAP_INVALID)
  {
    printf("wrong map name : %s\n", name);
    return( -1);
  } 

  map_ctl.map_p = (struct pev_map_blk *)0;
  pev_map_read( &map_ctl);
  if( map_ctl.sg_id == MAP_INVALID)
  {
    printf("map %s doesn't exist !!\n", name);
    return( -1);
  } 
  map_ctl.map_p = malloc( map_ctl.pg_num*(sizeof( struct pev_map_blk)));
  pev_map_read( &map_ctl);

  p = map_ctl.map_p;
  printf("\n");
  printf("+=========================================================+\n");
  printf("+ Map Name : %s\n", name);
  printf("+----------+----+----+----------+------------------+------+\n");
  printf("|  offset  |flag| usr|   size   |   remote address | mode |\n");
  printf("+----------+----+----+----------+------------------+------+\n");
  for( j = 0; j < map_ctl.pg_num; j++)
  {
    if( p[j].flag)
    {
      printf("| %08x | %02x | %02x | %08x | %016lx | %04x | ", j*map_ctl.pg_size,
	     p[j].flag, p[j].usr, p[j].npg*map_ctl.pg_size, (long)p[j].rem_addr, p[j].mode);
      switch( p[j].mode & 0xff00)
      {
	case 0x0000:
	{
	  printf("PCIe\n");
	  break;
	}
        case 0x1000:
	{
	  printf("VME CR/CSR\n");
	  break;
	}
        case 0x1100:
	{
	  printf("VME A16\n");
	  break;
	}
        case 0x1200:
	{
	  printf("VME A24\n");
	  break;
	}
        case 0x1300:
	{
	  printf("VME A32\n");
	  break;
	}
        case 0x1400:
	{
	  printf("VME BLT\n");
	  break;
	}
        case 0x1500:
	{
	  printf("VME MBLT\n");
	  break;
	}
        case 0x1f00:
	{
	  printf("VME IACK\n");
	  break;
	}
        case 0x2000:
	{
	  if( pev_board() == PEV_BOARD_IFC1210)
	  {
	    printf("SHM1\n");
	  }
	  else
	  {
	    printf("SHM\n");
	  }
	  break;
	}
        case 0x3000:
	{
	  if( pev_board() == PEV_BOARD_IFC1210)
	  {
	    printf("SHM2\n");
	  }
	  else
	  {
	    printf("USR\n");
	  }
	  break;
	}
        case 0x4000:
	{
	  printf("USR1\n");
	  break;
	}
        case 0x5000:
	{
	  printf("USR2\n");
	  break;
	}
        default:
	{
	  printf("\n");
	  break;
	}
      }
    }
  }
  free( map_ctl.map_p);

  return(0);

}


int 
xprs_map( struct cli_cmd_para *c)
{
  int retval;
  int cnt, i, j;
  char *p;

  retval = -1;
  cnt = c->cnt;
  i = 0;
  if( cnt--)
  {
    if( !strcmp( "clear", c->para[i]))
    {
      struct pev_ioctl_map_ctl map_ctl;
      struct pev_map_blk *p;
      i++;
      if( cnt)
      {
	if( map_get_id( c->para[i], &map_ctl) == MAP_INVALID)
	{
	  printf("wrong map name : %s\n", c->para[i]);
	  return( -1);
	} 
	pev_map_clear( &map_ctl);
	if( map_ctl.sg_id == MAP_INVALID)
	{
	  printf("map %s doesn't exist !!\n", c->para[i]);
	  return( -1);
	} 
	return( 0); 
      }
      return( -1); 
    }
    if( !strcmp( "show", c->para[i]))
    {
      struct pev_ioctl_map_ctl map_ctl;
      struct pev_map_blk *p;
      i++;
      if( cnt)
      {
	map_show( c->para[i]);
	printf("\n");
      }
      else
      {
	map_show( "mas_32");
	map_show( "mas_64");
	map_show( "slv_vme");
	printf("\n");
      }
      return( 0); 
    }
  }
  return(0);
}

