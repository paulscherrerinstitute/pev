/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : xilinx.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : october 11,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations and definitions used to enable
 *    debug messages
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
 * $Log: xilinx.h,v $
 * Revision 1.8  2012/06/29 08:47:00  kalantari
 * checked in the PEV_4_14 got from JF ioxos
 *
 * Revision 1.1  2008/11/12 14:38:08  ioxos
 * first cvs checkin [JFG]
 *
 *=============================< end file header >============================*/
#ifndef _H_XILINX
#define _H_XILINX

#define XC5VLX30T_ID      0x93e0a602
#define XC5VLX30T_SIZE    ((292576+272)*4)
#define XC5VLX50T_ID      0x9360a902
#define XC5VLX50T_SIZE    ((438864+272)*4)
#define XC5VSX35T_ID      0x9320e702
#define XC5VSX35T_SIZE    ((416888+272)*4)
#define XC5VSX50T_ID      0x93a0e902
#define XC5VSX50T_SIZE    ((625332+272)*4)
#define XC5VFX30T_ID      0x93602703
#define XC5VFX30T_SIZE    ((422136+272)*4)
#define XC5VFX70T_ID      0x93602c03
#define XC5VFX70T_SIZE    ((844272+272)*4)

struct xilinx_device
{
  char *name;
  uint id;
  uint size;
}
xilinx_device[] =
{
  { "XC5VLX30T", XC5VLX30T_ID, XC5VLX30T_SIZE},
  { "XC5VLX50T", XC5VLX50T_ID, XC5VLX50T_SIZE},
  { "XC5VSX35T", XC5VSX35T_ID, XC5VSX35T_SIZE},
  { "XC5VSX50T", XC5VSX50T_ID, XC5VSX50T_SIZE},
  { "XC5VFX30T", XC5VFX30T_ID, XC5VFX30T_SIZE},
  { "XC5VFX70T", XC5VFX70T_ID, XC5VFX70T_SIZE},
  { NULL       , 0           , 0}
}; 

#endif /*  _H_XILINX */
