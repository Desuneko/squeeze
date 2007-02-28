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
#include <thunar-vfs/thunar-vfs.h>

#include "libsqueeze-archive.h"
#include "libsqueeze-view.h"
#include "libsqueeze-module.h"
#include "archive-iter.h"
#include "archive-command.h"
#include "archive.h"
#include "slist.h"

#include "internals.h"

#ifndef LSQ_ENTRY_CHILD_BUFFER_SIZE
#define LSQ_ENTRY_CHILD_BUFFER_SIZE 500
#endif

#ifndef LSQ_MIME_DIRECTORY
#define LSQ_MIME_DIRECTORY "inode/directory"
#endif

/*************************
 * LSQArchiveEntry stuff *
 *************************/

static void
lsq_archive_entry_free(const LSQArchive *, LSQArchiveEntry *);

inline static const gchar *
lsq_archive_entry_get_filename(const LSQArchiveEntry *);
inline static const gchar *
lsq_archive_entry_get_mimetype(const LSQArchiveEntry *);

inline static guint
lsq_archive_entry_n_children(const LSQArchiveEntry *);
inline static LSQArchiveEntry*
lsq_archive_entry_nth_child(const LSQArchiveEntry *, guint);
inline static void
lsq_archive_entry_flush_buffer(LSQArchiveEntry *);
static LSQArchiveEntry *
lsq_archive_entry_get_child(const LSQArchiveEntry *, const gchar *);
static LSQArchiveEntry *
lsq_archive_entry_add_child(LSQArchiveEntry *, const gchar *);
static gboolean
lsq_archive_entry_remove_child(LSQArchiveEntry *entry, const gchar *filename);

inline static const gchar*
lsq_archive_entry_get_prop_str(const LSQArchive *, const LSQArchiveEntry *, guint);
inline static guint
lsq_archive_entry_get_prop_uint(const LSQArchive *, const LSQArchiveEntry *, guint);
inline static guint64
lsq_archive_entry_get_prop_uint64(const LSQArchive *, const LSQArchiveEntry *, guint);

static void
lsq_archive_entry_set_prop_str(const LSQArchive *, LSQArchiveEntry *, guint, const gchar *);
static void
lsq_archive_entry_set_prop_uint(const LSQArchive *, LSQArchiveEntry *, guint, guint);
static void
lsq_archive_entry_set_prop_uint64(const LSQArchive *, LSQArchiveEntry *, guint, guint64);
static void
lsq_archive_entry_set_propsv(const LSQArchive *, LSQArchiveEntry *, gconstpointer *);
static void
lsq_archive_entry_set_propsva(const LSQArchive *, LSQArchiveEntry *, va_list);

struct _LSQArchiveEntry
{
	gchar *filename;
	ThunarVfsMimeInfo *mime_info;
	gpointer props;
	LSQArchiveEntry **children;
	LSQSList *buffer;
};

struct _LSQArchiveIter
{
	LSQArchive *archive;
	LSQArchiveEntry *entry;
	LSQArchiveIter *parent;
	guint ref_count;
};

/****************************
 * LSQArchiveIterPool stuff *
 ****************************/

inline static void
lsq_archive_iter_pool_free(LSQArchiveIterPool *pool);

struct _LSQArchiveIterPool
{
	LSQArchiveIter **pool;
	guint size;
	guint reserved;
};

/**************
 * Init stuff *
 **************/

void
lsq_archive_init_iter(LSQArchive *archive)
{
	archive->pool = g_new0(LSQArchiveIterPool, 1);
	archive->root_entry = g_new0(LSQArchiveEntry, 1);
}

void
lsq_archive_free_iter(LSQArchive *archive)
{
	lsq_archive_iter_pool_free(archive->pool);
	lsq_archive_entry_free(archive, archive->root_entry);
}

/****************************
 * LSQArchiveIterPool stuff *
 ****************************/

inline static void
lsq_archive_iter_pool_free(LSQArchiveIterPool *pool)
{
	/* free the pool of iters */
	guint i;
	for(i = 0; i < pool->size; ++i)
	{
		if(!lsq_archive_iter_is_real(pool->pool[i]))
			lsq_archive_entry_free(pool->pool[i]->archive, pool->pool[i]->entry);
		g_free(pool->pool[i]);
	}
	g_free(pool->pool);
	g_free(pool);
}

static gboolean
lsq_archive_iter_pool_find_iter(LSQArchiveIterPool *ipool, LSQArchiveEntry *entry, LSQArchiveIter **ret_iter, guint *ret_pos)
{
	/* binary search */
	LSQArchiveIter **pool = ipool->pool;
	guint size = ipool->size;
	guint pos;
	guint off = 0;
	gint cmp;
	while(size)
	{
		pos = size / 2;
		cmp = entry - pool[off+pos]->entry;
		if(cmp == 0)
		{
			if(ret_iter)
				(*ret_iter) = pool[off+pos];
			if(ret_pos)
				(*ret_pos) = off+pos;
			return TRUE;
		}
		if(cmp > 0)
		{
			size -= ++pos;
			off += pos;
		}
		if(cmp < 0)
		{
			size = pos;
		}
	}
	if(ret_pos)
		(*ret_pos) = off;
	return FALSE;
}

static void
lsq_archive_iter_pool_insert_iter(LSQArchiveIterPool *ipool, LSQArchiveIter *iter, guint pos)
{
	LSQArchiveIter **pool, **old_pool = pool = ipool->pool;
	guint i;

	/* make space for new iter */
	if(ipool->size >= ipool->reserved)
	{
		pool = g_new(LSQArchiveIter*, ipool->reserved + ipool->size + 1);
		for(i = 0; i < pos; ++i)
		{
			pool[i] = old_pool[i];
		}
	}

	/* move all behind the iter */
	for(i = ipool->size; i > pos; --i)
	{
		pool[i] = old_pool[i-1];
	}

	/* finish up the new pool */
	ipool->size++;
	if(ipool->size > ipool->reserved)
	{
		ipool->reserved += ipool->size;
		ipool->pool = pool;
		g_free(old_pool);
	}

	/* insert the iter */
	pool[pos] = iter;
}

static void
lsq_archive_iter_pool_remove_iter(LSQArchiveIterPool *ipool, LSQArchiveIter *iter)
{
	LSQArchiveIter **pool = ipool->pool;
	guint pos;

	/* iter has been found (should allways) */
	if(G_LIKELY(lsq_archive_iter_pool_find_iter(ipool, iter->entry, NULL, &pos)))
	{
		ipool->size--;

		for(; pos < ipool->size; ++pos)
		{
			pool[pos] = pool[pos+1];
		}
	}
}

/************************
 * LSQArchiveIter stuff *
 ************************/

static LSQArchiveIter *
lsq_archive_iter_new(LSQArchiveEntry *entry, LSQArchiveIter *parent, LSQArchive *archive)
{
#ifdef DEBUG
	g_return_val_if_fail(entry, NULL);
#endif

	LSQArchiveIter *iter;
	guint pos;

	/* iter has been found */
	if(lsq_archive_iter_pool_find_iter(archive->pool, entry, &iter, &pos))
	{
		lsq_archive_iter_ref(iter);
		return iter;
	}

#ifdef DEBUG
	if(parent)
		g_return_val_if_fail(parent->archive == archive, NULL);
#endif

	/* create a new iter */
	iter = g_new(LSQArchiveIter, 1);
	iter->archive = archive;
	iter->entry = entry;
	iter->parent = parent;
	iter->ref_count = 1;

	lsq_archive_iter_pool_insert_iter(archive->pool, iter, pos);

	return iter;
}

static LSQArchiveIter *
lsq_archive_iter_get_for_path(LSQArchive *archive, GSList *path)
{
	if(!path)
		return NULL;

	LSQArchiveIter *iter;
	guint pos;

	/* iter has been found */
	if(lsq_archive_iter_pool_find_iter(archive->pool, path->data, &iter, &pos))
	{
		return lsq_archive_iter_ref(iter);
	}

	/* create a new iter */
	iter = g_new(LSQArchiveIter, 1);
	iter->archive = archive;
	iter->entry = path->data;
	iter->parent = lsq_archive_iter_get_for_path(archive, path->next);
	iter->ref_count = 1;

	lsq_archive_iter_pool_insert_iter(archive->pool, iter, pos);

	return iter;
}

static LSQArchiveIter *
lsq_archive_iter_get_with_parent(LSQArchiveEntry *entry, LSQArchiveIter *parent)
{
#ifdef DEBUG
	g_return_val_if_fail(entry, NULL);
#endif

	LSQArchiveIter *iter;
	guint pos;

	/* iter has been found */
	if(lsq_archive_iter_pool_find_iter(parent->archive->pool, entry, &iter, &pos))
	{
		return lsq_archive_iter_ref(iter);
	}

#ifdef DEBUG
	g_return_val_if_fail(parent, NULL);
#endif

	/* create a new iter */
	iter = g_new(LSQArchiveIter, 1);
	iter->archive = parent->archive;
	iter->entry = entry;
	iter->parent = parent;
	iter->ref_count = 1;

	lsq_archive_iter_pool_insert_iter(parent->archive->pool, iter, pos);

	return iter;
}

static void
lsq_archive_iter_free(LSQArchiveIter *iter)
{
#ifdef DEBUG
	g_return_if_fail(iter);
#endif

	lsq_archive_iter_pool_remove_iter(iter->archive->pool, iter);

	/* free the entry if it doesn't exist */
	if(!lsq_archive_iter_is_real(iter))
		lsq_archive_entry_free(iter->archive, iter->entry);

	/* free the iter */
	lsq_archive_iter_unref(iter->parent);
	g_free(iter);
}

void
lsq_archive_iter_unref(LSQArchiveIter* iter)
{
#ifdef DEBUG
	g_return_if_fail(iter);
#endif
	g_return_if_fail(iter->ref_count);
	
	iter->ref_count--;

	/* free the iter if there are no ref's left */
	if(!iter->ref_count)
	{
		lsq_archive_iter_free(iter);
	}
}

LSQArchiveIter *
lsq_archive_iter_ref(LSQArchiveIter* iter)
{
#ifdef DEBUG
	g_return_val_if_fail(iter, iter);
#endif
	g_return_val_if_fail(iter->ref_count, iter);

	iter->ref_count++;

	return iter;
}

gboolean
lsq_archive_iter_is_real(const LSQArchiveIter *iter)
{
#ifdef DEBUG
	g_return_val_if_fail(iter, FALSE);
#endif
	/* reverse the parent list */
	GSList *back_stack = NULL;
	GSList *back_iter;
	const LSQArchiveIter *parent = iter;
	while(parent)
	{
		back_stack = g_slist_prepend(back_stack, (gpointer)parent);
		parent = parent->parent;
	}
	/* the root entry is archive root entry */
	if(((LSQArchiveIter*)back_stack->data)->entry != iter->archive->root_entry)
	{
		g_slist_free(back_stack);
		return FALSE;
	}
	/* find the childeren */
	back_iter = back_stack;
	while(back_iter)
	{
		parent = (LSQArchiveIter*)back_iter->data;
		back_iter = g_slist_next(back_iter);
		if(!back_iter)
			break;
		if(!lsq_archive_entry_get_child(parent->entry, ((LSQArchiveIter*)back_iter->data)->entry->filename))
		{
			g_slist_free(back_stack);
			return FALSE;
		}
	}
	g_slist_free(back_stack);
	return TRUE;
}

LSQArchiveIter *
lsq_archive_iter_get_real_parent(LSQArchiveIter *iter)
{
#ifdef DEBUG
	g_return_val_if_fail(iter, NULL);
#endif
	/* reverse the parent list */
	GSList *back_stack = NULL;
	GSList *back_iter;
	LSQArchiveIter *parent = iter;
	while(parent)
	{
		back_stack = g_slist_prepend(back_stack, parent);
		parent = parent->parent;
	}
	/* the root entry is not archive root entry */
	if(((LSQArchiveIter*)back_stack->data)->entry != iter->archive->root_entry)
	{
		g_slist_free(back_stack);
		return lsq_archive_iter_new(iter->archive->root_entry, NULL, iter->archive);
	}
	/* find the childeren */
	back_iter = back_stack;
	while(back_iter)
	{
		parent = (LSQArchiveIter*)back_iter->data;
		back_iter = g_slist_next(back_iter);
		if(!back_iter)
			break;
		if(!lsq_archive_entry_get_child(parent->entry, iter->entry->filename))
		{
			iter = parent;
			break;
		}
	}
	g_slist_free(back_stack);
	return lsq_archive_iter_ref(iter);
}

gboolean
lsq_archive_iter_is_directory(const LSQArchiveIter *iter)
{
#ifdef debug
	g_return_val_if_fail(iter, FALSE);
#endif
	const gchar *mime = lsq_archive_entry_get_mimetype(iter->entry);
	if(!mime)
		return FALSE;
	if(!strcmp(mime, LSQ_MIME_DIRECTORY))
		return TRUE;
	return FALSE;
}

gboolean
lsq_archive_iter_has_parent(const LSQArchiveIter *iter)
{
	return iter->parent?TRUE:FALSE;
}

LSQArchiveIter *
lsq_archive_iter_get_parent(LSQArchiveIter *iter)
{
	return lsq_archive_iter_ref(iter->parent);
}

guint
lsq_archive_iter_n_children(const LSQArchiveIter *iter)
{
#ifdef DEBUG
	g_return_val_if_fail(iter, 0);
#endif
	return lsq_archive_entry_n_children(iter->entry);
}

LSQArchiveIter *
lsq_archive_iter_nth_child(LSQArchiveIter *parent, guint n)
{
#ifdef DEBUG
	g_return_val_if_fail(parent, NULL);
	g_return_val_if_fail(n >= 0, NULL); /* this can never be, it's unsigned */
#endif
	if(n >= lsq_archive_entry_n_children(parent->entry))
		return NULL;

	LSQArchiveEntry *entry;
	LSQArchiveIter *iter;
	lsq_archive_entry_flush_buffer(parent->entry);
	entry = lsq_archive_entry_nth_child(parent->entry, n);
	iter = lsq_archive_iter_get_with_parent(entry, parent);
	return iter;
}

LSQArchiveIter *
lsq_archive_iter_get_child(LSQArchiveIter *parent, const gchar *filename)
{
#ifdef DEBUG
	g_return_val_if_fail(parent, NULL);
	g_return_val_if_fail(filename, NULL);
#endif
	LSQArchiveEntry *entry;
	LSQArchiveIter *iter;
	entry = lsq_archive_entry_get_child(parent->entry, filename);
	iter = lsq_archive_iter_get_with_parent(entry, parent);
	return iter;
}

LSQArchiveIter *
lsq_archive_iter_add_file(LSQArchiveIter *parent, const gchar *filename)
{
#ifdef DEBUG
	g_return_val_if_fail(parent, NULL);
	g_return_val_if_fail(filename, NULL);
#endif
	LSQArchiveEntry *entry;
	LSQArchiveIter *iter;
	entry = lsq_archive_entry_get_child(parent->entry, filename);
	if(!entry)
		entry = lsq_archive_entry_add_child(parent->entry, filename);
	iter = lsq_archive_iter_get_with_parent(entry, parent);
	return iter;
}

void
lsq_archive_iter_remove(LSQArchiveIter *iter)
{
#ifdef DEBUG
	g_return_if_fail(iter);
	/* don't remove root entry */
	g_return_if_fail(iter->parent);
#endif
	LSQArchiveIter *prev_iter = iter;
	iter = iter->parent;

	while(iter->parent)
	{
		if(iter->entry->props || lsq_archive_entry_n_children(iter->entry) > 1)
			break;

		prev_iter = iter;
		iter = iter->parent;
	}

	gboolean result = lsq_archive_entry_remove_child(iter->entry, prev_iter->entry->filename);
	if(result && !lsq_archive_iter_pool_find_iter(prev_iter->archive->pool, prev_iter->entry, NULL, NULL))
	{
		lsq_archive_entry_free(prev_iter->archive, prev_iter->entry);
	}
}

guint
lsq_archive_iter_get_depth(const LSQArchiveIter *iter)
{
#ifdef debug
	g_return_val_if_fail(iter, 0);
#endif
	guint depth = 0;
	while((iter = iter->parent))
		depth++;
	return depth;
}

gchar*
lsq_archive_iter_get_path(const LSQArchiveIter *iter)
{
#ifdef debug
	g_return_val_if_fail(iter, NULL);
#endif
	const gchar **list;
	gchar *path;
	guint depth = lsq_archive_iter_get_depth(iter);
	
	if(lsq_archive_iter_is_directory(iter))
	{
		list = g_new(const gchar*, depth+2);
		list[depth] = "";
		list[depth+1] = NULL;
	}
	else
	{
		list = g_new(const gchar*, depth+1);
		list[depth] =	NULL;
	}

	while(depth > 0)
	{
		depth--;
		list[depth] = lsq_archive_entry_get_filename(iter->entry);
		iter = iter->parent;
	}

	path = g_strjoinv("/", (gchar**)list);
	
	g_free(list);

	return path;
}

const gchar*
lsq_archive_iter_get_filename(const LSQArchiveIter *iter)
{
#ifdef debug
	g_return_val_if_fail(iter, FALSE);
#endif
	return lsq_archive_entry_get_filename(iter->entry);
}

const gchar*
lsq_archive_iter_get_mime(const LSQArchiveIter *iter)
{
#ifdef debug
	g_return_val_if_fail(iter, FALSE);
#endif
	return lsq_archive_entry_get_mimetype(iter->entry);
}

gboolean
lsq_archive_iter_get_prop_value(const LSQArchiveIter *iter, guint n, GValue *value)
{
#ifdef debug
	g_return_val_if_fail(iter, FALSE);
	g_return_val_if_fail(n < lsq_archive_n_entry_properties(iter->archive), FALSE);
#endif
	if(n>=LSQ_ARCHIVE_PROP_USER)
		g_value_init(value, lsq_archive_get_entry_property_type(iter->archive, n));
	else
		g_value_init(value, G_TYPE_STRING);

	switch(G_VALUE_TYPE(value))
	{
		case G_TYPE_STRING:
			g_value_set_string(value, lsq_archive_entry_get_prop_str(iter->archive, iter->entry, n));
			break;
		case G_TYPE_UINT:
			g_value_set_uint(value, lsq_archive_entry_get_prop_uint(iter->archive, iter->entry, n));
			break;
		case G_TYPE_UINT64:
			g_value_set_uint64(value, lsq_archive_entry_get_prop_uint64(iter->archive, iter->entry, n));
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

void
lsq_archive_iter_set_prop_value(LSQArchiveIter *iter, guint n, const GValue *value)
{
#ifdef DEBUG
	g_return_if_fail(iter);
	g_return_if_fail(n >= LSQ_ARCHIVE_PROP_USER);
	g_return_if_fail(n < lsq_archive_n_entry_properties(iter->archive));
	g_return_if_fail(value);
	g_return_if_fail(G_TYPE_CHECK_VALUE_TYPE(value, lsq_archive_get_entry_property_type(iter->archive, n)));
#endif
	switch(G_VALUE_TYPE(value))
	{
		case G_TYPE_STRING:
			lsq_archive_entry_set_prop_str(iter->archive, iter->entry, n, g_value_get_string(value));
			break;
		case G_TYPE_UINT:
			lsq_archive_entry_set_prop_uint(iter->archive, iter->entry, n, g_value_get_uint(value));
			break;
		case G_TYPE_UINT64:
			lsq_archive_entry_set_prop_uint64(iter->archive, iter->entry, n, g_value_get_uint64(value));
			break;
	}
}

void
lsq_archive_iter_set_prop(LSQArchiveIter *iter, guint n, gconstpointer value)
{
#ifdef DEBUG
	g_return_if_fail(iter);
	g_return_if_fail(n >= LSQ_ARCHIVE_PROP_USER);
	g_return_if_fail(n < lsq_archive_n_entry_properties(iter->archive));
	g_return_if_fail(value);
#endif
	switch(lsq_archive_get_entry_property_type(iter->archive, n))
	{
		case G_TYPE_STRING:
			lsq_archive_entry_set_prop_str(iter->archive, iter->entry, n, g_value_get_string(value));
			break;
		case G_TYPE_UINT:
			lsq_archive_entry_set_prop_uint(iter->archive, iter->entry, n, g_value_get_uint(value));
			break;
		case G_TYPE_UINT64:
			lsq_archive_entry_set_prop_uint64(iter->archive, iter->entry, n, g_value_get_uint64(value));
			break;
	}
}

void
lsq_archive_iter_set_props(LSQArchiveIter *iter, ...)
{
#ifdef DEBUG
	g_return_if_fail(iter);
#endif
	va_list ap;
	va_start(ap, iter);
	lsq_archive_entry_set_propsva(iter->archive, iter->entry, ap);
	va_end(ap);
}

void
lsq_archive_iter_set_propsv(LSQArchiveIter *iter, gconstpointer *props)
{
#ifdef DEBUG
	g_return_if_fail(iter);
	g_return_if_fail(props);
	g_return_if_fail(props[lsq_archive_n_entry_properties(iter->archive) - LSQ_ARCHIVE_PROP_USER] == NULL);
#endif
	lsq_archive_entry_set_propsv(iter->archive, iter->entry, props);
}

LSQArchiveIter *
lsq_archive_get_iter(LSQArchive *archive, const gchar *path)
{
#ifdef debug
	g_return_val_if_fail(archive, NULL);
#endif
	if(!path)
		return lsq_archive_iter_new(archive->root_entry, NULL, archive);

	gchar **buf = g_strsplit_set(path, "/\n", -1);
	gchar **iter = buf;
	LSQArchiveEntry *entry = archive->root_entry;
	GSList *list = g_slist_prepend(NULL, entry);
	LSQArchiveIter *aiter;

	/* ignore '/' if we have no '/' in archive */
	if(path[0] == '/' && lsq_archive_entry_get_child(archive->root_entry, "/"))
	{
		g_free(iter[0]);
		iter[0] = strdup("/");
	}

	/* find entries and make list */
	while(*iter)
	{
		if((*iter)[0])
		{
			entry = lsq_archive_entry_get_child(entry, *iter);
			if(!entry)
			{
				g_strfreev(buf);
				g_slist_free(list);
				return NULL;
			}
			list = g_slist_prepend(list, entry);
		}
		iter++;
	}

	g_strfreev(buf);

	aiter = lsq_archive_iter_get_for_path(archive, list);
	g_slist_free(list);

	return aiter;
}

LSQArchiveIter *
lsq_archive_add_file(LSQArchive *archive, const gchar *path)
{
#ifdef debug
	g_return_val_if_fail(archive, NULL);
#endif
	if(!path)
		return lsq_archive_iter_new(archive->root_entry, NULL, archive);
	
	gchar **buf = g_strsplit_set(path, "/\n", -1);
	gchar **iter = buf;
	LSQArchiveEntry *parent = archive->root_entry;
	LSQArchiveEntry *child;
	gchar *basename;
	GSList *list = g_slist_prepend(NULL, parent);
	LSQArchiveIter *aiter;

	while(*iter)
	{
		basename = g_strconcat(*iter, *(iter+1)?"/":NULL, NULL);

		if(basename[0] != '\0')
		{
			child = lsq_archive_entry_get_child(parent, basename);

			if(!child)
				child = lsq_archive_entry_add_child(parent, basename);

			list = g_slist_prepend(list, child);

			parent = child;
		}

		g_free(basename);

		iter++;
	}

	g_strfreev(buf);

	aiter = lsq_archive_iter_get_for_path(archive, list);
	g_slist_free(list);

	return aiter;
}

gboolean
lsq_archive_remove_file(LSQArchive *archive, const gchar *path)
{
#ifdef DEBUG
	g_return_val_if_fail(archive, FALSE);
	g_return_val_if_fail(path, FALSE);
#endif

	gchar **buf = g_strsplit_set(path, "/\n", -1);
	gchar **iter = buf;
	LSQArchiveEntry *entry = archive->root_entry;
	GSList *prev_iter, *stack_iter, *stack = NULL;

	if(path[0] == '/' && lsq_archive_entry_get_child(archive->root_entry, "/"))
	{
		g_free(iter[0]);
		iter[0] = strdup("/");
	}

	while(*iter)
	{
		if((*iter)[0])
		{
			entry = lsq_archive_entry_get_child(entry, *iter);
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
		entry = (LSQArchiveEntry*)stack_iter->data;

		if(entry->props || lsq_archive_entry_n_children(entry) > 1)
			break;

		prev_iter = stack_iter;
		stack_iter = g_slist_next(stack_iter);
	}

	if(!stack_iter)
	{
		entry = archive->root_entry;
	}

	gboolean result = lsq_archive_entry_remove_child(entry, ((LSQArchiveEntry*)prev_iter->data)->filename);
	if(result && !lsq_archive_iter_pool_find_iter(archive->pool, prev_iter->data, NULL, NULL))
	{
		lsq_archive_entry_free(archive, entry);
	}

	g_slist_free(stack);

	return result;
}

/*************************
 * LSQArchiveEntry stuff *
 *************************/

static LSQArchiveEntry *
lsq_archive_entry_new(const gchar *filename)
{
	LSQArchiveEntry *entry = g_new0(LSQArchiveEntry, 1);

	const gchar *pos = strchr(filename, '/');

	if(pos)
	{
		entry->filename = g_strndup(filename, (gsize)(pos - filename));
		entry->mime_info = thunar_vfs_mime_database_get_info(lsq_mime_database, LSQ_MIME_DIRECTORY);
	}
	else
	{
		entry->filename = g_strdup(filename);
		if(g_utf8_validate (filename, -1, NULL))
		{
			entry->mime_info = thunar_vfs_mime_database_get_info_for_name(lsq_mime_database, entry->filename);
		}
		else
		{
			gchar *utf8_file = g_convert(filename, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
			entry->mime_info = thunar_vfs_mime_database_get_info_for_name(lsq_mime_database, utf8_file);
			g_free(utf8_file);
		}
	}

	return entry;
}

static void
lsq_archive_entry_free(const LSQArchive *archive, LSQArchiveEntry *entry)
{
	guint i = 0; 
	gpointer props_iter = entry->props;
	LSQSList *buffer_iter = entry->buffer;

	/* free the buffer */
	for(; buffer_iter; buffer_iter = buffer_iter->next)
	{
		lsq_archive_entry_free(archive, buffer_iter->entry);
	}
	lsq_slist_free(entry->buffer);
	entry->buffer = NULL;

	/* free the sorted list */
	if(entry->children)
	{
		/* first element of the array (*entry->children) contains the size of the array */
		for(i = 1; i <= GPOINTER_TO_UINT(*entry->children); ++i)
			lsq_archive_entry_free(archive, entry->children[i]);

		g_free(entry->children);
		entry->children = NULL;
	}

	/* free the properties */
	if(props_iter)
	{
		/* walk all properties */
		for(i=0; i<archive->entry_n_property; ++i)
		{
			switch(archive->entry_property_types[i])
			{
				case(G_TYPE_STRING):
					/* free only strings */
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

	/* free the mime info */
	if(entry->mime_info)
		thunar_vfs_mime_info_unref(entry->mime_info);

	/* free the entry */
	g_free(entry->filename);
	g_free(entry);
}

inline static const gchar *
lsq_archive_entry_get_filename(const LSQArchiveEntry *entry)
{
	return entry->filename;
}

inline static const gchar *
lsq_archive_entry_get_mimetype(const LSQArchiveEntry *entry)
{
	if(entry->mime_info)
		return thunar_vfs_mime_info_get_name(entry->mime_info);
	return NULL;
}

inline static guint
lsq_archive_entry_n_children(const LSQArchiveEntry *entry)
{
	/* the first element of the array (*entry->children) contains the size of the array */
	return ((entry->children?GPOINTER_TO_UINT(*entry->children):0) + lsq_slist_length(entry->buffer));
}

inline static LSQArchiveEntry*
lsq_archive_entry_nth_child(const LSQArchiveEntry *entry, guint n)
{
	/* the first element of the array (*entry->children) contains the size of the array */
	return entry->children[n+1];
}

inline static void
lsq_archive_entry_flush_buffer(LSQArchiveEntry *entry)
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
	LSQArchiveEntry **children_old = (LSQArchiveEntry **)entry->children;

	max_children = (n_children + lsq_slist_length(entry->buffer));
	
	/* do all elements of the buffer */
	entry->children = g_new(LSQArchiveEntry *, max_children+1);
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
			/* copy from old to new list */
			while(old_i < begin)
			{
				entry->children[new_i++] = children_old[old_i++];
			}
			entry->children[new_i++] = buffer_iter->entry;
		}
	}
	/* copy tailing from old to new list */
	while(old_i <= n_children)
	{
		entry->children[new_i++] = children_old[old_i++];
	}
	n_children = new_i - 1;
	/* the first element of the array (*entry->children) contains the size of the array */
	*entry->children = GUINT_TO_POINTER(n_children);
	
	/* free the buffer */
	lsq_slist_free(entry->buffer);
	entry->buffer = NULL;

	g_free(children_old);
}

static LSQArchiveEntry *
lsq_archive_entry_get_child(const LSQArchiveEntry *entry, const gchar *filename)
{
	LSQSList *buffer_iter = NULL;
	/* the first element of the array (*entry->children) contains the size of the array */
	guint size = entry->children?GPOINTER_TO_UINT(*entry->children):0;
	guint pos = 0;
	guint begin = 1;
	gint cmp = 0;
	gchar *_filename;
	const gchar *_pos = strchr(filename, '/');

	/* remove trailing '/' */
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

static gint
lsq_archive_entry_filename_compare(LSQArchiveEntry *a, LSQArchiveEntry *b)
{
	return strcmp(a->filename, b->filename);
}

static LSQArchiveEntry *
lsq_archive_entry_add_child(LSQArchiveEntry *parent, const gchar *filename)
{
	LSQArchiveEntry *child = lsq_archive_entry_new(filename);
	const gchar *mime = lsq_archive_entry_get_mimetype(parent);

	if(!mime || strcmp(mime, LSQ_MIME_DIRECTORY))
	{
		if(parent->mime_info)
			thunar_vfs_mime_info_unref(parent->mime_info);
		parent->mime_info = thunar_vfs_mime_database_get_info(lsq_mime_database, LSQ_MIME_DIRECTORY);
	}

	parent->buffer = lsq_slist_insert_sorted_single(parent->buffer, child, (GCompareFunc)lsq_archive_entry_filename_compare);

	if(lsq_slist_length(parent->buffer) == LSQ_ENTRY_CHILD_BUFFER_SIZE)
		lsq_archive_entry_flush_buffer(parent);
	
	return child;
}

static gboolean
lsq_archive_entry_remove_child(LSQArchiveEntry *entry, const gchar *filename)
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

inline static const gchar*
lsq_archive_entry_get_prop_str(const LSQArchive *archive, const LSQArchiveEntry *entry, guint i)
{
	const gchar *retval = NULL;
	gpointer props_iter = NULL;
	guint n;

	switch(i)
	{
		case LSQ_ARCHIVE_PROP_FILENAME:
			retval = lsq_archive_entry_get_filename(entry);
			break;
		case LSQ_ARCHIVE_PROP_MIME_TYPE:
			retval = lsq_archive_entry_get_mimetype(entry);
			break;
		default:
			props_iter = entry->props;
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

inline static guint
lsq_archive_entry_get_prop_uint(const LSQArchive *archive, const LSQArchiveEntry *entry, guint i)
{
	gpointer props_iter = entry->props;
	guint n;
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

inline static guint64
lsq_archive_entry_get_prop_uint64(const LSQArchive *archive, const LSQArchiveEntry *entry, guint i)
{
	gpointer props_iter = entry->props;
	guint n;
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

static gpointer
lsq_archive_entry_get_props(const LSQArchive *archive, LSQArchiveEntry *entry)
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

static void
lsq_archive_entry_set_prop_str(const LSQArchive *archive, LSQArchiveEntry *entry, guint n, const gchar *str_val)
{
	gpointer props_iter = lsq_archive_entry_get_props(archive, entry);
	guint i;

	for(i = 0; i < (n-LSQ_ARCHIVE_PROP_USER); ++i)
	{
		switch(archive->entry_property_types[i])
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
}

static void
lsq_archive_entry_set_prop_uint(const LSQArchive *archive, LSQArchiveEntry *entry, guint n, guint int_val)
{
	gpointer props_iter = lsq_archive_entry_get_props(archive, entry);
	guint i;

	for(i = 0; i < (n-LSQ_ARCHIVE_PROP_USER); ++i)
	{
		switch(archive->entry_property_types[i])
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

static void
lsq_archive_entry_set_prop_uint64(const LSQArchive *archive, LSQArchiveEntry *entry, guint n, guint64 int64_val)
{
	gpointer props_iter = lsq_archive_entry_get_props(archive, entry);
	guint i;

	for(i = 0; i < (n-LSQ_ARCHIVE_PROP_USER); ++i)
	{
		switch(archive->entry_property_types[i])
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

static void
lsq_archive_entry_set_propsv(const LSQArchive *archive, LSQArchiveEntry *entry, gconstpointer *props)
{
	gpointer props_iter = lsq_archive_entry_get_props(archive, entry);
	guint i;

	for(i = 0; i < archive->entry_n_property; ++i)
	{
		switch(archive->entry_property_types[i])
		{
			case G_TYPE_STRING:
				g_free(*((gchar **)props_iter));
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

static void
lsq_archive_entry_set_propsva(const LSQArchive *archive, LSQArchiveEntry *entry, va_list ap)
{
	gpointer props_iter = lsq_archive_entry_get_props(archive, entry);
	guint i;

	for(i = 0; i < archive->entry_n_property; ++i)
	{
		switch(archive->entry_property_types[i])
		{
			case G_TYPE_STRING:
				g_free(*((gchar **)props_iter));
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
}
