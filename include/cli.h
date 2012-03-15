/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : cli.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contain the declarations and definitions used by XprsMon
 *    to interpret user's commands.
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
 * $Log: cli.h,v $
 * Revision 1.4  2012/03/15 16:15:37  kalantari
 * added tosca-driver_4.05
 *
 * Revision 1.2  2008/08/08 09:11:43  ioxos
 *  cmdline[256] belong now to struct cli_cmd_para{} [JFG]
 *
 * Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 * Import sources for PEV1100 project [JFG]
 *
 *  
 *=============================< end file header >============================*/

#ifndef _H_CLI
#define _H_CLI

#define CLI_HISTORY_SIZE 80
#define CLI_COMMAND_SIZE 0x100

struct cli_cmd_history *cli_history_init( struct cli_cmd_history *);
char *cli_get_cmd( struct cli_cmd_history *, unsigned char*);

struct cli_cmd_list
{
  char *cmd;
  int (* func)();
  char **msg;
  long idx;
};


struct cli_cmd_history
{
  long status;
  unsigned short wr_idx;unsigned short rd_idx;
  unsigned short size;unsigned short cnt;
  unsigned short end_idx;unsigned short insert_idx;
  char bufline[CLI_HISTORY_SIZE][CLI_COMMAND_SIZE];
};

struct cli_cmd_para
{
  long idx;
  long cnt;
  char *cmd;
  char *ext;
  char *para[12];
  char cmdline[256];
};

#define CLI_ERR_ADDR   0x1
#define CLI_ERR_DATA   0x2
#define CLI_ERR_LEN    0x4
#define CLI_ERR_AM     0x8
#define CLI_ERR_CRATE  0x10

#endif /*  _H_CLI */
