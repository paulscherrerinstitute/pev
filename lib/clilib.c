/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : clilib.c
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *     That library contains a set of function to implement a command line
 *     interpreter running in an Xterm.
 *     It keeps a command history and allows to recall previous command.
 *     Characters can be inserted/deleted anywhere in the current line using
 *     arrows and delete keys.
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
 *  $Log: clilib.c,v $
 *  Revision 1.1  2012/03/15 14:51:27  kalantari
 *  added exact copy of tosca-driver_4.04 from afs
 *
 *  Revision 1.5  2011/10/19 14:01:05  ioxos
 *  add 0x8 for backspace [JFG]
 *
 *  Revision 1.4  2009/01/06 13:31:29  ioxos
 *  perform command comparaison on entry length [JFG]
 *
 *  Revision 1.3  2008/09/17 12:25:35  ioxos
 *  accept string parameters delimitted by quotes [JFG]
 *
 *  Revision 1.2  2008/08/08 09:11:58  ioxos
 *   cmdline[256] belong now to struct cli_cmd_para{} [JFG]
 *
 *  Revision 1.1.1.1  2008/07/01 09:48:07  ioxos
 *  Import sources for PEV1100 project [JFG]
 *
 *
 *=============================< end file header >============================*/

#ifndef lint
static char *rcsid = "$Id: clilib.c,v 1.1 2012/03/15 14:51:27 kalantari Exp $";
#endif

#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <cli.h>

struct cli_cmd_history *
cli_history_init( struct cli_cmd_history *h)
{
  h->status = 0;
  h->rd_idx = 0;
  h->wr_idx = 0;
  h->cnt = 0;
  h->size = CLI_HISTORY_SIZE;
  h->end_idx = 0;
  h->insert_idx = 0;

  return(h);

}

long
cli_insert_char( unsigned char c,
                 unsigned char *cmdline,
	         long insert_idx)
{
  long last_idx;
  long i;

  last_idx = strlen( cmdline);
  if( last_idx >= (CLI_COMMAND_SIZE - 2))
  {
    last_idx = (CLI_COMMAND_SIZE - 2);
    cmdline[CLI_COMMAND_SIZE - 1] = 0;
  }
  if( insert_idx > last_idx)
  {
    insert_idx = last_idx;
  }
  cmdline[last_idx+1] = 0;
  for( i = last_idx; i > insert_idx; i--)
  {
    cmdline[i] = cmdline[i-1];
  }
  cmdline[insert_idx] = c;
  return( insert_idx + 1);
}

long
cli_remove_char( unsigned char *cmdline,
		 long insert_idx)
{
  long last_idx;
  long i;

  if( !insert_idx)
  {
    return( 0);
  }
  last_idx = strlen( cmdline);
  if( insert_idx > last_idx)
  {
    insert_idx = last_idx;
  }
  for( i = insert_idx; i < last_idx; i++)
  {
    cmdline[i-1] = cmdline[i];
  }
  cmdline[last_idx - 1] = 0;
  return( insert_idx - 1);
}


cli_erase_line( unsigned char *prompt)
{
  printf("%c%c%d%c%c%c%c", 0x1b, '[',  strlen( prompt) + 1,'G',0x1b,'[','K');
}

void
cli_erase_end()
{
  putchar(0x1b);
  putchar('[');
  putchar('K');
}

long
cli_print_line( unsigned char *cmdline,
		long insert_idx)
{
  long last_idx;
  long i;

  last_idx = strlen( cmdline);
  for( i = insert_idx; i < last_idx; i++)
  {
    putchar( cmdline[i]);
  }
  for( i = last_idx; i > insert_idx + 1; i--)
  {
    putchar( 0x8);
  }
  if( last_idx == CLI_COMMAND_SIZE - 1)
  {
    putchar( 0x8);
  }
  return( insert_idx);
}

char *
cli_get_cmd( struct cli_cmd_history *h,
	     unsigned char *prompt)
{
  unsigned char *cmdline, c;
  long iex;
  long i;

  cmdline = &h->bufline[h->wr_idx][0];
  printf("%s",prompt);
  h->end_idx = 0;
  h->insert_idx = 0;
  h->rd_idx = h->wr_idx;
  iex = 1;
  while( iex)
  {
    c = getchar();
    switch( c)
    {
      case 0x7f:
      case 0x8:
      {
	if( h->insert_idx > 0)
	{
	  putchar('\b');
	  putchar(0x1b);
	  putchar('7');
	  cli_erase_line( prompt);
	  h->insert_idx = cli_remove_char( cmdline, h->insert_idx);
	  printf("%s", cmdline);
	  putchar(0x1b);
	  putchar('8');
	  h->end_idx -= 1;
	}
	break;
      }
      case '\r':
      {
        putchar( c);
	break;
      }
      case '\n':
      {
        putchar( c);
        cmdline[h->end_idx] = 0;
	iex = 0;
        break;
      }
      case 0x1b: /* escape sequence */
      {
        char c1,c2,c3;

	c1 = getchar();
	switch( c1)
	{
	  case '[':
	  {
	    c2 = getchar();
	    switch( c2)
	    {
	      case 'A': /* UP */
	      {
		if( h->rd_idx == 0)
		{
		  if( h->cnt > h->size)
		  {
		    h->rd_idx = h->size;
		  }
		}
                else
		{
		  h->rd_idx -= 1;
		}
		cli_erase_line( prompt);
		cmdline = &h->bufline[h->rd_idx][0];
		h->end_idx = strlen( cmdline);
		for( i = 0; i < h->end_idx; i++)
		{
		  putchar(cmdline[i]);
		}
		h->insert_idx = h->end_idx;
		break;
	      }
	      case 'B': /* DOWN */
	      {
		if( !(h->rd_idx == h->wr_idx))
		{
		  if( h->rd_idx == h->size - 1)
		  {
		    h->rd_idx = 0;
		  }
                  else
	 	  {
		    h->rd_idx += 1;
		  }
		}
		cli_erase_line(  prompt);
		cmdline = &h->bufline[h->rd_idx][0];
		h->end_idx = strlen( cmdline);
		for( i = 0; i < h->end_idx; i++)
		{
		  putchar(cmdline[i]);
		}
		h->insert_idx = h->end_idx;
		break;
	      }
	      case 'C': /* RIGTH */
	      {
		if( h->insert_idx < h->end_idx)
		{
		  putchar( cmdline[h->insert_idx]);
		  h->insert_idx++;
		}
		break;
	      }
	      case 'D': /* LEFT */
	      {
		if( h->insert_idx > 0)
		{
		  h->insert_idx--;
		  putchar('\b');
		}
		break;
	      }
	    }
	    break;
	  }
	}
	break;
      }
      default:
      {
	if( ( c > 0x1f) && ( c < 0x80))
	{
	  h->insert_idx = cli_insert_char( c, cmdline, h->insert_idx);
	  cli_print_line( cmdline, h->insert_idx - 1);
	  h->end_idx = strlen( cmdline);
	}
	break;
      }
    }
  }

  if( strlen( cmdline))
  {
    int last_idx;

    last_idx = h->wr_idx -1;
    if( last_idx < 0)
    {
      last_idx = h->size;
    }
    if( !strcmp( cmdline, &h->bufline[last_idx][0]))
    {
      return( cmdline);
    }
    if( h->rd_idx != h->wr_idx)
    {
      strcpy( &h->bufline[h->wr_idx][0], cmdline);
    }
    h->wr_idx++;
    if( h->wr_idx >= h->size)
    {
      h->wr_idx = 0;
    }
    h->cnt++;
  }

  return( cmdline);

}

struct cli_cmd_para    
*cli_cmd_parse( char *cmdline, 
		struct cli_cmd_para *c)
{
  char *p;
  long i;

  strcpy( c->cmdline, cmdline); 
  c->cmd = c->cmdline;
  p = strpbrk( c->cmd, ". \t");
  if( p)
  {
    if( *p == '.')
    {
      c->ext = p + strspn(p,". \t");
      *p = 0;
      p = strpbrk( c->ext, " \t");
    }
    else
    {
      c->ext = (char *)0;
    }
  }
  i = 0;
  while(p)
  {
    c->para[i] = p + strspn(p," \t");
    if( c->para[i][0] == '\"')
    {
      char *end;
      *p = 0;
      c->para[i] += 1;
      p = strpbrk( c->para[i], "\"");
      if( !p)
      {
	break;
      }
      end = p;
      p = strpbrk( p, " \t");
      *end = 0;
    }
    else
    {
      *p = 0;
      p = strpbrk( c->para[i], " \t");
    }
    i++;
    if( i > 11)
    {
      break;
    }
  }
  c->cnt = i;

  return( c);
}
