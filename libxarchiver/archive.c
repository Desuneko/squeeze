/*
 *  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
 *
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

#define EXO_API_SUBJECT_TO_CHANGE

#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h> 
#include <thunar-vfs/thunar-vfs.h>
#include "archive.h"
#include "archive-support.h"

#include "internals.h"


static void
lxa_archive_class_init(LXAArchiveClass *archive_class);

static void
lxa_archive_init(LXAArchive *archive);

static void
lxa_archive_finalize(GObject *object);

static gint lxa_archive_signals[1];


GType
lxa_archive_get_type ()
{
	static GType lxa_archive_type = 0;

 	if (!lxa_archive_type)
	{
 		static const GTypeInfo lxa_archive_info = 
		{
			sizeof (LXAArchiveClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_archive_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXAArchive),
			0,
			(GInstanceInitFunc) lxa_archive_init,
			NULL
		};

		lxa_archive_type = g_type_register_static (G_TYPE_OBJECT, "LXAArchive", &lxa_archive_info, 0);
	}
	return lxa_archive_type;
}

static void
lxa_archive_class_init(LXAArchiveClass *archive_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(archive_class);

	object_class->finalize = lxa_archive_finalize;
	
	lxa_archive_signals[0] = g_signal_new("lxa_status_changed",
			G_TYPE_FROM_CLASS(archive_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);
}

static void
lxa_archive_init(LXAArchive *archive)
{
}

static void
lxa_archive_finalize(GObject *object)
{
	LXAArchive *archive = LXA_ARCHIVE(object);
	if(archive->path)
		g_free(archive->path);
}

LXAArchive *
lxa_archive_new(gchar *path, gchar *mime)
{
	LXAArchive *archive;
	ThunarVfsMimeInfo *mime_info;

	archive = g_object_new(lxa_archive_get_type(), NULL);
	if(path)
		archive->path = g_strdup(path);
	else
		archive->path = NULL;

	if(!mime)
		mime_info = thunar_vfs_mime_database_get_info_for_file(lxa_mime_database, archive->path, g_path_get_basename(archive->path));
	else
		mime_info = thunar_vfs_mime_info_new(mime, -1);
	
	archive->mime = g_strdup(thunar_vfs_mime_info_get_name(mime_info));
	
	if(!lxa_get_support_for_mime(archive->mime))
	{
		g_object_unref(archive);
		archive = NULL;
	}

	return archive;
}

void 
lxa_archive_set_status(LXAArchive *archive, LXAArchiveStatus status)
{
	if(archive->status != status)
	{
		archive->old_status = archive->status;
		archive->status = status;
		g_signal_emit(G_OBJECT(archive), lxa_archive_signals[0], 0, archive);
	}
}

gint
lxa_archive_lookup_dir(gpointer entry, gconstpointer filename)
{
	return strcmp(((LXAEntry *)entry)->filename, filename);
}

/* 
 * LXAEntry *
 * lxa_archive_add_file(LXAArchive *archive, gchar *path);
 *
 * Add a file to the archive-tree or return
 */
LXAEntry *
lxa_archive_add_file(LXAArchive *archive, gchar *path)
{
	gint i = 0;
	GSList *tmp_list;
	GSList *tmp_list_children;
	gchar **path_items;
	LXAEntry *tmp_entry;
	path_items = g_strsplit_set(path, "/\n", -1);
	tmp_list = g_slist_find_custom(archive->root_entries, path_items[0], (GCompareFunc)lxa_archive_lookup_dir);
	if(!tmp_list)
	{
		tmp_entry = g_new0(LXAEntry, 1);
		tmp_entry->filename = g_strdup(path_items[0]);
		archive->root_entries = g_slist_prepend(archive->root_entries, tmp_entry);
		tmp_list = archive->root_entries;
	}
	for(i = 1; path_items[i]?strlen(path_items[i]):0;i++)
	{
		tmp_list_children = g_slist_find_custom(((LXAEntry *)tmp_list->data)->children, path_items[i], (GCompareFunc)lxa_archive_lookup_dir);
		if(!tmp_list_children)
		{
			tmp_entry = g_new0(LXAEntry, 1);
			tmp_entry->filename = g_strdup(path_items[i]);
			((LXAEntry *)tmp_list->data)->children = g_slist_prepend(((LXAEntry *)tmp_list->data)->children, tmp_entry);
			tmp_list_children = ((LXAEntry *)tmp_list->data)->children;
		}
		tmp_list = tmp_list_children;
	}
	g_strfreev(path_items);
	return tmp_entry;
}

GSList *
lxa_archive_get_children(LXAArchive *archive, gchar *path)
{
	gint i;
	GSList *tmp_list = archive->root_entries;
	gchar **path_items = g_strsplit_set(path, "/\n", -1);
	for(i = 0; path_items[i]?strlen(path_items[i]):0; i++)
	{
		tmp_list = g_slist_find_custom(tmp_list, path_items[i], (GCompareFunc)lxa_archive_lookup_dir);
		if(!tmp_list)
			break;
		tmp_list = ((LXAEntry *)tmp_list->data)->children;
	}
	g_strfreev(path_items);
	return tmp_list;
}
