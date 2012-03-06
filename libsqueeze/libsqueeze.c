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
#include <glib/gstdio.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "libsqueeze.h"
#include "support-factory.h"
#include "support-reader.h"
#include "archive-iter.h"
#include "archive.h"

#include "internals.h"

static void
lsq_read_squeeze_dir ( const gchar *dir )
{
    const gchar *filename;
    GDir *data_dir;

    g_return_if_fail( NULL != dir );

    data_dir = g_dir_open( dir, 0, NULL );

    if ( NULL != data_dir)
    {
        filename = g_dir_read_name( data_dir );
        while ( NULL != filename )
        {
            if ( g_str_has_suffix( filename, ".squeeze" ) )
            {
                /* see if a file with this name was already loaded.
                 * skip this file. configuration was overruled
                 */
                if ( NULL == g_slist_find_custom( support_factory_list, filename, (GCompareFunc)lsq_suport_factory_compare_filename ) )
                {
                    /**
                     * FIXME: factories should be per-mime-type, not per-template
                     */
                    gchar *path = g_build_filename( dir, filename, NULL );
                    LSQSupportFactory *factory = lsq_support_reader_parse_file( path );
                    if ( NULL != factory )
                    {
                        support_factory_list = g_slist_append( support_factory_list, factory );
                    }
                    g_free( path );
                }
            }
            filename = g_dir_read_name( data_dir );
        }

        g_dir_close( data_dir );
    }
}

void
lsq_init ( void )
{
    gchar *data_squeeze;
    const gchar * const *system_dirs = g_get_system_data_dirs();
    const gchar *user_dir = g_get_user_data_dir();

    lsq_datetime_register_type();

    support_factory_list = NULL;

    lsq_opened_archive_list = NULL;

    data_squeeze = g_build_filename( user_dir, "squeeze", NULL );
    lsq_read_squeeze_dir( data_squeeze );
    g_free( data_squeeze );

    data_squeeze = g_build_filename( DATADIR, "squeeze", NULL );
    lsq_read_squeeze_dir( data_squeeze );
    g_free( data_squeeze );

    for ( ; NULL != *system_dirs; ++system_dirs )
    {
        data_squeeze = g_build_filename( *system_dirs, "squeeze", NULL );
        lsq_read_squeeze_dir( data_squeeze );
        g_free( data_squeeze );
    }
}

void
lsq_shutdown ( void )
{
    g_slist_foreach( lsq_opened_archive_list, (GFunc)lsq_close_archive, NULL );
    g_slist_foreach( support_factory_list, (GFunc)g_object_unref, NULL );
    g_slist_free( support_factory_list );
}

/*
 * XAArchive* lsq_new_archive(gchar *path, LSQArchiveType type, gboolean overwrite)
 *
 */
gint
lsq_new_archive ( GFile *file, gboolean overwrite, LSQArchive **lp_archive )
{
    LSQArchive *archive;

    g_return_val_if_fail( G_IS_FILE( file ), 1 );
    g_return_val_if_fail( NULL != lp_archive, 1 );

    if ( TRUE == overwrite )
    {
        g_file_trash( file, NULL, NULL );
    }

    if ( TRUE == g_file_query_exists( file, NULL ) )
    {
        (*lp_archive) = NULL;
        return 1;
    }

    archive = lsq_archive_new( file );
    (*lp_archive) = archive;
    if( NULL == archive )
    {
        return 1;
    }
    return 0;
}

/*
 *
 * XAArchive* lsq_open_archive(gchar *path)
 *
 */
gint
lsq_open_archive ( GFile *file, LSQArchive **lp_archive )
{
    LSQArchive *archive = NULL; /*lsq_opened_archive_get_archive(path); */

    g_return_val_if_fail( G_IS_FILE( file ), 1 );
    g_return_val_if_fail( NULL != lp_archive, 1 );

    if ( FALSE == g_file_query_exists( file, NULL ) )
    {
        (*lp_archive) = NULL;
        return 1;
    }

    if ( NULL == archive )
    {
        archive = lsq_archive_new( file );
        if ( NULL != archive )
        {
            /* FIXME: Shouldn't this be part of lsq_archive_new? */
            lsq_opened_archive_list = g_slist_prepend( lsq_opened_archive_list, archive );
        }
    }
    (*lp_archive) = archive;
    if( NULL == archive )
    {
        return 1;
    }
    return 0;
}

gboolean
lsq_is_supported ( const gchar *filename )
{
    return FALSE;
}

GSList *
lsq_get_supported_mime_types ( LSQCommandType type )
{
    return NULL;
}

gchar **
lsq_iter_list_to_strv ( GSList *list )
{
    GSList *iter;
    guint i;

    gchar **strv;

    if(!list)
        return NULL;

    strv = g_new( gchar *, g_slist_length( list ) + 1 );

    i = 0;

    for ( iter = list; NULL != iter; iter = g_slist_next( iter ) )
    {
        strv[i++] = lsq_archive_iter_get_path( iter->data );
    }

    strv[i] = NULL;

    return strv;
}

