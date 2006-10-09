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
#include "archive_tree_store.h"

typedef struct _XAParentTree XAParentTree;

struct _XAParentTree
{
	LXAEntry *entry;
	gint pos;
	XAParentTree *parent;
};

static void
xa_archive_tree_store_class_init(XAArchiveTreeStoreClass *as_class);

static void
xa_archive_tree_store_init(XAArchiveTreeStore *as);

static void
xa_archive_tree_model_init(GtkTreeModelIface *tm_interface);

static void
xa_archive_tree_sortable_init(GtkTreeSortableIface *ts_interface);

/* properties */
enum {
	XA_ARCHIVE_TREE_STORE_SHOW_ICONS = 1,
	XA_ARCHIVE_TREE_STORE_SHOW_ONLY_FILENAMES,
	XA_ARCHIVE_TREE_STORE_SHOW_ONLY_DIR
};

static void
xa_archive_tree_store_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
xa_archive_tree_store_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

/* tree model */
static GtkTreeModelFlags
xa_archive_tree_store_get_flags(GtkTreeModel *tree_model);
static gint
xa_archive_tree_store_get_n_columns(GtkTreeModel *tree_model);
static GType
xa_archive_tree_store_get_column_type(GtkTreeModel *tree_model, gint index);
static gboolean
xa_archive_tree_store_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path);
static GtkTreePath *
xa_archive_tree_store_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter);
static void 
xa_archive_tree_store_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value);
static gboolean
xa_archive_tree_store_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter);
static gboolean
xa_archive_tree_store_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent);
static gboolean
xa_archive_tree_store_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter);
static gint
xa_archive_tree_store_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter);
static gboolean 
xa_archive_tree_store_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n);
static gboolean
xa_archive_tree_store_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child);
/*
static void
xa_archive_tree_store_refresh(XAArchiveTreeStore *store, gint prev_size);
*/
static void
cb_xa_archive_tree_store_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);

/* tree sortable */
static gboolean
xa_archive_tree_store_get_sort_column_id(GtkTreeSortable *sortable, gint *sort_col_id, GtkSortType *order);
static void
xa_archive_tree_store_set_sort_column_id(GtkTreeSortable *sortable, gint sort_col_id, GtkSortType order);
static void
xa_archive_tree_store_set_sort_func(GtkTreeSortable *, gint, GtkTreeIterCompareFunc, gpointer, GtkDestroyNotify);
static void
xa_archive_tree_store_set_default_sort_func(GtkTreeSortable *, GtkTreeIterCompareFunc, gpointer, GtkDestroyNotify);
static gboolean
xa_archive_tree_store_has_default_sort_func(GtkTreeSortable *);

static gint
xa_archive_entry_compare(XAArchiveTreeStore *store, LXAEntry *a, LXAEntry *b);
static void
xa_archive_quicksort(XAArchiveTreeStore *store, gint left, gint right);
static void
xa_archive_insertionsort(XAArchiveTreeStore *store, gint left, gint right);
/*
static void
xa_archive_tree_store_sort(XAArchiveTreeStore *store);
*/

GType
xa_archive_tree_store_get_type()
{
	static GType xa_archive_tree_store_type= 0;

	if(xa_archive_tree_store_type)
		return xa_archive_tree_store_type;

	if (!xa_archive_tree_store_type)
	{
		static const GTypeInfo xa_archive_tree_store_info = 
		{
			sizeof (XAArchiveTreeStoreClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_archive_tree_store_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XAArchiveTreeStore),
			0,
			(GInstanceInitFunc) xa_archive_tree_store_init,
			NULL
		};

		xa_archive_tree_store_type = g_type_register_static (G_TYPE_OBJECT, "XAArchiveTreeStore", &xa_archive_tree_store_info, 0);
	}
	static const GInterfaceInfo tree_model_info =
	{
		(GInterfaceInitFunc) xa_archive_tree_model_init,
			NULL,
			NULL
	};

	g_type_add_interface_static (xa_archive_tree_store_type, GTK_TYPE_TREE_MODEL, &tree_model_info);

	static const GInterfaceInfo tree_sort_info =
	{
		(GInterfaceInitFunc) xa_archive_tree_sortable_init,
			NULL,
			NULL
	};

//	g_type_add_interface_static (xa_archive_tree_store_type, GTK_TYPE_TREE_SORTABLE, &tree_sort_info);

	return xa_archive_tree_store_type;
}

static void
xa_archive_tree_model_init(GtkTreeModelIface *iface)
{
	iface->get_flags       = xa_archive_tree_store_get_flags;
	iface->get_n_columns   = xa_archive_tree_store_get_n_columns;
	iface->get_column_type = xa_archive_tree_store_get_column_type;
	iface->get_iter        = xa_archive_tree_store_get_iter;
	iface->get_path        = xa_archive_tree_store_get_path;
	iface->get_value       = xa_archive_tree_store_get_value;
	iface->iter_next       = xa_archive_tree_store_iter_next;
	iface->iter_children   = xa_archive_tree_store_iter_children;
	iface->iter_has_child  = xa_archive_tree_store_iter_has_child;
	iface->iter_n_children = xa_archive_tree_store_iter_n_children;
	iface->iter_nth_child  = xa_archive_tree_store_iter_nth_child;
	iface->iter_parent     = xa_archive_tree_store_iter_parent;
}

static void
xa_archive_tree_sortable_init(GtkTreeSortableIface *iface)
{
	iface->get_sort_column_id    = xa_archive_tree_store_get_sort_column_id;
	iface->set_sort_column_id    = xa_archive_tree_store_set_sort_column_id;
	iface->set_sort_func         = xa_archive_tree_store_set_sort_func;        	/*NOT SUPPORTED*/
	iface->set_default_sort_func = xa_archive_tree_store_set_default_sort_func;	/*NOT SUPPORTED*/
	iface->has_default_sort_func = xa_archive_tree_store_has_default_sort_func;
}

static void
xa_archive_tree_store_init(XAArchiveTreeStore *as)
{
	as->stamp = g_random_int();
	as->archive = NULL;
	as->props._show_icons = FALSE;
	as->props._show_only_filename = TRUE;
	as->props._show_only_dir = TRUE;
	as->sort_column = GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID;
	as->sort_order = GTK_SORT_ASCENDING;
	as->sort_list = NULL;
}

static void
xa_archive_tree_store_class_init(XAArchiveTreeStoreClass *as_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (as_class);
	GParamSpec *pspec = NULL;

	object_class->set_property = xa_archive_tree_store_set_property;
	object_class->get_property = xa_archive_tree_store_get_property;

	pspec = g_param_spec_boolean("show_icons",
		_("Show mime icons"),
		_("Show the mime type icons for each entry"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, XA_ARCHIVE_TREE_STORE_SHOW_ICONS, pspec);

	pspec = g_param_spec_boolean("show_only_names",
		_("Show only filenames"),
		_("Show only filenames (doesn't overwrite show icons)"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, XA_ARCHIVE_TREE_STORE_SHOW_ICONS, pspec);

	pspec = g_param_spec_boolean("show_only_dir",
		_("Show only directory entries"),
		_("Show only directory entries (no file)"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, XA_ARCHIVE_TREE_STORE_SHOW_ONLY_DIR, pspec);
}

static void
xa_archive_tree_store_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case XA_ARCHIVE_TREE_STORE_SHOW_ICONS:
			XA_ARCHIVE_TREE_STORE(object)->props._show_icons = g_value_get_boolean(value);
			break;
		case XA_ARCHIVE_TREE_STORE_SHOW_ONLY_FILENAMES:
			XA_ARCHIVE_TREE_STORE(object)->props._show_only_filename = g_value_get_boolean(value);
			break;
		case XA_ARCHIVE_TREE_STORE_SHOW_ONLY_DIR:
			XA_ARCHIVE_TREE_STORE(object)->props._show_only_dir = g_value_get_boolean(value);
			break;
	}
}

static void
xa_archive_tree_store_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case XA_ARCHIVE_TREE_STORE_SHOW_ICONS:
			g_value_set_boolean(value, XA_ARCHIVE_TREE_STORE(object)->props._show_icons);
			break;
		case XA_ARCHIVE_TREE_STORE_SHOW_ONLY_FILENAMES:
			g_value_set_boolean(value, XA_ARCHIVE_TREE_STORE(object)->props._show_only_filename);
			break;
		case XA_ARCHIVE_TREE_STORE_SHOW_ONLY_DIR:
			g_value_set_boolean(value, XA_ARCHIVE_TREE_STORE(object)->props._show_only_dir);
			break;
	}
}

static GtkTreeModelFlags
xa_archive_tree_store_get_flags(GtkTreeModel *tree_model)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_TREE_STORE(tree_model), (GtkTreeModelFlags)0);

	return (GTK_TREE_MODEL_ITERS_PERSIST);
}

static gint
xa_archive_tree_store_get_n_columns(GtkTreeModel *tree_model)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_TREE_STORE(tree_model), 0);

	XAArchiveTreeStore *store = XA_ARCHIVE_TREE_STORE(tree_model);
	LXAArchive *archive = store->archive;

	if(!archive)
		return 0;
	
	return (store->props._show_only_filename?1:archive->column_number) + (store->props._show_icons?1:0);
}

static GType
xa_archive_tree_store_get_column_type(GtkTreeModel *tree_model, gint index)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_TREE_STORE(tree_model), G_TYPE_INVALID);	

	XAArchiveTreeStore *store = XA_ARCHIVE_TREE_STORE(tree_model);
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
xa_archive_tree_store_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_TREE_STORE(tree_model), FALSE);	

	XAArchiveTreeStore *store = XA_ARCHIVE_TREE_STORE(tree_model);
	LXAArchive *archive = store->archive;

	g_return_val_if_fail(archive, FALSE);

	gint *indices = gtk_tree_path_get_indices(path);
	gint depth = gtk_tree_path_get_depth(path);
	gint i = 0;
	LXAEntry *entry = &archive->root_entry;
	LXAEntry *pentry = NULL;
	XAParentTree *tree = NULL;
	XAParentTree *ptree = NULL;
	gint ppos = 0;
	gint index = 0;

	for(i = 0; i < depth; i++)
	{
		ppos = index;
		index = indices[i];

		pentry = entry;
		entry = lxa_entry_children_nth_data(pentry, index);

		g_return_val_if_fail(entry, FALSE);

		ptree = tree;
		tree = g_new(XAParentTree, 1);
		tree->entry = pentry;
		tree->pos = ppos;
		tree->parent = ptree;
	}

	iter->stamp = store->stamp;
	iter->user_data = entry;
	iter->user_data2 = tree;
	iter->user_data3 = GINT_TO_POINTER(index);

	return TRUE;
}

static GtkTreePath *
xa_archive_tree_store_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_TREE_STORE(tree_model), NULL);	

	XAArchiveTreeStore *store = XA_ARCHIVE_TREE_STORE(tree_model);

	XAParentTree *tree = (XAParentTree*)iter->user_data2;
	gint pos = GPOINTER_TO_INT(iter->user_data3);
	GtkTreePath *path = gtk_tree_path_new();

	do
	{
		gtk_tree_path_prepend_index(path, pos);
		pos = tree->pos;
		tree = tree->parent;
	}
	while(tree);

	return path;
}


/* done */

static void 
xa_archive_tree_store_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value)
{
	g_return_if_fail (XA_IS_ARCHIVE_TREE_STORE (tree_model));

	XAArchiveTreeStore *store = XA_ARCHIVE_TREE_STORE(tree_model);
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
	const gchar *icon_name = NULL;
	ThunarVfsMimeDatabase *mime_base = NULL;
	ThunarVfsMimeInfo *mime_info = NULL;

	if(column == -1)
	{
		g_value_init(value, G_TYPE_STRING);

		if(store->icon_theme)
		{
			mime_base = thunar_vfs_mime_database_get_default();
			mime_info = thunar_vfs_mime_database_get_info(mime_base, entry->mime_type);
			icon_name = thunar_vfs_mime_info_lookup_icon_name(mime_info, store->icon_theme);

			if(gtk_icon_theme_has_icon(store->icon_theme, icon_name))
				g_value_set_string(value, icon_name);

			thunar_vfs_mime_info_unref(mime_info);
			g_object_unref(mime_base);
		}
	}
	else if(column == 0)
	{
		g_value_set_string(value, entry->filename);
	}
	else
	{
		if(props_iter)
		{
			for(i=1;i<column;i++)
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
xa_archive_tree_store_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_TREE_STORE(tree_model), FALSE);
	
	XAArchiveTreeStore *store = XA_ARCHIVE_TREE_STORE(tree_model);

	XAParentTree *tree = (XAParentTree*)iter->user_data2;
	LXAEntry *entry = tree->entry;
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
xa_archive_tree_store_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent)
{
	return xa_archive_tree_store_iter_nth_child(tree_model, iter, parent, 0);
}

static gboolean
xa_archive_tree_store_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	return xa_archive_tree_store_iter_n_children(tree_model, iter) > 0;
}

static gint
xa_archive_tree_store_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_TREE_STORE(tree_model), 0);

	XAArchiveTreeStore *store = XA_ARCHIVE_TREE_STORE(tree_model);
	LXAArchive *archive = store->archive;

	g_return_val_if_fail(archive, 0);

	LXAEntry *entry = &archive->root_entry;

	if(iter)
	{
		entry = (LXAEntry*)iter->user_data;
	}

	return lxa_entry_children_length(entry);
}

static gboolean 
xa_archive_tree_store_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_TREE_STORE(tree_model), FALSE);

	XAArchiveTreeStore *store = XA_ARCHIVE_TREE_STORE(tree_model);
	LXAArchive *archive = store->archive;
	XAParentTree *tree = NULL;
	XAParentTree *ptree = NULL;
	LXAEntry *entry = NULL;
	gint ppos = 0;

	g_return_val_if_fail(archive, FALSE);

	LXAEntry *pentry = &archive->root_entry;

	if(parent)
	{
		pentry = (LXAEntry*)parent->user_data;
		ptree = (XAParentTree*)parent->user_data2;
		ppos = GPOINTER_TO_INT(parent->user_data3);
	}

	entry = lxa_entry_children_nth_data(pentry, n);

	g_return_val_if_fail(entry, FALSE);

	tree = g_new(XAParentTree, 1);
	tree->entry = pentry;
	tree->pos = ppos;
	tree->parent = ptree;

	iter->stamp = store->stamp;
	iter->user_data = entry;
	iter->user_data2 = tree;
	iter->user_data3 = GINT_TO_POINTER(n);

	return TRUE;
}

static gboolean
xa_archive_tree_store_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_TREE_STORE(tree_model), FALSE);

	g_return_val_if_fail(iter, FALSE);

	XAArchiveTreeStore *store = XA_ARCHIVE_TREE_STORE(tree_model);
	XAParentTree *tree = (XAParentTree*)iter->user_data2;

	g_return_val_if_fail(iter->stamp == store->stamp, FALSE);

	g_debug("get parent of %s", ((LXAEntry*)iter->user_data)->filename);

	if(!tree || !tree->parent)
		return FALSE;

	iter->stamp = store->stamp;
	iter->user_data = tree->entry;
	iter->user_data2 = tree->parent;
	iter->user_data3 = GINT_TO_POINTER(tree->pos);

	return TRUE;
}


static gboolean
xa_archive_tree_store_get_sort_column_id(GtkTreeSortable *sortable, gint *sort_col_id, GtkSortType *order)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_TREE_STORE(sortable), FALSE);

	XAArchiveTreeStore *store = XA_ARCHIVE_TREE_STORE(sortable);

	if(sort_col_id)
		*sort_col_id = store->sort_column;

	if(order)
		*order = store->sort_order;

	return store->sort_order >= 0;
}

static void
xa_archive_tree_store_set_sort_column_id(GtkTreeSortable *sortable, gint sort_col_id, GtkSortType order)
{
	g_return_if_fail(XA_IS_ARCHIVE_TREE_STORE(sortable));

	XAArchiveTreeStore *store = XA_ARCHIVE_TREE_STORE(sortable);

	if(store->sort_column == sort_col_id && store->sort_order == order)
		return;

	if(store->props._show_icons && sort_col_id == 0)
		return;

	store->sort_column = sort_col_id;
	store->sort_order = order;

/*	xa_archive_tree_store_sort(store); */

	gtk_tree_sortable_sort_column_changed(sortable);
}

static void
xa_archive_tree_store_set_sort_func(GtkTreeSortable *s, gint i, GtkTreeIterCompareFunc f, gpointer p, GtkDestroyNotify d)
{
	g_warning("%s is not supported by the XAArchiveTreeStore model", __FUNCTION__);
}

static void
xa_archive_tree_store_set_default_sort_func(GtkTreeSortable *s, GtkTreeIterCompareFunc f, gpointer p, GtkDestroyNotify d)
{
	g_warning("%s is not supported by the XAArchiveTreeStore model", __FUNCTION__);
}

static gboolean
xa_archive_tree_store_has_default_sort_func(GtkTreeSortable *s)
{
	return TRUE;
}

static gint
xa_archive_entry_compare(XAArchiveTreeStore *store, LXAEntry *a, LXAEntry *b)
{
	LXAEntry *swap = b;
	if(store->sort_order == GTK_SORT_DESCENDING)
	{
		b = a;
		a = swap;
	}

	LXAArchive *archive = store->archive;
	gint column = store->sort_column;
	gpointer props_a = a->props;
	gpointer props_b = b->props;
	gint i = 0;

	if(store->props._show_icons)
		column--;

	if(column < 0)
		return;

	if(!props_a)
	{
		return props_b?-1:0;
	}
	if(!props_b)
	{
		return props_a?1:0;
	}

	if(column == 0)
	{
		props_a = &(a->filename);
		props_b = &(b->filename);
	}

	for(i=1;i<column;i++)
	{
		switch(archive->column_types[i])
		{
			case G_TYPE_STRING:
				props_a+=sizeof(gchar *);
				props_b+=sizeof(gchar *);
				break;
			case G_TYPE_UINT64:
				props_a+=sizeof(guint64);
				props_b+=sizeof(guint64);
				break;
			case G_TYPE_UINT:
				props_a+=sizeof(guint);
				props_b+=sizeof(guint);
				break;
		}
	}

	switch(archive->column_types[column])
	{
		case G_TYPE_STRING:
			switch(/* string compare type */1)
			{
				default:
					return strcmp(*((gchar**)props_a), *((gchar**)props_b));
			}
		case G_TYPE_UINT64:
			return (*((guint64*)props_a)) - (*((guint64*)props_b));
		case G_TYPE_UINT:
			return (*((guint*)props_a)) - (*((guint*)props_b));
	}
}

/*
static void
xa_archive_tree_store_sort(XAArchiveTreeStore *store)
{
	g_free(store->sort_list);
	store->sort_list = NULL;

	if(store->sort_column < 0)
		return;

	LXAEntry *pentry = (LXAEntry*)store->current_entry->data;
	gint psize = lxa_entry_children_length(pentry);
	gint i = 0;

	if(psize <= 1)
		return;

	store->sort_list = g_new(LXAEntry*, psize);

	for(i = 0; i < psize; i++)
	{
		store->sort_list[i] = lxa_entry_children_nth_data(pentry, i);
	}
	xa_archive_quicksort(store, 0, psize-1);
	xa_archive_insertionsort(store, 0, psize-1);
}
*/
/*
inline void
swap(LXAEntry **left, LXAEntry **right)
{
	LXAEntry *tmp = *right;
	*right = *left;
	*left = tmp;
}
*/
/*
static void
xa_archive_quicksort(XAArchiveTreeStore *store, gint left, gint right)
{
	if(right-left < 30)	return;

	gint i = (left+right)/2;
	gint j = right-1;
	LXAEntry *value = NULL;
	LXAEntry **list = store->sort_list;

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
*/
/*
static void
xa_archive_insertionsort(XAArchiveTreeStore *store, gint left, gint right)
{
	gint i = 0;
	gint j = 0;
	LXAEntry *value = NULL;
	LXAEntry **list = store->sort_list;

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
*/
GtkTreeModel *
xa_archive_tree_store_new(LXAArchive *archive, gboolean show_icons, gboolean show_only_names, gboolean show_only_dir, GtkIconTheme *icon_theme)
{
	XAArchiveTreeStore *tree_model;
	GType *column_types;
	gint x;

	tree_model = g_object_new(XA_TYPE_ARCHIVE_TREE_STORE, NULL);

	tree_model->props._show_icons = show_icons;
	tree_model->props._show_only_filename = show_only_names;
	tree_model->props._show_only_dir = show_only_dir;
	tree_model->icon_theme = icon_theme;

	xa_archive_tree_store_set_contents(tree_model, archive);

	return GTK_TREE_MODEL(tree_model);
}

void
xa_archive_tree_store_connect_treeview(XAArchiveTreeStore *store, GtkTreeView *treeview)
{
	g_signal_connect(G_OBJECT(treeview), "row-activated", G_CALLBACK(cb_xa_archive_tree_store_row_activated), store);
}

/*
static void
xa_archive_tree_store_refresh(XAArchiveTreeStore *store, gint prev_size)
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
		if(store->sort_list)
			iter.user_data = store->sort_list[i];
		else
			iter.user_data = lxa_entry_children_nth_data(&archive->root_entry, i);
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
*/

static void
cb_xa_archive_tree_store_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	g_return_if_fail(XA_IS_ARCHIVE_TREE_STORE(user_data));	

	XAArchiveTreeStore *store = XA_ARCHIVE_TREE_STORE(user_data);
	LXAArchive *archive = store->archive;
	LXAEntry *entry = &archive->root_entry;
	LXAEntry *pentry = NULL;

	g_return_if_fail(archive);

	gint *indices = gtk_tree_path_get_indices(path);
	gint depth = gtk_tree_path_get_depth(path);
	gint i = 0;

	for(i = 0; i < depth; i++)
	{
		pentry = entry;
		entry = lxa_entry_children_nth_data(pentry, indices[i]);

		g_return_if_fail(entry);
	}

	/* TODO Signal file-activated */
	g_debug("callback entry %s", entry->filename);
}

static void
xa_archive_tree_store_refresh(XAArchiveTreeStore *store, GtkTreePath *path, XAParentTree *ptree)
{
	GtkTreeIter iter;
	GtkTreePath *path_ = NULL;
	LXAEntry *entry = NULL;
	XAParentTree *tree = NULL;
	gint i = 0;

	for(i = 0; i < lxa_entry_children_length(ptree->entry); i++)
	{
		path_ = gtk_tree_path_copy(path);
		gtk_tree_path_append_index(path_, i);

		entry = lxa_entry_children_nth_data(ptree->entry, i);
		iter.stamp = store->stamp;
		iter.user_data = entry;
		iter.user_data2 = ptree;
		iter.user_data3 = GINT_TO_POINTER(i);

		gtk_tree_model_row_inserted(GTK_TREE_MODEL(store), path_, &iter);

		tree = g_new(XAParentTree, 1);
		tree->entry = entry;
		tree->pos = i;
		tree->parent = ptree;

		xa_archive_tree_store_refresh(store, path_, tree);

		gtk_tree_path_free(path_);
	}
}

void
xa_archive_tree_store_set_contents(XAArchiveTreeStore *store, LXAArchive *archive)
{
	gint i = 0;
	GtkTreePath *path_ = NULL;
	GtkTreeIter iter;
	g_return_if_fail(store);

	LXAEntry *entry = NULL;
	gint prev_size =  0;

	if(store->archive)
	{
		entry = &store->archive->root_entry;
		prev_size = lxa_entry_children_length(entry);
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
		return;
	}

	store->archive = archive;

	XAParentTree *tree = g_new(XAParentTree, 1);
	tree->entry = &archive->root_entry;
	tree->pos = 0;
	tree->parent = NULL;

	path_ = gtk_tree_path_new();

	xa_archive_tree_store_refresh(store, path_, tree);
}
/*
gchar *
xa_archive_tree_store_get_pwd(XAArchiveTreeStore *store)
{
	g_return_val_if_fail(store, NULL);

	gchar *path = NULL;
	gchar **buf = NULL;
	GSList *iter = store->current_entry;
	gint i = g_slist_length(iter);

	if(!i)
		return NULL;

	buf = g_new(gchar*, i+1);
	buf[i] = NULL;

	while(iter)
	{
		--i;
		buf[i] = ((LXAEntry*)iter->data)->filename;
		iter = iter->next;
	}

	path = g_strjoinv("/", buf);

	g_free(buf);

	return path;
}
*/
/*
GSList *
xa_archive_tree_store_get_pwd_list(XAArchiveTreeStore *store)
{
	g_return_val_if_fail(store, NULL);

	*//* made a copy, don't want someone play with the internals *//*
	return g_slist_copy(store->current_entry);
}
*/
/*
gchar *
xa_archive_tree_store_get_basename(XAArchiveTreeStore *store)
{
	g_return_val_if_fail(store, NULL);

	if(!store->current_entry)
		return NULL;

	return g_strdup(((LXAEntry*)store->current_entry->data)->filename);
}
*/
/*
gint
xa_archive_tree_store_set_pwd(XAArchiveTreeStore *store, gchar *path)
{
	g_return_val_if_fail(store, -1);

	if(!store->archive)
		return -1;

	gchar **buf = g_strsplit(path, "/", 0);
	gchar **iter = buf;
	LXAEntry *entry = &store->archive->root_entry;
	GSList *stack = NULL;

	while(iter)
	{
		if(*iter && iter[0])
		{
			entry = lxa_entry_get_child(entry, *iter);
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
	
	return 0;
}
*/

