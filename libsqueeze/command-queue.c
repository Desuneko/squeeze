/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or 
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <glib.h>
#include <glib-object.h> 
#include <signal.h>

#include <thunar-vfs/thunar-vfs.h>
#include "libsqueeze.h"

#include "command-queue.h"

typedef struct _LSQCommandEntry LSQCommandEntry;

struct _LSQCommandEntry
{
  LSQCommandEntry *next;
  gchar *command;
  GSList *args;
  gchar *redirect_in;
  gchar *redirect_out;
};

struct _LSQCommandQueue
{
	GObject parent;

  LSQCommandEntry *queue;
};

struct _LSQCommandQueueClass
{
	GObjectClass parent;
};

static void build_queue(LSQCommandQueue *queue, const gchar *commant_string);

G_DEFINE_TYPE(LSQCommandQueue, lsq_command_queue, G_TYPE_OBJECT);

static void
lsq_command_queue_init(LSQCommandQueue *self)
{
}

static void
lsq_command_queue_class_init(LSQCommandQueueClass *klass)
{
}

LSQCommandQueue *lsq_command_queue_new(const gchar *command_string)
{
  LSQCommandQueue *queue;

  queue = g_object_new(LSQ_TYPE_COMMAND_QUEUE, NULL);

  build_queue(queue, command_string);

  return LSQ_COMMAND_QUEUE(queue);
}

static gchar* strdup_escaped(const gchar *str, guint lng)/*{{{*/
{
  guint i;
  gchar *new_str;
  gchar ch;
  guint size = 0;

  for(i = 0; i < lng; i++)
  {
    switch(str[i])
    {
      case '\\':
        i++;
        switch(str[i])
        {
          case 'x':
            if(g_ascii_isxdigit(str[i+1]))
            {
              i++;
              if(g_ascii_isxdigit(str[i+1]))
              {
                i++;
              }
            }
          break;
          default:
            ch = str[i+1];
            if(ch>='0' && ch < '8')
            {
              i++;
              ch = str[i+1];
              if(ch>='0' && ch < '8')
              {
                ch = str[i];
                i++;
                if(ch < '4')
                {
                  ch = str[i+1];
                  if(str[i+1]>='0' && str[i+1] < '8')
                  {
                    i++;
                  }
                }
              }
            }
          break;
        }
      break;
    }
    size++;
  }

  new_str = g_new(gchar, size+1);
  new_str[size] = '\0';

  size = 0;
  for(i = 0; i < lng; i++)
  {
    switch(ch = str[i])
    {
      case '\\':
        i++;
        switch(ch = str[i])
        {
          case 'a':
            ch = '\a';
          break;
          case 'b':
            ch = '\b';
          break;
          case 'f':
            ch = '\f';
          break;
          case 'n':
            ch = '\n';
          break;
          case 'r':
            ch = '\r';
          break;
          case 't':
            ch = '\t';
          break;
          case 'v':
            ch = '\v';
          break;
          case 'x':
            if(g_ascii_isxdigit(str[i+1]))
            {
              i++;
              ch = g_ascii_xdigit_value(str[i]);
              if(g_ascii_isxdigit(str[i+1]))
              {
                i++;
                ch = (ch*0x10) + g_ascii_xdigit_value(str[i]);
              }
            }
          break;
          default:
            if(str[i+1]>='0' && str[i+1] < '8')
            {
              i++;
              ch = str[i]-'0';
              if(str[i+1]>='0' && str[i+1] < '8')
              {
                i++;
                if((ch = (ch*010) + (str[i]-'0')) < 040)
                {
                  if(str[i+1]>='0' && str[i+1] < '8')
                  {
                    i++;
                    ch = (ch*010) + (str[i]-'0');
                  }
                }
              }
            }
          break;
        }
      break;
    }
    new_str[size++] = ch;
  }

  return new_str;
}/*}}}*/

static void build_queue(LSQCommandQueue *queue, const gchar *command_string)
{
  const gchar *ptr;
  const gchar *cur;
  gchar ch;
  enum {
    STATE_COMMAND,
    STATE_ARGS,
    STATE_REDIRECT,
    STATE_FINISH
  } state = STATE_COMMAND;

  LSQCommandEntry *command = g_new0(LSQCommandEntry, 1);
  LSQCommandEntry *commands = command;
  LSQCommandEntry *prev_cmd = NULL;

  gchar **direct_to = NULL;

  cur = ptr = command_string;

  while((ch = *ptr++))
  {
    switch(ch)
    {
      case '\\':
        if(*ptr)
          ptr++;
      break;
      case ';':
        if(cur+1 != ptr)
        {
          switch(state)
          {
            case STATE_COMMAND:
              command->command = strdup_escaped(cur, (ptr-cur)-1);
              state = STATE_ARGS;
            break;
            case STATE_ARGS:
              command->args = g_slist_append(command->args, strdup_escaped(cur, (ptr-cur)-1));
            break;
            case STATE_REDIRECT:
              g_return_if_fail(cur[0] == '%');
              *direct_to = strdup_escaped(cur, (ptr-cur)-1);
              state = STATE_FINISH;
            break;
            case STATE_FINISH:
              g_return_if_reached();
            break;
          }
        }
        cur = ptr;
        g_return_if_fail(state == STATE_ARGS || state == STATE_FINISH);
        prev_cmd = command;
        prev_cmd->next = command = g_new0(LSQCommandEntry, 1);
        direct_to = NULL;
        state = STATE_COMMAND;
      break;
      case '>':
        if(cur+1 != ptr)
        {
          switch(state)
          {
            case STATE_COMMAND:
              command->command = strdup_escaped(cur, (ptr-cur)-1);
              state = STATE_ARGS;
            break;
            case STATE_ARGS:
              command->args = g_slist_append(command->args, strdup_escaped(cur, (ptr-cur)-1));
            break;
            case STATE_REDIRECT:
              g_return_if_fail(cur[0] == '%');
              *direct_to = strdup_escaped(cur, (ptr-cur)-1);
              state = STATE_FINISH;
            break;
            case STATE_FINISH:
              g_return_if_reached();
            break;
          }
        }
        cur = ptr;
        g_return_if_fail(state == STATE_ARGS || state == STATE_FINISH);
        direct_to = &command->redirect_out;
        state = STATE_REDIRECT;
      break;
      break;
      case '<':
        if(cur+1 != ptr)
        {
          switch(state)
          {
            case STATE_COMMAND:
              command->command = strdup_escaped(cur, (ptr-cur)-1);
              state = STATE_ARGS;
            break;
            case STATE_ARGS:
              command->args = g_slist_append(command->args, strdup_escaped(cur, (ptr-cur)-1));
            break;
            case STATE_REDIRECT:
              g_return_if_fail(cur[0] == '%');
              *direct_to = strdup_escaped(cur, (ptr-cur)-1);
              state = STATE_FINISH;
            break;
            case STATE_FINISH:
              g_return_if_reached();
            break;
          }
        }
        cur = ptr;
        g_return_if_fail(state == STATE_ARGS || state == STATE_FINISH);
        direct_to = &command->redirect_in;
        state = STATE_REDIRECT;
      break;
      case ' ':
      case '\t':
        if(cur+1 != ptr)
        {
          switch(state)
          {
            case STATE_COMMAND:
              command->command = strdup_escaped(cur, (ptr-cur)-1);
              state = STATE_ARGS;
            break;
            case STATE_ARGS:
              command->args = g_slist_append(command->args, strdup_escaped(cur, (ptr-cur)-1));
            break;
            case STATE_REDIRECT:
              g_return_if_fail(cur[0] == '%');
              *direct_to = strdup_escaped(cur, (ptr-cur)-1);
              state = STATE_FINISH;
            break;
            case STATE_FINISH:
              g_return_if_reached();
            break;
          }
        }
        cur = ptr;
      break;
    }
  }

  if(cur+1 != ptr)
  {
    switch(state)
    {
      case STATE_COMMAND:
        command->command = strdup_escaped(cur, (ptr-cur)-1);
        state = STATE_ARGS;
      break;
      case STATE_ARGS:
        command->args = g_slist_append(command->args, strdup_escaped(cur, (ptr-cur)-1));
      break;
      case STATE_REDIRECT:
        g_return_if_fail(cur[0] == '%');
        *direct_to = strdup_escaped(cur, (ptr-cur)-1);
        state = STATE_FINISH;
      break;
      case STATE_FINISH:
        g_return_if_reached();
      break;
    }
  }
  g_return_if_fail(state == STATE_ARGS || state == STATE_FINISH);

  if(!command->command)
  {
    g_return_if_fail(prev_cmd);
    prev_cmd->next = NULL;
    g_free(command);
  }

  queue->queue = commands;
}

