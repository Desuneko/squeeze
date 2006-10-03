/* *  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
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
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "archive.h"
#include "archive-support.h"

#include "internals.h"

#define LXA_ENTRY_CHILD_BUFFER_SIZE 300


static void
lxa_archive_class_init(LXAArchiveClass *archive_class);

static void
lxa_archive_init(LXAArchive *archive);

static void
lxa_archive_finalize(GObject *object);

void
lxa_archive_free_entry(LXAEntry *entry, LXAArchive *archive);


gint
lxa_archive_sort_entry_buffer(LXAEntry *entry1, LXAEntry *entry2)
{
	return strcmp(entry1->filename, entry2->filename);
}

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
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
			NULL);
}

static void
lxa_archive_init(LXAArchive *archive)
{
		archive->root_entry.filename = g_strdup("/");
		archive->root_entry.children = NULL;
}

static void
lxa_archive_finalize(GObject *object)
{
	LXAArchive *archive = LXA_ARCHIVE(object);
	if(archive->path)
		g_free(archive->path);
	lxa_archive_free_entry(&archive->root_entry, archive);
	switch(archive->status)
	{
		case(LXA_ARCHIVESTATUS_IDLE):
		case(LXA_ARCHIVESTATUS_ERROR):
		case(LXA_ARCHIVESTATUS_USERBREAK):
			break;
		default:
			kill ( archive->child_pid , SIGHUP);
			break;
	}
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
	g_debug("Mime-type: %s", archive->mime);
	
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
	if(LXA_IS_ARCHIVE(archive))
	{
		if(archive->status != status)
		{
			archive->old_status = archive->status;
			archive->status = status;
			g_signal_emit(G_OBJECT(archive), lxa_archive_signals[0], 0, archive);
		} 
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
	gchar **path_items;
	LXAEntry *tmp_entry = NULL, *parent = NULL;
	path_items = g_strsplit_set(path, "/\n", -1);
	tmp_entry = lxa_entry_get_child(&archive->root_entry, path_items[0]);
	if(!tmp_entry)
	{
		tmp_entry= g_new0(LXAEntry, 1);
		tmp_entry->filename = g_strdup(path_items[0]);
		if(path[strlen(path)-1] == '/')
			tmp_entry->is_folder = TRUE;
		else
			tmp_entry->is_folder = FALSE;
		lxa_entry_add_child(&archive->root_entry, tmp_entry);
		lxa_entry_flush_buffer(&archive->root_entry);
	}
	for(i = 1; path_items[i]?strlen(path_items[i]):0;i++)
	{
		parent = tmp_entry;
		tmp_entry = lxa_entry_get_child(parent, path_items[i]);
		if(!tmp_entry)
		{
			tmp_entry = g_new0(LXAEntry, 1);
			tmp_entry->filename = g_strdup(path_items[i]);
			lxa_entry_add_child(parent, tmp_entry);
			if(path[strlen(path)-1] == '/')
			{
				tmp_entry->is_folder = TRUE;
				lxa_entry_flush_buffer(parent);
			}
			else
				tmp_entry->is_folder = FALSE;
		}
		parent = tmp_entry;
	}
	g_strfreev(path_items);
	return tmp_entry;
}


void
lxa_archive_free_entry(LXAEntry *entry, LXAArchive *archive)
{
	gint i = 0; 
	gpointer props_iter = entry->props;

	for(i = 0; i < entry->n_children; i++)
		lxa_archive_free_entry(entry->children[i], archive);

	if(entry->props)
	{
		switch(archive->column_types[i])
		{
			case(G_TYPE_STRING):
				g_free(*(gchar **)props_iter);
				props_iter += sizeof(gchar *);
				break;
			case(G_TYPE_UINT):
				props_iter += sizeof(guint);
				break;
			case(G_TYPE_UINT64):
				props_iter += sizeof(guint64);
				break;
		}
		g_free(entry->props);
	}
	g_free(entry->filename);
}

gint
lxa_stop_archive_child( LXAArchive *archive )
{
	lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_USERBREAK);
	return 0;
}

gboolean
lxa_entry_add_child(LXAEntry *parent, LXAEntry *child)
{
	guint max_children = 0;
	guint begin = 0;
	guint pos = 0;
	gint cmp = 0;
	guint old_i = 0;
	guint new_i = 0;
	guint size = parent->n_children;
	guint org_size = parent->n_children;
	GSList *buffer_iter = NULL;
	LXAEntry **children_old = (LXAEntry **)parent->children;
	
	parent->buffer = g_slist_insert_sorted(parent->buffer, (gpointer)child, (GCompareFunc)lxa_archive_sort_entry_buffer);
	if(g_slist_length(parent->buffer) == LXA_ENTRY_CHILD_BUFFER_SIZE)
		lxa_entry_flush_buffer(parent);
}

LXAEntry *
lxa_entry_get_child(LXAEntry *entry, const gchar *filename)
{
	guint size = entry->n_children;
	guint pos = 0;
	gint cmp = 0;
	while(size)
	{
		pos = (size / 2);

		cmp = strcmp(filename, entry->children[pos]->filename);
		if(!cmp)
			return entry->children[pos];

		if(cmp < 0)
		{
			size = pos;
		}
		else
		{
			size = size - ++pos;
		}
	}
	return NULL;
}

void
lxa_entry_flush_buffer(LXAEntry *entry)
{
	g_debug("Flush");
	guint max_children = 0;
	guint begin = 0;
	guint pos = 0;
	gint cmp = 1;
	guint old_i = 0;
	guint new_i = 0;
	guint size = entry->n_children;
	GSList *buffer_iter = NULL;
	LXAEntry **children_old = (LXAEntry **)entry->children;

	max_children = (entry->n_children + g_slist_length(entry->buffer));
	
	entry->children = g_new(LXAEntry *, max_children);
	for(buffer_iter = entry->buffer;buffer_iter;buffer_iter = buffer_iter->next)
	{
		size = entry->n_children - begin;
		while(size)
		{
			pos = (size / 2)+begin;

			cmp = strcmp(((LXAEntry *)buffer_iter->data)->filename, children_old[pos]->filename);
			if(!cmp)
				break;

			if(cmp < 0)
			{
				size = pos - begin;
			}
			else
			{
				size = size + begin - ++pos;
				begin = pos;
			}
		}
		if(!cmp)
		{
			/* TODO: F*** (aka merge) */
		}
		else
		{
			while(old_i < pos)
			{
				entry->children[new_i++] = children_old[old_i++];
			}
			entry->children[new_i++] = buffer_iter->data;
		}
	}
	while(old_i < entry->n_children)
	{
		entry->children[new_i++] = children_old[old_i++];
	}
	entry->n_children = new_i;
	g_debug("%d", entry->n_children);

	g_slist_free(entry->buffer);
	entry->buffer = NULL;

	g_free(children_old);
}
