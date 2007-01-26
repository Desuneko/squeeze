/*  Copyright (c) 2006 Stephan Arts <stephan@xfce.org>
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

#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h> 
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <thunar-vfs/thunar-vfs.h>

#include "archive.h"
#include "archive-support.h"
#include "slist.h"
#include "archive-tempfs.h"

#include "internals.h"

#ifndef LSQ_ENTRY_CHILD_BUFFER_SIZE
#define LSQ_ENTRY_CHILD_BUFFER_SIZE 500
#endif

#ifndef LSQ_MIME_DIRECTORY
#define LSQ_MIME_DIRECTORY "inode/directory"
#endif

struct _LSQEntry
{
	gchar *filename;
	ThunarVfsMimeInfo *mime_info;
	gpointer props;
	LSQEntry **children;
	LSQSList *buffer;
};


static void
lsq_archive_class_init(LSQArchiveClass *archive_class);

static void
lsq_archive_init(LSQArchive *archive);

static void
lsq_archive_finalize(GObject *object);

static GType *
lsq_archive_get_entry_property_types(LSQArchive *archive, guint size);

static gchar **
lsq_archive_get_entry_property_names(LSQArchive *archive, guint size);

static LSQEntry *
lsq_entry_new(LSQArchive *, const gchar *);

static void
lsq_archive_entry_free(const LSQArchive *, LSQEntry *);

static LSQEntry *
lsq_entry_get_child(const LSQEntry *entry, const gchar *filename);

static gboolean
lsq_archive_entry_del_child(const LSQArchive*, LSQEntry *entry, const gchar *filename);

static void
lsq_archive_entry_add_child(LSQArchive *, LSQEntry *, LSQEntry *);

static void
lsq_archive_entry_flush_buffer(LSQArchive *, LSQEntry *);

static gpointer
lsq_archive_entry_get_props(LSQArchive *, LSQEntry *);

static const gchar *
lsq_archive_iter_get_prop_str(const LSQArchive *, const LSQArchiveIter *, guint);
static guint 
lsq_archive_iter_get_prop_uint(const LSQArchive *, const LSQArchiveIter *, guint);
static guint64 
lsq_archive_iter_get_prop_uint64(const LSQArchive *, const LSQArchiveIter *, guint);

static const gchar *
lsq_archive_iter_get_filename(const LSQArchive *, const LSQArchiveIter *);
static const gchar *
lsq_archive_iter_get_mimetype(const LSQArchive *, const LSQArchiveIter *);

static gchar *
lsq_archive_get_iter_part(const LSQArchive *archive, const gchar *path);

static gint
lsq_entry_filename_compare(LSQEntry *, LSQEntry *);

gint
lsq_archive_sort_entry_buffer(LSQEntry *entry1, LSQEntry *entry2)
{
	return strcmp(entry1->filename, entry2->filename);
}

enum
{
	LSQ_ARCHIVE_SIGNAL_STATUS_CHANGED = 0,
	LSQ_ARCHIVE_SIGNAL_REFRESHED,
	LSQ_ARCHIVE_SIGNAL_PATH_CHANGED,
	LSQ_ARCHIVE_SIGNAL_VIEW_PREPARED,
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
	
	lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_STATUS_CHANGED] = g_signal_new("lsq_status_changed",
			G_TYPE_FROM_CLASS(archive_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
			NULL);

	lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_REFRESHED] = g_signal_new("lsq_refreshed",
			G_TYPE_FROM_CLASS(archive_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
			NULL);

	lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_PATH_CHANGED] = g_signal_new("lsq_path_changed",
			G_TYPE_FROM_CLASS(archive_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING,
			NULL);

	lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_VIEW_PREPARED] = g_signal_new("lsq_view_prepared",
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
lsq_archive_init(LSQArchive *archive)
{
	archive->root_entry = g_new0(LSQEntry, 1);
	archive->status = LSQ_ARCHIVESTATUS_INIT;
	archive->old_status = LSQ_ARCHIVESTATUS_INIT;
	archive->files = NULL;
#ifdef LSQ_THREADSAFE
	g_static_rw_lock_init(&archive->rw_lock);
#endif /* LSQ_THREADSAFE */
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
	if(archive->path)
		g_free(archive->path);
	if(archive->path_info)
		thunar_vfs_path_unref(archive->path_info);
	if(archive->file_info)
		thunar_vfs_info_unref(archive->file_info);
	if(archive->mime_info)
		thunar_vfs_mime_info_unref(archive->mime_info);

	lsq_archive_entry_free(archive, archive->root_entry);
	switch(archive->status)
	{
		case(LSQ_ARCHIVESTATUS_IDLE):
		case(LSQ_ARCHIVESTATUS_ERROR):
		case(LSQ_ARCHIVESTATUS_USERBREAK):
			break;
		default:
			if(archive->child_pid)
				kill ( archive->child_pid , SIGHUP);
			break;
	}
	lsq_tempfs_clean_root_dir(archive);
	lsq_opened_archive_list = g_slist_remove(lsq_opened_archive_list, object);
}

LSQArchive *
lsq_archive_new(gchar *path, const gchar *mime)
{
	LSQArchive *archive;
	gchar *base = NULL;

	archive = g_object_new(lsq_archive_get_type(), NULL);

	g_signal_connect(G_OBJECT(archive), "lsq_view_prepared", (GCallback)lsq_archive_support_view_prepared, NULL);

	if(path)
	{
		if(g_path_is_absolute(path))
			archive->path_info = thunar_vfs_path_new(path, NULL);
		else
			archive->path_info = thunar_vfs_path_relative(lsq_relative_base_path, path);
		archive->path = thunar_vfs_path_dup_string(archive->path_info);
	}
	else
		archive->path_info = NULL;


	archive->file_info = thunar_vfs_info_new_for_path(archive->path_info, NULL);
	if(archive->file_info)
	{
		archive->mime_info = archive->file_info->mime_info;
		thunar_vfs_mime_info_ref(archive->mime_info);
	}
	else
	{
		if(mime)
			archive->mime_info = thunar_vfs_mime_database_get_info(lsq_mime_database, mime);
		else
		{
			base = g_path_get_basename(path);
			archive->mime_info = thunar_vfs_mime_database_get_info_for_file(lsq_mime_database, path, base);
			g_free(base);
		}
	}
#ifdef DEBUG
	g_debug("%s\n", thunar_vfs_mime_info_get_name(archive->mime_info));
#endif

	if(!lsq_get_support_for_mime(archive->mime_info))
	{
		g_object_unref(archive);
		archive = NULL;
	}
	
	return archive;
}

void 
lsq_archive_set_status(LSQArchive *archive, LSQArchiveStatus status)
{
	gchar *_path = NULL;
	gchar *_path_ = NULL;
	GSList *iter;

	if(LSQ_IS_ARCHIVE(archive))
	{
		if(archive->status != status)
		{
			archive->old_status = archive->status;
			archive->status = status;
			g_signal_emit(G_OBJECT(archive), lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_STATUS_CHANGED], 0, NULL);
			if((archive->old_status == LSQ_ARCHIVESTATUS_REFRESH) && (archive->status == LSQ_ARCHIVESTATUS_IDLE))
				g_signal_emit(G_OBJECT(archive), lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_REFRESHED], 0, NULL);
			if((archive->old_status == LSQ_ARCHIVESTATUS_PREPARE_VIEW))// && (archive->status == LSQ_ARCHIVESTATUS_IDLE))
			{
				g_signal_emit(G_OBJECT(archive), lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_VIEW_PREPARED], 0, archive->files, NULL);
				iter = archive->files;
				while(iter)
				{
					g_free(iter->data);
					iter = g_slist_next(iter);
				}
				g_slist_free(archive->files);
				archive->files = NULL;
			}

			if((archive->old_status == LSQ_ARCHIVESTATUS_REMOVE) && (archive->files))
			{
				/* FIXME: can not be space in path */
				_path = archive->files->data;
				_path = g_shell_unquote(_path, NULL);
				_path_ = lsq_archive_get_iter_part(archive, _path);
				g_signal_emit(G_OBJECT(archive), lsq_archive_signals[LSQ_ARCHIVE_SIGNAL_PATH_CHANGED], 0, _path_, NULL);
				g_free(_path_);
				g_free(_path);
				g_slist_free(archive->files);
				archive->files = NULL;
			}
		} 
	}
}

gint
lsq_stop_archive_child( LSQArchive *archive )
{
	lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_USERBREAK);
	return 0;
}

/********************
 * LSQArchive stuff *
 ********************/

LSQArchiveIter *
lsq_archive_add_file(LSQArchive *archive, const gchar *path)
{
	guint i = 0;
	gchar **path_items = g_strsplit_set(path, "/\n", -1);
	LSQArchiveIter *parent = (LSQArchiveIter*)archive->root_entry;
	LSQArchiveIter *child = NULL;
	gchar *basename;

	while(path_items[i])
	{
		basename = g_strconcat(path_items[i], path_items[i+1]?"/":NULL, NULL);

		if(basename[0] != '\0')
		{
			child = lsq_archive_iter_get_child(archive, parent, basename);

			if(!child)
				child = lsq_archive_iter_add_child(archive, parent, basename);
		}

		g_free(basename);

		parent = child;
		i++;
	}
	return child;
}

static GType *
lsq_archive_get_entry_property_types(LSQArchive *archive, guint size)
{
	GType *new_props;
	gchar **new_names;
	guint i;

	if(archive->entry_n_property < size)
	{
		new_props = g_new0(GType, size);
		new_names = g_new0(gchar*, size);
		for(i = 0; i < archive->entry_n_property; ++i)
		{
			new_props[i] = archive->entry_property_types[i];
			new_names[i] = archive->entry_property_names[i];
		}
		g_free(archive->entry_property_types);
		g_free(archive->entry_property_names);
		archive->entry_property_types = new_props;
		archive->entry_property_names = new_names;
		archive->entry_n_property = size;
	}
	return archive->entry_property_types;
}

static gchar **
lsq_archive_get_entry_property_names(LSQArchive *archive, guint size)
{
	GType *new_types;
	gchar **new_names;
	guint i;

	if(archive->entry_n_property < size)
	{
		new_types = g_new0(GType, size);
		new_names = g_new0(gchar*, size);
		for(i = 0; i < archive->entry_n_property; ++i)
		{
			new_types[i] = archive->entry_property_types[i];
			new_names[i] = archive->entry_property_names[i];
		}
		g_free(archive->entry_property_types);
		g_free(archive->entry_property_names);
		archive->entry_property_types = new_types;
		archive->entry_property_names = new_names;
		archive->entry_n_property = size;
	}
	return archive->entry_property_names;
}

/*
 * GType
 * lsq_archive_get_entry_property_type(LSQArchive *archive, guint i)
 *
 */
GType
lsq_archive_get_entry_property_type(LSQArchive *archive, guint i)
{
#ifdef DEBUG /* n_property + 2, filename and MIME */
	g_return_val_if_fail(i < (archive->entry_n_property+LSQ_ARCHIVE_PROP_USER), G_TYPE_INVALID);
#endif

	GType retval = G_TYPE_INVALID;
	switch(i)
	{
		case LSQ_ARCHIVE_PROP_FILENAME:
			retval = G_TYPE_STRING;
			break;
		case LSQ_ARCHIVE_PROP_MIME_TYPE:
			retval = G_TYPE_STRING;
			break;
		default:
			retval = archive->entry_property_types[i - LSQ_ARCHIVE_PROP_USER];
			break;
	}
	return retval;
}

/*
 * const gchar *
 * lsq_archive_get_entry_property_name(LSQArchive *, guint)
 *
 */
const gchar *
lsq_archive_get_entry_property_name(LSQArchive *archive, guint i)
{

#ifdef DEBUG /* n_property + 2, filename and MIME */
	g_return_val_if_fail(i < (archive->entry_n_property+LSQ_ARCHIVE_PROP_USER), NULL);
#endif
	
	const gchar *retval = NULL;

	switch(i)
	{
		case LSQ_ARCHIVE_PROP_FILENAME:
			retval = _("Filename");
			break;
		case LSQ_ARCHIVE_PROP_MIME_TYPE:
			retval = _("Mime type");
			break;
		default:
			retval = archive->entry_property_names[i - LSQ_ARCHIVE_PROP_USER];
			break;
	}

	return retval;
}

/*
 * void
 * lsq_archive_set_entry_property_type(LSQArchive *archive, guint i, GType *, const gchar *)
 *
 */
void
lsq_archive_set_entry_property_type(LSQArchive *archive, guint i, GType type, const gchar *name)
{

#ifdef DEBUG
	g_return_if_fail(i >= LSQ_ARCHIVE_PROP_USER);
#endif

	GType *types_iter = lsq_archive_get_entry_property_types(archive, i+1-LSQ_ARCHIVE_PROP_USER);
	gchar **names_iter = lsq_archive_get_entry_property_names(archive, i+1-LSQ_ARCHIVE_PROP_USER);

	types_iter[i-LSQ_ARCHIVE_PROP_USER] = type;
	g_free(names_iter[i-LSQ_ARCHIVE_PROP_USER]);
	names_iter[i-LSQ_ARCHIVE_PROP_USER] = g_strdup(name);

}

/*
 * void
 * lsq_archive_set_entry_property_typesv(LSQArchive *archive, GType *)
 *
 */
void
lsq_archive_set_entry_property_typesv(LSQArchive *archive, GType *types, const gchar **names)
{
	guint size = 0;
	GType *type_iter = types;
	const gchar **name_iter = names;
	while(type_iter && name_iter)
	{
		size++;
		type_iter++;
		name_iter++;
	}
	GType *types_iter = lsq_archive_get_entry_property_types(archive, size);
	gchar **names_iter = lsq_archive_get_entry_property_names(archive, size);
	type_iter = types;
	name_iter = names;
	while(type_iter && name_iter)
	{
		*types_iter = *type_iter;
		g_free(*names_iter);
		*names_iter = g_strdup(*name_iter);
		types_iter++;
		type_iter++;
		names_iter++;
		name_iter++;
	}
}

void
lsq_archive_clear_entry_property_types(LSQArchive *archive)
{
	g_free(archive->entry_property_types);
	g_free(archive->entry_property_names);
	archive->entry_property_types = NULL;
	archive->entry_property_names = NULL;
	archive->entry_n_property = 0;
}

guint
lsq_archive_n_property(LSQArchive *archive)
{
	return archive->entry_n_property + LSQ_ARCHIVE_PROP_USER;
}

LSQArchiveIter *
lsq_archive_get_iter(const LSQArchive *archive, const gchar *path)
{
	if(!path)
		return (LSQArchiveIter *)archive->root_entry;

	gchar **buf = g_strsplit_set(path, "/\n", -1);
	gchar **iter = buf;
	LSQArchiveIter *entry = (LSQArchiveIter *)archive->root_entry;

	if(path[0] == '/' && lsq_archive_iter_get_child(archive, archive->root_entry, "/"))
	{
		g_free(iter[0]);
		iter[0] = strdup("/");
	}

	while(*iter)
	{
		if((*iter)[0])
		{
			entry = lsq_archive_iter_get_child(archive, entry, *iter);
			if(!entry)
			{
				g_strfreev(buf);
				return NULL;
			}
		}
		iter++;
	}

	g_strfreev(buf);

	return entry;
}

static gchar *
lsq_archive_get_iter_part(const LSQArchive *archive, const gchar *path)
{
	if(!path)
		return NULL;

	gchar **buf = g_strsplit_set(path, "/\n", -1);
	gchar **iter = buf;
	gchar *result, *tmp = NULL;
	LSQArchiveIter *entry = (LSQArchiveIter *)archive->root_entry;

	if(path[0] == '/' && lsq_archive_iter_get_child(archive, archive->root_entry, "/"))
	{
		tmp = iter[0];
		iter[0] = strdup("/");
	}

	while(iter[1]) /* next iter must exist */
	{
		if((*iter)[0])
		{
			entry = lsq_archive_iter_get_child(archive, entry, *iter);
			if(!entry) /*&& lsq_archive_iter_is_directory(archive, entry))*/
			{
				break;
			}
		}
		iter++;
	}

	if(tmp)
	{
		g_free(iter[0]);
		iter[0] = tmp;
	}
	tmp = *iter;
	*iter = NULL;
	result = g_strjoinv("/", buf);
	*iter = tmp;

	g_strfreev(buf);

	return result;
}

/******************
 * LSQEntry stuff *
 ******************/

static LSQEntry *
lsq_entry_new(LSQArchive *archive, const gchar *filename)
{
	LSQEntry *entry = g_new0(LSQEntry, 1);

	const gchar *pos = strchr(filename, '/');

	if(pos)
	{
		entry->filename = g_strndup(filename, (gsize)(pos - filename));
		lsq_archive_iter_set_mime(archive, entry, thunar_vfs_mime_database_get_info(lsq_mime_database, LSQ_MIME_DIRECTORY));
	}
	else
	{
		entry->filename = g_strdup(filename);
		if(g_utf8_validate (filename, -1, NULL))
		{
			lsq_archive_iter_set_mime(archive, entry, thunar_vfs_mime_database_get_info_for_name(lsq_mime_database, entry->filename));
		}
		else
		{
			gchar *utf8_file = g_convert(filename, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
			lsq_archive_iter_set_mime(archive, entry, thunar_vfs_mime_database_get_info_for_name(lsq_mime_database, utf8_file));
			g_free(utf8_file);
		}
	}

	return entry;
}

static void
lsq_archive_entry_free(const LSQArchive *archive, LSQEntry *entry)
{
	guint i = 0; 
	gpointer props_iter = entry->props;
	LSQSList *buffer_iter = entry->buffer;

	for(; buffer_iter; buffer_iter = buffer_iter->next)
	{
		lsq_archive_entry_free(archive, buffer_iter->entry);
	}
	lsq_slist_free(entry->buffer);
	entry->buffer = NULL;

	if(entry->children)
	{
		/* first element of the array (*entry->children) contains the size of the array */
		for(i = 1; i <= GPOINTER_TO_UINT(*entry->children); ++i)
			lsq_archive_entry_free(archive, entry->children[i]);

		g_free(entry->children);
		entry->children = NULL;
	}

	if(props_iter)
	{
		for(i=0; i<archive->entry_n_property; ++i)
		{
			switch(archive->entry_property_types[i])
			{
				case(G_TYPE_STRING):
					g_free(*(gchar **)props_iter);
					*(gchar **)props_iter = (gchar *)0x1;
					props_iter += sizeof(gchar *);
					break;
				case(G_TYPE_UINT):
					props_iter += sizeof(guint);
					break;
				case(G_TYPE_UINT64):
					props_iter += sizeof(guint64);
					break;
			}
		}
		g_free(entry->props);
	}
	if(entry->mime_info)
		thunar_vfs_mime_info_unref(entry->mime_info);
	g_free(entry->filename);
	g_free(entry);
}

static gint
lsq_entry_filename_compare(LSQEntry *a, LSQEntry *b)
{
	return strcmp(a->filename, b->filename);
}

static LSQEntry *
lsq_entry_get_child(const LSQEntry *entry, const gchar *filename)
{
	LSQSList *buffer_iter = NULL;
	/* the first element of the array (*entry->children) contains the size of the array */
	guint size = entry->children?GPOINTER_TO_UINT(*entry->children):0;
	guint pos = 0;
	guint begin = 1;
	gint cmp = 0;
	const gchar *_pos = strchr(filename, '/');
	gchar *_filename;

	if(_pos)
		_filename = g_strndup(filename, (gsize)(_pos - filename));
	else
		_filename = g_strdup(filename);


	/* binary search algoritme */
	while(size)
	{
		pos = (size / 2);

		cmp = strcmp(_filename, entry->children[begin+pos]->filename);
		if(!cmp)
		{
			g_free(_filename);
			return entry->children[begin+pos];
		}

		if(cmp < 0)
		{
			size = pos;
		}
		else
		{
			size -= ++pos;
			begin += pos;
		}
	}

	/* search the buffer */
	for(buffer_iter = entry->buffer; buffer_iter; buffer_iter = buffer_iter->next)
	{
		cmp = strcmp(_filename, buffer_iter->entry->filename);

		if(!cmp)
		{
			g_free(_filename);
			return buffer_iter->entry;
		}
		if(cmp < 0)
			break;
	}

	g_free(_filename);
	return NULL;
}

static gboolean
lsq_archive_entry_del_child(const LSQArchive *archive, LSQEntry *entry, const gchar *filename)
{
	LSQSList *buffer_iter = NULL, *prev_iter = NULL;
	/* the first element of the array (*entry->children) contains the size of the array */
	guint total_size, size = total_size = entry->children?GPOINTER_TO_UINT(*entry->children):0;
	guint pos = 0;
	guint begin = 1;
	gint cmp = 0;
	const gchar *_pos = strchr(filename, '/');
	gchar *_filename;

	if(_pos)
		_filename = g_strndup(filename, (gsize)(_pos - filename));
	else
		_filename = g_strdup(filename);


	/* binary search algoritme */
	while(size)
	{
		pos = (size / 2);

		cmp = strcmp(_filename, entry->children[begin+pos]->filename);
		if(!cmp)
		{
			g_free(_filename);
			lsq_archive_entry_free(archive, entry->children[begin+pos]);
			for(;(begin + pos) < total_size; ++pos)
			{
				entry->children[begin+pos] = entry->children[begin+pos+1];
			}
			total_size -= 1;
			*entry->children = GUINT_TO_POINTER(total_size);
			return TRUE;
		}

		if(cmp < 0)
		{
			size = pos;
		}
		else
		{
			size -= ++pos;
			begin += pos;
		}
	}

	/* search the buffer */
	for(buffer_iter = entry->buffer; buffer_iter; buffer_iter = buffer_iter->next)
	{
		cmp = strcmp(_filename, buffer_iter->entry->filename);

		if(!cmp)
		{
			g_free(_filename);
			lsq_archive_entry_free(archive, buffer_iter->entry);
			if(prev_iter)
				prev_iter->next = buffer_iter->next;
			else
				entry->buffer = buffer_iter->next;
			g_free(buffer_iter);
			return TRUE;
		}
		if(cmp < 0)
			break;
		prev_iter = buffer_iter;
	}

	g_free(_filename);
	return FALSE;
}

static void
lsq_archive_entry_add_child(LSQArchive *archive, LSQEntry *parent, LSQEntry *child)
{
	parent->buffer = lsq_slist_insert_sorted_single(parent->buffer, child, (GCompareFunc)lsq_entry_filename_compare);

	if(lsq_slist_length(parent->buffer) == LSQ_ENTRY_CHILD_BUFFER_SIZE)
		lsq_archive_entry_flush_buffer(archive, parent);
}

static void
lsq_archive_entry_flush_buffer(LSQArchive *archive, LSQEntry *entry)
{
	if(!entry->buffer)
		return;

	guint max_children = 0;
	guint begin = 1;
	guint pos = 0;
	gint cmp = 1;
	guint old_i = 1;
	guint new_i = 1;
	/* the first element of the array (*entry->children) contains the size of the array */
	guint size = entry->children?GPOINTER_TO_UINT(*entry->children):0;
	guint n_children = size;
	LSQSList *buffer_iter = NULL;
	LSQEntry **children_old = (LSQEntry **)entry->children;

	max_children = (n_children + lsq_slist_length(entry->buffer));
	
	entry->children = g_new(LSQEntry *, max_children+1);
	for(buffer_iter = entry->buffer;buffer_iter;buffer_iter = buffer_iter->next)
	{
		size = n_children + 1 - begin;
		/* binary search algoritme */
		while(size)
		{
			pos = (size / 2);

			cmp = strcmp(buffer_iter->entry->filename, children_old[begin+pos]->filename);
			if(!cmp)
				break;

			if(cmp < 0)
			{
				size = pos;
			}
			else
			{
				size -= ++pos;
				begin += pos;
			}
		}
		if(!cmp)
		{
			g_critical("THIS SHOULD NOT HAPPEN!!! (the universe has just collapsed)");
		}
		else
		{
			while(old_i < begin)
			{
				entry->children[new_i++] = children_old[old_i++];
			}
			entry->children[new_i++] = buffer_iter->entry;
		}
	}
	while(old_i <= n_children)
	{
		entry->children[new_i++] = children_old[old_i++];
	}
	n_children = new_i - 1;
	/* the first element of the array (*entry->children) contains the size of the array */
	*entry->children = GUINT_TO_POINTER(n_children);
	
	lsq_slist_free(entry->buffer);
	entry->buffer = NULL;

	g_free(children_old);
}

static gpointer
lsq_archive_entry_get_props(LSQArchive *archive, LSQEntry *entry)
{
	guint size = 0;
	guint i;

	if(!entry->props)
	{
		for(i = 0; i < archive->entry_n_property; ++i)
		{
			switch(archive->entry_property_types[i])
			{
				case G_TYPE_STRING:
					size += sizeof(gchar *);
					break;
				case G_TYPE_UINT:
					size += sizeof(guint);
					break;
				case G_TYPE_UINT64:
					size += sizeof(guint64);
					break;
			}
		}

		entry->props = g_malloc0(size);
	}

	return entry->props;
}

/************************
 * LSQArchiveIter stuff *
 ************************/

/** 
 * gboolean
 * lsq_archive_iter_is_directory(const LSQArchive *, const LSQArchiveIter *)
 *
 * Check if archive entry is a directory
 **/
gboolean
lsq_archive_iter_is_directory(const LSQArchive *archive, const LSQArchiveIter *iter)
{
	const gchar *mime = lsq_archive_iter_get_mimetype(archive, iter);
	if(!mime)
		return FALSE;
	if(!strcmp(mime, LSQ_MIME_DIRECTORY))
		return TRUE;
	return FALSE;
}

/** 
 * guint
 * lsq_archive_iter_n_children(const LSQArchive *, const LSQArchiveIter *)
 *
 * return number of children
 **/
guint
lsq_archive_iter_n_children(const LSQArchive *archive, const LSQArchiveIter *iter)
{
#ifdef DEBUG
	g_return_val_if_fail(iter, 0);
#endif
	/* g_debug("%d: %d", iter->children?GPOINTER_TO_INT(*iter->children):0, lsq_slist_length(iter->buffer)); */
	/* the first element of the array (*iter->children) contains the size of the array */
	return ((iter->children?GPOINTER_TO_UINT(*iter->children):0) + lsq_slist_length(iter->buffer));
}

/** 
 * LSQArchiveIter* 
 * lsq_archive_iter_nth_child(const LSQArchive *, const LSQArchiveIter *, guint)
 *
 * return nth child
 **/
LSQArchiveIter*
lsq_archive_iter_nth_child(LSQArchive *archive, LSQArchiveIter *iter, guint n)
{
#ifdef DEBUG
	g_return_val_if_fail(n >= 0, NULL);
#endif
	if(n >= lsq_archive_iter_n_children(archive, iter))
		return NULL;

	lsq_archive_entry_flush_buffer(archive, (LSQEntry *)iter);
	/* the first element of the array (*iter->children) contains the size of the array */
	return ((LSQEntry *)iter->children[n+1]);
}

/**
 * LSQArchiveIter* 
 * lsq_archive_iter_add_child(LSQArchive *, LSQArchiveIter *, const gchar *)
 *
 * return a new child
 **/
LSQArchiveIter *
lsq_archive_iter_add_child(LSQArchive *archive, LSQArchiveIter *parent, const gchar *filename)
{
	LSQEntry *entry = lsq_entry_new(archive, filename);

	lsq_archive_entry_add_child(archive, (LSQEntry *)parent, entry);

	return (LSQArchiveIter*)entry;
}

/**
 * LSQArchiveIter*
 * lsq_archive_iter_get_child(const LSQArchive *, const LSQArchiveIter *, const gchar *)
 *
 * return the child iter if found
 **/
LSQArchiveIter *
lsq_archive_iter_get_child(const LSQArchive *archive, const LSQArchiveIter *parent, const gchar *filename)
{
	return (LSQArchiveIter*)lsq_entry_get_child(parent, filename);
}

/** 
 **/
gboolean
lsq_archive_del_file(LSQArchive *archive, const gchar *path)
{
	if(!path)
		return FALSE;

	gchar **buf = g_strsplit_set(path, "/\n", -1);
	gchar **iter = buf;
	LSQArchiveIter *entry = (LSQArchiveIter *)archive->root_entry;
	GSList *prev_iter, *stack_iter, *stack = NULL;

	if(path[0] == '/' && lsq_archive_iter_get_child(archive, archive->root_entry, "/"))
	{
		g_free(iter[0]);
		iter[0] = strdup("/");
	}

	while(*iter)
	{
		if((*iter)[0])
		{
			entry = lsq_archive_iter_get_child(archive, entry, *iter);
			if(!entry)
			{
				g_slist_free(stack);
				g_strfreev(buf);
				return FALSE;
			}
			stack = g_slist_prepend(stack, entry);
		}
		iter++;
	}

	g_strfreev(buf);

	stack_iter = g_slist_next(stack);
	prev_iter = stack;

	while(stack_iter)
	{
		entry = (LSQEntry*)stack_iter->data;

		if(entry->props || lsq_archive_iter_n_children(archive, entry) > 1)
			break;

		prev_iter = stack_iter;
		stack_iter = g_slist_next(stack_iter);
	}

	if(!stack_iter)
	{
		entry = archive->root_entry;
	}

	gboolean result = lsq_archive_entry_del_child(archive, entry, ((LSQEntry*)prev_iter->data)->filename);

	g_slist_free(stack);

	return result;
}

/**
 * const gchar *
 * lsq_archive_iter_get_filename(const LSQArchive *, const LSQArchiveIter *)
 *
 * returns filename
 */
static const gchar*
lsq_archive_iter_get_filename(const LSQArchive *archive, const LSQArchiveIter *iter)
{
	return ((LSQEntry *)iter)->filename;
}

/**
 * const gchar *
 * lsq_archive_iter_get_mimetype(const LSQArchive *, const LSQArchiveIter *)
 *
 * returns mime type
 */
static const gchar *
lsq_archive_iter_get_mimetype(const LSQArchive *archive, const LSQArchiveIter *iter)
{
	if(((LSQEntry *)iter)->mime_info)
		return thunar_vfs_mime_info_get_name(((LSQEntry *)iter)->mime_info);
	return NULL;
}

/**
 * void
 * lsq_archive_iter_set_mime(const LSQArchive *, const LSQArchiveIter *, const gchar *)
 *
 * set mime type to entry
 */
void
lsq_archive_iter_set_mime(LSQArchive *archive, LSQArchiveIter *iter, ThunarVfsMimeInfo *mime_info)
{
	if(((LSQEntry *)iter)->mime_info)
		thunar_vfs_mime_info_unref(((LSQEntry *)iter)->mime_info);
	((LSQEntry *)iter)->mime_info = mime_info;
}

/**
 * void
 * lsq_archive_iter_set_prop_str(const LSQArchive *, const LSQArchiveIter *, guint, const gchar *)
 *
 */
void
lsq_archive_iter_set_prop_str(LSQArchive *archive, LSQArchiveIter *iter, guint i, const gchar *str_val)
{

	gpointer props_iter = NULL;
	guint n;
#ifdef DEBUG
	g_return_if_fail(i < (archive->entry_n_property+LSQ_ARCHIVE_PROP_USER));
	if(i >= LSQ_ARCHIVE_PROP_USER)
		g_return_if_fail(archive->entry_property_types[i-LSQ_ARCHIVE_PROP_USER] == G_TYPE_STRING);
#endif

	switch(i)
	{
		case LSQ_ARCHIVE_PROP_FILENAME:
#ifdef DEBUG
			g_critical("DON'T set filename");
#endif
			break;
		case LSQ_ARCHIVE_PROP_MIME_TYPE:
#ifdef DEBUG
			g_critical("DON'T set mimetype");
#endif
			break;
		default:
			props_iter = lsq_archive_entry_get_props(archive, (LSQEntry *)iter);
			for(n = 0; n < (i-LSQ_ARCHIVE_PROP_USER); ++n)
			{
				switch(archive->entry_property_types[n])
				{
					case G_TYPE_STRING:
						props_iter += sizeof(gchar *);
						break;
					case G_TYPE_UINT:
						props_iter += sizeof(guint);
						break;
					case G_TYPE_UINT64:
						props_iter += sizeof(guint64);
						break;
				}
			}
			g_free(*((gchar **)props_iter));
			(*((gchar **)props_iter)) = g_strdup(str_val);
			break;
	}

}

/**
 * void
 * lsq_archive_iter_set_prop_uint(const LSQArchive *, const LSQArchiveIter *, guint, guint) 
 *
 */
void
lsq_archive_iter_set_prop_uint(LSQArchive *archive, LSQArchiveIter *iter, guint i, guint int_val)
{

#ifdef DEBUG
	g_return_if_fail(i < (archive->entry_n_property+LSQ_ARCHIVE_PROP_USER));
	g_return_if_fail(i >= LSQ_ARCHIVE_PROP_USER);
	g_return_if_fail(archive->entry_property_types[i-LSQ_ARCHIVE_PROP_USER] == G_TYPE_UINT);
#endif /* DEBUG */
	gpointer props_iter = lsq_archive_entry_get_props(archive, (LSQEntry *)iter);
	guint n;

	for(n = 0; n < (i-LSQ_ARCHIVE_PROP_USER); ++n)
	{
		switch(archive->entry_property_types[n])
		{
			case G_TYPE_STRING:
				props_iter += sizeof(gchar *);
				break;
			case G_TYPE_UINT:
				props_iter += sizeof(guint);
				break;
			case G_TYPE_UINT64:
				props_iter += sizeof(guint64);
				break;
		}
	}
	(*((guint *)props_iter)) = int_val;

}

/**
 * void
 * lsq_archive_iter_set_prop_uint64(const LSQArchive *, const LSQArchiveIter *, guint, guint64) 
 *
 */
void
lsq_archive_iter_set_prop_uint64(LSQArchive *archive, LSQArchiveIter *iter, guint i, guint64 int64_val)
{

#ifdef DEBUG
	g_return_if_fail(i < (archive->entry_n_property+LSQ_ARCHIVE_PROP_USER));
	g_return_if_fail(i >= LSQ_ARCHIVE_PROP_USER);
	g_return_if_fail(archive->entry_property_types[i-LSQ_ARCHIVE_PROP_USER] == G_TYPE_UINT64);
#endif /* DEBUG */
	gpointer props_iter = lsq_archive_entry_get_props(archive, (LSQEntry *)iter);
	guint n;

	for(n = 0; n < (i-LSQ_ARCHIVE_PROP_USER); ++n)
	{
		switch(archive->entry_property_types[n])
		{
			case G_TYPE_STRING:
				props_iter += sizeof(gchar *);
				break;
			case G_TYPE_UINT:
				props_iter += sizeof(guint);
				break;
			case G_TYPE_UINT64:
				props_iter += sizeof(guint64);
				break;
		}
	}
	(*((guint64 *)props_iter)) = int64_val;

}

/**
 * void
 * lsq_archive_iter_set_prop_value(const LSQArchive *, const LSQArchiveIter *, guint, const GValue *) 
 *
 */
void
lsq_archive_iter_set_prop_value(LSQArchive *archive, LSQArchiveIter *iter, guint i, const GValue *value)
{
	switch(G_VALUE_TYPE(value))
	{
		case G_TYPE_STRING:
			lsq_archive_iter_set_prop_str(archive, iter, i, g_value_get_string(value));
			break;
		case G_TYPE_UINT:
			lsq_archive_iter_set_prop_uint(archive, iter, i, g_value_get_uint(value));
			break;
		case G_TYPE_UINT64:
			lsq_archive_iter_set_prop_uint64(archive, iter, i, g_value_get_uint64(value));
			break;
	}
}

/**
 * void
 * lsq_archive_iter_set_props(const LSQArchive *, const LSQArchiveIter *, guint, ...) 
 *
 */
void
lsq_archive_iter_set_props(LSQArchive *archive, LSQArchiveIter *iter, ...)
{


	gpointer props_iter = lsq_archive_entry_get_props(archive, (LSQEntry *)iter);
	guint i;
	va_list ap;

	va_start(ap, iter);

	for(i = 0; i < archive->entry_n_property; ++i)
	{
		switch(archive->entry_property_types[i])
		{
			case G_TYPE_STRING:
				(*((gchar **)props_iter)) = g_strdup(va_arg(ap, gchar*));
				props_iter += sizeof(gchar *);
				break;
			case G_TYPE_UINT:
				(*((guint *)props_iter)) = va_arg(ap, guint);
				props_iter += sizeof(guint);
				break;
			case G_TYPE_UINT64:
				(*((guint64 *)props_iter)) = va_arg(ap, guint64);
				props_iter += sizeof(guint64);
				break;
		}
	}

	va_end(ap);

}

/**
 * void
 * lsq_archive_iter_set_propsv(const LSQArchive *, const LSQArchiveIter *, guint, gconstpointer *) 
 *
 */
void
lsq_archive_iter_set_propsv(LSQArchive *archive, LSQArchiveIter *iter, gconstpointer *props)
{
	gpointer props_iter = lsq_archive_entry_get_props(archive, (LSQEntry *)iter);
	guint i;

	for(i = 0; i < archive->entry_n_property; ++i)
	{
		switch(archive->entry_property_types[i])
		{
			case G_TYPE_STRING:
				(*((gchar **)props_iter)) = g_strdup((const gchar*)props[i]);
				props_iter += sizeof(gchar *);
				break;
			case G_TYPE_UINT:
				(*((guint *)props_iter)) = *((const guint*)props[i]);
				props_iter += sizeof(guint);
				break;
			case G_TYPE_UINT64:
				(*((guint64 *)props_iter)) = *((const guint64*)props[i]);
				props_iter += sizeof(guint64);
				break;
		}
	}

}

/**
 * gboolean 
 * lsq_archive_iter_get_prop_value(const LSQArchive *, const LSQArchiveIter *, guint, const GValue *) 
 *
 */
gboolean
lsq_archive_iter_get_prop_value(const LSQArchive *archive, const LSQArchiveIter *iter, guint i, GValue *value)
{

	if(i>=LSQ_ARCHIVE_PROP_USER)
		g_value_init(value, archive->entry_property_types[i-LSQ_ARCHIVE_PROP_USER]);
	else
		g_value_init(value, G_TYPE_STRING);

	switch(G_VALUE_TYPE(value))
	{
		case G_TYPE_STRING:
			g_value_set_string(value, lsq_archive_iter_get_prop_str(archive, iter, i));
			break;
		case G_TYPE_UINT:
			g_value_set_uint(value, lsq_archive_iter_get_prop_uint(archive, iter, i));
			break;
		case G_TYPE_UINT64:
			g_value_set_uint64(value, lsq_archive_iter_get_prop_uint64(archive, iter, i));
			break;
	}

	return TRUE;
}

/**
 * static const gchar *
 * lsq_archive_iter_get_prop_str(const LSQArchive *, const LSQArchiveIter *, guint) 
 *
 */
static const gchar*
lsq_archive_iter_get_prop_str(const LSQArchive *archive, const LSQArchiveIter *iter, guint i)
{
	const gchar *retval = NULL;
	gpointer props_iter = NULL;
	guint n;
#ifdef DEBUG
	g_return_val_if_fail(i < (archive->entry_n_property+LSQ_ARCHIVE_PROP_USER), NULL);
	if(i >= LSQ_ARCHIVE_PROP_USER)
		g_return_val_if_fail(archive->entry_property_types[i-LSQ_ARCHIVE_PROP_USER] == G_TYPE_STRING, NULL);
#endif

	switch(i)
	{
		case LSQ_ARCHIVE_PROP_FILENAME:
			retval = lsq_archive_iter_get_filename(archive, iter);
			break;
		case LSQ_ARCHIVE_PROP_MIME_TYPE:
			retval = lsq_archive_iter_get_mimetype(archive, iter);
			break;
		default:
			props_iter = ((LSQEntry *)iter)->props;
			if(props_iter)
			{
				for(n = 0; n < (i-LSQ_ARCHIVE_PROP_USER); ++n)
				{
					switch(archive->entry_property_types[n])
					{
						case G_TYPE_STRING:
							props_iter += sizeof(gchar *);
							break;
						case G_TYPE_UINT:
							props_iter += sizeof(guint);
							break;
						case G_TYPE_UINT64:
							props_iter += sizeof(guint64);
							break;
					}
				}
				retval = (*((gchar **)props_iter));
			}
			break;
	}
	return retval;
}

/**
 * static guint
 * lsq_archive_iter_get_prop_uint(const LSQArchive *, const LSQArchiveIter *, guint) 
 *
 */
static guint
lsq_archive_iter_get_prop_uint(const LSQArchive *archive, const LSQArchiveIter *iter, guint i)
{
	gpointer props_iter = ((LSQEntry *)iter)->props;
	guint n;
#ifdef DEBUG
	g_return_val_if_fail(i < (archive->entry_n_property+LSQ_ARCHIVE_PROP_USER), 0);
	g_return_val_if_fail(i >= LSQ_ARCHIVE_PROP_USER, 0);
	g_return_val_if_fail(archive->entry_property_types[i-LSQ_ARCHIVE_PROP_USER] == G_TYPE_UINT, 0);
#endif
	if(!props_iter)
		return 0;
	for(n = 0; n < (i-LSQ_ARCHIVE_PROP_USER); ++n)
	{
		switch(archive->entry_property_types[n])
		{
			case G_TYPE_STRING:
				props_iter += sizeof(gchar *);
				break;
			case G_TYPE_UINT:
				props_iter += sizeof(guint);
				break;
			case G_TYPE_UINT64:
				props_iter += sizeof(guint64);
				break;
		}
	}
	return (*((guint *)props_iter));
}

/**
 * static guint64
 * lsq_archive_iter_get_prop_uint64(const LSQArchive *, const LSQArchiveIter *, guint) 
 *
 */
static guint64
lsq_archive_iter_get_prop_uint64(const LSQArchive *archive, const LSQArchiveIter *iter, guint i)
{
	gpointer props_iter = ((LSQEntry *)iter)->props;
	guint n;
#ifdef DEBUG
	g_return_val_if_fail(i < (archive->entry_n_property+LSQ_ARCHIVE_PROP_USER), 0);
	g_return_val_if_fail(i >= LSQ_ARCHIVE_PROP_USER, 0);
	g_return_val_if_fail(archive->entry_property_types[i-LSQ_ARCHIVE_PROP_USER] == G_TYPE_UINT64, 0);
#endif
	if(!props_iter)
		return 0;
	for(n = 0; n < (i-LSQ_ARCHIVE_PROP_USER); ++n)
	{
		switch(archive->entry_property_types[n])
		{
			case G_TYPE_STRING:
				props_iter += sizeof(gchar *);
				break;
			case G_TYPE_UINT:
				props_iter += sizeof(guint);
				break;
			case G_TYPE_UINT64:
				props_iter += sizeof(guint64);
				break;
		}
	}
	return (*((guint64 *)props_iter));
}

const gchar *
lsq_archive_get_filename(LSQArchive *archive)
{
	g_return_val_if_fail(LSQ_IS_ARCHIVE(archive), "<unknown>");
	return thunar_vfs_path_get_name(archive->path_info);
}

const gchar *
lsq_archive_get_mimetype(LSQArchive *archive)
{
	g_return_val_if_fail(LSQ_IS_ARCHIVE(archive), "");
	return thunar_vfs_mime_info_get_name(archive->mime_info);
}

LSQArchiveStatus
lsq_archive_get_status(LSQArchive *archive)
{
	g_return_val_if_fail(LSQ_IS_ARCHIVE(archive), LSQ_ARCHIVESTATUS_ERROR);
	return archive->status;
}

LSQArchiveStatus
lsq_archive_get_old_status(LSQArchive *archive)
{
	g_return_val_if_fail(LSQ_IS_ARCHIVE(archive), LSQ_ARCHIVESTATUS_ERROR);
	return archive->old_status;
}

const gchar *
lsq_archive_get_status_msg(LSQArchive *archive)
{
	const gchar *msg = "";
	g_return_val_if_fail(LSQ_IS_ARCHIVE(archive), "");

	switch(archive->status)
	{
		case LSQ_ARCHIVESTATUS_INIT:
			msg = N_("Initializing archive");
			break;
		case LSQ_ARCHIVESTATUS_REFRESH:
			msg = N_("Refreshing archive contents");
			break;
		case LSQ_ARCHIVESTATUS_ADD:
			msg = N_("Adding file(s) to archive");
			break;
		case LSQ_ARCHIVESTATUS_EXTRACT:
			msg = N_("Extracting file(s) from archive");
			break;
		case LSQ_ARCHIVESTATUS_REMOVE:
			msg = N_("Removing file(s) from archive");
			break;
		case LSQ_ARCHIVESTATUS_IDLE:
			msg = N_("Done");
			break;
		case LSQ_ARCHIVESTATUS_PREPARE_VIEW:
			msg = N_("Extracting file(s) to temporary directory");
		case LSQ_ARCHIVESTATUS_CUSTOM:
			msg = N_("Performing an extended action");
			break;
		case LSQ_ARCHIVESTATUS_USERBREAK:
			msg = N_("Cancelled");
			break;
		case LSQ_ARCHIVESTATUS_ERROR:
			msg = N_("Error");
			break;
	}
	return msg;
}

void
lsq_archive_iter_get_icon_name(const LSQArchive *archive, const LSQArchiveIter *iter, GValue *value, GtkIconTheme *icon_theme)
{
	g_value_init(value, G_TYPE_STRING);
	if(!iter->mime_info)
		return;
	const gchar *icon_name = thunar_vfs_mime_info_lookup_icon_name(iter->mime_info, icon_theme);
	if(icon_name && gtk_icon_theme_has_icon(icon_theme, icon_name))
		g_value_set_string(value, icon_name);
}

void
lsq_archive_add_children(LSQArchive *archive, GSList *files)
{
	GSList *iter = files;
	LSQArchiveIter *entry;
	guint i;

	/* TODO: could by cyclic? */
	while(iter)
	{
		entry = lsq_archive_get_iter(archive, (const gchar*)iter->data);
		for(i = 0; i < lsq_archive_iter_n_children(archive, entry); ++i)
		{
			files = g_slist_append(files, g_strconcat((const gchar*)iter->data, "/", lsq_archive_iter_nth_child(archive, entry, i)->filename, NULL));
		}

		iter = g_slist_next(iter);
	}
}

gboolean
lsq_archive_stop(LSQArchive *archive)
{
	if(archive->child_pid)
		kill ( archive->child_pid , SIGHUP);
	return TRUE;
}
