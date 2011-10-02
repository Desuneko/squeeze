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

#include <libxfce4util/libxfce4util.h>

#include "libsqueeze.h"
#include "libsqueeze-view.h"
#include "support-factory.h"
#include "btree.h"

#include "internals.h"

#ifndef LSQ_ENTRY_CHILD_BUFFER_SIZE
#define LSQ_ENTRY_CHILD_BUFFER_SIZE 500
#endif

#ifdef LSQ_ENTRY_CHILD_BUFFER_DYNAMIC_SIZE
guint buffer_flush_size = LSQ_ENTRY_CHILD_BUFFER_SIZE;
#undef LSQ_ENTRY_CHILD_BUFFER_SIZE
#define LSQ_ENTRY_CHILD_BUFFER_SIZE buffer_flush_size
#endif

#ifndef LSQ_MIME_DIRECTORY
#define LSQ_MIME_DIRECTORY "inode/directory"
#endif

/*************************
 * LSQArchiveEntry stuff *
 *************************/

static void
lsq_archive_entry_props_free(const LSQArchive *archive, LSQArchiveEntry *entry);
static void
lsq_archive_entry_free(const LSQArchive *, LSQArchiveEntry *);
static void
lsq_archive_entry_save_free(const LSQArchive *, LSQArchiveEntry *);

inline static const gchar *
lsq_archive_entry_get_filename(const LSQArchiveEntry *);
inline static const gchar *
lsq_archive_entry_get_contenttype(const LSQArchiveEntry *);

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
lsq_archive_entry_set_propsv(const LSQArchive *, LSQArchiveEntry *, gpointer *);
static void
lsq_archive_entry_set_propsva(const LSQArchive *, LSQArchiveEntry *, va_list);

struct _LSQArchiveEntry
{
	gchar *filename;
	gchar *content_type;
	gpointer props;
	LSQArchiveEntry **children;
	LSQBTree *buffer;
        guint buffer_length;
};


/**************
 * Init stuff *
 **************/
volatile LSQArchiveIterPool* pool;

void
lsq_archive_init_iter(LSQArchive *archive)
{
	pool = archive->pool = lsq_archive_iter_pool_new();
	archive->root_entry = g_new0(LSQArchiveEntry, 1);
}

void
lsq_archive_free_iter(LSQArchive *archive)
{
	lsq_archive_iter_pool_free(archive->pool);
	lsq_archive_entry_free(archive, archive->root_entry);
}

/************************
 * LSQArchiveIter stuff *
 ************************/

static LSQArchiveIter *
lsq_archive_iter_new(LSQArchiveEntry *entry, LSQArchiveIter *parent, LSQArchive *archive)
{
	/* create a new iter */
	LSQArchiveIter *iter;
#ifdef USE_LSQITER_SLICES
	/* Lets see if there is an iter we can use */
	if(lsq_archive_iter_pool_get_size(archive->pool) >= lsq_archive_iter_pool_get_reserved(archive->pool) || 
	   !(iter = lsq_archive_iter_pool_get_iter(archive->pool, lsq_archive_iter_pool_get_size(archive->pool))))
	{
		/* No iter found, make a new one */
#ifdef USE_GSLICES
			iter = g_slice_new(LSQArchiveIter);
#else
			iter = g_new(LSQArchiveIter, 1);
#endif
	}
#elif defined(USE_GSLICES)
	iter = g_slice_new(LSQArchiveIter);
#else
	iter = g_new(LSQArchiveIter, 1);
#endif
	iter->archive = archive;
	iter->entry = entry;
	iter->parent = parent?lsq_archive_iter_ref(parent):NULL;
	iter->ref_count = 1;

	return iter;
}

static LSQArchiveIter *
lsq_archive_iter_get_for_path(LSQArchive *archive, GSList *path)
{
	LSQArchiveIter *iter;
	guint pos;

	if ( NULL == path )
		return NULL;

	/* iter has been found */
	if (TRUE == lsq_archive_iter_pool_find_iter(archive->pool, path->data, &iter, &pos))
	{
		return lsq_archive_iter_ref(iter);
	}

	/* create a new iter */
	iter = lsq_archive_iter_new(path->data, NULL, archive);

	lsq_archive_iter_pool_insert_iter(archive->pool, iter, pos);

	/* must be done here, otherwise the pool gets currupted */
	iter->parent = lsq_archive_iter_get_for_path(archive, path->next);

	return iter;
}

static LSQArchiveIter *
lsq_archive_iter_get_with_archive(LSQArchiveEntry *entry, LSQArchiveIter *parent, LSQArchive *archive)
{
	LSQArchiveIter *iter;
	guint pos;

#ifdef DEBUG
	g_return_val_if_fail(entry, NULL);
#endif

	/* iter has been found */
	if ( TRUE == lsq_archive_iter_pool_find_iter(archive->pool, entry, &iter, &pos) )
	{
		return lsq_archive_iter_ref(iter);
	}

#ifdef DEBUG
	if ( NULL != parent )
	{
		g_return_val_if_fail ( parent->archive == archive, NULL );
	}
#endif

	/* create a new iter */
	iter = lsq_archive_iter_new(entry, parent, archive);

	lsq_archive_iter_pool_insert_iter(archive->pool, iter, pos);

	return iter;
}

static LSQArchiveIter *
lsq_archive_iter_get_with_parent(LSQArchiveEntry *entry, LSQArchiveIter *parent)
{
	LSQArchiveIter *iter;
	guint pos;

#ifdef DEBUG
	g_return_val_if_fail(entry, NULL);
#endif

	/* iter has been found */
	if ( TRUE == lsq_archive_iter_pool_find_iter(parent->archive->pool, entry, &iter, &pos) )
	{
		return lsq_archive_iter_ref(iter);
	}

#ifdef DEBUG
	g_return_val_if_fail(parent, NULL);
#endif

	/* create a new iter */
	iter = lsq_archive_iter_new(entry, parent, parent->archive);

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
	if ( FALSE == lsq_archive_iter_is_real(iter) )
	{
		lsq_archive_entry_free(iter->archive, iter->entry);
	}

	/* free the iter */
	if ( NULL != iter->parent )
	{
		lsq_archive_iter_unref(iter->parent);
	}
#ifdef USE_LSQITER_SLICES
	/* We don't free the poiter we moved it */
#elif defined(USE_GSLICES)
	g_slice_free(LSQArchiveIter, iter);
#else
	g_free(iter);
#endif
}

#ifdef DEBUG
void
_lsq_archive_iter_unref(LSQArchiveIter* iter, const gchar *file, int line)
{
	if ( ( NULL == iter ) || ( 0 == iter->ref_count ) )
	{
		g_debug("unref: %p file: %s line: %d", iter, file, line);
	}

	g_return_if_fail(iter);
	g_return_if_fail(iter->ref_count);
	
	iter->ref_count--;

	/* free the iter if there are no ref's left */
	if ( 0 == iter->ref_count )
	{
		lsq_archive_iter_free(iter);
	}
}
#endif

void
lsq_archive_iter_unref(LSQArchiveIter* iter)
{
#ifdef DEBUG
	g_return_if_fail(iter);
#endif
	g_return_if_fail(iter->ref_count);
	
	iter->ref_count--;

	/* free the iter if there are no ref's left */
	if ( 0 == iter->ref_count )
	{
		lsq_archive_iter_free(iter);
	}
}

#ifdef DEBUG
LSQArchiveIter *
_lsq_archive_iter_ref(LSQArchiveIter* iter, const gchar *file, int line)
{
	if ( ( NULL == iter ) || ( 0 == iter->ref_count ) )
	{
		g_debug("ref: %p file: %s line: %d", iter, file, line);
	}

	g_return_val_if_fail(iter, NULL);
	g_return_val_if_fail(iter->ref_count, NULL);

	iter->ref_count++;

	return iter;
}
#endif

LSQArchiveIter *
lsq_archive_iter_ref(LSQArchiveIter* iter)
{
#ifdef DEBUG
	g_return_val_if_fail(iter, iter);
#endif
	g_return_val_if_fail(iter->ref_count, NULL);

	iter->ref_count++;

	return iter;
}

gboolean
lsq_archive_iter_is_real(const LSQArchiveIter *iter)
{
	const LSQArchiveIter *parent;
#ifdef DEBUG
	g_return_val_if_fail(iter, FALSE);
#endif
	/* reverse the parent list */
	parent = iter->parent;
	if ( G_UNLIKELY ( NULL == parent ) )
	{
		/* the root entry is archive root entry */
		if ( G_UNLIKELY ( iter->entry != iter->archive->root_entry ) )
		{
			return FALSE;
		}
	}
	else
	{
		/* find the childeren */
		if ( G_UNLIKELY ( 
				iter->entry != lsq_archive_entry_get_child ( 
						parent->entry,
						lsq_archive_entry_get_filename(iter->entry) ) 
				) 
		)
		{
			return FALSE;
		}
	}
	return TRUE;
}

LSQArchiveIter *
lsq_archive_iter_get_real_parent(LSQArchiveIter *iter)
{
	GSList *back_stack = NULL;
	GSList *back_iter;
	LSQArchiveIter *parent;
	LSQArchiveEntry *entry;
	GSList *list;
#ifdef DEBUG
	g_return_val_if_fail(iter, NULL);
#endif
	/* reverse the parent list */
	parent = iter;
	while ( NULL != parent )
	{
		back_stack = g_slist_prepend(back_stack, parent);
		parent = parent->parent;
	}
	/* the root entry is not archive root entry */
	if ( ((LSQArchiveIter*)back_stack->data)->entry != iter->archive->root_entry )
	{
		/* TODO: Should do iter recovery here too? */
		g_slist_free(back_stack);
		return lsq_archive_iter_get_with_archive(iter->archive->root_entry, NULL, iter->archive);
	}
	list = g_slist_prepend(NULL, entry = iter->archive->root_entry);
	/* find the childeren */
	for(back_iter = g_slist_next(back_stack); NULL != back_iter; back_iter = g_slist_next(back_iter))
	{
		if(!(entry = lsq_archive_entry_get_child(entry, lsq_archive_entry_get_filename(((LSQArchiveIter*)back_iter->data)->entry))))
			break;
		list = g_slist_prepend(list, entry);
	}
	g_slist_free(back_stack);
	iter = lsq_archive_iter_get_for_path(iter->archive, list);
	g_slist_free(list);
	return iter;
}

gboolean
lsq_archive_iter_is_directory(const LSQArchiveIter *iter)
{
	const gchar *contenttype;
#ifdef debug
	g_return_val_if_fail(iter, FALSE);
#endif
	contenttype = lsq_archive_entry_get_contenttype(iter->entry);
	if ( NULL == contenttype )
	{
		return FALSE;
	}
	if ( 0 == strcmp ( contenttype, LSQ_MIME_DIRECTORY ) )
	{
		return TRUE;
	}
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
#ifdef DEBUG
	g_return_val_if_fail(iter, NULL);
#endif
	return iter->parent?lsq_archive_iter_ref(iter->parent):NULL;
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
	LSQArchiveEntry *entry;
	LSQArchiveIter *iter;
#ifdef DEBUG
	g_return_val_if_fail(parent, NULL);
#endif
	if ( n >= lsq_archive_entry_n_children(parent->entry) )
	{
		return NULL;
	}

	lsq_archive_entry_flush_buffer(parent->entry);
	entry = lsq_archive_entry_nth_child(parent->entry, n);
	iter = lsq_archive_iter_get_with_parent(entry, parent);
	return iter;
}

LSQArchiveIter *
lsq_archive_iter_get_child(LSQArchiveIter *parent, const gchar *filename)
{
	LSQArchiveEntry *entry;
	LSQArchiveIter *iter;
#ifdef DEBUG
	g_return_val_if_fail(parent, NULL);
	g_return_val_if_fail(filename, NULL);
#endif
	entry = lsq_archive_entry_get_child(parent->entry, filename);
	iter = lsq_archive_iter_get_with_parent(entry, parent);
	return iter;
}

LSQArchiveIter *
lsq_archive_iter_add_file(LSQArchiveIter *parent, const gchar *filename)
{
	LSQArchiveEntry *entry;
	LSQArchiveIter *iter;
#ifdef DEBUG
	g_return_val_if_fail(parent, NULL);
	g_return_val_if_fail(filename, NULL);
#endif
	entry = lsq_archive_entry_get_child(parent->entry, filename);
	if ( NULL == entry )
	{
		entry = lsq_archive_entry_add_child(parent->entry, filename);
	}
	iter = lsq_archive_iter_get_with_parent(entry, parent);
	return iter;
}

void
lsq_archive_iter_remove(LSQArchiveIter *iter, gboolean recursive)
{
	LSQArchiveIter *prev_iter;
#ifdef DEBUG
	g_return_if_fail(iter);
	/* don't remove root entry */
	g_return_if_fail(iter->parent);
#endif
	prev_iter = iter;
	iter = iter->parent;

	if ( 0 == lsq_archive_entry_n_children(iter->entry) )
	{
		recursive = TRUE;
	}

	if ( TRUE == recursive )
	{
		gboolean result;
		while ( NULL != iter->parent )
		{
			if ( ( NULL != iter->entry->props ) || ( 1 < lsq_archive_entry_n_children(iter->entry) ) )
			{
				break;
			}

			prev_iter = iter;
			iter = iter->parent;
		}

		result = lsq_archive_entry_remove_child(iter->entry, lsq_archive_entry_get_filename(prev_iter->entry));
		if ( ( TRUE == result ) && 
			 ( FALSE == lsq_archive_iter_pool_find_iter(prev_iter->archive->pool, prev_iter->entry, NULL, NULL) ) )
		{
			lsq_archive_entry_free(prev_iter->archive, prev_iter->entry);
		}
		else
		{
			lsq_archive_entry_save_free(prev_iter->archive, prev_iter->entry);
		}
	}
	else
	{
		lsq_archive_entry_props_free(iter->archive, iter->entry);
	}
}

guint
lsq_archive_iter_get_depth(const LSQArchiveIter *iter)
{
	guint depth = 0;

#ifdef DEBUG
	g_return_val_if_fail(iter, 0);
#endif

	while ( NULL != iter )
	{
		iter = iter->parent;
		depth++;
	}
	return depth;
}

gchar*
lsq_archive_iter_get_path(const LSQArchiveIter *iter)
{
	const gchar **list;
	gchar *path;
	guint depth;

#ifdef DEBUG
	g_return_val_if_fail(iter, NULL);
#endif

	depth = lsq_archive_iter_get_depth(iter);
	
	if ( TRUE == lsq_archive_iter_is_directory(iter) )
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

	while ( 0 < depth )
	{
		depth--;
		list[depth] = lsq_archive_entry_get_filename(iter->entry);
		iter = iter->parent;
	}

	if ( NULL != list[0] )
	{
		path = g_strjoinv("/", (gchar**)list);
	}
	else
	{
		path = g_strjoinv("/", (gchar **)&list[1]);
	}
	
	g_free(list);

	return path;
}

const gchar*
lsq_archive_iter_get_filename(const LSQArchiveIter *iter)
{
#ifdef DEBUG
	g_return_val_if_fail(iter, NULL);
#endif
	return lsq_archive_entry_get_filename(iter->entry);
}

const gchar*
lsq_archive_iter_get_contenttype(const LSQArchiveIter *iter)
{
#ifdef DEBUG
	g_return_val_if_fail(iter, FALSE);
#endif
	return lsq_archive_entry_get_contenttype(iter->entry);
}

gboolean
lsq_archive_iter_get_prop_value(const LSQArchiveIter *iter, guint n, GValue *value)
{
#ifdef DEBUG
	g_return_val_if_fail(iter, FALSE);
	g_return_val_if_fail(n < lsq_archive_n_entry_properties(iter->archive), FALSE);
#endif
	if ( LSQ_ARCHIVE_PROP_USER <= n )
	{
		g_value_init(value, lsq_archive_get_entry_property_type(iter->archive, n));
	}
	else
	{
		g_value_init(value, G_TYPE_STRING);
	}

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
			lsq_archive_entry_set_prop_str(iter->archive, iter->entry, n, value);
			break;
		case G_TYPE_UINT:
			lsq_archive_entry_set_prop_uint(iter->archive, iter->entry, n, *(const guint*)value);
			break;
		case G_TYPE_UINT64:
			lsq_archive_entry_set_prop_uint64(iter->archive, iter->entry, n, *(const guint64*)value);
			break;
	}
}

void
lsq_archive_iter_set_props(LSQArchiveIter *iter, ...)
{
	va_list ap;
#ifdef DEBUG
	g_return_if_fail(iter);
#endif
	va_start(ap, iter);
	lsq_archive_entry_set_propsva(iter->archive, iter->entry, ap);
	va_end(ap);
}

void
lsq_archive_iter_set_propsv(LSQArchiveIter *iter, gpointer *props)
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
	gchar **buf;
	gchar **iter;
	LSQArchiveEntry *entry;
	GSList *list;
	LSQArchiveIter *aiter;
#ifdef debug
	g_return_val_if_fail(archive, NULL);
#endif
	if ( NULL == path )
	{
		return lsq_archive_iter_get_with_archive(archive->root_entry, NULL, archive);
	}

	buf = g_strsplit_set(path, "/\n", -1);
	iter = buf;
	entry = archive->root_entry;
	list = g_slist_prepend(NULL, entry);

	/* ignore '/' if we have no '/' in archive */
	if ( ( '/' == path[0] ) && ( NULL != lsq_archive_entry_get_child(archive->root_entry, "/") ) )
	{
		g_free(iter[0]);
		iter[0] = strdup("/");
	}

	/* find entries and make list */
	while ( NULL != *iter )
	{
		if ( 0 != (*iter)[0] )
		{
			entry = lsq_archive_entry_get_child(entry, *iter);
			if ( NULL == entry )
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
	gchar **buf;
	gchar **iter;
	LSQArchiveEntry *parent;
	LSQArchiveEntry *child;
	gchar *basefname;
	GSList *list;
	LSQArchiveIter *aiter;
#ifdef debug
	g_return_val_if_fail(archive, NULL);
#endif
	if ( NULL == path )
	{
		return lsq_archive_iter_get_with_archive(archive->root_entry, NULL, archive);
	}
	
	buf = g_strsplit_set(path, "/\n", -1);
	iter = buf;
	parent = archive->root_entry;
	list = g_slist_prepend(NULL, parent);

	while ( NULL != *iter )
	{
		basefname = g_strconcat(*iter, *(iter+1)?"/":NULL, NULL);

		if ( '\0' != basefname[0] )
		{
			child = lsq_archive_entry_get_child(parent, basefname);

			if ( NULL == child )
			{
				child = lsq_archive_entry_add_child(parent, basefname);
			}

			list = g_slist_prepend(list, child);

			parent = child;
		}

		g_free(basefname);

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
	gchar **buf;
	gchar **iter;
	LSQArchiveEntry *entry;
	GSList *prev_iter, *stack_iter, *stack = NULL;
	gboolean result;

#ifdef DEBUG
	g_return_val_if_fail(archive, FALSE);
	g_return_val_if_fail(path, FALSE);
#endif

	buf = g_strsplit_set(path, "/\n", -1);
	iter = buf;
	entry = archive->root_entry;

	if ( ( '/' == path[0] ) && ( NULL != lsq_archive_entry_get_child(archive->root_entry, "/") ) )
	{
		g_free(iter[0]);
		iter[0] = strdup("/");
	}

	while ( NULL != *iter )
	{
		if ( 0 != (*iter)[0] )
		{
			entry = lsq_archive_entry_get_child(entry, *iter);
			if ( NULL == entry )
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

	while ( NULL != stack_iter )
	{
		entry = (LSQArchiveEntry*)stack_iter->data;

		if ( (NULL != entry->props ) || ( 1 < lsq_archive_entry_n_children(entry) ) )
		{
			break;
		}

		prev_iter = stack_iter;
		stack_iter = g_slist_next(stack_iter);
	}

	if ( NULL == stack_iter )
	{
		entry = archive->root_entry;
	}

	result = lsq_archive_entry_remove_child(entry, lsq_archive_entry_get_filename((LSQArchiveEntry*)prev_iter->data));
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

	if ( NULL != pos )
	{
		entry->filename = g_strndup(filename, (gsize)(pos - filename));
		entry->content_type = g_strdup(LSQ_MIME_DIRECTORY);
	}
	else
	{
		entry->filename = g_strdup(filename);
		if ( TRUE == g_utf8_validate (filename, -1, NULL) )
		{
			entry->content_type = g_content_type_guess(entry->filename, NULL, 0, NULL);
		}
		else
		{
			gchar *utf8_file = g_convert(filename, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL);
			entry->content_type = g_content_type_guess(utf8_file, NULL, 0, NULL);
			g_free(utf8_file);
		}
	}

	return entry;
}

static void
lsq_archive_entry_props_free(const LSQArchive *archive, LSQArchiveEntry *entry)
{
	guint i;
	guint offset;
	gpointer props_iter = entry->props;
	/* free the properties */
	if ( NULL != props_iter )
	{
		/* walk all properties */
		for ( i=0 ; i < (lsq_archive_n_entry_properties(archive) - LSQ_ARCHIVE_PROP_USER) ; ++i )
		{
			switch ( lsq_archive_get_entry_property_type(archive, i+LSQ_ARCHIVE_PROP_USER) )
			{
				case ( G_TYPE_STRING ):
					/* free only strings */
					offset = lsq_archive_get_entry_property_offset(archive, i+LSQ_ARCHIVE_PROP_USER);
					g_free(((gchar **)props_iter)[offset]);
					((gchar **)props_iter)[offset] = NULL;
					break;
			}
		}
		g_free(entry->props);
		entry->props = NULL;
	}
}

static void
lsq_archive_entry_save_free(const LSQArchive *archive, LSQArchiveEntry *entry)
{
	guint i = 0; 
	LSQBTree *buffer_iter;

	entry->buffer = lsq_btree_flatten(entry->buffer);
	buffer_iter = entry->buffer;

	/* free the buffer */
	for ( ; NULL != buffer_iter ; buffer_iter = buffer_iter->next )
	{
		if(!lsq_archive_iter_pool_find_iter(archive->pool, buffer_iter->entry, NULL, NULL))
			lsq_archive_entry_free(archive, buffer_iter->entry);
		else
			lsq_archive_entry_save_free(archive, buffer_iter->entry);
	}
	lsq_btree_free(entry->buffer);
	entry->buffer = NULL;
        entry->buffer_length = 0;

	/* free the sorted list */
	if ( NULL != entry->children )
	{
		/* first element of the array (*entry->children) contains the size of the array */
		for ( i = 1 ; i <= GPOINTER_TO_UINT(*entry->children) ; ++i )
		{
			if(!lsq_archive_iter_pool_find_iter(archive->pool, entry->children[i], NULL, NULL))
				lsq_archive_entry_free(archive, entry->children[i]);
			else
				lsq_archive_entry_save_free(archive, entry->children[i]);
		}

		g_free(entry->children);
		entry->children = NULL;
	}

	/* free the properties */
	lsq_archive_entry_props_free(archive, entry);
}

static void
lsq_archive_entry_free(const LSQArchive *archive, LSQArchiveEntry *entry)
{
	guint i = 0; 
	LSQBTree *buffer_iter;

	entry->buffer = lsq_btree_flatten(entry->buffer);
	buffer_iter = entry->buffer;

	/* free the buffer */
	for ( ; NULL != buffer_iter; buffer_iter = buffer_iter->next )
	{
		lsq_archive_entry_free(archive, buffer_iter->entry);
	}
	lsq_btree_free(entry->buffer);
	entry->buffer = NULL;
        entry->buffer_length = 0;

	/* free the sorted list */
	if ( NULL != entry->children )
	{
		/* first element of the array (*entry->children) contains the size of the array */
		for(i = 1; i <= GPOINTER_TO_UINT(*entry->children); ++i)
			lsq_archive_entry_free(archive, entry->children[i]);

		g_free(entry->children);
		entry->children = NULL;
	}

	/* free the properties */
	lsq_archive_entry_props_free(archive, entry);

	/* free the content type */
	if ( NULL != entry->content_type )
	{
		g_free (entry->content_type);
		entry->content_type = NULL;
	}

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
lsq_archive_entry_get_contenttype(const LSQArchiveEntry *entry)
{
	return entry->content_type;
}

inline static guint
lsq_archive_entry_n_children(const LSQArchiveEntry *entry)
{
	/* the first element of the array (*entry->children) contains the size of the array */
	return ((entry->children?GPOINTER_TO_UINT(*entry->children):0) + lsq_btree_length(entry->buffer));
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
	guint max_children;
	guint begin = 1;
	guint pos = 0;
	gint cmp = 1;
	guint old_i = 1;
	guint new_i = 1;
	guint size;
	guint n_children;
	LSQBTree *buffer_iter = NULL;
	LSQArchiveEntry **children_old;

	if ( NULL == entry->buffer )
	{
		return;
	}

	//lsq_btree_print(entry->buffer);

	/* Flatten the btree so we can iterate */
	entry->buffer = lsq_btree_flatten(entry->buffer);

	/* the first element of the array (*entry->children) contains the size of the array */
	size = entry->children?GPOINTER_TO_UINT(*entry->children):0;
	n_children = size;
	children_old = (LSQArchiveEntry **)entry->children;

	max_children = (n_children + lsq_btree_length(entry->buffer));
	
	/* do all elements of the buffer */
	entry->children = g_new(LSQArchiveEntry *, max_children+1);
	for ( buffer_iter = entry->buffer ; NULL != buffer_iter ; buffer_iter = buffer_iter->next )
	{
		size = n_children + 1 - begin;
		/* binary search algoritme */
		while ( 0 != size )
		{
			pos = (size / 2);

			cmp = strcmp(buffer_iter->entry->filename, children_old[begin+pos]->filename);
			if ( 0 == cmp )
			{
				break;
			}

			if ( 0 > cmp )
			{
				size = pos;
			}
			else
			{
				size -= ++pos;
				begin += pos;
			}
		}
		if ( 0 == cmp )
		{
			g_critical("THIS SHOULD NOT HAPPEN!!! (the universe has just collapsed)");
		}
		else
		{
			/* copy from old to new list */
			while ( old_i < begin )
			{
				entry->children[new_i++] = children_old[old_i++];
			}
			entry->children[new_i++] = buffer_iter->entry;
		}
	}
	/* copy tailing from old to new list */
	while ( old_i <= n_children )
	{
		entry->children[new_i++] = children_old[old_i++];
	}
	n_children = new_i - 1;
	/* the first element of the array (*entry->children) contains the size of the array */
	*entry->children = GUINT_TO_POINTER(n_children);
	
	/* free the buffer */
	lsq_btree_free(entry->buffer);
	entry->buffer = NULL;
        entry->buffer_length = 0;

	g_free(children_old);
}

static LSQArchiveEntry *
lsq_archive_entry_get_child(const LSQArchiveEntry *entry, const gchar *filename)
{
	LSQBTree *buffer_iter = NULL;
	/* the first element of the array (*entry->children) contains the size of the array */
	guint size = entry->children?GPOINTER_TO_UINT(*entry->children):0;
	guint pos = 0;
	guint begin = 1;
	gint cmp = 0;
	gchar *_filename = NULL;
	const gchar *_pos = strchr(filename, '/');

	/* remove trailing '/' */
	if ( 0 != _pos )
	{
		_filename = g_strndup(filename, (gsize)(_pos - filename));
		filename = _filename;
	}

	/* binary search algoritme */
	while ( 0 != size )
	{
		pos = (size / 2);

		cmp = strcmp(filename, entry->children[begin+pos]->filename);
		if ( 0 == cmp )
		{
			g_free(_filename);
			return entry->children[begin+pos];
		}

		if ( 0 > cmp )
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
	buffer_iter = entry->buffer;
	while ( NULL != buffer_iter )
	{
		/* archive can be NULL */
		cmp = strcmp(filename, buffer_iter->entry->filename);

		if ( cmp < 0 )
		{
		    buffer_iter = buffer_iter->left;
		}
		else if ( cmp > 0 )
		{
		    buffer_iter = buffer_iter->right;
		}
		else
		{
		    g_free(_filename);
		    return buffer_iter->entry;
		}
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
	const gchar *contenttype = lsq_archive_entry_get_contenttype(parent);

	if ( ( NULL == contenttype ) || ( 0 != strcmp ( contenttype, LSQ_MIME_DIRECTORY ) ) )
	{
		/*
		if(parent->content_type)
			thunar_vfs_mime_info_unref(parent->mime_info);
		parent->mime_info = thunar_vfs_mime_database_get_info(lsq_mime_database, LSQ_MIME_DIRECTORY);
		*/
	}

	//g_debug("i: %s", filename);
	parent->buffer = lsq_btree_insert_sorted_single(parent->buffer, child, (GCompareFunc)lsq_archive_entry_filename_compare);

	/* Cache the length so we doen't have to check every time */
	parent->buffer_length++;

	if ( LSQ_ENTRY_CHILD_BUFFER_SIZE == parent->buffer_length )
	{
		lsq_archive_entry_flush_buffer(parent);
	}
	
	return child;
}

static gboolean
lsq_archive_entry_remove_child(LSQArchiveEntry *entry, const gchar *filename)
{
	LSQBTree *buffer_iter = NULL, **prev_iter = NULL;

	/* the first element of the array (*entry->children) contains the size of the array */
	guint total_size, size = total_size = entry->children?GPOINTER_TO_UINT(*entry->children):0;
	guint pos = 0;
	guint begin = 1;
	gint cmp = 0;
	const gchar *_pos = strchr(filename, '/');
	gchar *_filename;

	if ( 0 != _pos )
	{
		_filename = g_strndup(filename, (gsize)(_pos - filename));
	}
	else
	{
		_filename = g_strdup(filename);
	}


	/* binary search algoritme */
	while ( 0 != size )
	{
		pos = (size / 2);

		cmp = strcmp(_filename, entry->children[begin+pos]->filename);
		if ( 0 == cmp )
		{
			g_free(_filename);
			for ( ;(begin + pos) < total_size; ++pos )
			{
				entry->children[begin+pos] = entry->children[begin+pos+1];
			}
			total_size -= 1;
			*entry->children = GUINT_TO_POINTER(total_size);
			return TRUE;
		}

		if ( 0 > cmp )
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
        prev_iter = &entry->buffer;
        if ( NULL == entry->buffer || NULL != entry->buffer->next )
        {
            for(buffer_iter = entry->buffer; buffer_iter; buffer_iter = buffer_iter->next)
            {
                cmp = strcmp(_filename, buffer_iter->entry->filename);

                if ( 0 == cmp )
                {
                    g_free(_filename);
                    *prev_iter = buffer_iter->next;
                    g_free(buffer_iter);
                    return TRUE;
                }
                if(cmp < 0)
                    break;
                prev_iter = &buffer_iter->next;
            }
        }
        else
        {
            for ( buffer_iter = entry->buffer; NULL != buffer_iter; buffer_iter = *prev_iter )
            {
                /* archive can be NULL */
                cmp = strcmp(filename, buffer_iter->entry->filename);

                if ( cmp < 0 )
                {
                    prev_iter = &buffer_iter->left;
                }
                else if ( cmp > 0 )
                {
                    prev_iter = &buffer_iter->right;
                }
                else
                {
                    g_critical("todo");
                    return TRUE;
                }
            }
        }

	g_free(_filename);
	return FALSE;
}

inline static const gchar*
lsq_archive_entry_get_prop_str(const LSQArchive *archive, const LSQArchiveEntry *entry, guint i)
{
	const gchar *retval = NULL;
	gpointer props_iter = NULL; 
	guint offset;

	switch(i)
	{
		case LSQ_ARCHIVE_PROP_FILENAME:
			retval = lsq_archive_entry_get_filename(entry);
			break;
		case LSQ_ARCHIVE_PROP_MIME_TYPE:
			retval = lsq_archive_entry_get_contenttype(entry);
			break;
		default:
			props_iter = entry->props;
			if(props_iter)
			{
			  if ( G_TYPE_STRING == lsq_archive_get_entry_property_type(archive, i) )
			  {
				offset = lsq_archive_get_entry_property_offset(archive, i);

				retval = (((gchar **)props_iter)[offset]);
			  }
			}
			break;
	}
	return retval;
}

inline static guint
lsq_archive_entry_get_prop_uint(const LSQArchive *archive, const LSQArchiveEntry *entry, guint i)
{
	gpointer props_iter = entry->props;
	guint offset;
	if ( NULL == props_iter )
	{
		return 0;
	}

	if ( G_TYPE_UINT != lsq_archive_get_entry_property_type(archive, i) )
	{
		return 0;
	}

	offset = lsq_archive_get_entry_property_offset(archive, i);

	return (((guint *)props_iter)[offset]);
}

inline static guint64
lsq_archive_entry_get_prop_uint64(const LSQArchive *archive, const LSQArchiveEntry *entry, guint i)
{
	gpointer props_iter = entry->props;
	guint offset;
	if ( NULL == props_iter )
	{
		return 0;
	}

	if ( G_TYPE_UINT64 != lsq_archive_get_entry_property_type(archive, i) )
	{
		return 0;
	}

	offset = lsq_archive_get_entry_property_offset(archive, i);

	return (((guint64 *)props_iter)[offset]);
}

static gpointer
lsq_archive_entry_get_props(const LSQArchive *archive, LSQArchiveEntry *entry)
{
	guint size = 0;

	if ( NULL == entry->props )
	{
		size = lsq_archive_entry_properties_size(archive);

		entry->props = g_malloc0(size);
	}

	return entry->props;
}

static void
lsq_archive_entry_set_prop_str(const LSQArchive *archive, LSQArchiveEntry *entry, guint n, const gchar *str_val)
{
	gpointer props_iter = lsq_archive_entry_get_props(archive, entry);
	guint offset;

	if ( G_TYPE_STRING != lsq_archive_get_entry_property_type(archive, n) )
	{
		return;
	}

	offset = lsq_archive_get_entry_property_offset(archive, n);

	g_free(((gchar **)props_iter)[offset]);
	((gchar **)props_iter)[offset] = g_strdup(str_val);
}

static void
lsq_archive_entry_set_prop_uint(const LSQArchive *archive, LSQArchiveEntry *entry, guint n, guint int_val)
{
	gpointer props_iter = lsq_archive_entry_get_props(archive, entry);
	guint offset;

	if ( G_TYPE_UINT != lsq_archive_get_entry_property_type(archive, n) )
	{
		return;
	}

	offset = lsq_archive_get_entry_property_offset(archive, n);

	((guint *)props_iter)[offset] = int_val;
}

static void
lsq_archive_entry_set_prop_uint64(const LSQArchive *archive, LSQArchiveEntry *entry, guint n, guint64 int64_val)
{
	gpointer props_iter = lsq_archive_entry_get_props(archive, entry);
	guint offset;

	if ( G_TYPE_UINT64 !=  lsq_archive_get_entry_property_type(archive, n) )
	{
		return;
	}

	offset = lsq_archive_get_entry_property_offset(archive, n);

	((guint64 *)props_iter)[offset] = int64_val;
}

static void
lsq_archive_entry_set_propsv(const LSQArchive *archive, LSQArchiveEntry *entry, gpointer *props)
{
	gpointer props_iter = lsq_archive_entry_get_props(archive, entry);
	guint offset;
	guint i;

	for(i=0; i < (lsq_archive_n_entry_properties(archive) - LSQ_ARCHIVE_PROP_USER); ++i)
	{
		offset = lsq_archive_get_entry_property_offset(archive, i+LSQ_ARCHIVE_PROP_USER);
		switch(lsq_archive_get_entry_property_type(archive, i+LSQ_ARCHIVE_PROP_USER))
		{
			case G_TYPE_STRING:
				g_free(((gchar **)props_iter)[offset]);
				//(*((gchar **)props_iter)) = g_strdup((const gchar*)props[i]);
				((gchar **)props_iter)[offset] = (gchar*)props[i];
				break;
			case G_TYPE_UINT:
				((guint *)props_iter)[offset] = *((const guint *)props[i]);
				break;
			case G_TYPE_UINT64:
				((guint64 *)props_iter)[offset] = *((const guint64 *)props[i]);
				break;
		}
	}
}

static void
lsq_archive_entry_set_propsva(const LSQArchive *archive, LSQArchiveEntry *entry, va_list ap)
{
	gpointer props_iter = lsq_archive_entry_get_props(archive, entry);
	guint offset;
	guint i;

	for(i=0; i < (lsq_archive_n_entry_properties(archive) - LSQ_ARCHIVE_PROP_USER); ++i)
	{
		offset = lsq_archive_get_entry_property_offset(archive, i+LSQ_ARCHIVE_PROP_USER);
		switch(lsq_archive_get_entry_property_type(archive, i+LSQ_ARCHIVE_PROP_USER))
		{
			case G_TYPE_STRING:
				g_free(((gchar **)props_iter)[offset]);
				((gchar **)props_iter)[offset] = g_strdup(va_arg(ap, gchar*));
				break;
			case G_TYPE_UINT:
				((guint *)props_iter)[offset] = va_arg(ap, guint);
				break;
			case G_TYPE_UINT64:
				((guint64 *)props_iter)[offset] = va_arg(ap, guint64);
				break;
		}
	}
}

/******************************
 * Other iter/entry functions *
 ******************************/

static gchar *
lsq_concat_child_filenames(LSQArchiveIter *iter)
{
	gchar *concat_str;
	guint i, size = lsq_archive_iter_n_children(iter);
	LSQArchiveIter *child;
	gchar **part = g_new(gchar*, (size*2)+1);
	part[size*2] = NULL;
	for ( i=0; i < size; i++ )
	{
		gchar *path;
		child = lsq_archive_iter_nth_child(iter, i);

		part[i*2] = lsq_concat_child_filenames(child);
		path = lsq_archive_iter_get_path(child);
		part[(i*2)+1] = g_shell_quote(path);
		g_free(path);

		lsq_archive_iter_unref(child);
	}
	concat_str = g_strjoinv(" ", part);
	g_strfreev(part);
	return concat_str;
}

gchar *
lsq_concat_iter_filenames(GSList *file_iters, gboolean recursive)
{
	GSList *iter = file_iters;
	gchar *concat_str = g_strdup(""), *_concat_str;
	gchar *children = "";
	for ( iter = file_iters; NULL != iter; iter = iter->next )
	{
		gchar *path = lsq_archive_iter_get_path(iter->data);
		gchar *quote = g_shell_quote(path);
		g_free(path);

		if ( TRUE == recursive )
		{
			children = lsq_concat_child_filenames(iter->data);
		}

		_concat_str = concat_str;
		concat_str = g_strconcat(_concat_str, " ", children, " ", quote , NULL);
		g_free(_concat_str);

		if ( TRUE == recursive )
		{
			g_free(children);
		}

		g_free(quote);
	}
	return concat_str;
}

GSList *
lsq_iter_slist_copy(GSList *iters)
{
	GSList *new_list = g_slist_copy(iters);
	GSList *iter;
	for ( iter = iters; NULL != iter; iter = iter->next )
	{
		lsq_archive_iter_ref(iter->data);
	}
	//g_slist_foreach(iters, (GFunc)lsq_archive_iter_ref, NULL);
	return new_list;
}

void
lsq_iter_slist_free(GSList *iters)
{
	GSList *iter;
	for ( iter = iters; NULL != iter; iter = iter->next )
	{
		lsq_archive_iter_unref(iter->data);
	}
	//g_slist_foreach(iters, (GFunc)lsq_archive_iter_unref, NULL);
	g_slist_free(iters);
}

