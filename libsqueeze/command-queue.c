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
#include <string.h>

#include <thunar-vfs/thunar-vfs.h>
#include "libsqueeze.h"
#include "archive-tempfs.h"
#include "parser-context.h"
#include "parser.h"

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

struct _LSQExecuteContext
{
  LSQCommandEntry *queue;
  LSQParser *parser;
  LSQArchive *archive;
  gchar **files;
  gchar *directory;
  gchar *tempfile;
  LSQParserContext *ctx;
  GIOChannel *redir_out;
  enum {
    LSQ_EXEC_CTX_STATE_RUNNING = 1<<0,
    LSQ_EXEC_CTX_STATE_PARSING = 1<<1
  } state;
};

static void lsq_command_entry_start(LSQCommandEntry *entry, LSQExecuteContext *ctx);

static void build_queue(LSQCommandQueue *queue, const gchar *command_string);

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

  g_return_val_if_fail(command_string, NULL);

  queue = g_object_new(LSQ_TYPE_COMMAND_QUEUE, NULL);

  build_queue(queue, command_string);

  return LSQ_COMMAND_QUEUE(queue);
}

static const gchar *lsq_execute_context_get_temp_file(LSQExecuteContext *ctx)
{
  if(!ctx->tempfile)
  {
    ctx->tempfile = lsq_archive_request_temp_file(ctx->archive, NULL);
  }

  return ctx->tempfile;
}

static gchar *format_get_filename(const gchar *format, LSQExecuteContext *ctx)
{
  if((format[0] == '%') && (format[2] == '\0'))
  {
    switch(format[1])
    {
      case 'a':
        return lsq_archive_get_path(ctx->archive);
      case 't':
        return g_strdup(lsq_execute_context_get_temp_file(ctx));
    }
  }
  return NULL;
}

static gchar **lsq_command_entry_to_argv(LSQCommandEntry *entry, LSQExecuteContext *ctx)
{
  gchar **argv, **argi;
  guint size;
  GSList *iter;
  gchar **filei;
  
  size = 2;

  for(iter = entry->args; iter; iter = iter->next)
  {
    if(0==strcmp((const gchar*)iter->data, "%F"))
    {
      if(ctx->files)
        size += g_strv_length(ctx->files);
    }
    else
      size++;
  }

  argv = g_new(gchar *, size);

  argi = argv;

  *argi++ = g_strdup(entry->command);

  for(iter = entry->args; iter; iter = iter->next)
  {
    const gchar *arg = (const gchar*)iter->data;
    if((arg[0] == '%') && (arg[2] == '\0'))
    {
      switch(arg[1])
      {
        case 'F':
          if(ctx->files)
          {
            for(filei = ctx->files; *filei; filei++)
            {
              *argi++ = g_strdup(*filei);
            }
          }
          break;
        case 'a':
          *argi++ = lsq_archive_get_path(ctx->archive);
          break;
        case 't':
          *argi++ = g_strdup(lsq_execute_context_get_temp_file(ctx));
          break;
        case 'd':
          *argi++ = g_strdup(ctx->directory);
          break;
        default:
          //...
          break;
      }
    }
    else
      *argi++ = g_strdup(arg);
  }

  *argi = NULL;

  return argv;
}

static void child_exit(GPid pid, gint status, LSQExecuteContext *ctx)
{
  g_spawn_close_pid(pid);
  ctx->state &= ~LSQ_EXEC_CTX_STATE_RUNNING;
  if(!ctx->state)
  {
    if((ctx->queue = ctx->queue->next))
      lsq_command_entry_start(ctx->queue, ctx);
    else
    {
        //...ERROR | DONE//
    }
  }
  else
  {
        //...ERROR | DONE//
  }
}

static void in_channel(GIOChannel *source, GIOCondition condition, GIOChannel *dest)
{
	GIOStatus stat = G_IO_STATUS_NORMAL;
  static gchar buffer[1024];

  if(condition & G_IO_IN)
  {
    gsize n;
    stat = g_io_channel_read_chars(source, buffer, 1024, &n, NULL);
    if(stat == G_IO_STATUS_NORMAL)
      g_io_channel_write_chars(dest, buffer, n, NULL, NULL);
  }

  if(condition & G_IO_HUP || (stat != G_IO_STATUS_NORMAL && stat != G_IO_STATUS_AGAIN))
  {
    g_io_channel_unref(source);
    g_io_channel_flush(dest, NULL);
    g_io_channel_unref(dest);
  }
}

static void out_channel(GIOChannel *source, GIOCondition condition, LSQExecuteContext *ctx)
{
	GIOStatus stat = G_IO_STATUS_NORMAL;
  GIOChannel *dest = ctx->redir_out;
  static gchar buffer[1024];

  if(condition & G_IO_IN)
  {
    gsize n;
    stat = g_io_channel_read_chars(source, buffer, 1024, &n, NULL);
    if(stat == G_IO_STATUS_NORMAL)
      g_io_channel_write_chars(dest, buffer, n, NULL, NULL);
  }

  if(condition & G_IO_HUP || (stat != G_IO_STATUS_NORMAL && stat != G_IO_STATUS_AGAIN))
  {
    g_io_channel_unref(source);
    g_io_channel_flush(dest, NULL);
    g_io_channel_unref(dest);
    ctx->redir_out = NULL;
    ctx->state &= ~LSQ_EXEC_CTX_STATE_PARSING;
    if(!ctx->state)
    {
      if((ctx->queue = ctx->queue->next))
        lsq_command_entry_start(ctx->queue, ctx);
      //else
        //...//done
    }
  }
}

static gboolean
parse_channel(GIOChannel *source, GIOCondition condition, LSQExecuteContext *ctx)
{
  if(condition & G_IO_IN)
  {
    do {
      lsq_parser_parse(ctx->parser, ctx->ctx);
    } while(lsq_parser_context_read_again(ctx->ctx));
  }

  if(condition & G_IO_HUP || !lsq_parser_context_is_good(ctx->ctx))
  {
    lsq_parser_context_set_channel(ctx->ctx, NULL);
    g_io_channel_unref(source);
    ctx->state &= ~LSQ_EXEC_CTX_STATE_PARSING;
    if(!ctx->state)
    {
      if((ctx->queue = ctx->queue->next))
        lsq_command_entry_start(ctx->queue, ctx);
      //else
        //...//done
    }
    //FIXME: this is not entirely the correct place, or is it?
    lsq_archive_refreshed(ctx->archive);
    return FALSE;
  }
  return TRUE;
}

static void lsq_command_entry_start(LSQCommandEntry *entry, LSQExecuteContext *ctx)
{
  GError *error = NULL;
  gint fd_in = FALSE;
  gint fd_out = TRUE;
  GIOChannel *redir_in = NULL;
  GIOChannel *chan_in;
  GIOChannel *chan_out;
  gchar **argv;
  GPid pid;
  GSpawnFlags flags = G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL;
  if(entry->redirect_out)
  {
    gchar *file = format_get_filename(entry->redirect_out, ctx);
    ctx->redir_out = g_io_channel_new_file(file, "w", NULL);
    g_free(file);
  }
  else if(!ctx->ctx)
  {
    flags |= G_SPAWN_STDOUT_TO_DEV_NULL;
    fd_out = FALSE;
  }
  if(entry->redirect_in)
  {
    gchar *file = format_get_filename(entry->redirect_in, ctx);
    redir_in = g_io_channel_new_file(file, "r", NULL);
    g_free(file);
    fd_in = TRUE;
  }

  argv = lsq_command_entry_to_argv(entry, ctx);

  g_debug("command: %s", argv[0]);

  gchar **argvi = argv;
  while (argvi[1])
  {
    g_debug("arg: '%s'", argvi[1]);
    argvi++;
  }

  if(!g_spawn_async_with_pipes(NULL, argv, NULL, flags, NULL, NULL, &pid, fd_in?&fd_in:NULL, fd_out?&fd_out:NULL, NULL, &error))
  {
    g_debug("spawn failed: %s", error->message);
    g_error_free(error);
    return;
  }

  g_strfreev(argv);

  g_child_watch_add(pid, (GChildWatchFunc)child_exit, ctx);
  ctx->state |= LSQ_EXEC_CTX_STATE_RUNNING;

  if(entry->redirect_in)
  {
    chan_in = g_io_channel_unix_new(fd_in);
    g_io_add_watch(redir_in, G_IO_IN|G_IO_HUP, (GIOFunc)in_channel, chan_in);
  }
  if(entry->redirect_out)
  {
    chan_out = g_io_channel_unix_new(fd_out);
    g_io_add_watch(chan_out, G_IO_IN|G_IO_HUP, (GIOFunc)out_channel, ctx);
    ctx->state |= LSQ_EXEC_CTX_STATE_PARSING;
  }
  else if(ctx->ctx)
  {
    chan_out = g_io_channel_unix_new(fd_out);
    lsq_parser_context_set_channel(ctx->ctx, chan_out);
    g_io_add_watch(chan_out, G_IO_IN|G_IO_HUP, (GIOFunc)parse_channel, ctx);
    ctx->state |= LSQ_EXEC_CTX_STATE_PARSING;
  }
}

LSQExecuteContext *lsq_command_queue_execute(LSQCommandQueue *queue, LSQArchive *archive, const gchar **files, const gchar *directory, LSQParser *parser)
{
  LSQExecuteContext *ctx;

  ctx = g_new(LSQExecuteContext, 1);

  ctx->queue = queue->queue;
  ctx->archive = archive;
  ctx->files = g_strdupv((gchar**)files);
  ctx->directory = g_strdup(directory);
  ctx->parser = parser;
  ctx->ctx = parser?lsq_parser_get_context(parser, archive):NULL;

  lsq_command_entry_start(ctx->queue, ctx);

  return ctx;
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

