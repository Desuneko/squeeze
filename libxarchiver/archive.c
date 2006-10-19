/*  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or *  (at your option) any later version.
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
#include <gettext.h>

#include "mime.h"

//#include "entry.h"
#include "archive.h"
#include "archive-support.h"
#include "slist.h"

#include "internals.h"

#ifndef LXA_ENTRY_CHILD_BUFFER_SIZE
#define LXA_ENTRY_CHILD_BUFFER_SIZE 300
#endif

#ifndef LXA_MIME_DIRECTORY
#define LXA_MIME_DIRECTORY "inode/directory"
#endif

struct _LXAEntry
{
	gchar *filename;
	gchar *mime_type;
	gpointer props;
	LXAEntry **children;
	LXASList *buffer;
};


static void
lxa_archive_class_init(LXAArchiveClass *archive_class);

static void
lxa_archive_init(LXAArchive *archive);

static void
lxa_archive_finalize(GObject *object);

static GType *
lxa_archive_get_property_types(LXAArchive *archive, guint size);

static gchar **
lxa_archive_get_property_names(LXAArchive *archive, guint size);

static LXAEntry *
lxa_entry_new(const gchar *);

static void
lxa_archive_entry_free(LXAArchive *, LXAEntry *);

static LXAEntry *
lxa_entry_get_child(const LXAEntry *entry, const gchar *filename);

static void
lxa_archive_entry_add_child(LXAArchive *, LXAEntry *, LXAEntry *);

static void
lxa_archive_entry_flush_buffer(LXAArchive *, LXAEntry *);

static gpointer
lxa_archive_entry_get_props(LXAArchive *, LXAEntry *);

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
	archive->root_entry = g_new0(LXAEntry, 1);
}

/** static void
 * lxa_archive_finalize(GObject *object)
 *
 * 
 */
static void
lxa_archive_finalize(GObject *object)
{
	LXAArchive *archive = LXA_ARCHIVE(object);
	if(archive->path)
		g_free(archive->path);
	lxa_archive_entry_free(archive, archive->root_entry);
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

	archive = g_object_new(lxa_archive_get_type(), NULL);

	if(path)
		archive->path = g_strdup(path);
	else
		archive->path = NULL;

	if(!mime)
		archive->mime = lxa_mime_get_mime_type_for_file(archive->path);
	else
		archive->mime = g_strdup(mime);
#ifdef DEBUG	
	g_debug("Mime-type: %s", archive->mime);
#endif
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
lxa_stop_archive_child( LXAArchive *archive )
{
	lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_USERBREAK);
	return 0;
}

/********************
 * LXAArchive stuff *
 ********************/

LXAArchiveIter *
lxa_archive_add_file(LXAArchive *archive, const gchar *path)
{
	guint i = 0;
	gchar **path_items = g_strsplit_set(path, "/\n", -1);
	LXAArchiveIter *parent = (LXAArchiveIter*)archive->root_entry;
	LXAArchiveIter *child = NULL;
	gchar *basename;

	while(path_items[i])
	{
		basename = g_strconcat(path_items[i], path_items[i+1]?"/":NULL, NULL);

		if(basename[0] != '\0')
		{
			child = lxa_archive_iter_get_child(archive, parent, basename);

			if(!child)
				child = lxa_archive_iter_add_child(archive, parent, basename);
		}

		g_free(basename);

		parent = child;
		i++;
	}

	return child;
}

static GType *
lxa_archive_get_property_types(LXAArchive *archive, guint size)
{
	GType *new_props;
	gchar **new_names;
	guint i;

	if(archive->n_property < size)
	{
		new_props = g_new0(GType, size);
		new_names = g_new0(gchar*, size);
		for(i = 0; i < archive->n_property; ++i)
		{
			new_props[i] = archive->property_types[i];
			new_names[i] = archive->property_names[i];
		}
		g_free(archive->property_types);
		g_free(archive->property_names);
		archive->property_types = new_props;
		archive->property_names = new_names;
		archive->n_property = size;
	}
	return archive->property_types;
}

static gchar **
lxa_archive_get_property_names(LXAArchive *archive, guint size)
{
	GType *new_types;
	gchar **new_names;
	guint i;

	if(archive->n_property < size)
	{
		new_types = g_new0(GType, size);
		new_names = g_new0(gchar*, size);
		for(i = 0; i < archive->n_property; ++i)
		{
			new_types[i] = archive->property_types[i];
			new_names[i] = archive->property_names[i];
		}
		g_free(archive->property_types);
		g_free(archive->property_names);
		archive->property_types = new_types;
		archive->property_names = new_names;
		archive->n_property = size;
	}
	return archive->property_names;
}

/*
 * GType
 * lxa_archive_get_property_type(LXAArchive *archive, guint i)
 *
 */
GType
lxa_archive_get_property_type(LXAArchive *archive, guint i)
{
#ifdef DEBUG /* n_property + 2, filename and MIME */
	g_return_val_if_fail(i < (archive->n_property+LXA_ARCHIVE_PROP_USER), G_TYPE_INVALID);
#endif
	switch(i)
	{
		case LXA_ARCHIVE_PROP_FILENAME:
			return G_TYPE_STRING;
		case LXA_ARCHIVE_PROP_MIME_TYPE:
			return G_TYPE_STRING;
		default:
			return archive->property_types[i - LXA_ARCHIVE_PROP_USER];
	}
	g_return_val_if_reached(G_TYPE_NONE);
}

/*
 * const gchar *
 * lxa_archive_get_property_name(LXAArchive *, guint)
 *
 */
const gchar *
lxa_archive_get_property_name(LXAArchive *archive, guint i)
{
#ifdef DEBUG /* n_property + 2, filename and MIME */
	g_return_val_if_fail(i < (archive->n_property+LXA_ARCHIVE_PROP_USER), NULL);
#endif
	switch(i)
	{
		case LXA_ARCHIVE_PROP_FILENAME:
			return _("Filename");
		case LXA_ARCHIVE_PROP_MIME_TYPE:
			return _("Mime type");
		default:
			return archive->property_names[i - LXA_ARCHIVE_PROP_USER];
	}
	g_return_val_if_reached(NULL);
}

/*
 * void
 * lxa_archive_set_property_type(LXAArchive *archive, guint i, GType *, const gchar *)
 *
 */
void
lxa_archive_set_property_type(LXAArchive *archive, guint i, GType type, const gchar *name)
{
#ifdef DEBUG
	g_return_if_fail(i >= LXA_ARCHIVE_PROP_USER);
#endif
	GType *types_iter = lxa_archive_get_property_types(archive, i+1-LXA_ARCHIVE_PROP_USER);
	gchar **names_iter = lxa_archive_get_property_names(archive, i+1-LXA_ARCHIVE_PROP_USER);

	types_iter[i-LXA_ARCHIVE_PROP_USER] = type;
	g_free(names_iter[i-LXA_ARCHIVE_PROP_USER]);
	names_iter[i-LXA_ARCHIVE_PROP_USER] = g_strdup(name);
}

/*
 * void
 * lxa_archive_set_property_typesv(LXAArchive *archive, GType *)
 *
 */
void
lxa_archive_set_property_typesv(LXAArchive *archive, GType *types, const gchar **names)
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
	GType *types_iter = lxa_archive_get_property_types(archive, size);
	gchar **names_iter = lxa_archive_get_property_names(archive, size);
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

guint
lxa_archive_n_property(LXAArchive *archive)
{
	return archive->n_property + LXA_ARCHIVE_PROP_USER;
}

LXAArchiveIter *
lxa_archive_get_iter(LXAArchive *archive, const gchar *path)
{
	if(!path)
		return (LXAArchiveIter *)archive->root_entry;

	gchar **buf = g_strsplit_set(path, "/\n", -1);
	gchar **iter = buf;
	LXAArchiveIter *entry = (LXAArchiveIter *)archive->root_entry;

	if(path[0] == '/' && lxa_archive_iter_get_child(archive, archive->root_entry, "/"))
	{
		iter[0] = strdup("/");
	}

	while(*iter)
	{
		if((*iter)[0])
		{
			entry = lxa_archive_iter_get_child(archive, entry, *iter);
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


/******************
 * LXAEntry stuff *
 ******************/

static LXAEntry *
lxa_entry_new(const gchar *filename)
{
	LXAEntry *entry = g_new0(LXAEntry, 1);

	const gchar *pos = strchr(filename, '/');

	if(pos)
	{
		entry->filename = g_strndup(filename, (gsize)(pos - filename));
		entry->mime_type = g_strdup(LXA_MIME_DIRECTORY);
	}
	else
	{
		entry->filename = g_strdup(filename);
		entry->mime_type = lxa_mime_get_mime_type_for_filename(entry->filename);
	}

	return entry;
}

void
lxa_archive_entry_free(LXAArchive *archive, LXAEntry *entry)
{
	gint i = 0; 
	gpointer props_iter = entry->props;
	LXASList *buffer_iter = entry->buffer;

	for(; buffer_iter; buffer_iter = buffer_iter->next)
	{
		lxa_archive_entry_free(archive, buffer_iter->entry);
	}
	lxa_slist_free(entry->buffer);
	entry->buffer = NULL;

	if(entry->children)
	{
		/* first elemant of the array (*entry->children) contains the size of the array */
		/* WHY DOES i end up being 2 when *entry->children == 1 ?! */
		for(i = 1; i <= GPOINTER_TO_INT(*entry->children); ++i)
			lxa_archive_entry_free(archive, entry->children[i]);

		g_free(entry->children);
		entry->children = NULL;
	}

	if(props_iter)
	{
		for(i=0; i<archive->n_property; ++i)
		{
			switch(archive->property_types[i])
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
		}
		g_free(entry->props);
	}
	g_free(entry->mime_type);
	g_free(entry->filename);
	g_free(entry);
}

static LXAEntry *
lxa_entry_get_child(const LXAEntry *entry, const gchar *filename)
{
	LXASList *buffer_iter = NULL;
	/* the first element of the array (*entry->children) contains the size of the array */
	guint size = entry->children?GPOINTER_TO_INT(*entry->children):0;
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

static void
lxa_archive_entry_add_child(LXAArchive *archive, LXAEntry *parent, LXAEntry *child)
{
	parent->buffer = lxa_slist_insert_sorted_single(parent->buffer, child);

	if(lxa_slist_length(parent->buffer) == LXA_ENTRY_CHILD_BUFFER_SIZE)
		lxa_archive_entry_flush_buffer(archive, parent);
}

static void
lxa_archive_entry_flush_buffer(LXAArchive *archive, LXAEntry *entry)
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
	guint size = entry->children?GPOINTER_TO_INT(*entry->children):0;
	gint n_children = size;
	LXASList *buffer_iter = NULL;
	LXAEntry **children_old = (LXAEntry **)entry->children;

	max_children = (n_children + lxa_slist_length(entry->buffer));
	
	entry->children = g_new(LXAEntry *, max_children+1);
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
	*entry->children = GINT_TO_POINTER(n_children);
	
	lxa_slist_free(entry->buffer);
	entry->buffer = NULL;

	g_free(children_old);
}

static gpointer
lxa_archive_entry_get_props(LXAArchive *archive, LXAEntry *entry)
{
	guint size = 0;
	guint i;

	if(!entry->props)
	{
		for(i = 0; i < archive->n_property; ++i)
		{
			switch(archive->property_types[i])
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
 * LXAArchiveIter stuff *
 ************************/

/** 
 * gboolean
 * lxa_archive_iter_is_directory(const LXAArchive *, const LXAArchiveIter *)
 *
 * Check if archive entry is a directory
 **/
gboolean
lxa_archive_iter_is_directory(const LXAArchive *archive, const LXAArchiveIter *iter)
{
	if(!strcmp(lxa_archive_iter_get_mime(archive, iter), LXA_MIME_DIRECTORY))
		return TRUE;
	return FALSE;
}

/** 
 * guint
 * lxa_archive_iter_n_children(const LXAArchive *, const LXAArchiveIter *)
 *
 * return number of children
 **/
guint
lxa_archive_iter_n_children(const LXAArchive *archive, const LXAArchiveIter *iter)
{
#ifdef DEBUG
	g_return_val_if_fail(iter, 0);
#endif
	/* the first element of the array (*iter->children) contains the size of the array */
	return iter->children?GPOINTER_TO_INT(*iter->children):0 + lxa_slist_length(iter->buffer);
}

/** 
 * LXAArchiveIter* 
 * lxa_archive_iter_nth_child(const LXAArchive *, const LXAArchiveIter *, guint)
 *
 * return nth child
 **/
LXAArchiveIter*
lxa_archive_iter_nth_child(LXAArchive *archive, LXAArchiveIter *iter, guint n)
{
#ifdef DEBUG
	g_return_val_if_fail(n >= 0, NULL);
#endif
	if(n >= lxa_archive_iter_n_children(archive, iter))
		return NULL;

	lxa_archive_entry_flush_buffer(archive, (LXAEntry *)iter);
	/* the first element of the array (*iter->children) contains the size of the array */
	return ((LXAEntry *)iter->children[n+1]);
}

/**
 * LXAArchiveIter* 
 * lxa_archive_iter_add_child(LXAArchive *, LXAArchiveIter *, const gchar *)
 *
 * return a new child
 **/
LXAArchiveIter *
lxa_archive_iter_add_child(LXAArchive *archive, LXAArchiveIter *parent, const gchar *filename)
{
	LXAEntry *entry = lxa_entry_new(filename);

	lxa_archive_entry_add_child(archive, (LXAEntry *)parent, entry);

	return (LXAArchiveIter*)entry;
}

/**
 * LXAArchiveIter*
 * lxa_archive_iter_get_child(const LXAArchive *, const LXAArchiveIter *, const gchar *)
 *
 * return the child iter if found
 **/
LXAArchiveIter *
lxa_archive_iter_get_child(const LXAArchive *archive, const LXAArchiveIter *parent, const gchar *filename)
{
	return (LXAArchiveIter*)lxa_entry_get_child(parent, filename);
}

/** 
 * gboolean 
 * lxa_archive_iter_del_child(const LXAArchive *, LXAArchiveIter *, LXAArchiveIter *)
 *
 * delete a child (if it can be found)
 **/
gboolean
lxa_archive_iter_del_child(LXAArchive *archive, LXAArchiveIter *parent, LXAArchiveIter *child)
{
	g_warning("not implemented yet");
	return FALSE;
}

/**
 * const gchar *
 * lxa_archive_iter_get_filename(const LXAArchive *, const LXAArchiveIter *)
 *
 * returns filename
 */
const gchar*
lxa_archive_iter_get_filename(const LXAArchive *archive, const LXAArchiveIter *iter)
{
	return ((LXAEntry *)iter)->filename;
}

/**
 * const gchar *
 * lxa_archive_iter_get_mime(const LXAArchive *, const LXAArchiveIter *)
 *
 * returns mime type
 */
const gchar*
lxa_archive_iter_get_mime(const LXAArchive *archive, const LXAArchiveIter *iter)
{
	return ((LXAEntry *)iter)->mime_type;
}

/**
 * void
 * lxa_archive_iter_set_mime(const LXAArchive *, const LXAArchiveIter *, const gchar *)
 *
 * set mime type to entry
 */
void
lxa_archive_iter_set_mime(LXAArchive *archive, LXAArchiveIter *iter, const gchar *mime)
{
	g_free(((LXAEntry *)iter)->mime_type);
	((LXAEntry *)iter)->mime_type = g_strdup(mime);
}

/**
 * void
 * lxa_archive_iter_set_prop_str(const LXAArchive *, const LXAArchiveIter *, guint, const gchar *)
 *
 */
void
lxa_archive_iter_set_prop_str(LXAArchive *archive, LXAArchiveIter *iter, guint i, const gchar *str_val)
{
	gpointer props_iter = NULL;
	guint n;
#ifdef DEBUG
	g_return_if_fail(i < (archive->n_property+LXA_ARCHIVE_PROP_USER));
	if(i >= LXA_ARCHIVE_PROP_USER)
		g_return_if_fail(archive->property_types[i-LXA_ARCHIVE_PROP_USER] == G_TYPE_STRING);
#endif

	switch(i)
	{
		case LXA_ARCHIVE_PROP_FILENAME:
#ifdef DEBUG
			g_critical("DON'T set filename");
#endif
			break;
		case LXA_ARCHIVE_PROP_MIME_TYPE:
			lxa_archive_iter_set_mime(archive, iter, str_val);
			break;
		default:
			props_iter = lxa_archive_entry_get_props(archive, (LXAEntry *)iter);
			for(n = 0; n < (i-LXA_ARCHIVE_PROP_USER); ++n)
			{
				switch(archive->property_types[n])
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
 * lxa_archive_iter_set_prop_uint(const LXAArchive *, const LXAArchiveIter *, guint, guint) 
 *
 */
void
lxa_archive_iter_set_prop_uint(LXAArchive *archive, LXAArchiveIter *iter, guint i, guint int_val)
{
#ifdef DEBUG
	g_return_if_fail(i < (archive->n_property+LXA_ARCHIVE_PROP_USER));
	g_return_if_fail(i >= LXA_ARCHIVE_PROP_USER);
	g_return_if_fail(archive->property_types[i-LXA_ARCHIVE_PROP_USER] == G_TYPE_UINT);
#endif
	gpointer props_iter = lxa_archive_entry_get_props(archive, (LXAEntry *)iter);
	guint n;

	for(n = 0; n < (i-LXA_ARCHIVE_PROP_USER); ++n)
	{
		switch(archive->property_types[n])
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
 * lxa_archive_iter_set_prop_uint64(const LXAArchive *, const LXAArchiveIter *, guint, guint64) 
 *
 */
void
lxa_archive_iter_set_prop_uint64(LXAArchive *archive, LXAArchiveIter *iter, guint i, guint64 int64_val)
{
#ifdef DEBUG
	g_return_if_fail(i < (archive->n_property+LXA_ARCHIVE_PROP_USER));
	g_return_if_fail(i >= LXA_ARCHIVE_PROP_USER);
	g_return_if_fail(archive->property_types[i-LXA_ARCHIVE_PROP_USER] == G_TYPE_UINT64);
#endif
	gpointer props_iter = lxa_archive_entry_get_props(archive, (LXAEntry *)iter);
	guint n;

	for(n = 0; n < (i-LXA_ARCHIVE_PROP_USER); ++n)
	{
		switch(archive->property_types[n])
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
 * lxa_archive_iter_set_prop_value(const LXAArchive *, const LXAArchiveIter *, guint, const GValue *) 
 *
 */
void
lxa_archive_iter_set_prop_value(LXAArchive *archive, LXAArchiveIter *iter, guint i, const GValue *value)
{
	switch(G_VALUE_TYPE(value))
	{
		case G_TYPE_STRING:
			lxa_archive_iter_set_prop_str(archive, iter, i, g_value_get_string(value));
			break;
		case G_TYPE_UINT:
			lxa_archive_iter_set_prop_uint(archive, iter, i, g_value_get_uint(value));
			break;
		case G_TYPE_UINT64:
			lxa_archive_iter_set_prop_uint64(archive, iter, i, g_value_get_uint64(value));
			break;
	}
}

/**
 * void
 * lxa_archive_iter_set_props(const LXAArchive *, const LXAArchiveIter *, guint, ...) 
 *
 */
void
lxa_archive_iter_set_props(LXAArchive *archive, LXAArchiveIter *iter, ...)
{
	gpointer props_iter = lxa_archive_entry_get_props(archive, (LXAEntry *)iter);
	guint i;
	va_list ap;

	va_start(ap, iter);

	for(i = 0; i < archive->n_property; ++i)
	{
		switch(archive->property_types[i])
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
 * lxa_archive_iter_set_propsv(const LXAArchive *, const LXAArchiveIter *, guint, gconstpointer *) 
 *
 */
void
lxa_archive_iter_set_propsv(LXAArchive *archive, LXAArchiveIter *iter, gconstpointer *props)
{
	gpointer props_iter = lxa_archive_entry_get_props(archive, (LXAEntry *)iter);
	guint i;

	for(i = 0; i < archive->n_property; ++i)
	{
		switch(archive->property_types[i])
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
				(*((guint64 *)props_iter)) = *((const guint*)props[i]);
				props_iter += sizeof(guint64);
				break;
		}
	}
}

/**
 * gboolean 
 * lxa_archive_iter_get_prop_value(const LXAArchive *, const LXAArchiveIter *, guint, const GValue *) 
 *
 */
gboolean
lxa_archive_iter_get_prop_value(const LXAArchive *archive, const LXAArchiveIter *iter, guint i, GValue *value)
{
	if(i>=LXA_ARCHIVE_PROP_USER)
		g_value_init(value, archive->property_types[i-LXA_ARCHIVE_PROP_USER]);
	else
		g_value_init(value, G_TYPE_STRING);

	switch(G_VALUE_TYPE(value))
	{
		case G_TYPE_STRING:
			g_value_set_string(value, lxa_archive_iter_get_prop_str(archive, iter, i));
			break;
		case G_TYPE_UINT:
			g_value_set_uint(value, lxa_archive_iter_get_prop_uint(archive, iter, i));
			break;
		case G_TYPE_UINT64:
			g_value_set_uint64(value, lxa_archive_iter_get_prop_uint64(archive, iter, i));
			break;
	}
	return TRUE;
}

/**
 * const gchar *
 * lxa_archive_iter_get_prop_str(const LXAArchive *, const LXAArchiveIter *, guint) 
 *
 */
const gchar*
lxa_archive_iter_get_prop_str(const LXAArchive *archive, const LXAArchiveIter *iter, guint i)
{
	const gchar *retval = NULL;
	gpointer props_iter = NULL;
	guint n;
#ifdef DEBUG
	g_return_val_if_fail(i < (archive->n_property+LXA_ARCHIVE_PROP_USER), NULL);
	if(i >= LXA_ARCHIVE_PROP_USER)
		g_return_val_if_fail(archive->property_types[i-LXA_ARCHIVE_PROP_USER] == G_TYPE_STRING, NULL);
#endif

	switch(i)
	{
		case LXA_ARCHIVE_PROP_FILENAME:
			retval = lxa_archive_iter_get_filename(archive, iter);
			break;
		case LXA_ARCHIVE_PROP_MIME_TYPE:
			retval = lxa_archive_iter_get_mime(archive, iter);
			break;
		default:
			props_iter = ((LXAEntry *)iter)->props;
			if(props_iter)
			{
				for(n = 0; n < (i-LXA_ARCHIVE_PROP_USER); ++n)
				{
					switch(archive->property_types[n])
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
 * guint
 * lxa_archive_iter_get_prop_uint(const LXAArchive *, const LXAArchiveIter *, guint) 
 *
 */
guint
lxa_archive_iter_get_prop_uint(const LXAArchive *archive, const LXAArchiveIter *iter, guint i)
{
	gpointer props_iter = ((LXAEntry *)iter)->props;
	guint n;
#ifdef DEBUG
	g_return_val_if_fail(i < (archive->n_property+LXA_ARCHIVE_PROP_USER), 0);
	g_return_val_if_fail(i >= LXA_ARCHIVE_PROP_USER, 0);
	g_return_val_if_fail(archive->property_types[i-LXA_ARCHIVE_PROP_USER] == G_TYPE_UINT, 0);
#endif
	if(!props_iter)
		return 0;
	for(n = 0; n < (i-LXA_ARCHIVE_PROP_USER); ++n)
	{
		switch(archive->property_types[n])
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
 * guint64
 * lxa_archive_iter_get_prop_uint64(const LXAArchive *, const LXAArchiveIter *, guint) 
 *
 */
guint64
lxa_archive_iter_get_prop_uint64(const LXAArchive *archive, const LXAArchiveIter *iter, guint i)
{
	gpointer props_iter = ((LXAEntry *)iter)->props;
	guint n;
#ifdef DEBUG
	g_return_val_if_fail(i < (archive->n_property+LXA_ARCHIVE_PROP_USER), 0);
	g_return_val_if_fail(i >= LXA_ARCHIVE_PROP_USER, 0);
	g_return_val_if_fail(archive->property_types[i-LXA_ARCHIVE_PROP_USER] == G_TYPE_UINT64, 0);
#endif
	if(!props_iter)
		return 0;
	for(n = 0; n < (i-LXA_ARCHIVE_PROP_USER); ++n)
	{
		switch(archive->property_types[n])
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

