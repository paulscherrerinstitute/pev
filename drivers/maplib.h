/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : maplib.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations of all exported functions define in
 *    maplib.c
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
 * $Log: maplib.h,v $
 * Revision 1.11  2012/09/04 07:34:33  kalantari
 * added tosca driver 4.18 from ioxos
 *
 * Revision 1.2  2008/07/04 07:40:12  ioxos
 * update address mapping functions [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:06  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *  
 *=============================< end file header >============================*/


#ifndef _H_MAPLIB
#define _H_MAPLIB

int map_blk_alloc( struct pev_ioctl_map_ctl *, struct pev_ioctl_map_pg *);
int map_blk_find( struct pev_ioctl_map_ctl *, struct pev_ioctl_map_pg *);
int map_blk_modify( struct pev_ioctl_map_ctl *, struct pev_ioctl_map_pg *);
int map_blk_free( struct pev_ioctl_map_ctl *, int);

#endif /*  _H_MAPLIB */

/*================================< end file >================================*/
