	/*
	 *  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
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
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libxarchiver/libxarchiver.h>
#include <libxarchiver/mime.h>
#include <gettext.h>

#include "archive_store.h"

static void
xa_archive_store_class_init(XAArchiveStoreClass *as_class);

static void
xa_archive_store_init(XAArchiveStore *as);

static void
xa_archive_tree_model_init(GtkTreeModelIface *tm_interface);

static void
xa_archive_tree_sortable_init(GtkTreeSortableIface *ts_interface);

/* properties */
enum {
	XA_ARCHIVE_STORE_SHOW_ICONS = 1, 
	XA_ARCHIVE_STORE_SHOW_UP_DIR,
	XA_ARCHIVE_STORE_SORT_FOLDERS_FIRST,
	XA_ARCHIVE_STORE_SORT_CASE_SENSITIVE
};

/* signals */
enum {
	XA_ARCHIVE_STORE_SIGNAL_PWD_CHANGED = 0,
	XA_ARCHIVE_STORE_SIGNAL_NEW_ARCHIVE,
	XA_ARCHIVE_STORE_SIGNAL_NUMBER
};
static gint xa_archive_store_signals[XA_ARCHIVE_STORE_SIGNAL_NUMBER];

static void
xa_archive_store_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
xa_archive_store_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

/* tree model */
static GtkTreeModelFlags
xa_archive_store_get_flags(GtkTreeModel *tree_model);
static gint
xa_archive_store_get_n_columns(GtkTreeModel *tree_model);
static GType
xa_archive_store_get_column_type(GtkTreeModel *tree_model, gint index);
static gboolean
xa_archive_store_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path);
static GtkTreePath *
xa_archive_store_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter);
static void 
xa_archive_store_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value);
static gboolean
xa_archive_store_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter);
static gboolean
xa_archive_store_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent);
static gboolean
xa_archive_store_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter);
static gint
xa_archive_store_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter);
static gboolean 
xa_archive_store_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n);
static gboolean
xa_archive_store_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child);
static void
xa_archive_store_refresh(XAArchiveStore *store, gint prev_size);

static void
cb_xa_archive_store_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);

/* tree sortable */
static gboolean
xa_archive_store_get_sort_column_id(GtkTreeSortable *sortable, gint *sort_col_id, GtkSortType *order);
static void
xa_archive_store_set_sort_column_id(GtkTreeSortable *sortable, gint sort_col_id, GtkSortType order);
static void
xa_archive_store_set_sort_func(GtkTreeSortable *, gint, GtkTreeIterCompareFunc, gpointer, GtkDestroyNotify);
static void
xa_archive_store_set_default_sort_func(GtkTreeSortable *, GtkTreeIterCompareFunc, gpointer, GtkDestroyNotify);
static gboolean
xa_archive_store_has_default_sort_func(GtkTreeSortable *);

static gint
xa_archive_entry_compare(XAArchiveStore *store, LXAArchiveIter *a, LXAArchiveIter *b);
static void
xa_archive_quicksort(XAArchiveStore *store, gint left, gint right);
static void
xa_archive_insertionsort(XAArchiveStore *store, gint left, gint right);

static void
xa_archive_store_sort(XAArchiveStore *store);

GType
xa_archive_store_get_type()
{
	static GType xa_archive_store_type= 0;

	if(xa_archive_store_type)
		return xa_archive_store_type;

	if (!xa_archive_store_type)
	{
		static const GTypeInfo xa_archive_store_info = 
		{
			sizeof (XAArchiveStoreClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_archive_store_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAArchiveStore),
			0,
			(GInstanceInitFunc) xa_archive_store_init,
			NULL
		};

		xa_archive_store_type = g_type_register_static (G_TYPE_OBJECT, "XAArchiveStore", &xa_archive_store_info, 0);
	}
	static const GInterfaceInfo tree_model_info =
	{
		(GInterfaceInitFunc) xa_archive_tree_model_init,
			NULL,
			NULL
	};

	g_type_add_interface_static (xa_archive_store_type, GTK_TYPE_TREE_MODEL, &tree_model_info);

	static const GInterfaceInfo tree_sort_info =
	{
		(GInterfaceInitFunc) xa_archive_tree_sortable_init,
			NULL,
			NULL
	};

	g_type_add_interface_static (xa_archive_store_type, GTK_TYPE_TREE_SORTABLE, &tree_sort_info);

	return xa_archive_store_type;
}

static void
xa_archive_tree_model_init(GtkTreeModelIface *iface)
{
	iface->get_flags       = xa_archive_store_get_flags;
	iface->get_n_columns   = xa_archive_store_get_n_columns;
	iface->get_column_type = xa_archive_store_get_column_type;
	iface->get_iter        = xa_archive_store_get_iter;
	iface->get_path        = xa_archive_store_get_path;
	iface->get_value       = xa_archive_store_get_value;
	iface->iter_next       = xa_archive_store_iter_next;
	iface->iter_children   = xa_archive_store_iter_children;
	iface->iter_has_child  = xa_archive_store_iter_has_child;
	iface->iter_n_children = xa_archive_store_iter_n_children;
	iface->iter_nth_child  = xa_archive_store_iter_nth_child;
	iface->iter_parent     = xa_archive_store_iter_parent;
}

static void
xa_archive_tree_sortable_init(GtkTreeSortableIface *iface)
{
	iface->get_sort_column_id    = xa_archive_store_get_sort_column_id;
	iface->set_sort_column_id    = xa_archive_store_set_sort_column_id;
	iface->set_sort_func         = xa_archive_store_set_sort_func;        	/*NOT SUPPORTED*/
	iface->set_default_sort_func = xa_archive_store_set_default_sort_func;	/*NOT SUPPORTED*/
	iface->has_default_sort_func = xa_archive_store_has_default_sort_func;
}

static void
xa_archive_store_init(XAArchiveStore *as)
{
	as->stamp = g_random_int();
	as->archive = NULL;
	as->current_entry = NULL;
	as->props._show_icons = 0;
	as->props._show_up_dir = 1;
	as->props._sort_folders_first = 1;
	as->props._sort_case_sensitive = 1;
	as->sort_column = GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID;
	as->sort_order = GTK_SORT_ASCENDING;
	as->sort_list = NULL;
}

static void
xa_archive_store_class_init(XAArchiveStoreClass *as_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (as_class);
	GParamSpec *pspec = NULL;

	object_class->set_property = xa_archive_store_set_property;
	object_class->get_property = xa_archive_store_get_property;

	pspec = g_param_spec_boolean("show_icons",
		_("Show mime icons"),
		_("Show the mime type icons for each entry"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, XA_ARCHIVE_STORE_SHOW_ICONS, pspec);

	pspec = g_param_spec_boolean("show_up_dir",
		_("Show up dir entry"),
		_("Show \'..\' to go to the parent directory"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, XA_ARCHIVE_STORE_SHOW_UP_DIR, pspec);

	pspec = g_param_spec_boolean("sort_folders_first",
		_("Sort folders before files"),
		_("The folders will be put at the top of the list"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, XA_ARCHIVE_STORE_SORT_FOLDERS_FIRST, pspec);

	pspec = g_param_spec_boolean("sort_case_sensitive",
		_("Sort text case sensitive"),
		_("Sort text case sensitive"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, XA_ARCHIVE_STORE_SORT_CASE_SENSITIVE, pspec);

	xa_archive_store_signals[XA_ARCHIVE_STORE_SIGNAL_PWD_CHANGED] = g_signal_new("xa-pwd-changed",
	   G_TYPE_FROM_CLASS(as_class),
		 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		 0,
		 NULL,
		 NULL,
		 g_cclosure_marshal_VOID__VOID,
		 G_TYPE_NONE,
		 0,
		 NULL);

	xa_archive_store_signals[XA_ARCHIVE_STORE_SIGNAL_NEW_ARCHIVE] = g_signal_new("xa-new-archive",
	   G_TYPE_FROM_CLASS(as_class),
		 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		 0,
		 NULL,
		 NULL,
		 g_cclosure_marshal_VOID__VOID,
		 G_TYPE_NONE,
		 0,
		 NULL);
/*
	xa_archive_store_up_entry.filename = "..";
	xa_archive_store_up_entry.props = NULL;
	xa_archive_store_up_entry.children = NULL;
	xa_archive_store_up_entry.buffer = NULL;
*/
}

static void
xa_archive_store_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	gint prev_size = 0;
	XAArchiveStore *store = XA_ARCHIVE_STORE(object);
	switch(prop_id)
	{
		case XA_ARCHIVE_STORE_SHOW_ICONS:
			if(XA_ARCHIVE_STORE(object)->props._show_icons != g_value_get_boolean(value)?1:0)
			{
				if(store->current_entry)
					prev_size = lxa_archive_iter_n_children(store->archive, ((LXAArchiveIter*)store->current_entry->data));
				XA_ARCHIVE_STORE(object)->props._show_icons = g_value_get_boolean(value)?1:0;
				xa_archive_store_refresh(XA_ARCHIVE_STORE(object), prev_size);
			}
			break;
		case XA_ARCHIVE_STORE_SHOW_UP_DIR:
			if(XA_ARCHIVE_STORE(object)->props._show_up_dir != g_value_get_boolean(value)?1:0)
			{
				if(store->current_entry)
					prev_size = lxa_archive_iter_n_children(store->archive, ((LXAArchiveIter*)store->current_entry->data));
				XA_ARCHIVE_STORE(object)->props._show_up_dir = g_value_get_boolean(value)?1:0;
				xa_archive_store_refresh(XA_ARCHIVE_STORE(object), prev_size);
			}
			break;
		case XA_ARCHIVE_STORE_SORT_FOLDERS_FIRST:
			if(XA_ARCHIVE_STORE(object)->props._sort_folders_first != g_value_get_boolean(value)?1:0)
			{
				XA_ARCHIVE_STORE(object)->props._sort_folders_first = g_value_get_boolean(value)?1:0;
				if(XA_ARCHIVE_STORE(object)->sort_column < 0)
					XA_ARCHIVE_STORE(object)->sort_column = 1;
				xa_archive_store_sort(XA_ARCHIVE_STORE(object));
			}
			break;
		case XA_ARCHIVE_STORE_SORT_CASE_SENSITIVE:
			if(XA_ARCHIVE_STORE(object)->props._sort_case_sensitive != g_value_get_boolean(value)?1:0)
			{
				XA_ARCHIVE_STORE(object)->props._sort_case_sensitive = g_value_get_boolean(value)?1:0;
				xa_archive_store_sort(XA_ARCHIVE_STORE(object));
			}
			break;
	}
}

static void
xa_archive_store_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case XA_ARCHIVE_STORE_SHOW_ICONS:
			g_value_set_boolean(value, XA_ARCHIVE_STORE(object)->props._show_icons?TRUE:FALSE);
			break;
		case XA_ARCHIVE_STORE_SHOW_UP_DIR:
			g_value_set_boolean(value, XA_ARCHIVE_STORE(object)->props._show_up_dir?TRUE:FALSE);
			break;
		case XA_ARCHIVE_STORE_SORT_FOLDERS_FIRST:
			g_value_set_boolean(value, XA_ARCHIVE_STORE(object)->props._sort_folders_first?TRUE:FALSE);
			break;
		case XA_ARCHIVE_STORE_SORT_CASE_SENSITIVE:
			g_value_set_boolean(value, XA_ARCHIVE_STORE(object)->props._sort_case_sensitive?TRUE:FALSE);
			break;
	}
}

static GtkTreeModelFlags
xa_archive_store_get_flags(GtkTreeModel *tree_model)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), (GtkTreeModelFlags)0);

	return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

static gint
xa_archive_store_get_n_columns(GtkTreeModel *tree_model)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), 0);

	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	LXAArchive *archive = store->archive;

	if(!archive)
		return 0;
	
	if(store->props._show_icons)
		return lxa_archive_n_property(archive) + 1;
	else
		return lxa_archive_n_property(archive);
}

static GType
xa_archive_store_get_column_type(GtkTreeModel *tree_model, gint index)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), G_TYPE_INVALID);	

	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	LXAArchive *archive = store->archive;

	if(!archive)
		return G_TYPE_INVALID;

	if(store->props._show_icons)
	{
		index--;

		if(index < 0)
			return G_TYPE_STRING;
	}

	return lxa_archive_get_property_type(archive, index);
}

static gboolean
xa_archive_store_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), FALSE);	

	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	LXAArchive *archive = store->archive;

	LXAArchiveIter *entry = NULL;
	if(!store->current_entry)
		return FALSE;

	entry = store->current_entry->data;


	gint *indices = gtk_tree_path_get_indices(path);
	gint depth = gtk_tree_path_get_depth(path) - 1;

	/* only support list: depth is always 0 */
	g_return_val_if_fail(depth == 0, FALSE);

	gint index = indices[depth];

	if(store->props._show_up_dir && lxa_archive_get_iter(archive, NULL) != entry)
		index--;

	if(index == -1)
	{
		entry = NULL;
	}
	else
	{
		/* as long as it is a list depth is 0 other wise current_entry should be synced ? */
		if(store->sort_list)
			entry = store->sort_list[index];
		else
			entry = lxa_archive_iter_nth_child(archive, entry, index);

		g_return_val_if_fail(entry, FALSE);

	}

	iter->stamp = store->stamp;
	iter->user_data = entry;
	iter->user_data2 = store->current_entry->data;
	iter->user_data3 = GINT_TO_POINTER(index);

	return TRUE;
}

static GtkTreePath *
xa_archive_store_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), NULL);	

	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	LXAArchive *archive = store->archive;

	g_return_val_if_fail(archive, NULL);

	LXAArchiveIter *entry = (LXAArchiveIter*)iter->user_data2;
	gint pos = GPOINTER_TO_INT(iter->user_data3);

	if(store->props._show_up_dir && lxa_archive_get_iter(archive, NULL) != entry)
		pos++;

	GtkTreePath *path = gtk_tree_path_new();
	gtk_tree_path_append_index(path, pos);

	return path;
}


/* done */

static void 
xa_archive_store_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value)
{
	g_return_if_fail (XA_IS_ARCHIVE_STORE (tree_model));

	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	LXAArchive *archive = store->archive;
	LXAArchiveIter *entry = iter->user_data;

	g_return_if_fail(archive);

	if(store->props._show_icons)
		column--;

	if(entry)
	{
		if(column == -1)
		{
			lxa_archive_iter_get_prop_value(archive, entry, LXA_ARCHIVE_PROP_MIME_TYPE, value);
			lxa_mime_convert_to_icon_name(store->icon_theme, value);
		}
		else
		{
			lxa_archive_iter_get_prop_value(archive, entry, column, value);
		}
	}
	else
	{
		switch(column)
		{
			case -1:
				g_value_init(value, G_TYPE_STRING);
				g_value_set_string(value, GTK_STOCK_GO_UP);
				break;
			case LXA_ARCHIVE_PROP_FILENAME:
				g_value_init(value, G_TYPE_STRING);
				g_value_set_string(value, "..");
				break;
			default:
				g_value_init(value, lxa_archive_get_property_type(archive, column));
				break;
		}
	}
}

static gboolean
xa_archive_store_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), FALSE);
	
	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);

	LXAArchiveIter *entry = (LXAArchiveIter*)iter->user_data2;
	gint pos = GPOINTER_TO_INT(iter->user_data3);
	pos++;

	if(store->sort_list)
	{
		if(pos < lxa_archive_iter_n_children(store->archive, entry))
			entry = store->sort_list[pos];
		else
			entry = NULL;
	}
	else
		entry = lxa_archive_iter_nth_child(store->archive, entry, pos);

	if(!entry)
		return FALSE;

	iter->stamp = store->stamp;
	iter->user_data = entry;
	iter->user_data3 = GINT_TO_POINTER(pos);

	return TRUE;
}

static gboolean
xa_archive_store_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), FALSE);

	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	LXAArchive *archive = store->archive;
	LXAArchiveIter *entry = store->current_entry->data;

	g_return_val_if_fail(archive, FALSE);
	g_return_val_if_fail(entry, FALSE);

	/* only support lists: parent is always NULL */
	g_return_val_if_fail(parent == NULL, FALSE);

	if(store->props._show_up_dir && lxa_archive_get_iter(archive, NULL) != entry)
	{
		entry = NULL;
		iter->user_data3 = GINT_TO_POINTER(-1);
	}
	else
	{
		if(store->sort_list)
			entry = store->sort_list[0];
		else
			entry = lxa_archive_iter_nth_child(archive, entry, 0);
	
		g_return_val_if_fail(entry, FALSE);
	
		iter->user_data3 = GINT_TO_POINTER(0);
	}

	iter->stamp = store->stamp;
	iter->user_data = entry;
	iter->user_data2 = store->current_entry->data;

	return TRUE;
}

static gboolean
xa_archive_store_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	return FALSE;
}

static gint
xa_archive_store_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), 0);

	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	LXAArchive *archive = store->archive;
	LXAArchiveIter *entry = store->current_entry->data;

	g_return_val_if_fail(archive, 0);
	g_return_val_if_fail(entry, 0);

	/* only support lists: iter is always NULL */
	g_return_val_if_fail(iter == NULL, FALSE);

	return lxa_archive_iter_n_children(archive, entry) + (lxa_archive_get_iter(archive, NULL) == entry)?0:1;
}

static gboolean 
xa_archive_store_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), FALSE);

	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	LXAArchive *archive = store->archive;
	LXAArchiveIter *entry = store->current_entry->data;

	g_return_val_if_fail(archive, FALSE);
	g_return_val_if_fail(entry, FALSE);
	g_return_val_if_fail(iter, FALSE);

	/* only support lists: parent is always NULL */
	g_return_val_if_fail(parent == NULL, FALSE);

	if(store->props._show_up_dir && lxa_archive_get_iter(archive, NULL) != entry)
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
			entry = lxa_archive_iter_nth_child(archive, entry, n);
	
		g_return_val_if_fail(entry, FALSE);
	}

	iter->stamp = store->stamp;
	iter->user_data = entry;
	iter->user_data2 = store->current_entry->data;
	iter->user_data3 = GINT_TO_POINTER(n);

	return TRUE;
}

static gboolean
xa_archive_store_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
	return FALSE;
}


static gboolean
xa_archive_store_get_sort_column_id(GtkTreeSortable *sortable, gint *sort_col_id, GtkSortType *order)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(sortable), FALSE);

	XAArchiveStore *store = XA_ARCHIVE_STORE(sortable);

	if(sort_col_id)
		*sort_col_id = store->sort_column;
	
	if(order)
		*order = store->sort_order;

	return store->sort_order >= 0;
}

static void
xa_archive_store_set_sort_column_id(GtkTreeSortable *sortable, gint sort_col_id, GtkSortType order)
{
	g_return_if_fail(XA_IS_ARCHIVE_STORE(sortable));

	XAArchiveStore *store = XA_ARCHIVE_STORE(sortable);

	if(store->sort_column == sort_col_id && store->sort_order == order)
		return;

	
	if(store->props._show_icons)
	{
		if(sort_col_id == 0)
			return;
	}

	store->sort_column = sort_col_id;
	store->sort_order = order;

	xa_archive_store_sort(store);

	gtk_tree_sortable_sort_column_changed(sortable);
}

static void
xa_archive_store_set_sort_func(GtkTreeSortable *s, gint i, GtkTreeIterCompareFunc f, gpointer p, GtkDestroyNotify d)
{
	g_warning("%s is not supported by the XAArchiveStore model", __FUNCTION__);
}

static void
xa_archive_store_set_default_sort_func(GtkTreeSortable *s, GtkTreeIterCompareFunc f, gpointer p, GtkDestroyNotify d)
{
	g_warning("%s is not supported by the XAArchiveStore model", __FUNCTION__);
}

static gboolean
xa_archive_store_has_default_sort_func(GtkTreeSortable *s)
{
	return XA_ARCHIVE_STORE(s)->props._sort_folders_first?FALSE:TRUE;
}

/* FIXME */
static gint
xa_archive_entry_compare(XAArchiveStore *store, LXAArchiveIter *a, LXAArchiveIter *b)
{
	gint retval = 0;
	gint column = 0;
	gboolean cmp_a = 0;
	gboolean cmp_b = 0;
	GValue  *prop_a = g_new0(GValue, 1);
	GValue  *prop_b = g_new0(GValue, 1);
	if(store->props._sort_folders_first)
	{
		cmp_a = lxa_archive_iter_is_directory(store->archive, a);
		cmp_b = lxa_archive_iter_is_directory(store->archive, b);

		if(cmp_a && !cmp_b)
			return -1;
		if(cmp_b && !cmp_a)
			return 1;
	}

	LXAArchiveIter *swap = b;
	if(store->sort_order == GTK_SORT_DESCENDING)
	{
		b = a;
		a = swap;
	}

	LXAArchive *archive = store->archive;
	if(store->props._show_icons)
		column = store->sort_column - 1;
	else
	{
		column = store->sort_column;
	}

	lxa_archive_iter_get_prop_value(archive, a, column, prop_a);
	lxa_archive_iter_get_prop_value(archive, b, column, prop_b);

	switch(lxa_archive_get_property_type(archive, column))
	{
		case G_TYPE_STRING:
			switch(store->props._sort_case_sensitive)
			{
				case 0: /* case insensitive */
					retval = g_ascii_strcasecmp(g_value_get_string(prop_a), g_value_get_string(prop_b));
					break;
				case 1: /* case sensitive */
					retval = strcmp(g_value_get_string(prop_a), g_value_get_string(prop_b));
					break;
			}
			break;
		case G_TYPE_UINT64:
			retval = g_value_get_uint64(prop_a) - g_value_get_uint64(prop_b);
			break;
		case G_TYPE_UINT:
			retval = g_value_get_uint(prop_a) - g_value_get_uint(prop_b);
			break;
	}
	g_value_unset(prop_a);
	g_value_unset(prop_b);
	g_free(prop_a);
	g_free(prop_b);
	return retval;
}

static void
xa_archive_store_sort(XAArchiveStore *store)
{
	if(store->sort_list)
	{
		g_free(store->sort_list);
		store->sort_list = NULL;
	}

	if(store->sort_column < 0)
		return;

	LXAArchiveIter *pentry = (LXAArchiveIter*)store->current_entry->data;
	gint psize = lxa_archive_iter_n_children(store->archive, pentry);
	gint i = 0;

	if(psize <= 1)
		return;

	store->sort_list = g_new(LXAArchiveIter*, psize);

	for(i = 0; i < psize; i++)
	{
		store->sort_list[i] = lxa_archive_iter_nth_child(store->archive, pentry, i);
	}
	xa_archive_quicksort(store, 0, psize-1);
	xa_archive_insertionsort(store, 0, psize-1);
}

inline void
swap(LXAArchiveIter **left, LXAArchiveIter **right)
{
	LXAArchiveIter *tmp = *right;
	*right = *left;
	*left = tmp;
}

static void
xa_archive_quicksort(XAArchiveStore *store, gint left, gint right)
{
	if(right-left < 30)	return;

	gint i = (left+right)/2;
	gint j = right-1;
	LXAArchiveIter *value = NULL;
	LXAArchiveIter **list = store->sort_list;

	if(xa_archive_entry_compare(store, list[left], list[i]) > 0)
		swap(list+left, list+i);
	if(xa_archive_entry_compare(store, list[left], list[right]) > 0)
		swap(list+left, list+right);
	if(xa_archive_entry_compare(store, list[i], list[right]) > 0)
		swap(list+i, list+right);
	
	swap(list+i, list+j);
	i = left;
	value = list[j];

	for(;;)
	{
		while(xa_archive_entry_compare(store, list[++i], value) < 0);
		while(xa_archive_entry_compare(store, list[--j], value) > 0);
		if(j<i) break;

		swap(list+i, list+j);
	}
	swap(list+i, list+right-1);
	xa_archive_quicksort(store, left, j);
	xa_archive_quicksort(store, i+1, right);
}

static void
xa_archive_insertionsort(XAArchiveStore *store, gint left, gint right)
{
	gint i = 0;
	gint j = 0;
	LXAArchiveIter *value = NULL;
	LXAArchiveIter **list = store->sort_list;

	for(i = left+1; i <= right; i++)
	{
		j = i;
		value = list[i];
		while(j > left && xa_archive_entry_compare(store, list[j-1], value) > 0)
		{
			list[j] = list[j-1];
			j--;
		}
		list[j] = value;
	}
}

GtkTreeModel *
xa_archive_store_new(LXAArchive *archive, gboolean show_icons, gboolean show_up_dir, GtkIconTheme *icon_theme)
{
	XAArchiveStore *tree_model;

	tree_model = g_object_new(XA_TYPE_ARCHIVE_STORE, NULL);

	tree_model->props._show_icons = show_icons?1:0;
	tree_model->props._show_up_dir = show_up_dir?1:0;
	tree_model->icon_theme = icon_theme;

	if(tree_model->props._sort_folders_first)
		tree_model->sort_column = 1;

	xa_archive_store_set_contents(tree_model, archive);

	return GTK_TREE_MODEL(tree_model);
}

void
xa_archive_store_connect_treeview(XAArchiveStore *store, GtkTreeView *treeview)
{
	g_signal_connect(G_OBJECT(treeview), "row-activated", G_CALLBACK(cb_xa_archive_store_row_activated), store);
}

static void
xa_archive_store_refresh(XAArchiveStore *store, gint prev_size)
{
	LXAArchive *archive = store->archive;
	LXAArchiveIter *entry = store->current_entry->data;

	g_return_if_fail(archive);
	g_return_if_fail(entry);

	gint new_size = lxa_archive_iter_n_children(archive, entry);
	gint i = 0;
	gint index = 0;
	GtkTreePath *path_ = NULL;
	GtkTreeIter iter;

	if(store->props._show_up_dir && lxa_archive_get_iter(archive, NULL) != entry) { 
		path_ = gtk_tree_path_new();
		gtk_tree_path_append_index(path_, 0);

		iter.stamp = store->stamp;
		iter.user_data = NULL;
		iter.user_data2 = entry;
		iter.user_data3 = GINT_TO_POINTER(-1);

		if(0 < prev_size)
			gtk_tree_model_row_changed(GTK_TREE_MODEL(store), path_, &iter);
		else
			gtk_tree_model_row_inserted(GTK_TREE_MODEL(store), path_, &iter);

		gtk_tree_path_free(path_);
		i=1;
	}

	for(index = 0; index < new_size; i++, index++)
	{
		path_ = gtk_tree_path_new();
		gtk_tree_path_append_index(path_, i);

		iter.stamp = store->stamp;
		if(store->sort_list)
			iter.user_data = store->sort_list[i];
		else
			iter.user_data = lxa_archive_iter_nth_child(archive, lxa_archive_get_iter(archive, NULL), i);
		iter.user_data2 = entry;
		iter.user_data3 = GINT_TO_POINTER(index);

		if(i < prev_size)
			gtk_tree_model_row_changed(GTK_TREE_MODEL(store), path_, &iter);
		else
			gtk_tree_model_row_inserted(GTK_TREE_MODEL(store), path_, &iter);

		gtk_tree_path_free(path_);
	}
	for(index = prev_size - 1; index >= i; index--)
	{
		path_ = gtk_tree_path_new();
		gtk_tree_path_append_index(path_, index);

		gtk_tree_model_row_deleted(GTK_TREE_MODEL(store), path_);

		gtk_tree_path_free(path_);
	}
}

static void
cb_xa_archive_store_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	g_return_if_fail(XA_IS_ARCHIVE_STORE(user_data));	
	XAArchiveStore *store = XA_ARCHIVE_STORE(user_data);

	g_return_if_fail(store->current_entry);


	LXAArchive *archive = store->archive;
	LXAArchiveIter *entry = store->current_entry->data;

	g_return_if_fail(archive);
	g_return_if_fail(entry);

	gint *indices = gtk_tree_path_get_indices(path);
	gint depth = gtk_tree_path_get_depth(path) - 1;

	/* only support list: depth is always 0 */
	g_return_if_fail(depth == 0);

	gint prev_size = lxa_archive_iter_n_children(archive, entry);
	gint index = indices[depth];

	if(store->props._show_up_dir && lxa_archive_get_iter(archive, NULL) != entry)
	{
		prev_size++;
		index--;
	}

	if(index == -1)
	{
		store->current_entry = g_slist_delete_link(store->current_entry, store->current_entry);
		entry = store->current_entry->data;
	}
	else
	{
		if(store->sort_list)
			entry = store->sort_list[index];
		else
			entry = lxa_archive_iter_nth_child(archive, entry, index);

		g_return_if_fail(entry);

		/* TODO Signal file-activated */
		if(!lxa_archive_iter_is_directory(archive, entry))
		{
#ifdef DEBUG
			g_debug("file clicked");
#endif
			return;
		}

		store->current_entry = g_slist_prepend(store->current_entry, entry);
	}

	xa_archive_store_sort(store);

	xa_archive_store_refresh(store, prev_size);
	g_signal_emit(store, xa_archive_store_signals[XA_ARCHIVE_STORE_SIGNAL_PWD_CHANGED], 0,NULL);
}

void
xa_archive_store_go_up(XAArchiveStore *store)
{
	LXAArchive *archive = store->archive;
	LXAArchiveIter *entry = store->current_entry->data;

	g_return_if_fail(archive);
	g_return_if_fail(entry);

	gint prev_size = lxa_archive_iter_n_children(archive, entry);

	if(store->props._show_up_dir && lxa_archive_get_iter(archive, NULL) != entry)
	{
		prev_size++;
	}

	/* TODO: signal or something */
	g_return_if_fail(store->current_entry->next);

	store->current_entry = g_slist_delete_link(store->current_entry, store->current_entry);
	entry = store->current_entry->data;

	xa_archive_store_sort(store);

	xa_archive_store_refresh(store, prev_size);
	g_signal_emit(store, xa_archive_store_signals[XA_ARCHIVE_STORE_SIGNAL_PWD_CHANGED], 0,NULL);
}

void
xa_archive_store_set_contents(XAArchiveStore *store, LXAArchive *archive)
{
	gint i = 0;
	GtkTreePath *path_ = NULL;
	GtkTreeIter iter;
	g_return_if_fail(store);

	LXAArchiveIter *entry = NULL;
	gint prev_size =  0;

	if(store->sort_list)
	{
		g_free(store->sort_list);
		store->sort_list = NULL;
	}

	if(store->current_entry)
	{
		entry = store->current_entry->data;
		prev_size = lxa_archive_iter_n_children(archive, entry);
		

		if(store->props._show_up_dir && lxa_archive_get_iter(store->archive, NULL) != entry)
			prev_size++;
		
		g_slist_free(store->current_entry);
	}

	for(i = prev_size - 1; i >= 0; i--)
	{
		path_ = gtk_tree_path_new();
		gtk_tree_path_append_index(path_, i);

		gtk_tree_model_row_deleted(GTK_TREE_MODEL(store), path_);

		gtk_tree_path_free(path_);
	}

	if(store->archive)
		g_object_unref(store->archive);

	if(!archive)
	{
		store->archive = NULL;
		store->current_entry = NULL;
		g_signal_emit(store, xa_archive_store_signals[XA_ARCHIVE_STORE_SIGNAL_NEW_ARCHIVE], 0,NULL);
		return;
	}

	g_object_ref(archive);
	store->archive = archive;
	store->current_entry = g_slist_prepend(NULL, lxa_archive_get_iter(archive, NULL));

	xa_archive_store_sort(store);

	for(i = 0; i < lxa_archive_iter_n_children(archive, lxa_archive_get_iter(archive, NULL)); i++)
	{
		path_ = gtk_tree_path_new();
		gtk_tree_path_append_index(path_, i);

		iter.stamp = store->stamp;
		if(store->sort_list)
			iter.user_data = store->sort_list[i];
		else
			iter.user_data = lxa_archive_iter_nth_child(archive, lxa_archive_get_iter(archive, NULL), i);
		iter.user_data2 = lxa_archive_get_iter(archive, NULL);
		iter.user_data3 = GINT_TO_POINTER(i);

		gtk_tree_model_row_inserted(GTK_TREE_MODEL(store), path_, &iter);

		gtk_tree_path_free(path_);
	}

	g_signal_emit(store, xa_archive_store_signals[XA_ARCHIVE_STORE_SIGNAL_NEW_ARCHIVE], 0,NULL);
	g_signal_emit(store, xa_archive_store_signals[XA_ARCHIVE_STORE_SIGNAL_PWD_CHANGED], 0,NULL);
}

gchar *
xa_archive_store_get_pwd(XAArchiveStore *store)
{
	g_return_val_if_fail(store, NULL);

	GValue *basename = g_new0(GValue, 1);
	gchar *path = NULL;
	gchar **buf = NULL;
	GSList *iter = store->current_entry;
	gint i = g_slist_length(iter);
	gchar *lastfile = NULL;
	gint namelen = 0;

	/* we don't want to include de archive rootentry */
	if(i<=1)
		return g_strdup("");

	buf = g_new(gchar*, i);
	i--;
	buf[i] = NULL;

	lxa_archive_iter_get_prop_value(store->archive, (LXAArchiveIter*)iter->data, LXA_ARCHIVE_PROP_FILENAME, basename);
	namelen = strlen(g_value_get_string(basename));
	lastfile = g_new(gchar, namelen+2);
	strcpy(lastfile, g_value_get_string(basename));
	g_value_unset(basename);
	if(lastfile[namelen-1] != '/')
	{
		lastfile[namelen] = '/';
		lastfile[namelen+1] = '\0';
	}

	i--;
	buf[i] = lastfile;

	iter = iter->next;
	if(iter)
	{
		while(iter->next)
		{
			--i;
			lxa_archive_iter_get_prop_value(store->archive, (LXAArchiveIter*)iter->data, LXA_ARCHIVE_PROP_FILENAME, basename);
			buf[i] = g_value_dup_string(basename);
			g_value_unset(basename);
			iter = iter->next;
		}
	}

	if(buf[0] != lastfile && buf[0][0] == '/')
	{
		buf[0] = g_strdup("");
	}

	/* why does glib want buf to be gchar** instead of const gchar** ? */
	path = g_strjoinv("/", (gchar**)buf);

	g_strfreev(buf);
	g_free(basename);
	return path;
}

GSList *
xa_archive_store_get_pwd_list(XAArchiveStore *store)
{
	g_return_val_if_fail(store, NULL);

	GValue basename;
	GSList *iter = store->current_entry;
	GSList *path = NULL;

	if(!iter)
		return NULL;

	/* we don't want to include de archive rootentry */
	while(iter->next)
	{
		lxa_archive_iter_get_prop_value(store->archive, (LXAArchiveIter*)iter->data, LXA_ARCHIVE_PROP_FILENAME, &basename);
		path = g_slist_prepend(path, g_value_dup_string(&basename));
		g_value_unset(&basename);
		iter = iter->next;
	}

	return path;
}

gboolean
xa_archive_store_set_pwd(XAArchiveStore *store, const gchar *path)
{
	gint result = xa_archive_store_set_pwd_silent(store, path);

	g_signal_emit(store, xa_archive_store_signals[XA_ARCHIVE_STORE_SIGNAL_PWD_CHANGED], 0,NULL);
	return result;
}

gboolean
xa_archive_store_set_pwd_silent(XAArchiveStore *store, const gchar *path)
{
	g_return_val_if_fail(store, -1);

	if(!store->archive)
		return FALSE;

	gchar **buf = g_strsplit_set(path, "/\n", -1);
	gchar **iter = buf;
	LXAArchiveIter *entry = lxa_archive_get_iter(store->archive, NULL);
	GSList *stack = g_slist_prepend(NULL, entry);
	gint prev_size = lxa_archive_iter_n_children(store->archive, ((LXAArchiveIter*)store->current_entry->data));

	if(store->props._show_up_dir && lxa_archive_get_iter(store->archive, NULL) != store->current_entry->data)
		prev_size++;

	if(path[0] == '/' && lxa_archive_iter_get_child(store->archive, entry, "/"))
	{
		iter[0] = strdup("/");
	}

	while(*iter)
	{
		if((*iter)[0])
		{
			entry = lxa_archive_iter_get_child(store->archive, entry, *iter);
			if(!entry)
			{
				g_strfreev(buf);
				g_slist_free(stack);
				return -1;
			}
			stack = g_slist_prepend(stack, entry);
		}
		iter++;
	}

	g_strfreev(buf);

	g_slist_free(store->current_entry);
	store->current_entry = stack;

	xa_archive_store_sort(store);

	xa_archive_store_refresh(store, prev_size);
	
	return TRUE;
}

void
xa_archive_store_set_icon_theme(XAArchiveStore *store, GtkIconTheme *icon_theme)
{
	if(store)
		store->icon_theme = icon_theme;
}

gboolean
xa_archive_store_get_show_icons(XAArchiveStore *store)
{
	return store->props._show_icons;
}

gboolean
xa_archive_store_get_sort_case_sensitive(XAArchiveStore *store)
{
	return store->props._sort_case_sensitive;
}

gboolean
xa_archive_store_get_sort_folders_first(XAArchiveStore *store)
{
	return store->props._sort_folders_first;
}

void
xa_archive_store_set_show_icons(XAArchiveStore *store, gboolean show)
{
	GtkSortType sort_order;
	gint sort_col = 0;
	gint prev_size = lxa_archive_iter_n_children(store->archive, (LXAEntry*)store->current_entry->data);
	store->props._show_icons = show?1:0;
	if(show)
	{
		xa_archive_store_get_sort_column_id(GTK_TREE_SORTABLE(store), &sort_col, &sort_order);
		xa_archive_store_set_sort_column_id(GTK_TREE_SORTABLE(store), sort_col+1, sort_order);
	}
	else
	{
		xa_archive_store_get_sort_column_id(GTK_TREE_SORTABLE(store), &sort_col, &sort_order);
		xa_archive_store_set_sort_column_id(GTK_TREE_SORTABLE(store), sort_col-1, sort_order);
	}
	xa_archive_store_refresh(store, prev_size);
}

void
xa_archive_store_set_sort_case_sensitive(XAArchiveStore *store, gboolean sort)
{
	store->props._sort_case_sensitive= sort?1:0;

	if(store->archive)
	{
		gint prev_size = lxa_archive_iter_n_children(store->archive, (LXAEntry*)store->current_entry->data);
		xa_archive_store_refresh(store, prev_size);
	}
}

void
xa_archive_store_set_sort_folders_first(XAArchiveStore *store, gboolean sort)
{
	store->props._sort_folders_first = sort?1:0;

	if(store->archive)
	{
		gint prev_size = lxa_archive_iter_n_children(store->archive, (LXAEntry*)store->current_entry->data);
		xa_archive_store_refresh(store, prev_size);
	}
}
