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

#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

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

static void
lsq_command_entry_start ( LSQCommandEntry *entry, LSQExecuteContext *ctx );
static void
lsq_command_entry_free ( LSQCommandEntry *entry );

static gboolean
build_queue ( LSQCommandQueue *queue, const gchar *command_string );

static void
lsq_command_queue_finalize ( GObject *object );

G_DEFINE_TYPE( LSQCommandQueue, lsq_command_queue, G_TYPE_OBJECT );

static void
lsq_command_queue_class_init ( LSQCommandQueueClass *klass )
{
    GObjectClass *object_class = G_OBJECT_CLASS( klass );

    object_class->finalize = lsq_command_queue_finalize;
}

static void
lsq_command_queue_init ( LSQCommandQueue *self )
{
}

static void
lsq_command_queue_finalize ( GObject *object )
{
    LSQCommandEntry *entry, *next;

    LSQCommandQueue *queue = LSQ_COMMAND_QUEUE( object );

    for ( entry = queue->queue; NULL != entry; entry = next )
    {
        next = entry->next;

        lsq_command_entry_free( entry );
    }
}

LSQCommandQueue *
lsq_command_queue_new ( const gchar *command_string )
{
    LSQCommandQueue *queue;

    g_return_val_if_fail( command_string, NULL );

    queue = g_object_new( LSQ_TYPE_COMMAND_QUEUE, NULL );

    if ( FALSE == build_queue( queue, command_string ) )
    {
        g_object_unref( queue );
        return NULL;
    }

    return LSQ_COMMAND_QUEUE( queue );
}

static const gchar *
lsq_execute_context_get_temp_file ( LSQExecuteContext *ctx )
{
    g_return_val_if_fail( NULL != ctx, NULL );

    if ( NULL == ctx->tempfile )
    {
        ctx->tempfile = lsq_archive_request_temp_file( ctx->archive, NULL );
    }

    return ctx->tempfile;
}

static gboolean
validate_format ( const gchar *format )
{
    g_return_val_if_fail( NULL != format, FALSE );

    if ( ( '%' == format[0] ) && ( '\0' == format[2] ) )
    {
        switch ( format[1] )
        {
            case 'a':
            case 't':
                return TRUE;
        }
    }
    return FALSE;
}

static gchar *
format_get_filename ( const gchar *format, LSQExecuteContext *ctx )
{
    g_return_val_if_fail( NULL != format, NULL );
    g_return_val_if_fail( NULL != ctx, NULL );

    if ( ( '%' == format[0] ) && ( '\0' == format[2] ) )
    {
        switch ( format[1] )
        {
            case 'a':
                return lsq_archive_get_path( ctx->archive );

            case 't':
                return g_strdup( lsq_execute_context_get_temp_file( ctx ) );
        }
    }
    return NULL;
}

static gchar **
lsq_command_entry_to_argv ( LSQCommandEntry *entry, LSQExecuteContext *ctx )
{
    gchar **argv, **argi;
    guint size;
    GSList *iter;
    gchar **filei;

    g_return_val_if_fail( NULL != entry, NULL );
    g_return_val_if_fail( NULL != ctx, NULL );

    size = 2;

    for ( iter = entry->args; NULL != iter; iter = iter->next )
    {
        if ( 0 == strcmp( (const gchar *)iter->data, "%F" ) )
        {
            if ( ctx->files )
	    {
                size += g_strv_length( ctx->files );
	    }
        }
        else
            size++;
    }

    argv = g_new( gchar *, size );

    argi = argv;

    *argi++ = g_strdup( entry->command );

    for ( iter = entry->args; iter; iter = iter->next )
    {
        const gchar *arg = (const gchar *)iter->data;
        if ( ( '%' == arg[0] ) && ( '\0' == arg[2] ) )
        {
            switch ( arg[1] )
            {
                case 'F':
                    if ( ctx->files )
                    {
                        for ( filei = ctx->files; NULL != *filei; filei++ )
                        {
                            *argi++ = g_strdup( *filei );
                        }
                    }
                    break;

                case 'a':
                    *argi++ = lsq_archive_get_path( ctx->archive );
                    break;

                case 't':
                    *argi++ = g_strdup( lsq_execute_context_get_temp_file( ctx ) );
                    break;

                case 'd':
                    *argi++ = g_strdup( ctx->directory );
                    break;

                default:
                    //...
                    break;
            }
        }
        else
            *argi++ = g_strdup( arg );
    }

    *argi = NULL;

    return argv;
}

static void
child_exit ( GPid pid, gint status, LSQExecuteContext *ctx )
{
    g_spawn_close_pid( pid );
    ctx->state &= ~LSQ_EXEC_CTX_STATE_RUNNING;
    if ( 0 == ctx->state )
    {
        ctx->queue = ctx->queue->next;
        if ( NULL != ctx->queue )
        {
            lsq_command_entry_start( ctx->queue, ctx );
        }
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

static void
in_channel ( GIOChannel *source, GIOCondition condition, GIOChannel *dest )
{
    GIOStatus stat = G_IO_STATUS_NORMAL;
    static gchar buffer[1024];

    if ( G_IO_IN & condition )
    {
        gsize n;
        stat = g_io_channel_read_chars( source, buffer, 1024, &n, NULL );
        if ( G_IO_STATUS_NORMAL == stat )
        {
            g_io_channel_write_chars( dest, buffer, n, NULL, NULL );
        }
    }

    if ( G_IO_HUP & condition || ( G_IO_STATUS_NORMAL != stat && G_IO_STATUS_AGAIN != stat ) )
    {
        g_io_channel_unref( source );
        g_io_channel_flush( dest, NULL );
        g_io_channel_unref( dest );
    }
}

static void
out_channel ( GIOChannel *source, GIOCondition condition, LSQExecuteContext *ctx )
{
    GIOStatus stat = G_IO_STATUS_NORMAL;
    GIOChannel *dest = ctx->redir_out;
    static gchar buffer[1024];

    if ( G_IO_IN & condition )
    {
        gsize n;
        stat = g_io_channel_read_chars( source, buffer, 1024, &n, NULL );
        if ( G_IO_STATUS_NORMAL == stat )
        {
            g_io_channel_write_chars( dest, buffer, n, NULL, NULL );
        }
    }

    if ( G_IO_HUP & condition || ( G_IO_STATUS_NORMAL != stat && G_IO_STATUS_AGAIN != stat ) )
    {
        g_io_channel_unref( source );
        g_io_channel_flush( dest, NULL );
        g_io_channel_unref( dest );
        ctx->redir_out = NULL;
        ctx->state &= ~LSQ_EXEC_CTX_STATE_PARSING;
        if ( 0 == ctx->state )
        {
            ctx->queue = ctx->queue->next;
            if ( NULL != ctx->queue )
            {
                lsq_command_entry_start( ctx->queue, ctx );
            }
            //else
            //...//done
        }
    }
}

static gboolean
parse_channel ( GIOChannel *source, GIOCondition condition, LSQExecuteContext *ctx )
{
    if ( G_IO_IN & condition )
    {
        do
        {
            lsq_parser_parse( ctx->parser, ctx->ctx );
        }
        while ( TRUE == lsq_parser_context_read_again( ctx->ctx ) );
    }

    if ( G_IO_HUP & condition || FALSE == lsq_parser_context_is_good( ctx->ctx ) )
    {
        lsq_parser_context_set_channel( ctx->ctx, NULL );
        g_io_channel_unref( source );
        ctx->state &= ~LSQ_EXEC_CTX_STATE_PARSING;
        if ( 0 == ctx->state )
        {
            ctx->queue = ctx->queue->next;
            if ( NULL != ctx->queue )
            {
                lsq_command_entry_start( ctx->queue, ctx );
            }
            //else
            //...//done
        }
        //FIXME: this is not entirely the correct place, or is it?
        lsq_archive_refreshed( ctx->archive );
        return FALSE;
    }
    return TRUE;
}

static void
lsq_command_entry_start ( LSQCommandEntry *entry, LSQExecuteContext *ctx )
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
    gchar **argvi;

    g_return_if_fail( NULL != entry );
    g_return_if_fail( NULL != ctx );

    if ( NULL != entry->redirect_out )
    {
        gchar *file = format_get_filename( entry->redirect_out, ctx );
        ctx->redir_out = g_io_channel_new_file( file, "w", NULL );
        g_free( file );
        g_return_if_fail( NULL != ctx->redir_out );
    }
    else if ( NULL == ctx->ctx )
    {
        flags |= G_SPAWN_STDOUT_TO_DEV_NULL;
        fd_out = FALSE;
    }
    if ( NULL != entry->redirect_in )
    {
        gchar *file = format_get_filename( entry->redirect_in, ctx );
        redir_in = g_io_channel_new_file( file, "r", NULL );
        g_free( file );
        g_return_if_fail( NULL != redir_in );
        fd_in = TRUE;
    }

    argv = lsq_command_entry_to_argv( entry, ctx );

    g_debug( "command: %s", argv[0] );

#ifdef DEBUG
    argvi = argv;
    while ( NULL != argvi[1] )
    {
        g_debug( "arg: '%s'", argvi[1] );
        argvi++;
    }
#endif

    if ( FALSE == g_spawn_async_with_pipes( NULL, argv, NULL, flags, NULL, NULL, &pid, ( TRUE == fd_in ) ? &fd_in : NULL, ( TRUE == fd_out ) ? &fd_out : NULL , NULL, &error ) )
    {
        g_debug( "spawn failed: %s", error->message );
        g_error_free( error );
        return;
    }

    g_strfreev( argv );

    g_child_watch_add( pid, (GChildWatchFunc)child_exit, ctx );
    ctx->state |= LSQ_EXEC_CTX_STATE_RUNNING;

    if ( NULL != entry->redirect_in )
    {
        chan_in = g_io_channel_unix_new( fd_in );
        g_return_if_fail( NULL != chan_in );
        g_io_add_watch( redir_in, G_IO_IN|G_IO_HUP, (GIOFunc)in_channel, chan_in );
    }
    if ( NULL != entry->redirect_out )
    {
        chan_out = g_io_channel_unix_new( fd_out );
        g_return_if_fail( NULL != chan_out );
        g_io_add_watch( chan_out, G_IO_IN|G_IO_HUP, (GIOFunc)out_channel, ctx );
        ctx->state |= LSQ_EXEC_CTX_STATE_PARSING;
    }
    else if ( NULL != ctx->ctx )
    {
        chan_out = g_io_channel_unix_new( fd_out );
        g_return_if_fail( NULL != chan_out );
        lsq_parser_context_set_channel( ctx->ctx, chan_out );
        g_io_add_watch( chan_out, G_IO_IN|G_IO_HUP, (GIOFunc)parse_channel, ctx );
        ctx->state |= LSQ_EXEC_CTX_STATE_PARSING;
    }
}

LSQExecuteContext *
lsq_command_queue_execute ( LSQCommandQueue *queue, LSQArchive *archive, gchar **files, const gchar *directory, LSQParser *parser )
{
    LSQExecuteContext *ctx;

    g_return_val_if_fail( LSQ_IS_COMMAND_QUEUE( queue ), NULL );
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), NULL );

    ctx = g_new0( LSQExecuteContext, 1 );

    ctx->queue = queue->queue;
    ctx->archive = archive;
    ctx->files = g_strdupv( files );
    ctx->directory = g_strdup( directory );
    ctx->parser = parser;
    ctx->ctx = ( NULL != parser ) ? lsq_parser_get_context( parser, archive ) : NULL;

    lsq_command_entry_start( ctx->queue, ctx );

    return ctx;
}

static gchar *
strdup_escaped ( const gchar *str, guint lng )/*{{{*/
{
    guint i;
    gchar *new_str;
    gchar ch;
    guint size = 0;

    g_return_val_if_fail( NULL != str, NULL );

    for ( i = 0; i < lng; ++i )
    {
        switch ( str[i] )
        {
            case '\\':
                ++i;
                switch ( str[i] )
                {
                    case 'x':
                        if ( g_ascii_isxdigit( str[i+1] ) )
                        {
                            ++i;
                            if ( g_ascii_isxdigit( str[i+1] ) )
                            {
                                ++i;
                            }
                        }
                        break;

                    default:
                        ch = str[i+1];
                        if ( '0' <= ch && '8' > ch )
                        {
                            ++i;
                            ch = str[i+1];
                            if ( '0' <= ch && '8' > ch )
                            {
                                ch = str[i];
                                ++i;
                                if ( '4' > ch )
                                {
                                    ch = str[i+1];
                                    if ( '0' <= str[i+1] && '8' > str[i+1] )
                                    {
                                        ++i;
                                    }
                                }
                            }
                        }
                        break;
                }
                break;
        }
        ++size;
    }

    new_str = g_new( gchar, size + 1 );
    new_str[size] = '\0';

    size = 0;
    for ( i = 0; i < lng; ++i )
    {
        ch = str[i];
        switch ( ch )
        {
            case '\\':
                ++i;
                ch = str[i];
                switch ( ch )
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
                        if ( TRUE == g_ascii_isxdigit( str[i+1] ) )
                        {
                            ++i;
                            ch = g_ascii_xdigit_value( str[i] );
                            if ( TRUE == g_ascii_isxdigit( str[i+1] ) )
                            {
                                ++i;
                                ch = ( ch * 0x10 ) + g_ascii_xdigit_value( str[i] );
                            }
                        }
                        break;

                    default:
                        if ( '0' <= str[i+1] && '8' > str[i+1] )
                        {
                            ++i;
                            ch = str[i] - '0';
                            if ( '0' <= str[i+1] && '8' > str[i+1] )
                            {
                                ++i;
                                ch = ( ch * 010 ) + ( str[i] - '0' );
                                if ( 040 > ch )
                                {
                                    if ( '0' <= str[i+1] && '8' > str[i+1] )
                                    {
                                        ++i;
                                        ch = ( ch * 010 ) + ( str[i] - '0' );
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

static gboolean
build_queue ( LSQCommandQueue *queue, const gchar *command_string )
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

    LSQCommandEntry *command;
    LSQCommandEntry *prev_cmd = NULL;

    gchar **direct_to = NULL;

    g_return_val_if_fail( LSQ_IS_COMMAND_QUEUE( queue ), FALSE );
    g_return_val_if_fail( NULL != command_string, FALSE );

    queue->queue = command = g_new0( LSQCommandEntry, 1 );

    cur = ptr = command_string;

    do
    {
        ch = *ptr++;
        /* Split the code into multiple pieces, so multiple switch statements on ch */
        /* Check for data separators and finish previous state */
        switch ( ch )
        {
            case '\0':
            case ';':
            case '>':
            case '<':
            case ' ':
            case '\t':
                if ( cur + 1 != ptr )
                {
                    switch ( state )
                    {
                        case STATE_COMMAND:
                            command->command = strdup_escaped( cur, ( ptr - cur ) - 1 );
                            state = STATE_ARGS;
                            break;

                        case STATE_ARGS:
                            command->args = g_slist_append( command->args, strdup_escaped( cur, ( ptr - cur ) - 1 ) );
                            break;

                        case STATE_REDIRECT:
                            /* We only support redirecting to %a or %t */
                            *direct_to = strdup_escaped( cur, ( ptr - cur ) - 1 );
                            if ( FALSE == validate_format( *direct_to ) )
                            {
                                g_warning( "Invalid redirect near character %"G_GSIZE_FORMAT" in \'%s\'", (gsize)(cur - command_string), command_string );
                                return FALSE;
                            }
                            state = STATE_FINISH;
                            break;

                        case STATE_FINISH:
                            g_warning( "Trailing data near character %"G_GSIZE_FORMAT" in \'%s\'", (gsize)(cur - command_string), command_string );
                            return FALSE;
                    }
                }
                cur = ptr;
                break;
        }
        /* Check for invalid stats on command part separators */
        switch ( ch )
        {
            case '\0':
            case ';':
            case '>':
            case '<':
                if ( STATE_ARGS != state && STATE_FINISH != state )
                {
                    g_warning( "Incomplete %s near character %"G_GSIZE_FORMAT" in \'%s\'", ( STATE_REDIRECT == state ) ? "redirect" : "command", (gsize)(cur - command_string), command_string );
                    return FALSE;
                }
                break;
        }
        /* Setup the next state */
        switch ( ch )
        {
            case '\\':
                if ( '\0' != *ptr )
                {
                    ptr++;
                }
                break;

            case ';':
                prev_cmd = command;
                prev_cmd->next = command = g_new0( LSQCommandEntry, 1 );
                direct_to = NULL;
                state = STATE_COMMAND;
                break;

            case '>':
                direct_to = &command->redirect_out;
                state = STATE_REDIRECT;
                break;

            case '<':
                direct_to = &command->redirect_in;
                state = STATE_REDIRECT;
                break;
        }
    }
    while ( '\0' !=  ch );

    return TRUE;
}

static void
lsq_command_entry_free ( LSQCommandEntry *entry )
{
    g_return_if_fail( NULL != entry );

    g_free( entry->command );
    g_slist_foreach( entry->args, (GFunc)g_free, NULL );
    g_slist_free( entry->args );
    g_free( entry->redirect_in );
    g_free( entry->redirect_out );

    g_free( entry );
}

