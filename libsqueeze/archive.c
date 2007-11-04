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
#include <thunar-vfs/thunar-vfs.h>

#include "libsqueeze.h"
#include "libsqueeze-view.h"
#include "support-template.h"
#include "support-factory.h"

#include "slist.h"
#include "archive-tempfs.h"


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

LSQArchive *
lsq_archive_new(gchar *path, const gchar *mime)
{
	LSQArchive *archive;
	gchar *base = NULL;

	archive = g_object_new(lsq_archive_get_type(), NULL);

	if(path)
	{
		if(g_path_is_absolute(path))
			archive->priv->path_info = thunar_vfs_path_new(path, NULL);
		else
			archive->priv->path_info = thunar_vfs_path_relative(lsq_relative_base_path, path);
	}
	else
		archive->priv->path_info = NULL;


	archive->priv->file_info = thunar_vfs_info_new_for_path(archive->priv->path_info, NULL);
	if(archive->priv->file_info)
	{
		archive->priv->mime_info = archive->priv->file_info->mime_info;
		thunar_vfs_mime_info_ref(archive->priv->mime_info);
	}
	else
	{
		if(mime)
			archive->priv->mime_info = thunar_vfs_mime_database_get_info(lsq_mime_database, mime);
		else
		{
			base = g_path_get_basename(path);
			archive->priv->mime_info = thunar_vfs_mime_database_get_info_for_file(lsq_mime_database, path, base);
			g_free(base);
		}
	}
#ifdef DEBUG
	g_debug("%s\n", thunar_vfs_mime_info_get_name(archive->priv->mime_info));
#endif

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
const gchar *
lsq_archive_get_filename(const LSQArchive *archive)
{
	return thunar_vfs_path_get_name(archive->priv->path_info);
}

/*
 * lsq_archive_get_path:
 * @archive: LSQArchive object
 *
 * Return value: newly allocated path string
 */
gchar *
lsq_archive_get_path(const LSQArchive *archive)
{
	return thunar_vfs_path_dup_string(archive->priv->path_info);
}

/*
 * lsq_archive_get_mimetype:
 *
 * @archive: LSQArchive object
 */
const gchar *
lsq_archive_get_mimetype(const LSQArchive *archive)
{
	return thunar_vfs_mime_info_get_name(archive->priv->mime_info);
}

/*
 * lsq_archive_exists:
 *
 * @archive: LSQArchive object
 */
gboolean
lsq_archive_exists(const LSQArchive *archive)
{
	if(!archive->priv->file_info)
		archive->priv->file_info = thunar_vfs_info_new_for_path(archive->priv->path_info, NULL);

	if(archive->priv->file_info)
		return TRUE;

	return FALSE;
}

void
lsq_archive_refreshed(const LSQArchive *archive)
{
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

	if(archive->priv->path_info)
		thunar_vfs_path_unref(archive->priv->path_info);
	if(archive->priv->file_info)
		thunar_vfs_info_unref(archive->priv->file_info);
	if(archive->priv->mime_info)
		thunar_vfs_mime_info_unref(archive->priv->mime_info);

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
lsq_archive_get_status(const LSQArchive *archive)
{
	return NULL;
}

/**
 * lsq_archive_get_path_info:
 * @archive: the archive
 *
 * Return value: the ThunarVfsPath information of the archive.
 */
ThunarVfsPath *
lsq_archive_get_path_info(LSQArchive *archive)
{
	return archive->priv->path_info;
}

LSQSupportType
lsq_archive_get_support_mask(const LSQArchive *archive)
{
	return archive->priv->s_template->support_mask;
}

gboolean
lsq_archive_operate(LSQArchive *archive, LSQCommandType type)
{
	return FALSE;
}

void
lsq_archive_set_property_type(LSQArchive *archive, guint n, GType type)
{
  if (n >= LSQ_ARCHIVE_PROP_USER)
  {
        lsq_support_template_set_property_type(archive->priv->s_template, n, type);
  }
}
