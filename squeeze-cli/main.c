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
#include <glib-object.h>
#include <gio/gio.h>

#include <libsqueeze/libsqueeze.h>

#include "cli.h"

gboolean version = FALSE;

gchar *extract_archive_path = NULL;
gchar *new_archive = NULL;
gchar *add_archive_path = NULL;

gboolean verbose = FALSE;
gboolean silent = FALSE;

#ifdef LSQ_ENTRY_CHILD_BUFFER_DYNAMIC_SIZE
extern guint buffer_flush_size;
#endif

gpointer command;

gint opened_archives = 0;

static GOptionEntry entries[] =
{
    {
        "extract-to", 'x', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &extract_archive_path,
        NULL,
        N_("[destination path]")
    },
    {
        "add-to", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &add_archive_path,
        NULL,
        N_("[archive path] [file1] [file2] ... [fileN]")
    },
    {
        "new", 'n', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &new_archive,
        NULL,
        N_("[file1] [file2] ... [fileN]")
    },
    {
        "verbose", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &verbose,
        N_("Version information"),
        NULL
    },
    {
        "silent", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &silent,
        N_("Version information"),
        NULL
    },
    {
        "version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &version,
        N_("Version information"),
        NULL
    },
#ifdef LSQ_ENTRY_CHILD_BUFFER_DYNAMIC_SIZE
    {
        "flush", 'f', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT, &buffer_flush_size,
        NULL,
        NULL
    },
#endif
    { NULL }
};

int
main ( int argc, char **argv )
{
    GError *cli_error = NULL;
    gint i = 0;
    GFile *file = NULL;
    GMainLoop *main_loop;
    GOptionContext *option_ctx;
    SQCli *cli;

#ifdef ENABLE_NLS
    bindtextdomain( GETTEXT_PACKAGE, LOCALEDIR );
    bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
    textdomain( GETTEXT_PACKAGE );
#endif

    /*
     * Setup the command line options
     */
    option_ctx = g_option_context_new( "[archive]" );
    g_option_context_add_main_entries( option_ctx, entries, PACKAGE );

    /*
     * Parse the command line options
     */
    if ( FALSE == g_option_context_parse( option_ctx, &argc, &argv, &cli_error ) )
    {
        /*
         * We should always have an error here
         */
        if ( G_LIKELY( NULL != cli_error ) )
        {
            g_print(
                    _("%s: %s\nTry %s --help to see a full list of available command line options.\n"),
                    PACKAGE,
                    cli_error->message,
                    g_get_prgname()
                );

            g_error_free( cli_error );
            g_option_context_free( option_ctx );

            return 1;
        }
    }

    g_option_context_free( option_ctx );

    if ( version )
    {
        g_print( "%s\n", PACKAGE_STRING );
        return 0;
    }

    g_type_init();
    main_loop = g_main_loop_new( NULL, FALSE );

    lsq_init();

    cli = sq_cli_new();

    g_object_weak_ref( G_OBJECT(cli), (GWeakNotify)g_main_loop_quit, main_loop );

    sq_cli_set_silent ( cli, silent );

    /*
     * Extract files
     */
    if ( NULL != extract_archive_path )
    {
        gint err = 0;
        if ( 1 >= argc )
        {
            g_print( _("%s: No archive file.\n"), PACKAGE );
            return 1;
        }
        if ( 2 < argc )
        {
            g_print( _("%s: To many files.\n"), PACKAGE );
            return 1;
        }
        for ( i = 1; i < argc; ++i )
        {
            file = g_file_new_for_path( argv[i] );
            if ( NULL != file )
            {
                if ( 0 != sq_cli_extract_archive( cli, file, extract_archive_path ) )
                {
                    ++err;
                }
                g_object_unref( file );
            }
            else
            {
                ++err;
            }
        }
        if ( 0 < err )
        {
            g_print( _("%s: %d archive(s) failed to start extracting.\n"), PACKAGE, err );
            return 1;
        }
    }
    /*
     * Add files
     */
    else if ( NULL != new_archive || NULL != add_archive_path )
    {
        gint err = 0;
        GSList *files = NULL;

        /*
         * Remove prefix if it is the pwd
         */
        for ( i = 1; i < argc; ++i )
        {
            file = g_file_new_for_path( new_archive );
            files = g_slist_prepend( files, file );
        }

        if ( NULL != new_archive )
        {
            file = g_file_new_for_path( new_archive );
            if ( NULL != file )
            {
                if ( 0 != sq_cli_new_archive( cli, file, files ) )
                {
                    ++err;
                }
            }
            else
            {
                ++err;
            }
            if ( 0 < err )
            {
                g_print( _("%s: failed to create new archive.\n"), PACKAGE );
                return 1;
            }
        }
        else
        {
            file = g_file_new_for_path( add_archive_path );
            if ( NULL != file )
            {
                if ( 0 != sq_cli_add_archive( cli, file, files ) )
                {
                    ++err;
                }
            }
            else
            {
                ++err;
            }
            if ( 0 < err )
            {
                g_print( _("%s: failed to add files to archive.\n"), PACKAGE );
                return 1;
            }
        }
    }
    /*
     * List archive
     */
    else
    {
        gint err = 0;
        if ( 1 >= argc )
        {
            g_print(
                    _("%s: %s\nTry %s --help to see a full list of available command line options.\n"),
                    PACKAGE,
                    _("No archive file."),
                    g_get_prgname()
               );
            return 1;
        }
        if ( 2 < argc )
        {
            g_print( _("%s: To many files.\n"), PACKAGE );
            return 1;
        }
        for(i = 1; i < argc; i++)
        {
            file = g_file_new_for_path( argv[i] );
            if ( NULL != file )
            {
                if ( 0 != sq_cli_open_archive( cli, file ) )
                {
                    ++err;
                }
            }
            else
            {
                ++err;
            }
        }
        if ( 0 < err )
        {
            g_print( _("%s: %d archive(s) failed to open.\n"), PACKAGE, err );
            return 1;
        }
    }

    g_object_unref( cli );

    g_main_loop_run( main_loop );

    lsq_shutdown();
    g_main_loop_unref( main_loop );

    return 0;
}
