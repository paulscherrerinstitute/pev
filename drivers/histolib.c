/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : histolib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the low level functions to drive the SFLAH device
 *    implemented on the PEV1100.
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
 *  $Log: histolib.c,v $
 *  Revision 1.11  2012/09/04 07:34:33  kalantari
 *  added tosca driver 4.18 from ioxos
 *
 *  Revision 1.1  2009/04/06 09:47:13  ioxos
 *  first check-in [JFG]
 *
 *  
 *=============================< end file header >============================*/

#include <asm/uaccess.h>         // copy_to_user and copy_from_user
#include <linux/errno.h>         // error codes


#define DBGno

#ifdef DBG
#define debugk(x) printk x
#else
#define debugk(x) 
#endif

int histo[16][1024];

int *
histo_init( void)
{
  int i, j;

  for( i = 0; i < 16; i++)
  {
    for( j = 0; j < 1024; j++)
    {
      histo[i][j] = 0;
    }
  }
  return( &histo[0][0]);
}

void
histo_clear( int idx)
{
  int j;

  for( j = 0; j < 1024; j++)
  {
    histo[idx][j] = 0;
  }
  return;
}

void
histo_inc( int idx,
	   int chan,
	   int max)
{
  if( idx > 15) return;
  if( max > 1024) max = 1024;
  if( chan >= max) chan = max-1;
  histo[idx][chan] += 1;

  return;
}

int *
histo_get( int idx,
	   int max)
{
  if( ( idx > 15) || ( max > 1024))return( NULL);
  return( &histo[idx][0]);
}

int
histo_print( int idx,
	     int max)
{
  int i, chan;

  if( idx > 15) return( -EFAULT);
  if( max > 1024) max = 1024;
  for( chan = 0; chan < max; chan += 10)
  {
    printk("%4d : ", chan);
    for( i = 0; i < 10; i++)
    {
      if( ( chan+i) >= max) break;
      printk("%6d ", histo[idx][chan+i]);
    }
    printk("\n");
  }

  return(0);
}

