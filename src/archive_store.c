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
#include <gtk/gtk.h>
#include <libxarchiver/libxarchiver.h>
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
	XA_ARCHIVE_STORE_SHOW_UP_DIR
};

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
	as->props._show_icons = FALSE;
	as->props._show_up_dir = TRUE;
	as->up_entry.filename = "..";
	as->up_entry.props = NULL;
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
}

static void
xa_archive_store_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case XA_ARCHIVE_STORE_SHOW_ICONS:
			XA_ARCHIVE_STORE(object)->props._show_icons = g_value_get_boolean(value);
			break;
		case XA_ARCHIVE_STORE_SHOW_UP_DIR:
			XA_ARCHIVE_STORE(object)->props._show_up_dir = g_value_get_boolean(value);
			break;
	}
}

static void
xa_archive_store_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case XA_ARCHIVE_STORE_SHOW_ICONS:
			g_value_set_boolean(value, XA_ARCHIVE_STORE(object)->props._show_icons);
			break;
		case XA_ARCHIVE_STORE_SHOW_UP_DIR:
			g_value_set_boolean(value, XA_ARCHIVE_STORE(object)->props._show_up_dir);
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
	
	return archive->column_number + store->props._show_icons?1:0;
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
		if(index == 0)
			return G_TYPE_STRING;
		index--;
	}

	g_return_val_if_fail(index < archive->column_number, G_TYPE_INVALID);

	return archive->column_types[index];
}

static gboolean
xa_archive_store_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), FALSE);	

	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	LXAArchive *archive = store->archive;

	LXAEntry *entry = NULL;
	if(!store->current_entry)
		return FALSE;
	else
		entry = store->current_entry->data;


	gint *indices = gtk_tree_path_get_indices(path);
	gint depth = gtk_tree_path_get_depth(path) - 1;

	/* only support list: depth is always 0 */
	g_return_val_if_fail(depth == 0, FALSE);

	gint index = indices[depth];

	if(store->props._show_up_dir && &archive->root_entry != entry)
		index--;

	if(index == -1)
	{
		entry = &store->up_entry;
	}
	else
	{
		//as long as it is a list depth is 0 other wise current_entry should be synced ?
		//TODO: use lxa_entry_nth() or something
		entry = lxa_entry_children_nth_data(entry, index);

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

	LXAEntry *entry = (LXAEntry*)iter->user_data2;
//	gint pos = lxa_entry_children_index(store->current_entry, iter->user_data);
	gint pos = GPOINTER_TO_INT(iter->user_data3);

	if(store->props._show_up_dir && &archive->root_entry != entry)
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

	g_return_if_fail(archive);

	if(store->props._show_icons)
		column--;

	g_return_if_fail (column < (gint)archive->column_number);

	if(column >= 0)
		g_value_init(value, archive->column_types[column]);

	LXAEntry *entry = ((LXAEntry *)iter->user_data);
	gpointer props_iter = entry->props;
	gint i = 1;

	if(column == -1)
	{
		g_value_init(value, G_TYPE_STRING);
		g_value_set_string(value, (strcmp(entry->filename, ".."))?entry->is_folder?"folder":"unknown":"go-up");
	}
	else if(column == 0)
	{
		g_value_set_string(value, (strcmp(entry->filename, ".."))?entry->filename:"..");
	}
	else
	{
		if(!props_iter)
		{
			switch(archive->column_types[column])
			{
				case G_TYPE_STRING:
					g_value_set_string(value, "");
					break;
				case G_TYPE_UINT64:
					g_value_set_uint64(value, 0);
					break;
				case G_TYPE_UINT:
					g_value_set_uint(value, 0);
					break;
			}
		}
		else
		{
			for(;i<column;i++)
			{
				switch(archive->column_types[i])
				{
					case G_TYPE_STRING:
						props_iter+=sizeof(gchar *);
						break;
					case G_TYPE_UINT64:
						props_iter+=sizeof(guint64);
						break;
					case G_TYPE_UINT:
						props_iter+=sizeof(guint);
						break;
				}
			}
			switch(archive->column_types[column])
			{
				case G_TYPE_STRING:
					g_value_set_string(value, *(gchar **)props_iter);
					break;
				case G_TYPE_UINT64:
					g_value_set_uint64(value, *(guint64 *)props_iter);
					break;
				case G_TYPE_UINT:
					g_value_set_uint(value, *(guint *)props_iter);
					break;
			}
		}
	}
}

static gboolean
xa_archive_store_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), FALSE);
	
	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);

	LXAEntry *entry = (LXAEntry*)iter->user_data2;
	gint pos = GPOINTER_TO_INT(iter->user_data3);
	pos++;
	
	entry = lxa_entry_children_nth_data(entry, pos);

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
	LXAEntry *entry = store->current_entry->data;

	g_return_val_if_fail(archive, FALSE);
	g_return_val_if_fail(entry, FALSE);

	/* only support lists: parent is always NULL */
	g_return_val_if_fail(parent == NULL, FALSE);

	if(store->props._show_up_dir && &archive->root_entry != entry)
	{
		entry = &store->up_entry;
		iter->user_data3 = GINT_TO_POINTER(-1);
	}
	else
	{
		entry = lxa_entry_children_nth_data(entry, 0);
	
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
	//g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), FALSE);
	return FALSE;
}

static gint
xa_archive_store_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), 0);

	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	LXAArchive *archive = store->archive;
	LXAEntry *entry = store->current_entry->data;

	g_return_val_if_fail(archive, 0);
	g_return_val_if_fail(entry, 0);

	/* only support lists: iter is always NULL */
	g_return_val_if_fail(iter == NULL, FALSE);

	return lxa_entry_children_length(entry) + (&archive->root_entry == entry)?0:1;
}

static gboolean 
xa_archive_store_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), FALSE);

	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	LXAArchive *archive = store->archive;
	LXAEntry *entry = store->current_entry->data;

	g_return_val_if_fail(archive, FALSE);
	g_return_val_if_fail(entry, FALSE);

	/* only support lists: parent is always NULL */
	g_return_val_if_fail(parent == NULL, FALSE);

	if(store->props._show_up_dir && &archive->root_entry != entry)
		n--;

	if(n == -1)
	{
		entry = &store->up_entry;
	}
	else
	{
		entry = lxa_entry_children_nth_data(entry, n);
	
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

	store->sort_column = sort_col_id;
	store->sort_order = order;
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
	return TRUE;
}


GtkTreeModel *
xa_archive_store_new(LXAArchive *archive, gboolean show_icons, gboolean show_up_dir)
{
	XAArchiveStore *tree_model;
	GType *column_types;
	gint x;

	tree_model = g_object_new(XA_TYPE_ARCHIVE_STORE, NULL);
	/*
	if(show_icons)
	{
		column_types = g_new0(GType, archive->column_number+1);
		for(x = 0; x < archive->column_number; x++)
		{
			column_types[x+1] = archive->column_types[x];
		}
		column_types[0] = G_TYPE_STRING;
		tree_model->n_columns = archive->column_number + 1;
	}
	else
	{
		column_types = g_new0(GType, archive->column_number);
		for(x = 0; x < archive->column_number; x++)
		{
			column_types[x] = archive->column_types[x];
		}
		tree_model->n_columns = archive->column_number;
	}
	tree_model->column_types = archive->column_types;
	*/
	tree_model->props._show_icons = show_icons;
	tree_model->props._show_up_dir = show_up_dir;

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
	LXAEntry *entry = store->current_entry->data;

	g_return_if_fail(archive);
	g_return_if_fail(entry);

	gint new_size = lxa_entry_children_length(entry);
	gint i = 0;
	gint index = 0;
	GtkTreePath *path_ = NULL;
	GtkTreeIter iter;

	if(store->props._show_up_dir && &archive->root_entry != entry) { 
		path_ = gtk_tree_path_new();
		gtk_tree_path_append_index(path_, 0);

		iter.stamp = store->stamp;
		iter.user_data = &store->up_entry;
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
		iter.user_data = lxa_entry_children_nth_data(entry, index);
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
	LXAArchive *archive = store->archive;
	LXAEntry *entry = store->current_entry->data;

	g_return_if_fail(archive);
	g_return_if_fail(entry);

	gint *indices = gtk_tree_path_get_indices(path);
	gint depth = gtk_tree_path_get_depth(path) - 1;

	/* only support list: depth is always 0 */
	g_return_if_fail(depth == 0);

	gint prev_size = lxa_entry_children_length(entry);
	gint index = indices[depth];

	if(store->props._show_up_dir && &archive->root_entry != entry)
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
		entry = lxa_entry_children_nth_data(entry, index);

		g_return_if_fail(entry);
		g_return_if_fail(entry->is_folder);

		store->current_entry = g_slist_prepend(store->current_entry, entry);
	}

	xa_archive_store_refresh(store, prev_size);
}

void
xa_archive_store_go_up(XAArchiveStore *store)
{
	LXAArchive *archive = store->archive;
	LXAEntry *entry = store->current_entry->data;

	g_return_if_fail(archive);
	g_return_if_fail(entry);

	gint prev_size = lxa_entry_children_length(entry);

	if(store->props._show_up_dir && &archive->root_entry != entry)
	{
		prev_size++;
	}

	store->current_entry = g_slist_delete_link(store->current_entry, store->current_entry);
	entry = store->current_entry->data;

	// TODO: signal or something
	g_return_if_fail(entry);

	xa_archive_store_refresh(store, prev_size);
}

/*
gboolean
xa_archive_store_add_item(gpointer key, gpointer value, gpointer data)
{
//	GtkTreeIter iter;
	XA_ARCHIVE_STORE(data)->rows = g_slist_prepend(XA_ARCHIVE_STORE(data)->rows, value);
	GtkTreePath *path = gtk_tree_path_new();
	gtk_tree_path_append_index(path, 0);

//	xa_archive_store_get_iter(GTK_TREE_MODEL(data), &iter, path);
//	gtk_tree_model_row_inserted(GTK_TREE_MODEL(data), path, &iter);
	gtk_tree_path_free(path);
	return FALSE;
}
*/

void
xa_archive_store_set_contents(XAArchiveStore *store, LXAArchive *archive)
{
	gint i = 0;
	GtkTreePath *path_ = NULL;
	GtkTreeIter iter;
	g_return_if_fail(store);

	LXAEntry *entry = NULL;
	gint prev_size =  0;

	if(store->current_entry)
	{
		entry = store->current_entry->data;
		prev_size = lxa_entry_children_length(entry);
		

		if(store->props._show_up_dir && &store->archive->root_entry != entry)
			prev_size++;

	}

	for(i = prev_size - 1; i >= 0; i--)
	{
		path_ = gtk_tree_path_new();
		gtk_tree_path_append_index(path_, i);

		gtk_tree_model_row_deleted(GTK_TREE_MODEL(store), path_);

		gtk_tree_path_free(path_);
	}

	if(!archive)
	{
		store->archive = NULL;
		store->current_entry = NULL;
		return;
	}

	store->archive = archive;
	store->current_entry = g_slist_prepend(NULL, &archive->root_entry);

	for(i = 0; i < lxa_entry_children_length(&archive->root_entry); i++)
	{
		path_ = gtk_tree_path_new();
		gtk_tree_path_append_index(path_, i);

		iter.stamp = store->stamp;
		iter.user_data = lxa_entry_children_nth_data(&archive->root_entry, i);
		iter.user_data2 = &archive->root_entry;
		iter.user_data3 = GINT_TO_POINTER(i);

		gtk_tree_model_row_inserted(GTK_TREE_MODEL(store), path_, &iter);

		gtk_tree_path_free(path_);
	}
}
