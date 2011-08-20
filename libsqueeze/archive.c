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

#include "slist.h"
#include "archive-tempfs.h"
#include "command-queue.h"

#include "internals.h"

#ifndef LSQ_ENTRY_CHILD_BUFFER_SIZE
#define LSQ_ENTRY_CHILD_BUFFER_SIZE 500
#endif

#ifndef LSQ_MIME_DIRECTORY
#define LSQ_MIME_DIRECTORY "inode/directory"
#endif

static void
lsq_archive_class_init(LSQArchiveClass *archive_class);

static void
lsq_archive_init(LSQArchive *archive);

static void
lsq_archive_finalize(GObject *object);


enum
{
	LSQ_ARCHIVE_SIGNAL_STATE_CHANGED = 0,
	LSQ_ARCHIVE_SIGNAL_COMMAND_TERMINATED,
	LSQ_ARCHIVE_SIGNAL_REFRESHED,
	LSQ_ARCHIVE_SIGNAL_COUNT
};

static gint lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_COUNT];

GType
lsq_archive_get_type ()
{
	static GType lsq_archive_type = 0;

 	if (!lsq_archive_type)
	{
 		static const GTypeInfo lsq_archive_info = 
		{
			sizeof (LSQArchiveClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_archive_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQArchive),
			0,
			(GInstanceInitFunc) lsq_archive_init,
			NULL
		};

		lsq_archive_type = g_type_register_static (G_TYPE_OBJECT, "LSQArchive", &lsq_archive_info, 0);
	}
	return lsq_archive_type;
}

static void
lsq_archive_class_init(LSQArchiveClass *archive_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(archive_class);

	object_class->finalize = lsq_archive_finalize;
	lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_STATE_CHANGED] = g_signal_new("state-changed",
			G_TYPE_FROM_CLASS(archive_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
			NULL);
	lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_REFRESHED] = g_signal_new("refreshed",
			G_TYPE_FROM_CLASS(archive_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
			NULL);
}

static void
lsq_archive_init(LSQArchive *archive)
{
	lsq_archive_init_iter(archive);
#ifdef LSQ_THREADSAFE
	g_static_rw_lock_init(&archive->rw_lock);
#endif /* LSQ_THREADSAFE */
	archive->priv = g_new0(LSQArchivePrivate, 1);
}

/** static void
 * lsq_archive_finalize(GObject *object)
 *
 * 
 */
static void
lsq_archive_finalize(GObject *object)
{
	g_return_if_fail(LSQ_IS_ARCHIVE(object));
	LSQArchive *archive = (LSQArchive *)(object);

	lsq_archive_free_iter(archive);
	lsq_tempfs_clean_root_dir(archive);
	lsq_opened_archive_list = g_slist_remove(lsq_opened_archive_list, object);
}

/**
 * lsq_archive_new:
 *
 * @path: path to archive
 * @mime: mime-type (or NULL)
 *
 * Return value: LSQArchive object
 *
 */
LSQArchive *
lsq_archive_new (GFile *file)
{
	LSQArchive *archive;
    GFileInfo *file_info;
    const gchar *content_type;

	archive = g_object_new(lsq_archive_get_type(), NULL);

	if(file)
	{
        archive->priv->file = file;
        g_object_ref (archive->priv->file);
	}
	else
    {
		archive->priv->file= NULL;
    }


    file_info = g_file_query_info (file, "standard::content-type,standard::type", 0, NULL, NULL);
	if(file_info)
	{
        content_type = g_file_info_get_attribute_string (file_info, "standard::content-type");
        archive->priv->content_type = g_strdup (content_type); 
	}
#ifdef DEBUG
	g_debug("mime: %s\n", thunar_vfs_mime_info_get_name(archive->priv->mime_info));
#endif

  GSList *iter;
  for(iter = lsq_mime_support_list; iter; iter = iter->next)
  {
    if(0 == strcmp(((LSQSupportTemplate*)iter->data)->content_type, archive->priv->content_type))
    {
#ifdef DEBUG
      g_debug("found template");
#endif
      archive->priv->s_template = iter->data;
      break;
    }
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
lsq_archive_n_entry_properties(const LSQArchive *archive)
{
#ifdef DEBUG
	g_return_val_if_fail(archive, 0);
#endif
	return lsq_support_template_get_n_properties(archive->priv->s_template) + LSQ_ARCHIVE_PROP_USER;
}

/*
 * lsq_archive_get_entry_property_type:
 *
 * @archive: LSQArchive object
 *
 */
GType
lsq_archive_get_entry_property_type(const LSQArchive *archive, guint n)
{
	switch(n)
	{
		case LSQ_ARCHIVE_PROP_FILENAME:
		case LSQ_ARCHIVE_PROP_MIME_TYPE:
			return G_TYPE_STRING;
			break;
		default:
			return lsq_support_template_get_property_type(archive->priv->s_template, n - LSQ_ARCHIVE_PROP_USER);
			break;
	}
}

/*
 * lsq_archive_get_entry_property_name:
 *
 * @archive: LSQArchive object
 *
 */
const gchar *
lsq_archive_get_entry_property_name(const LSQArchive *archive, guint n)
{
	switch(n)
	{
		case LSQ_ARCHIVE_PROP_FILENAME:
			return _("Name");
		case LSQ_ARCHIVE_PROP_MIME_TYPE:
			return _("Mime type");
			break;
		default:
			return lsq_support_template_get_property_name(archive->priv->s_template, n - LSQ_ARCHIVE_PROP_USER);
			break;
	}
}

/*
 * lsq_archive_get_filename:
 * @archive: LSQArchive object
 *
 * Return value: filename string
 */
GFile *
lsq_archive_get_file (LSQArchive *archive)
{
	return archive->priv->file;
}

void
lsq_archive_refreshed(const LSQArchive *archive)
{
    g_debug("%s", __FUNCTION__);
	g_signal_emit(G_OBJECT(archive), lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_REFRESHED], 0, NULL);
}

void lsq_archive_add_children(GSList *files)
{
	GSList *iter;
	for(iter = files; iter; iter = iter->next)
	{
		unsigned int i, size = lsq_archive_iter_n_children(iter->data);
		for(i = 0; i < size; ++i)
		{
			files = g_slist_append(iter, lsq_archive_iter_nth_child(iter->data, i));
		}
	}
}

void
lsq_archive_state_changed(const LSQArchive *archive)
{
	g_signal_emit(G_OBJECT(archive), lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_STATE_CHANGED], 0, NULL);
}

void
lsq_close_archive(LSQArchive *archive)
{
	lsq_opened_archive_list = g_slist_remove(lsq_opened_archive_list, archive);

	if(archive->priv->file)
    {
		g_object_unref (archive->priv->file);
        archive->priv->file = NULL;
    }
	if(archive->priv->content_type)
    {
		g_free (archive->priv->content_type);
        archive->priv->content_type = NULL;
    }

	lsq_archive_stop(archive);
	g_object_unref(archive);
}

gboolean
lsq_archive_can_stop(const LSQArchive *archive)
{
	return FALSE;
}

gboolean
lsq_archive_stop(const LSQArchive *archive)
{
	return FALSE;
}

const gchar *
lsq_archive_get_state_msg(const LSQArchive *archive)
{
	return archive->priv->state_msg;
}

LSQArchiveState
lsq_archive_get_state(const LSQArchive *archive)
{
    return archive->priv->state;
}

LSQSupportType
lsq_archive_get_support_mask(const LSQArchive *archive)
{
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
lsq_archive_operate(LSQArchive *archive, LSQCommandType type, const gchar **files, const gchar *directory)
{
    g_return_val_if_fail(archive, FALSE);

    LSQSupportTemplate *s_template = archive->priv->s_template;

    switch (type)
    {
        case LSQ_COMMAND_TYPE_ADD:
            lsq_command_queue_execute(s_template->add_cmd_queue, archive, files, NULL, NULL);
            break;
        case LSQ_COMMAND_TYPE_REMOVE:
            lsq_command_queue_execute(s_template->remove_cmd_queue, archive, files, NULL, NULL);
            break;
        case LSQ_COMMAND_TYPE_EXTRACT:
            lsq_command_queue_execute(s_template->extract_cmd_queue, archive, files, directory, NULL);
            break;
        case LSQ_COMMAND_TYPE_REFRESH:
            lsq_command_queue_execute(s_template->refresh_cmd_queue, archive, files, NULL, s_template->parser);
            break;
        default:
            return FALSE;
            break;
    }
    return TRUE;
}

LSQCommandOptionPair **
lsq_archive_get_command_options(LSQArchive *archive, LSQCommandType type)
{
    LSQCommandOptionPair **option_list = NULL;

    LSQSupportTemplate *s_template = archive->priv->s_template;

    switch (type)
    {
        case LSQ_COMMAND_TYPE_ADD:
            option_list = lsq_command_option_create_pair(s_template->add_options);
            break;
        case LSQ_COMMAND_TYPE_REMOVE:
            option_list = lsq_command_option_create_pair(s_template->remove_options);
            break;
        case LSQ_COMMAND_TYPE_EXTRACT:
            option_list = lsq_command_option_create_pair(s_template->extract_options);
            break;
        default:
            break;
    }

    return option_list;
}

