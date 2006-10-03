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
xa_archive_store_init(XAArchiveStore *as)
{
	as->stamp = g_random_int();
}

static void
xa_archive_store_class_init(XAArchiveStoreClass *as_class)
{

}


static gboolean
xa_archive_store_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);

  gint *indices = gtk_tree_path_get_indices(path);

	if(indices[0] >= g_slist_length(store->rows) || indices[0] < 0)
		return FALSE;

	GSList *entry = g_slist_nth(store->rows, indices[0]);
	if(!entry)
		return FALSE;

	iter->stamp = store->stamp;
	iter->user_data = entry;
	iter->user_data2 = NULL;
	iter->user_data3 = NULL;
	return TRUE;
}

static GtkTreePath *
xa_archive_store_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	GtkTreePath *path = NULL;
	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	GSList *list = g_slist_find(store->rows, iter->user_data);
	if(list)
	{
		path = gtk_tree_path_new();
		gtk_tree_path_append_index(path, g_slist_position(store->rows, list));
	}
	return path;
}


static gboolean 
xa_archive_store_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
	XAArchiveStore *store = XA_ARCHIVE_STORE(tree_model);
	LXAEntry *entry = g_slist_nth_data(store->rows, n);
	if(!entry)
		return FALSE;
	iter->stamp = store->stamp;
	iter->user_data = entry;
	return FALSE;
}


/* done */

static void 
xa_archive_store_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value)
{
	guint i = 0;
	g_return_if_fail (XA_IS_ARCHIVE_STORE (tree_model));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (column < XA_ARCHIVE_STORE(tree_model)->n_columns);

	g_value_init(value, XA_ARCHIVE_STORE(tree_model)->column_types[column]);

	XAArchiveStore *archive_store = XA_ARCHIVE_STORE(tree_model);

	LXAEntry *entry = ((GSList *)iter->user_data)->data;
	gpointer props_iter = entry->props;

	if((column == 1 && archive_store->props._show_icons) || (column == 0))
	{
		if(archive_store->props._show_icons && column == 0)
			g_value_set_string(value, "unknown");
		else
			g_value_set_string(value, entry->filename);
	} else
	{
		if(archive_store->props._show_icons)
			i = 2;
		else
			i = 1;
		for(;i<column;i++)
		{
			switch(archive_store->column_types[i])
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
		switch(archive_store->column_types[column])
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

static gboolean
xa_archive_store_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), FALSE);
	if(iter == NULL || iter->user_data == NULL)
		return FALSE;
	
	if(!((GSList *)iter->user_data)->next)
		return FALSE;
	
	iter->stamp = XA_ARCHIVE_STORE(tree_model)->stamp;
	iter->user_data = ((GSList *)iter->user_data)->next;
	return TRUE;
}

static gboolean
xa_archive_store_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
	return FALSE;
}


static gboolean
xa_archive_store_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent)
{
	g_return_val_if_fail(parent == NULL || parent->user_data == NULL, FALSE);
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), FALSE);
	XAArchiveStore *archive_store = XA_ARCHIVE_STORE(tree_model);
	
	if(g_slist_length(archive_store->rows) == 0)
		return FALSE;
	
	iter->stamp = archive_store->stamp;
	iter->user_data = archive_store->rows;

	return TRUE;
}

static gboolean
xa_archive_store_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	return FALSE;
}

static GType
xa_archive_store_get_column_type(GtkTreeModel *tree_model, gint index)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), G_TYPE_INVALID);	
	return XA_ARCHIVE_STORE(tree_model)->column_types[index];
}

static gint
xa_archive_store_get_n_columns(GtkTreeModel *tree_model)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), 0);
	
	return XA_ARCHIVE_STORE(tree_model)->n_columns;
}

static GtkTreeModelFlags
xa_archive_store_get_flags(GtkTreeModel *tree_model)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), (GtkTreeModelFlags)0);

  return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

static gint
xa_archive_store_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail(XA_IS_ARCHIVE_STORE(tree_model), -1);
	g_return_val_if_fail(iter == NULL || iter->user_data != NULL, 0);

	XAArchiveStore *archive_store = XA_ARCHIVE_STORE(tree_model);

	return g_slist_length(archive_store->rows);
}

GtkTreeModel *
xa_archive_store_new(LXAArchive *archive, gboolean show_icons)
{
	XAArchiveStore *tree_model;
	GType *column_types;
	gint x;

	tree_model = g_object_new(XA_TYPE_ARCHIVE_STORE, NULL);
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
	tree_model->props._show_icons = show_icons;


	return GTK_TREE_MODEL(tree_model);
}

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

void
xa_archive_store_set_contents(XAArchiveStore *archive_store, GTree *items, gboolean is_root)
{
	g_tree_foreach(items, (GTraverseFunc)xa_archive_store_add_item, archive_store);
}
