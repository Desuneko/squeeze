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
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include <libsqueeze/libsqueeze.h>

#include "cli.h"

static void sq_cli_refreshed_cb ( LSQArchive *, SQCli * );

struct _SQCli
{
    GObject parent;

    gboolean silent;
};

struct _SQCliClass
{
    GObjectClass parent;
};

G_DEFINE_TYPE( SQCli, sq_cli, G_TYPE_OBJECT )

static void
sq_cli_init ( SQCli *cli )
{
}

static void
sq_cli_class_init ( SQCliClass *cli )
{
}

SQCli *
sq_cli_new ( void )
{
    SQCli *cli;

    cli = g_object_new( SQ_TYPE_CLI, NULL );

    return cli;
}

gint
sq_cli_extract_archive ( SQCli *cli, GFile *file, gchar *dest_path )
{
    LSQArchive *lp_archive = NULL;
    if ( 0 != lsq_open_archive( file, &lp_archive ) )
    {
        /*
         * Could not open archive (mime type not supported or file did not exist)
         * Should be a more specific error message.
         */ 
        g_error( "%s", _("Could not open archive, MIME-type unsupported or file does not exist") );
        return 1;
    }
    if ( NULL == dest_path )
    {
        lsq_close_archive( lp_archive );
        return 1;
    }
    if ( FALSE == lsq_archive_operate( lp_archive, LSQ_COMMAND_TYPE_EXTRACT, NULL, dest_path ) )
    {
        g_warning( "%s", _("Squeeze cannot extract this archive type,\nthe application to support this is missing.") );
    }
    return 0;
}

gint
sq_cli_new_archive ( SQCli *cli, GFile *file, GSList *files )
{
    LSQArchive *lp_archive = NULL;

    if ( NULL == file )
    {
        return 1;
    }
    else
    {
        if ( 0 != lsq_new_archive( file, FALSE, &lp_archive ) )
        {
            /*
             * Could not open archive (mime type not supported or file did not exist)
             * Should be a more specific error message.
             */ 
            g_error( "%s", _("Could not open archive, MIME-type unsupported or file exists") );
            return 1;
        }
    }
    if ( FALSE == lsq_archive_operate( lp_archive, LSQ_COMMAND_TYPE_ADD, NULL, NULL ) )
    {
        /* FIXME: show warning dialog */
        g_warning( "%s", _("Squeeze cannot add files to this archive type,\nthe application to support this is missing.") );
    }
    return 0;
}

gint
sq_cli_add_archive ( SQCli *cli, GFile *file, GSList *files )
{
    LSQArchive *lp_archive = NULL;

    if ( NULL == file )
    {
        return 1;
    }
    else
    {
        if ( 0 != lsq_open_archive( file, &lp_archive ) )
        {
            /*
             * Could not open archive (mime type not supported or file did not exist)
             * Should be a more specific error message.
             */ 
            g_error( "%s", _("Could not open archive, MIME-type unsupported or file does not exist") );
            return 1;
        }
    }
    if ( FALSE == lsq_archive_operate( lp_archive, LSQ_COMMAND_TYPE_ADD, NULL, NULL ) )
    {
        /* FIXME: show warning dialog */
        g_warning( "%s", _("Squeeze cannot add files to this archive type,\nthe application to support this is missing.") );
    }
    return 0;
}

gint
sq_cli_open_archive ( SQCli *cli, GFile *file )
{
    LSQArchive *archive = NULL;

    if ( 0 != lsq_open_archive( file, &archive ) )
    {
        g_error( "%s", _("Failed to open file") );
        return 1;
    }
    else
    {
        g_signal_connect( archive, "refreshed", G_CALLBACK(sq_cli_refreshed_cb), cli );
        lsq_archive_operate( archive, LSQ_COMMAND_TYPE_REFRESH, NULL, NULL );
        g_object_ref( cli );
    }
    return 0;
}

void
sq_cli_set_silent ( SQCli *cli, gboolean silent )
{
    cli->silent = silent;
}


typedef struct _SQdepth SQdepth;
struct _SQdepth
{
    SQdepth *next;
    gchar line;
};

static void
sq_cli_print_iter ( LSQArchiveIter *node, SQdepth *depth )
{
    const gchar *filename;
    const gchar *dir;
    LSQArchiveIter *iter;
    guint i, size;
    SQdepth *depth_iter;
    SQdepth *depth_last = NULL;

    filename = lsq_archive_iter_get_filename( node );
    dir = ( FALSE == lsq_archive_iter_is_directory( node ) ) ? "" : "/";
    if ( NULL == filename )
    {
        filename = "/";
    }

    for ( depth_iter = depth; depth_iter; depth_iter = depth_iter->next )
    {
        gchar symbol = ( NULL == depth_iter->next ) ? '+' : depth_iter->line;
        g_printf( " %c", symbol );
        depth_last = depth_iter;
    }

    g_printf( "-%s%s\n", filename, dir );

    size = lsq_archive_iter_n_children( node );

    if ( 0 < size )
    {
        depth_iter = g_new( SQdepth, 1 );
        depth_iter->next = 0;
        depth_iter->line = '|';
        if ( NULL == depth_last )
        {
            depth = depth_iter;
        }
        else
        {
            depth_last->next = depth_iter;
        }

        for ( i = 0; i < size; ++i )
        {
            if ( size - 1 == i )
            {
                depth_iter->line = ' ';
            }
            iter = lsq_archive_iter_nth_child( node, i );
            sq_cli_print_iter( iter, depth );
        }

        if ( NULL != depth_last )
        {
            depth_last->next = NULL;
        }
        g_free( depth_iter );
    }
}

static void
sq_cli_refreshed_cb ( LSQArchive *archive, SQCli *cli )
{
    LSQArchiveIter *root;

    if ( FALSE == cli->silent )
    {
        root = lsq_archive_get_iter( archive, NULL );
        sq_cli_print_iter( root, NULL );
    }

    g_object_unref( archive );

    g_object_unref( cli );
}

