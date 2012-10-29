/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : script.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : july 17,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That file contains a set of function called by XprsMon to handle
 *     script files.
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
 * $Log: script.c,v $
 * Revision 1.11  2012/10/29 10:06:56  kalantari
 * added the tosca driver version 4.22 from IoxoS
 *
 * Revision 1.5  2012/06/01 13:59:44  ioxos
 * -Wall cleanup [JFG]
 *
 * Revision 1.4  2009/07/17 14:13:08  ioxos
 * allow for script execution when launching XprsMon [JFG]
 *
 * Revision 1.3  2009/06/02 12:00:43  ioxos
 * bug correction + skip \n \r  [jfg]
 *
 * Revision 1.2  2008/08/08 11:45:24  ioxos
 * support script argument+linux command+exit [JFG]
 *
 * Revision 1.1  2008/07/18 13:26:00  ioxos
 * fil creation [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: script.c,v 1.11 2012/10/29 10:06:56 kalantari Exp $";
#endif

#define DEBUGno
#include <debug.h>

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cli.h>
#include <pevioctl.h>
#include <pevulib.h>

char line[256];
extern struct cli_cmd_list cmd_list[];
extern char cli_prompt[];
extern struct cli_cmd_para cmd_para;
int xprs_cmd_exec( struct cli_cmd_list *, struct cli_cmd_para *);

char *
script_rcsid()
{
  return( rcsid);
}

int 
xprs_script( char *filename,
	     struct cli_cmd_para *s)

{
  FILE *file;
  int retval;

  filename = strtok( filename, " \t");
  retval = 0;
  file = fopen( filename, "r");
  if( !file)
  {
    printf("Cannot open script file %s\n", filename);
    return( -1);
  }
  printf("Start execution of script file %s\n", s->cmd);
  while( fgets( line, 256, file))
  {
    strtok( line, "\n\r#");
    if( line[0] == '#') continue;
    if( line[0] == '\n') continue;
    if( line[0] == '\r') continue;
    if( line[0] == '$')
    {
      int para;

      if( !strncmp( "exit", &line[1], 4))
      {
	retval = 1;
	break;
      }
      if( !strncmp( "sleep", &line[1], 5))
      {
	sscanf(line, "$sleep %d", &para);
	sleep( para);
      }
      if( !strncmp( "usleep", &line[1], 5))
      {
	sscanf(line, "$usleep %d", &para);
	usleep( para);
      }
      continue;
    }
    if( line[0] == 'q') break;
    if( line[0] == '@')
    {
      struct cli_cmd_para script_para;

      printf("%s%s\n", cli_prompt, line);
      cli_cmd_parse(  &line[1], &script_para);
      if( xprs_script( &line[1], &script_para) == 1)
      {
	retval = 1;
	break;
      }
      continue;
    }
    if( line[0])
    {
      int i, j;
      char arg[3];

      printf("%s%s\n", cli_prompt, line);
      cli_cmd_parse( line, &cmd_para);
      arg[0] = '$';
      arg[2] = 0;
      for( i = 0; i < cmd_para.cnt; i++)
      {
        for( j = 0; j < s->cnt; j++)
        {
	  arg[1] = '0'+j;
          if( !strcmp( arg, cmd_para.para[i]))
          {
	    cmd_para.para[i] = s->para[j];
	  }
        }
      }
      xprs_cmd_exec( &cmd_list[0], &cmd_para);
    }
  }
  fclose( file);
  return( retval);
}

