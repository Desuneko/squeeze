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
#include <gtk/gtk.h>
#include <thunar-vfs/thunar-vfs.h>
#include <libsqueeze/libsqueeze.h>

#include "archive_store.h"

#ifndef SQ_ARCHIVE_STORE_MAX_HISTORY
#define SQ_ARCHIVE_STORE_MAX_HISTORY 10
#endif

static void
sq_archive_store_class_init(SQArchiveStoreClass *as_class);

static void
sq_archive_store_init(SQArchiveStore *as);

static void
sq_archive_tree_model_init(GtkTreeModelIface *tm_interface);

static void
sq_archive_tree_sortable_init(GtkTreeSortableIface *ts_interface);

static void
sq_archive_store_dispose(GObject *object);

/* properties */
enum {
	SQ_ARCHIVE_STORE_SHOW_ICONS = 1, 
	SQ_ARCHIVE_STORE_SHOW_UP_DIR,
	SQ_ARCHIVE_STORE_SORT_FOLDERS_FIRST,
	SQ_ARCHIVE_STORE_SORT_CASE_SENSITIVE
};

/* signals */
enum {
	SQ_ARCHIVE_STORE_SIGNAL_PWD_CHANGED = 0,
	SQ_ARCHIVE_STORE_SIGNAL_NEW_ARCHIVE,
	SQ_ARCHIVE_STORE_SIGNAL_FILE_ACTIVATED,
	SQ_ARCHIVE_STORE_SIGNAL_COUNT
};
static gint sq_archive_store_signals[SQ_ARCHIVE_STORE_SIGNAL_COUNT];

static GObjectClass *parent_class;

static void
sq_archive_store_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
sq_archive_store_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

/* tree model */
static GtkTreeModelFlags
sq_archive_store_get_flags(GtkTreeModel *tree_model);
static gint
sq_archive_store_get_n_columns(GtkTreeModel *tree_model);
static GType
sq_archive_store_get_column_type(GtkTreeModel *tree_model, gint index);
static gboolean
sq_archive_store_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path);
static GtkTreePath *
sq_archive_store_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter);
static void 
sq_archive_store_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value);
static gboolean
sq_archive_store_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter);
static gboolean
sq_archive_store_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent);
static gboolean
sq_archive_store_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter);
static gint
sq_archive_store_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter);
static gboolean 
sq_archive_store_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n);
static gboolean
sq_archive_store_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child);

static void
sq_archive_store_refresh(SQArchiveStore *store);

static void
sq_archive_store_file_activated(SQArchiveStore *store, GtkTreePath *path);

static void
cb_sq_archive_store_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);

/* tree sortable */
static gboolean
sq_archive_store_get_sort_column_id(GtkTreeSortable *sortable, gint *sort_col_id, GtkSortType *order);
static void
sq_archive_store_set_sort_column_id(GtkTreeSortable *sortable, gint sort_col_id, GtkSortType order);
static void
sq_archive_store_set_sort_func(GtkTreeSortable *, gint, GtkTreeIterCompareFunc, gpointer, GtkDestroyNotify);
static void
sq_archive_store_set_default_sort_func(GtkTreeSortable *, GtkTreeIterCompareFunc, gpointer, GtkDestroyNotify);
static gboolean
sq_archive_store_has_default_sort_func(GtkTreeSortable *);

static gint
sq_archive_entry_compare(SQArchiveStore *store, LSQArchiveIter *a, LSQArchiveIter *b);
static void
sq_archive_quicksort(SQArchiveStore *store, gint left, gint right);
static void
sq_archive_insertionsort(SQArchiveStore *store, gint left, gint right);

static void
sq_archive_store_sort(SQArchiveStore *store);

static const gchar *
sq_archive_store_get_icon_name_for_iter(SQArchiveStore *store, LSQArchiveIter *iter);

static void
sq_archive_store_append_history(SQArchiveStore *store, LSQArchiveIter *entry);

static void
sq_archive_store_check_trailing(SQArchiveStore *store);

static void
cb_sq_archive_store_archive_refreshed(LSQArchive *archive, gpointer user_data);
/* static void                                                                                           */
/* cb_sq_archive_store_archive_path_changed(LSQArchive *archive, const gchar *path, gpointer user_data); */

GType
sq_archive_store_get_type()
{
	static GType sq_archive_store_type= 0;

	if(sq_archive_store_type)
		return sq_archive_store_type;

	if (!sq_archive_store_type)
	{
		static const GTypeInfo sq_archive_store_info = 
		{
			sizeof (SQArchiveStoreClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_archive_store_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQArchiveStore),
			0,
			(GInstanceInitFunc) sq_archive_store_init,
			NULL
		};

		sq_archive_store_type = g_type_register_static (G_TYPE_OBJECT, "SQArchiveStore", &sq_archive_store_info, 0);
	}
	static const GInterfaceInfo tree_model_info =
	{
		(GInterfaceInitFunc) sq_archive_tree_model_init,
			NULL,
			NULL
	};

	g_type_add_interface_static (sq_archive_store_type, GTK_TYPE_TREE_MODEL, &tree_model_info);

	static const GInterfaceInfo tree_sort_info =
	{
		(GInterfaceInitFunc) sq_archive_tree_sortable_init,
			NULL,
			NULL
	};

	g_type_add_interface_static (sq_archive_store_type, GTK_TYPE_TREE_SORTABLE, &tree_sort_info);

	return sq_archive_store_type;
}

static void
sq_archive_tree_model_init(GtkTreeModelIface *iface)
{
	iface->get_flags       = sq_archive_store_get_flags;
	iface->get_n_columns   = sq_archive_store_get_n_columns;
	iface->get_column_type = sq_archive_store_get_column_type;
	iface->get_iter        = sq_archive_store_get_iter;
	iface->get_path        = sq_archive_store_get_path;
	iface->get_value       = sq_archive_store_get_value;
	iface->iter_next       = sq_archive_store_iter_next;
	iface->iter_children   = sq_archive_store_iter_children;
	iface->iter_has_child  = sq_archive_store_iter_has_child;
	iface->iter_n_children = sq_archive_store_iter_n_children;
	iface->iter_nth_child  = sq_archive_store_iter_nth_child;
	iface->iter_parent     = sq_archive_store_iter_parent;
}

static void
sq_archive_tree_sortable_init(GtkTreeSortableIface *iface)
{
	iface->get_sort_column_id    = sq_archive_store_get_sort_column_id;
	iface->set_sort_column_id    = sq_archive_store_set_sort_column_id;
	iface->set_sort_func         = sq_archive_store_set_sort_func;        	/*NOT SUPPORTED*/
	iface->set_default_sort_func = sq_archive_store_set_default_sort_func;	/*NOT SUPPORTED*/
	iface->has_default_sort_func = sq_archive_store_has_default_sort_func;
}

static void
sq_archive_store_init(SQArchiveStore *as)
{
	as->stamp = g_random_int();
	as->archive = NULL;
	as->support = NULL;
	as->sort_column = GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID;
	as->sort_order = GTK_SORT_ASCENDING;
	as->sort_list = NULL;
	as->icon_theme = NULL;
	as->props._show_icons = 0;
	as->props._show_up_dir = 1;
	as->props._sort_folders_first = 1;
	as->props._sort_case_sensitive = 1;
	as->navigation.history = NULL;
	as->navigation.present = NULL;
	as->navigation.maxhistory = SQ_ARCHIVE_STORE_MAX_HISTORY;
	as->navigation.trailing = NULL;
}

static void
sq_archive_store_class_init(SQArchiveStoreClass *as_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (as_class);
	GParamSpec *pspec = NULL;

	object_class->set_property = sq_archive_store_set_property;
	object_class->get_property = sq_archive_store_get_property;
	object_class->dispose = sq_archive_store_dispose;

	parent_class = gtk_type_class (G_TYPE_OBJECT);

	pspec = g_param_spec_boolean("show-icons",
		_("Show mime icons"),
		_("Show the mime type icons for each entry"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_ARCHIVE_STORE_SHOW_ICONS, pspec);

	pspec = g_param_spec_boolean("show-up-dir",
		_("Show up dir entry"),
		_("Show \'..\' to go to the parent directory"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_ARCHIVE_STORE_SHOW_UP_DIR, pspec);

	pspec = g_param_spec_boolean("sort-folders-first",
		_("Sort folders before files"),
		_("The folders will be put at the top of the list"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_ARCHIVE_STORE_SORT_FOLDERS_FIRST, pspec);

	pspec = g_param_spec_boolean("sort-case-sensitive",
		_("Sort text case sensitive"),
		_("Sort text case sensitive"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_ARCHIVE_STORE_SORT_CASE_SENSITIVE, pspec);

	sq_archive_store_signals[SQ_ARCHIVE_STORE_SIGNAL_PWD_CHANGED] = g_signal_new("sq-pwd-changed",
	   G_TYPE_FROM_CLASS(as_class),
		 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		 0,
		 NULL,
		 NULL,
		 g_cclosure_marshal_VOID__VOID,
		 G_TYPE_NONE,
		 0, 
		 NULL);

	sq_archive_store_signals[SQ_ARCHIVE_STORE_SIGNAL_NEW_ARCHIVE] = g_signal_new("sq-new-archive",
	   G_TYPE_FROM_CLASS(as_class),
		 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		 0,
		 NULL,
		 NULL,
		 g_cclosure_marshal_VOID__VOID,
		 G_TYPE_NONE,
		 0,
		 NULL);

	sq_archive_store_signals[SQ_ARCHIVE_STORE_SIGNAL_FILE_ACTIVATED] = g_signal_new("sq-file-activated",
	   G_TYPE_FROM_CLASS(as_class),
		 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		 0,
		 NULL,
		 NULL,
		 g_cclosure_marshal_VOID__POINTER,
		 G_TYPE_NONE,
		 1,
		 G_TYPE_STRING,
		 NULL);
}

static void
sq_archive_store_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	SQArchiveStore *store = SQ_ARCHIVE_STORE(object);
	switch(prop_id)
	{
		case SQ_ARCHIVE_STORE_SHOW_ICONS:
			sq_archive_store_set_show_icons(store, g_value_get_boolean(value));
			break;
		case SQ_ARCHIVE_STORE_SHOW_UP_DIR:
			if(store->props._show_up_dir != g_value_get_boolean(value)?1:0)
			{
				store->props._show_up_dir = g_value_get_boolean(value)?1:0;
				sq_archive_store_refresh(store);
			}
			break;
		case SQ_ARCHIVE_STORE_SORT_FOLDERS_FIRST:
			sq_archive_store_set_sort_folders_first(store, g_value_get_boolean(value));
			break;
		case SQ_ARCHIVE_STORE_SORT_CASE_SENSITIVE:
			sq_archive_store_set_sort_case_sensitive(store, g_value_get_boolean(value));
			break;
	}
}

static void
sq_archive_store_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case SQ_ARCHIVE_STORE_SHOW_ICONS:
			g_value_set_boolean(value, SQ_ARCHIVE_STORE(object)->props._show_icons?TRUE:FALSE);
			break;
		case SQ_ARCHIVE_STORE_SHOW_UP_DIR:
			g_value_set_boolean(value, SQ_ARCHIVE_STORE(object)->props._show_up_dir?TRUE:FALSE);
			break;
		case SQ_ARCHIVE_STORE_SORT_FOLDERS_FIRST:
			g_value_set_boolean(value, SQ_ARCHIVE_STORE(object)->props._sort_folders_first?TRUE:FALSE);
			break;
		case SQ_ARCHIVE_STORE_SORT_CASE_SENSITIVE:
			g_value_set_boolean(value, SQ_ARCHIVE_STORE(object)->props._sort_case_sensitive?TRUE:FALSE);
			break;
	}
}

static GtkTreeModelFlags
sq_archive_store_get_flags(GtkTreeModel *tree_model)
{
	g_return_val_if_fail(SQ_IS_ARCHIVE_STORE(tree_model), (GtkTreeModelFlags)0);

	return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

static gint
sq_archive_store_get_n_columns(GtkTreeModel *tree_model)
{
	g_return_val_if_fail(SQ_IS_ARCHIVE_STORE(tree_model), 0);

	SQArchiveStore *store = SQ_ARCHIVE_STORE(tree_model);
	LSQArchive *archive = store->archive;

	if(!archive)
		return 0;
	
	return lsq_archive_n_entry_properties(archive) + 1;
}

static GType
sq_archive_store_get_column_type(GtkTreeModel *tree_model, gint index)
{
	g_return_val_if_fail(SQ_IS_ARCHIVE_STORE(tree_model), G_TYPE_INVALID);	

	SQArchiveStore *store = SQ_ARCHIVE_STORE(tree_model);
	LSQArchive *archive = store->archive;
	g_return_val_if_fail(index < lsq_archive_n_entry_properties(archive), G_TYPE_INVALID);

	if(!archive)
		return G_TYPE_INVALID;

	index--;

	if(index == -1) /* icon */
		return G_TYPE_STRING; 

	return lsq_archive_get_entry_property_type(archive, index);
}

static gboolean
sq_archive_store_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
#ifdef DEBUG
	g_return_val_if_fail(SQ_IS_ARCHIVE_STORE(tree_model), FALSE);	
#endif

	SQArchiveStore *store = SQ_ARCHIVE_STORE(tree_model);

	if(!store->navigation.present)
		return FALSE;
#ifdef DEBUG
	g_return_val_if_fail(store->navigation.present->data, FALSE);
#endif

	/* get the present history */
	LSQArchiveIter *entry = store->navigation.present->data;

	gint *indices = gtk_tree_path_get_indices(path);
	gint depth = gtk_tree_path_get_depth(path) - 1;

	/* only support list: depth is always 0 */
	g_return_val_if_fail(depth == 0, FALSE);

	gint index = indices[depth];

	/* if this is the root entry we don't need the ".." */
	if(store->props._show_up_dir && lsq_archive_iter_has_parent(entry))
		index--;

	if(index == -1)
	{
		/* it is the ".." */
		entry = NULL;
	}
	else
	{
		/* as long as it is a list depth is 0 other wise current_entry should be synced ? */
		if(store->sort_list)
			entry = store->sort_list[index];
		else
			entry = NULL;

		if(!entry)
			return FALSE;
	}

	iter->stamp = store->stamp;
	iter->user_data = entry;
	/* the index in the child list */
	iter->user_data3 = GINT_TO_POINTER(index);

	return TRUE;
}

static GtkTreePath *
sq_archive_store_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail(SQ_IS_ARCHIVE_STORE(tree_model), NULL);	

	SQArchiveStore *store = SQ_ARCHIVE_STORE(tree_model);
	LSQArchive *archive = store->archive;

	g_return_val_if_fail(archive, NULL);

	LSQArchiveIter *entry = (LSQArchiveIter*)iter->user_data;
	gint pos = GPOINTER_TO_INT(iter->user_data3);

	if(store->props._show_up_dir && lsq_archive_iter_has_parent(entry))
		pos++;

	GtkTreePath *path = gtk_tree_path_new();
	gtk_tree_path_append_index(path, pos);

	return path;
}


static void 
sq_archive_store_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value)
{
	g_return_if_fail (SQ_IS_ARCHIVE_STORE (tree_model));

	SQArchiveStore *store = SQ_ARCHIVE_STORE(tree_model);
	LSQArchive *archive = store->archive;
	LSQArchiveIter *entry = iter->user_data;

	g_return_if_fail(archive);

	column--;

	if(entry)
	{
		if(column == -1)
		{
			g_value_init(value, G_TYPE_STRING);

			if(store->props._show_icons)
				g_value_set_string(value, sq_archive_store_get_icon_name_for_iter(store, entry));
		}
		else
		{
			if(column < (gint)lsq_archive_n_entry_properties(archive))
				lsq_archive_iter_get_prop_value(entry, column, value);
			/* what if it isn't utf-8 */
			if(G_VALUE_HOLDS_STRING(value) && g_value_get_string(value) && !g_utf8_validate(g_value_get_string(value), -1, NULL))
				g_value_take_string(value, g_convert(g_value_get_string(value), -1, "UTF-8", "WINDOWS-1252", NULL, NULL, NULL));
		}
	}
	else
	{
		switch(column)
		{
			case -1:
				g_value_init(value, G_TYPE_STRING);
				if(store->props._show_icons)
					g_value_set_string(value, GTK_STOCK_GO_UP);
				break;
			case LSQ_ARCHIVE_PROP_FILENAME:
				g_value_init(value, G_TYPE_STRING);
				g_value_set_string(value, "..");
				break;
			default:
				g_value_init(value, lsq_archive_get_entry_property_type(archive, column));
				break;
		}
	}
}

static gboolean
sq_archive_store_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail(SQ_IS_ARCHIVE_STORE(tree_model), FALSE);
	
	SQArchiveStore *store = SQ_ARCHIVE_STORE(tree_model);
	if(iter->stamp != store->stamp)
		return FALSE;

	LSQArchiveIter *entry = iter->user_data;
	gint pos = GPOINTER_TO_INT(iter->user_data3);
	pos++;

	if(store->sort_list)
		entry = store->sort_list[pos];
	else
		entry = NULL;

	if(!entry)
		return FALSE;

	iter->stamp = store->stamp;
	iter->user_data = entry;
	iter->user_data3 = GINT_TO_POINTER(pos);

	return TRUE;
}

static gboolean
sq_archive_store_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent)
{
#ifdef DEBUG
	g_return_val_if_fail(SQ_IS_ARCHIVE_STORE(tree_model), FALSE);
#endif

	SQArchiveStore *store = SQ_ARCHIVE_STORE(tree_model);
	LSQArchive *archive = store->archive;

#ifdef DEBUG
	g_return_val_if_fail(store->navigation.present, FALSE);
	g_return_val_if_fail(store->navigation.present->data, FALSE);
#endif

	LSQArchiveIter *entry = store->navigation.present->data;

#ifdef DEBUG
	g_return_val_if_fail(archive, FALSE);
	g_return_val_if_fail(entry, FALSE);
#endif

	/* only support lists: parent is always NULL */
	g_return_val_if_fail(parent == NULL, FALSE);

	if(store->props._show_up_dir && lsq_archive_iter_has_parent(entry))
	{
		entry = NULL;
		iter->user_data3 = GINT_TO_POINTER(-1);
	}
	else
	{
		if(store->sort_list)
			entry = store->sort_list[0];
		else
			entry = NULL;
	
		g_return_val_if_fail(entry, FALSE);
	
		iter->user_data3 = GINT_TO_POINTER(0);
	}

	iter->stamp = store->stamp;
	iter->user_data = entry;

	return TRUE;
}

static gboolean
sq_archive_store_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	return FALSE;
}

static gint
sq_archive_store_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
#ifdef DEBUG
	g_return_val_if_fail(SQ_IS_ARCHIVE_STORE(tree_model), 0);
#endif

	SQArchiveStore *store = SQ_ARCHIVE_STORE(tree_model);
	LSQArchive *archive = store->archive;

#ifdef DEBUG
	g_return_val_if_fail(store->navigation.present, 0);
	g_return_val_if_fail(store->navigation.present->data, 0);
#endif

	LSQArchiveIter *entry = store->navigation.present->data;

#ifdef DEBUG
	g_return_val_if_fail(archive, 0);
	g_return_val_if_fail(entry, 0);
#endif

	/* only support lists: iter is always NULL */
	g_return_val_if_fail(iter == NULL, FALSE);

	return store->list_size + (lsq_archive_iter_has_parent(entry)?1:0);
}

static gboolean 
sq_archive_store_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
#ifdef DEBUG
	g_return_val_if_fail(SQ_IS_ARCHIVE_STORE(tree_model), FALSE);
#endif

	SQArchiveStore *store = SQ_ARCHIVE_STORE(tree_model);
	LSQArchive *archive = store->archive;

#ifdef DEBUG
	g_return_val_if_fail(store->navigation.present, FALSE);
	g_return_val_if_fail(store->navigation.present->data, FALSE);
#endif

	LSQArchiveIter *entry = store->navigation.present->data;

#ifdef DEBUG
	g_return_val_if_fail(archive, FALSE);
	g_return_val_if_fail(entry, FALSE);
	g_return_val_if_fail(iter, FALSE);
#endif

	/* only support lists: parent is always NULL */
	g_return_val_if_fail(parent == NULL, FALSE);

	if(store->props._show_up_dir && lsq_archive_iter_has_parent(entry))
		n--;

	if(n == -1)
	{
		entry = NULL;
	}
	else
	{
		if(store->sort_list)
			entry = store->sort_list[n];
		else
			entry = NULL;
	
		if(!entry)
			return FALSE;
	}

	iter->stamp = store->stamp;
	iter->user_data = entry;
	iter->user_data3 = GINT_TO_POINTER(n);

	return TRUE;
}

static gboolean
sq_archive_store_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
	return FALSE;
}


static gboolean
sq_archive_store_get_sort_column_id(GtkTreeSortable *sortable, gint *sort_col_id, GtkSortType *order)
{
	g_return_val_if_fail(SQ_IS_ARCHIVE_STORE(sortable), FALSE);

	SQArchiveStore *store = SQ_ARCHIVE_STORE(sortable);

	if(sort_col_id)
		*sort_col_id = store->sort_column;

	if(order)
		*order = store->sort_order;

	return store->sort_order >= 0;
}

static void
sq_archive_store_set_sort_column_id(GtkTreeSortable *sortable, gint sort_col_id, GtkSortType order)
{
	g_return_if_fail(SQ_IS_ARCHIVE_STORE(sortable));

	SQArchiveStore *store = SQ_ARCHIVE_STORE(sortable);

	if(store->sort_column == sort_col_id && store->sort_order == order)
		return;

	if(sort_col_id == 0)
		return;

	store->sort_column = sort_col_id;
	store->sort_order = order;

	sq_archive_store_sort(store);
	sq_archive_store_refresh(store);

	gtk_tree_sortable_sort_column_changed(sortable);
}

static void
sq_archive_store_set_sort_func(GtkTreeSortable *s, gint i, GtkTreeIterCompareFunc f, gpointer p, GtkDestroyNotify d)
{
	g_warning("%s is not supported by the SQArchiveStore model", __FUNCTION__);
}

static void
sq_archive_store_set_default_sort_func(GtkTreeSortable *s, GtkTreeIterCompareFunc f, gpointer p, GtkDestroyNotify d)
{
	g_warning("%s is not supported by the SQArchiveStore model", __FUNCTION__);
}

static gboolean
sq_archive_store_has_default_sort_func(GtkTreeSortable *s)
{
	return SQ_ARCHIVE_STORE(s)->props._sort_folders_first?FALSE:TRUE;
}

static gint
sq_archive_entry_compare(SQArchiveStore *store, LSQArchiveIter *a, LSQArchiveIter *b)
{
	gint retval = 0;
	gint column = 0;
	gboolean cmp_a = 0;
	gboolean cmp_b = 0;
	GValue  prop_a;
	GValue  prop_b;
	memset(&prop_a, 0, sizeof(GValue));
	memset(&prop_b, 0, sizeof(GValue));
	if(store->props._sort_folders_first)
	{
		cmp_a = lsq_archive_iter_is_directory(a);
		cmp_b = lsq_archive_iter_is_directory(b);

		if(cmp_a && !cmp_b)
			return -1;
		if(cmp_b && !cmp_a)
			return 1;
	}

	LSQArchiveIter *swap = b;
	if(store->sort_order == GTK_SORT_DESCENDING)
	{
		b = a;
		a = swap;
	}

	LSQArchive *archive = store->archive;
	column = store->sort_column - 1;

	lsq_archive_iter_get_prop_value(a, column, &prop_a);
	lsq_archive_iter_get_prop_value(b, column, &prop_b);

	switch(lsq_archive_get_entry_property_type(archive, column))
	{
		case G_TYPE_STRING:
			if(g_value_get_string(&prop_a) == NULL)
			{
				retval = -1;
				break;
			}
			if(g_value_get_string(&prop_b) == NULL)
			{
				retval = 1;
				break;
			}
			switch(store->props._sort_case_sensitive)
			{
				case 0: /* case insensitive */
					retval = g_ascii_strcasecmp(g_value_get_string(&prop_a), g_value_get_string(&prop_b));
					break;
				case 1: /* case sensitive */
					retval = strcmp(g_value_get_string(&prop_a), g_value_get_string(&prop_b));
					break;
			}
			break;
		case G_TYPE_UINT64:
			retval = g_value_get_uint64(&prop_a) - g_value_get_uint64(&prop_b);
			break;
		case G_TYPE_UINT:
			retval = g_value_get_uint(&prop_a) - g_value_get_uint(&prop_b);
			break;
	}
	g_value_unset(&prop_a);
	g_value_unset(&prop_b);
	return retval;
}

static void
sq_archive_store_sort(SQArchiveStore *store)
{
	if(store->sort_list)
	{
		LSQArchiveIter **iter;
		for(iter = store->sort_list; *iter; iter++)
		{
			lsq_archive_iter_unref(*iter);
		}
		g_free(store->sort_list);
		store->sort_list = NULL;
	}

#ifdef DEBUG
	g_return_if_fail(store->navigation.present);
	g_return_if_fail(store->navigation.present->data);
#endif

	LSQArchiveIter *pentry = store->navigation.present->data;
	guint psize = lsq_archive_iter_n_children(pentry);
	guint i = 0;

	store->sort_list = g_new(LSQArchiveIter*, psize+1);

	for(i = 0; i < psize; ++i)
	{
		store->sort_list[i] = lsq_archive_iter_nth_child(pentry, i);
	}
	if(psize && store->sort_column > 0)
	{
		sq_archive_quicksort(store, 0, psize-1);
		sq_archive_insertionsort(store, 0, psize-1);
	}
	store->sort_list[psize] = NULL;
}

static inline void
swap(LSQArchiveIter **left, LSQArchiveIter **right)
{
	LSQArchiveIter *tmp = *right;
	*right = *left;
	*left = tmp;
}

static void
sq_archive_quicksort(SQArchiveStore *store, gint left, gint right)
{
	if(right-left < 30)	return;

	gint i = (left+right)/2;
	gint j = right-1;
	LSQArchiveIter *value = NULL;
	LSQArchiveIter **list = store->sort_list;

	if(sq_archive_entry_compare(store, list[left], list[i]) > 0)
		swap(list+left, list+i);
	if(sq_archive_entry_compare(store, list[left], list[right]) > 0)
		swap(list+left, list+right);
	if(sq_archive_entry_compare(store, list[i], list[right]) > 0)
		swap(list+i, list+right);

	swap(list+i, list+j);
	i = left;
	value = list[j];

	for(;;)
	{
		while(sq_archive_entry_compare(store, list[++i], value) < 0);
		while(sq_archive_entry_compare(store, list[--j], value) > 0);
		if(j<i) break;

		swap(list+i, list+j);
	}
	swap(list+i, list+right-1);
	sq_archive_quicksort(store, left, j);
	sq_archive_quicksort(store, i+1, right);
}

static void
sq_archive_insertionsort(SQArchiveStore *store, gint left, gint right)
{
	gint i = 0;
	gint j = 0;
	LSQArchiveIter *value = NULL;
	LSQArchiveIter **list = store->sort_list;

	for(i = left+1; i <= right; ++i)
	{
		j = i;
		value = list[i];
		while(j > left && sq_archive_entry_compare(store, list[j-1], value) > 0)
		{
			list[j] = list[j-1];
			j--;
		}
		list[j] = value;
	}
}

static const gchar *
sq_archive_store_get_icon_name_for_iter(SQArchiveStore *store, LSQArchiveIter *iter)
{
	ThunarVfsMimeDatabase *mimedb = thunar_vfs_mime_database_get_default();
	ThunarVfsMimeInfo *mime_info = thunar_vfs_mime_database_get_info(mimedb, lsq_archive_iter_get_mime(iter));
	const gchar *icon_name = thunar_vfs_mime_info_lookup_icon_name(mime_info, store->icon_theme);
	if(icon_name && !gtk_icon_theme_has_icon(store->icon_theme, icon_name))
		icon_name = NULL;
	/* may we do this? */
	thunar_vfs_mime_info_unref(mime_info);
	g_object_unref(mimedb);
	return icon_name;
}

GtkTreeModel *
sq_archive_store_new(LSQArchive *archive, gboolean show_icons, gboolean show_up_dir, GtkIconTheme *icon_theme)
{
	SQArchiveStore *tree_model;

	tree_model = g_object_new(SQ_TYPE_ARCHIVE_STORE, NULL);

	tree_model->props._show_icons = show_icons?1:0;
	tree_model->props._show_up_dir = show_up_dir?1:0;
	tree_model->icon_theme = icon_theme;

	if(tree_model->props._sort_folders_first)
		tree_model->sort_column = 1;

	sq_archive_store_set_archive(tree_model, archive);

	return GTK_TREE_MODEL(tree_model);
}

void
sq_archive_store_connect_treeview(SQArchiveStore *store, GtkTreeView *treeview)
{
	store->treeview = treeview;
	g_signal_connect(G_OBJECT(treeview), "row-activated", G_CALLBACK(cb_sq_archive_store_row_activated), store);
}

static void
sq_archive_store_refresh(SQArchiveStore *store)
{
	LSQArchive *archive = store->archive;

	if(!store->navigation.present)
		return;
#ifdef DEBUG
	g_return_if_fail(store->navigation.present->data);
#endif

	LSQArchiveIter *entry = store->navigation.present->data;

	g_return_if_fail(archive);
	g_return_if_fail(entry);

	guint prev_size = store->list_size;
	guint new_size = lsq_archive_iter_n_children(entry);
	guint i = 0;
	GtkTreePath *path_ = NULL;
	GtkTreeIter iter;

	/* if(store->treeview) */
	{
		/* we need to add up dir .. */
		if(store->props._show_up_dir && lsq_archive_iter_has_parent(entry))
		{ 
			path_ = gtk_tree_path_new();
			gtk_tree_path_append_index(path_, 0);

			iter.stamp = store->stamp;
			iter.user_data = NULL;
			iter.user_data3 = GINT_TO_POINTER(-1);

			if(0 < prev_size)
				gtk_tree_model_row_changed(GTK_TREE_MODEL(store), path_, &iter);
			else
				gtk_tree_model_row_inserted(GTK_TREE_MODEL(store), path_, &iter);

			gtk_tree_path_free(path_);
			i=1;
			new_size++;
		}

		if(store->sort_list)
		{
			/* notify the tree view that we have rows */
			for(; i < new_size; ++i)
			{
				path_ = gtk_tree_path_new();
				gtk_tree_path_append_index(path_, i);

				iter.stamp = store->stamp;
				iter.user_data = store->sort_list[i];
				iter.user_data3 = GINT_TO_POINTER(i);

				if(i < prev_size)
					gtk_tree_model_row_changed(GTK_TREE_MODEL(store), path_, &iter);
				else
					gtk_tree_model_row_inserted(GTK_TREE_MODEL(store), path_, &iter);

				gtk_tree_path_free(path_);
			}
		}
  
		/* notify tree view we romeved all the remaining rows */
		i = prev_size;
		while(i > new_size)
		{
			--i;
			path_ = gtk_tree_path_new();
			gtk_tree_path_append_index(path_, i);

			gtk_tree_model_row_deleted(GTK_TREE_MODEL(store), path_);

			gtk_tree_path_free(path_);
		}
	}

	store->list_size = new_size;
}

static void
cb_sq_archive_store_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	sq_archive_store_file_activated(SQ_ARCHIVE_STORE(user_data), path);
}

static void
sq_archive_store_file_activated(SQArchiveStore *store, GtkTreePath *path)
{
	g_return_if_fail(store->navigation.present);
	g_return_if_fail(store->navigation.present->data);

	LSQArchive *archive = store->archive;
	LSQArchiveIter *entry = store->navigation.present->data;

	g_return_if_fail(archive);
	g_return_if_fail(entry);

	gint *indices = gtk_tree_path_get_indices(path);
	gint depth = gtk_tree_path_get_depth(path) - 1;

	/* only support list: depth is always 0 */
	g_return_if_fail(depth == 0);

	gint index = indices[depth];

	if(store->props._show_up_dir && lsq_archive_iter_has_parent(entry))
		index--;

	if(index == -1)
	{
		entry = lsq_archive_iter_get_parent(entry);
		sq_archive_store_append_history(store, entry);
	}
	else
	{
		if(store->sort_list)
			entry = store->sort_list[index];
		else
			entry = NULL;

		g_return_if_fail(entry);

		/* Signal file-activated */
		if(!lsq_archive_iter_is_directory(entry))
		{
#ifdef DEBUG
			g_debug("file clicked");
#endif
			g_signal_emit(store, sq_archive_store_signals[SQ_ARCHIVE_STORE_SIGNAL_FILE_ACTIVATED], 0, lsq_archive_iter_get_filename(entry), NULL); 
			return;
		}

		sq_archive_store_append_history(store, lsq_archive_iter_ref(entry));
	}

	sq_archive_store_sort(store);
	sq_archive_store_refresh(store);
	g_signal_emit(store, sq_archive_store_signals[SQ_ARCHIVE_STORE_SIGNAL_PWD_CHANGED], 0,NULL);
}

void
sq_archive_store_go_up(SQArchiveStore *store)
{
	LSQArchive *archive = store->archive;

#ifdef DEBUG
	g_return_if_fail(store->navigation.present);
	g_return_if_fail(store->navigation.present->data);
#endif

	LSQArchiveIter *entry = store->navigation.present->data;

	g_return_if_fail(archive);
	g_return_if_fail(entry);

	g_return_if_fail((entry = lsq_archive_iter_get_parent(entry)));

	sq_archive_store_append_history(store, entry);

	sq_archive_store_sort(store);
	sq_archive_store_refresh(store);
	g_signal_emit(store, sq_archive_store_signals[SQ_ARCHIVE_STORE_SIGNAL_PWD_CHANGED], 0,NULL);
}

void
sq_archive_store_set_archive(SQArchiveStore *store, LSQArchive *archive)
{
	g_return_if_fail(store);

	if(store->archive == archive)
		return;

	guint i = 0;
	GtkTreePath *path_ = NULL;
	GtkTreeIter iter;
	GList *list_iter;
	LSQArchiveIter *root_entry;

	/* free the sort data */
	if(store->sort_list)
	{
		LSQArchiveIter **iter;
		for(iter = store->sort_list; *iter; iter++)
		{
			lsq_archive_iter_unref(*iter);
		}
		g_free(store->sort_list);
		store->sort_list = NULL;
	}

  /* notify the tree view, all rows are gone */
	i = store->list_size;
	while(i)
	{
		--i;
		path_ = gtk_tree_path_new();
		gtk_tree_path_append_index(path_, i);

		gtk_tree_model_row_deleted(GTK_TREE_MODEL(store), path_);

		gtk_tree_path_free(path_);
	}

	store->list_size = 0;

	/* clear the history */
	for(list_iter = store->navigation.history; list_iter; list_iter = list_iter->next)
		lsq_archive_iter_unref(list_iter->data);

	g_list_free(store->navigation.history);
	if(store->navigation.trailing)
		lsq_archive_iter_unref(store->navigation.trailing);

	store->navigation.history = NULL;
	store->navigation.present = NULL;
	store->navigation.trailing = NULL;

	/* disconnect from the archive */
	if(store->archive)
	{
		g_signal_handlers_disconnect_by_func(store->archive, cb_sq_archive_store_archive_refreshed, store);
		/* g_signal_handlers_disconnect_by_func(store->archive, cb_sq_archive_store_archive_path_changed, store); */
		g_object_unref(store->archive);
		store->archive = NULL;
	}

	/* notify all that we have a new NULL archive */
	if(!archive)
	{
		g_signal_emit(store, sq_archive_store_signals[SQ_ARCHIVE_STORE_SIGNAL_NEW_ARCHIVE], 0, NULL);
		return;
	}

	/* take a ownership of the archive */
	g_object_ref(archive);
	store->archive = archive;

	/* only update if we are not busy */
	/* if(lsq_archive_get_status(archive) == LSQ_ARCHIVESTATUS_IDLE) */
	{
		root_entry = lsq_archive_get_iter(archive, NULL);

		sq_archive_store_append_history(store, root_entry);

		sq_archive_store_sort(store);

		/* lets notify the tree view we have new rows */
		store->list_size = lsq_archive_iter_n_children(root_entry);

		if(store->sort_list)
		{
			for(i = 0; i < store->list_size; ++i)
			{
				path_ = gtk_tree_path_new();
				gtk_tree_path_append_index(path_, i);

				iter.stamp = store->stamp;
				iter.user_data = store->sort_list[i];
				iter.user_data3 = GINT_TO_POINTER(i);

				gtk_tree_model_row_inserted(GTK_TREE_MODEL(store), path_, &iter);

				gtk_tree_path_free(path_);
			}
		}
	}

	/* notify all we have a new archive and connect with the archive */
	g_signal_emit(store, sq_archive_store_signals[SQ_ARCHIVE_STORE_SIGNAL_NEW_ARCHIVE], 0, NULL);
	g_signal_connect(store->archive, "refreshed", G_CALLBACK(cb_sq_archive_store_archive_refreshed), store);
	/* g_signal_connect(store->archive, "lsq_path_changed", G_CALLBACK(cb_sq_archive_store_archive_path_changed), store); */
}

gchar *
sq_archive_store_get_pwd(SQArchiveStore *store)
{
#ifdef DEBUG
	g_return_val_if_fail(store, NULL);
	g_return_val_if_fail(SQ_IS_ARCHIVE_STORE(store), NULL);
#endif

	if(!store->navigation.present)
		return NULL;

	LSQArchiveIter *iter = store->navigation.present->data;

	return lsq_archive_iter_get_path(iter);
}

GSList *
sq_archive_store_get_pwd_list(SQArchiveStore *store)
{
#ifdef DEBUG
	g_return_val_if_fail(store, NULL);
#endif

	if(!store->navigation.present)
		return NULL;

	const gchar *basename;
	LSQArchiveIter *child = NULL, *iter = store->navigation.present->data;
	GSList *path = NULL;

	if(!iter)
		return NULL;

	/* we don't want to include de archive rootentry */
	while(lsq_archive_iter_has_parent(iter))
	{
		basename = lsq_archive_iter_get_filename(iter);
		path = g_slist_prepend(path, g_strdup(basename));
		iter = lsq_archive_iter_get_parent(iter);
		if(child)
			lsq_archive_iter_unref(child);
		child = iter;
	}
	if(child)
		lsq_archive_iter_unref(child);

	return path;
}

gboolean
sq_archive_store_set_pwd(SQArchiveStore *store, const gchar *path)
{
	g_return_val_if_fail(store, FALSE);

	if(!store->archive)
		return FALSE;

#ifdef DEBUG
	g_return_val_if_fail(store->navigation.present, FALSE);
	g_return_val_if_fail(store->navigation.present->data, FALSE);
#endif

	LSQArchiveIter *iter = lsq_archive_get_iter(store->archive, path);

	if(lsq_archive_iter_is_directory(iter))
	{
		sq_archive_store_append_history(store, iter);

		sq_archive_store_sort(store);
		sq_archive_store_refresh(store);

		g_signal_emit(store, sq_archive_store_signals[SQ_ARCHIVE_STORE_SIGNAL_PWD_CHANGED], 0,NULL);
		return TRUE;
	}

	return FALSE;
}

void
sq_archive_store_set_icon_theme(SQArchiveStore *store, GtkIconTheme *icon_theme)
{
	if(store)
		store->icon_theme = icon_theme;
}

gboolean
sq_archive_store_get_show_icons(SQArchiveStore *store)
{
	return store->props._show_icons;
}

gboolean
sq_archive_store_get_sort_case_sensitive(SQArchiveStore *store)
{
	return store->props._sort_case_sensitive;
}

gboolean
sq_archive_store_get_sort_folders_first(SQArchiveStore *store)
{
	return store->props._sort_folders_first;
}

void
sq_archive_store_set_show_icons(SQArchiveStore *store, gboolean show)
{
	show = show?1:0;

	if(store->props._show_icons != show)
	{
		store->props._show_icons = show;
		if(store->archive)
			sq_archive_store_refresh(store);
		g_object_notify(G_OBJECT(store), "show-icons");
	}
}

void
sq_archive_store_set_sort_case_sensitive(SQArchiveStore *store, gboolean sort)
{
	sort = sort?1:0;

	if(store->props._sort_case_sensitive != sort)
	{
		store->props._sort_case_sensitive = sort;
		if(store->sort_column <= 0)
			store->sort_column = 1;

		if(store->archive)
		{
			sq_archive_store_sort(store);
			sq_archive_store_refresh(store);
		}
	}
}

void
sq_archive_store_set_sort_folders_first(SQArchiveStore *store, gboolean sort)
{
	sort = sort?1:0;

	if(store->props._sort_folders_first != sort)
	{
		store->props._sort_folders_first = sort;
		if(store->sort_column <= 0)
			store->sort_column = 1;

		if(store->archive)
		{
			sq_archive_store_sort(store);
			sq_archive_store_refresh(store);
		}
	}
}

gboolean
sq_archive_store_has_history(SQArchiveStore *store)
{
	if(!store->navigation.present)
		return FALSE;
	return store->navigation.present->prev?TRUE:FALSE;
}

gboolean
sq_archive_store_has_future(SQArchiveStore *store)
{
	if(!store->navigation.present)
		return FALSE;
	return store->navigation.present->next?TRUE:FALSE;
}

void
sq_archive_store_go_back(SQArchiveStore *store)
{
	LSQArchive *archive = store->archive;

#ifdef DEBUG
	g_return_if_fail(store->navigation.present);
	g_return_if_fail(store->navigation.present->data);
#endif

	g_return_if_fail(archive);

	if(sq_archive_store_has_history(store))
		store->navigation.present = store->navigation.present->prev;

	sq_archive_store_sort(store);
	sq_archive_store_refresh(store);

	sq_archive_store_check_trailing(store);

	g_signal_emit(store, sq_archive_store_signals[SQ_ARCHIVE_STORE_SIGNAL_PWD_CHANGED], 0,NULL);
}

void
sq_archive_store_go_forward(SQArchiveStore *store)
{
	LSQArchive *archive = store->archive;

#ifdef DEBUG
	g_return_if_fail(store->navigation.present);
	g_return_if_fail(store->navigation.present->data);
#endif

	g_return_if_fail(archive);

	if(sq_archive_store_has_future(store))
		store->navigation.present = store->navigation.present->next;

	sq_archive_store_sort(store);
	sq_archive_store_refresh(store);

	sq_archive_store_check_trailing(store);

	g_signal_emit(store, sq_archive_store_signals[SQ_ARCHIVE_STORE_SIGNAL_PWD_CHANGED], 0,NULL);
}

static void
sq_archive_store_append_history(SQArchiveStore *store, LSQArchiveIter *entry)
{
/*	if(lsq_archive_get_status(store->archive) != LSQ_ARCHIVESTATUS_IDLE)
		return;
*/
	GList *iter = store->navigation.present;

	if(store->navigation.present)
	{
		if(store->navigation.present->next)
			store->navigation.present->next->prev = NULL;

		while((iter = iter->next))
			lsq_archive_iter_unref(iter->data);

		g_list_free(store->navigation.present->next);

		store->navigation.present->next = NULL;
	}

	store->navigation.history = g_list_append(store->navigation.history, entry);
	store->navigation.present = g_list_last(store->navigation.history);

	while(g_list_length(store->navigation.history) > store->navigation.maxhistory)
	{
		lsq_archive_iter_unref(g_list_first(store->navigation.history)->data);
		store->navigation.history = g_list_delete_link(store->navigation.history, g_list_first(store->navigation.history));
	}

	sq_archive_store_check_trailing(store);
}

static void
sq_archive_store_check_trailing(SQArchiveStore *store)
{
	LSQArchiveIter *piter = store->navigation.present->data;
	LSQArchiveIter *titer = store->navigation.trailing;
	LSQArchiveIter *child = NULL;

	if(titer)
	{
		while(titer)
		{
			if(titer == piter)
			{
				if(child)
					lsq_archive_iter_unref(child);
				return;
			}

			titer = lsq_archive_iter_get_parent(titer);
			if(child)
				lsq_archive_iter_unref(child);
			child = titer;
		}
		if(child)
			lsq_archive_iter_unref(child);

		lsq_archive_iter_unref(store->navigation.trailing);
	}
	store->navigation.trailing = lsq_archive_iter_ref(piter);
}

GSList *
sq_archive_store_get_trailing(SQArchiveStore *store)
{
#ifdef DEBUG
	g_return_val_if_fail(store, NULL);
#endif

	const gchar *basename;
	LSQArchiveIter *child = NULL, *iter = store->navigation.trailing;
	GSList *path = NULL;

	if(!iter)
		return NULL;

	/* we don't want to include de archive rootentry */
	while(lsq_archive_iter_has_parent(iter))
	{
		basename = lsq_archive_iter_get_filename(iter);
		path = g_slist_prepend(path, &basename);
		iter = lsq_archive_iter_get_parent(iter);
		if(child)
			lsq_archive_iter_unref(child);
		child = iter;
	}
	if(child)
		lsq_archive_iter_unref(child);

	return path;
}

LSQArchive *
sq_archive_store_get_archive(SQArchiveStore *archive_store)
{
	return archive_store->archive;
}

LSQArchiveSupport *
sq_archive_store_get_support(SQArchiveStore *archive_store)
{
	return archive_store->support;
}

void
sq_archive_store_set_support(SQArchiveStore *archive_store, LSQArchiveSupport *support)
{
	archive_store->support = support;
}

static void
sq_archive_store_dispose(GObject *object)
{
	SQArchiveStore *store = SQ_ARCHIVE_STORE(object);
	if(store->archive)
	{
		g_object_unref(store->archive);
		store->archive = NULL;
	}
	parent_class->dispose(object);
}

static void
cb_sq_archive_store_archive_refreshed(LSQArchive *archive, gpointer user_data)
{
	SQArchiveStore *store = SQ_ARCHIVE_STORE(user_data);
	GList *iter;
	LSQArchiveIter *aIter;

	for(iter = store->navigation.history; iter; iter = g_list_next(iter))
	{
		aIter = lsq_archive_iter_get_real_parent(iter->data);
		lsq_archive_iter_unref(iter->data);
		iter->data = aIter;
	}

	sq_archive_store_sort(store);
	sq_archive_store_refresh(store);
}

