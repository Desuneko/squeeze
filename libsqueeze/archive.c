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
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "libsqueeze.h"
#include "libsqueeze-view.h"
#include "support-template.h"
#include "support-factory.h"

#include "btree.h"
#include "archive-tempfs.h"
#include "command-queue.h"

#include "internals.h"

static void
lsq_archive_finalize ( GObject *object );


enum
{
    LSQ_ARCHIVE_SIGNAL_STATE_CHANGED = 0,
    LSQ_ARCHIVE_SIGNAL_COMMAND_TERMINATED,
    LSQ_ARCHIVE_SIGNAL_REFRESHED,
    LSQ_ARCHIVE_SIGNAL_COUNT
};

static gint lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_COUNT];

G_DEFINE_TYPE ( LSQArchive, lsq_archive, G_TYPE_OBJECT );

static void
lsq_archive_class_init ( LSQArchiveClass *archive_class )
{
    GObjectClass *object_class = G_OBJECT_CLASS( archive_class );

    object_class->finalize = lsq_archive_finalize;
    lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_STATE_CHANGED] = g_signal_new(
            "state-changed",
            G_TYPE_FROM_CLASS( archive_class ),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE,
            0,
            NULL
        );
    lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_REFRESHED] = g_signal_new(
            "refreshed",
            G_TYPE_FROM_CLASS( archive_class ),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE,
            0,
            NULL
        );
}

static void
lsq_archive_init ( LSQArchive *archive )
{
    lsq_archive_init_iter( archive );
#ifdef LSQ_THREADSAFE
    g_static_rw_lock_init( &archive->rw_lock );
#endif /* LSQ_THREADSAFE */
    archive->priv = g_new0( LSQArchivePrivate, 1 );
}

/** static void
 * lsq_archive_finalize(GObject *object)
 *
 * 
 */
static void
lsq_archive_finalize ( GObject *object )
{
    LSQArchive *archive = LSQ_ARCHIVE( object );

    lsq_archive_free_iter( archive );
    lsq_tempfs_clean_root_dir( archive );
    lsq_opened_archive_list = g_slist_remove( lsq_opened_archive_list, object );

    G_OBJECT_CLASS(lsq_archive_parent_class)->finalize( object );
}

/**
 * lsq_archive_new:
 *
 * @path: path to archive
 *
 * Return value: LSQArchive object
 *
 */
LSQArchive *
lsq_archive_new ( GFile *file )
{
    LSQArchive *archive;
    GFileInfo *file_info;
    const gchar *content_type;
    GSList *iter;
    gchar *_basename;

    /* We don't support no file. We can't get a content type of a NULL pointer */
    g_return_val_if_fail( G_IS_FILE( file ), NULL );

    archive = g_object_new( lsq_archive_get_type(), NULL );

    archive->priv->file = file;
    g_object_ref( archive->priv->file );

    file_info = g_file_query_info( file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL );
    if ( NULL != file_info )
    {
        content_type = g_file_info_get_attribute_string( file_info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE );
        archive->priv->content_type = g_strdup( content_type ); 
        g_object_unref( file_info );
    }
    else
    {
        /* The file might not exist yet. Get the content type from the file name */
        _basename = g_file_get_basename( file );
        if ( NULL != _basename )
        {
            archive->priv->content_type = g_content_type_guess( _basename, NULL, 0, NULL );
        }
        g_free( _basename );
    }
#ifdef DEBUG
    g_debug( "mime: %s\n", archive->priv->content_type );
#endif
    if ( NULL == archive->priv->content_type )
    {
        /* Setting the content_type later on is not supported */
#ifdef DEBUG
        g_debug( "not supported" );
#endif
        g_object_unref( archive );
        return NULL;
    }

    for ( iter = lsq_mime_support_list; NULL != iter; iter = iter->next )
    {
        if ( 0 == strcmp( ((LSQSupportTemplate *)iter->data)->content_type, archive->priv->content_type ) )
        {
#ifdef DEBUG
            g_debug( "found template" );
#endif
            archive->priv->s_template = iter->data;
            break;
        }
    }

    if ( NULL == archive->priv->s_template )
    {
#ifdef DEBUG
        g_debug( "not supported" );
#endif
        g_object_unref( archive );
        archive = NULL;
    }

    return archive;
}

/*
 * lsq_archive_n_entry_properties:
 *
 * @archive: LSQArchive object
 *
 */
guint
lsq_archive_n_entry_properties ( const LSQArchive *archive )
{
#ifdef DEBUG
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), 0 );
#endif
    return lsq_support_template_get_n_properties( archive->priv->s_template ) + LSQ_ARCHIVE_PROP_USER;
}

/*
 * lsq_archive_get_entry_property_type:
 *
 * @archive: LSQArchive object
 *
 */
GType
lsq_archive_get_entry_property_type ( const LSQArchive *archive, guint n )
{
#ifdef DEBUG
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), G_TYPE_NONE );
    g_return_val_if_fail( lsq_archive_n_entry_properties( archive ) > n , G_TYPE_NONE );
#endif
    switch ( n )
    {
        case LSQ_ARCHIVE_PROP_FILENAME:
        case LSQ_ARCHIVE_PROP_MIME_TYPE:
            return G_TYPE_STRING;

        default:
            return lsq_support_template_get_property_type( archive->priv->s_template, n - LSQ_ARCHIVE_PROP_USER );
    }
}

guint
lsq_archive_get_entry_property_offset ( const LSQArchive *archive, guint n )
{
#ifdef DEBUG
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), 0 );
    g_return_val_if_fail( lsq_archive_n_entry_properties( archive ) > n , 0 );
#endif
    switch ( n )
    {
        case LSQ_ARCHIVE_PROP_FILENAME:
        case LSQ_ARCHIVE_PROP_MIME_TYPE:
            g_return_val_if_reached( 0 );

        default:
            return lsq_support_template_get_property_offset( archive->priv->s_template, n - LSQ_ARCHIVE_PROP_USER );
    }
}

/*
 * lsq_archive_get_entry_property_name:
 *
 * @archive: LSQArchive object
 *
 */
const gchar *
lsq_archive_get_entry_property_name ( const LSQArchive *archive, guint n )
{
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), NULL );
    g_return_val_if_fail( lsq_archive_n_entry_properties( archive ) > n , NULL );

    switch( n )
    {
        case LSQ_ARCHIVE_PROP_FILENAME:
            return _("Name");

        case LSQ_ARCHIVE_PROP_MIME_TYPE:
            return _("Mime type");

        default:
            return lsq_support_template_get_property_name( archive->priv->s_template, n - LSQ_ARCHIVE_PROP_USER );
    }
}

guint
lsq_archive_entry_properties_size ( const LSQArchive *archive )
{
#ifdef DEBUG
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), 0 );
#endif
    return lsq_support_template_get_properties_size( archive->priv->s_template );
}

/*
 * lsq_archive_get_filename:
 * @archive: LSQArchive object
 *
 * Return value: filename string
 */
gchar *
lsq_archive_get_filename ( const LSQArchive *archive )
{
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), NULL );

    return g_file_get_basename( archive->priv->file );
}

/*
 * lsq_archive_get_path:
 * @archive: LSQArchive object
 *
 * Return value: filename string
 */
gchar *
lsq_archive_get_path ( const LSQArchive *archive )
{
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), NULL );

    return g_file_get_path( archive->priv->file );
}

GFile *
lsq_archive_get_file ( const LSQArchive *archive )
{
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), NULL );

    return archive->priv->file;
}

void
lsq_archive_refreshed ( const LSQArchive *archive )
{
    g_return_if_fail( LSQ_IS_ARCHIVE( archive ) );

    g_signal_emit( G_OBJECT( archive ), lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_REFRESHED], 0, NULL );
}

void lsq_archive_add_children ( GSList *files )
{
    GSList *iter;
    for ( iter = files; NULL != iter; iter = iter->next )
    {
        unsigned int i, size = lsq_archive_iter_n_children( iter->data );
        for ( i = 0; i < size; ++i )
        {
            files = g_slist_append( iter, lsq_archive_iter_nth_child( iter->data, i ) );
        }
    }
}

void
lsq_archive_state_changed ( const LSQArchive *archive )
{
    g_return_if_fail( LSQ_IS_ARCHIVE( archive ) );

    g_signal_emit( G_OBJECT( archive ), lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_STATE_CHANGED], 0, NULL );
}

void
lsq_close_archive ( LSQArchive *archive )
{
    g_return_if_fail( LSQ_IS_ARCHIVE( archive ) );

    lsq_opened_archive_list = g_slist_remove( lsq_opened_archive_list, archive );

    if ( NULL != archive->priv->file )
    {
        g_object_unref( archive->priv->file );
        archive->priv->file = NULL;
    }
    if ( NULL != archive->priv->content_type )
    {
        g_free( archive->priv->content_type );
        archive->priv->content_type = NULL;
    }

    lsq_archive_stop( archive );
    g_object_unref( archive );
}

gboolean
lsq_archive_can_stop ( const LSQArchive *archive )
{
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), FALSE );

    return FALSE;
}

gboolean
lsq_archive_stop ( const LSQArchive *archive )
{
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), FALSE );

    return FALSE;
}

const gchar *
lsq_archive_get_state_msg ( const LSQArchive *archive )
{
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), NULL );

    return archive->priv->state_msg;
}

LSQArchiveState
lsq_archive_get_state ( const LSQArchive *archive )
{
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), LSQ_ARCHIVE_STATE_IDLE );

    return archive->priv->state;
}

LSQSupportType
lsq_archive_get_support_mask ( const LSQArchive *archive )
{
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), 0 );

    return archive->priv->s_template->support_mask;
}

/**
 * lsq_archive_operate:
 * @archive: the archive
 * @type: The command-type to be executed
 *
 * Return value: TRUE on success
 */
gboolean
lsq_archive_operate ( LSQArchive *archive, LSQCommandType type, gchar **files, const gchar *directory )
{
    LSQSupportTemplate *s_template;
    LSQExecuteContext *leaked = NULL; /* FIXME */

    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), FALSE );

    s_template = archive->priv->s_template;

    switch ( type )
    {
        case LSQ_COMMAND_TYPE_ADD:
            g_return_val_if_fail( files, FALSE );
            leaked = lsq_command_queue_execute( s_template->add_cmd_queue, archive, files, NULL, NULL );
            break;

        case LSQ_COMMAND_TYPE_REMOVE:
            g_return_val_if_fail( files, FALSE );
            leaked = lsq_command_queue_execute( s_template->remove_cmd_queue, archive, files, NULL, NULL );
            break;

        case LSQ_COMMAND_TYPE_EXTRACT:
            g_return_val_if_fail( directory, FALSE );
            leaked = lsq_command_queue_execute( s_template->extract_cmd_queue, archive, files, directory, NULL );
            break;

        case LSQ_COMMAND_TYPE_REFRESH:
            leaked = lsq_command_queue_execute( s_template->refresh_cmd_queue, archive, NULL, NULL, s_template->parser );
            break;

        default:
            g_return_val_if_reached( FALSE );
    }

    (void)leaked;

    return TRUE;
}

LSQCommandOptionPair **
lsq_archive_get_command_options ( LSQArchive *archive, LSQCommandType type )
{
    LSQCommandOptionPair **option_list = NULL;
    LSQSupportTemplate *s_template;

    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), NULL );

    s_template = archive->priv->s_template;

    switch ( type )
    {
        case LSQ_COMMAND_TYPE_ADD:
            option_list = lsq_command_option_create_pair( s_template->add_options );
            break;

        case LSQ_COMMAND_TYPE_REMOVE:
            option_list = lsq_command_option_create_pair( s_template->remove_options );
            break;

        case LSQ_COMMAND_TYPE_EXTRACT:
            option_list = lsq_command_option_create_pair( s_template->extract_options );
            break;

        default:
            break;
    }

    return option_list;
}

