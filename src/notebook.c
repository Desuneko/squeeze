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
#include "navigation_bar.h"
#include "tool_bar.h"
#include "path_bar.h"
#include "notebook.h"

static void
xa_notebook_class_init(XANotebookClass *archive_class);

static void
xa_notebook_init(XANotebook *archive);
static void
xa_notebook_finalize(GObject *object);

static void
xa_notebook_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
xa_notebook_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static void
xa_notebook_treeview_reset_columns(LXAArchive *archive, GtkTreeView *treeview);

void
cb_notebook_close_archive(GtkButton *button, GtkTreeView *treeview);

void
cb_notebook_archive_status_changed(LXAArchive *archive, XANotebook *notebook);
void
cb_notebook_archive_refreshed(LXAArchive *archive, GtkTreeView *tree_view);
static void
cb_xa_notebook_page_switched(XANotebook *notebook, GtkNotebookPage *, guint page_nr, gpointer data);
static void
cb_xa_notebook_page_removed(XANotebook *notebook, GtkNotebookPage *page, guint page_nr, gpointer data);

enum {
	XA_NOTEBOOK_MULTI_TAB = 1
};

GType
xa_notebook_get_type ()
{
	static GType xa_notebook_type = 0;

 	if (!xa_notebook_type)
	{
 		static const GTypeInfo xa_notebook_info = 
		{
			sizeof (XANotebookClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_notebook_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (XANotebook),
			0,
			(GInstanceInitFunc) xa_notebook_init,
			NULL
		};

		xa_notebook_type = g_type_register_static (GTK_TYPE_NOTEBOOK, "XANotebook", &xa_notebook_info, 0);
	}
	return xa_notebook_type;
}

static void
xa_notebook_class_init(XANotebookClass *notebook_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (notebook_class);
	GParamSpec *pspec = NULL;

	object_class->set_property = xa_notebook_set_property;
	object_class->get_property = xa_notebook_get_property;
	object_class->finalize     = xa_notebook_finalize;

	pspec = g_param_spec_boolean("multi_tab",
		"",
		"",
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, XA_NOTEBOOK_MULTI_TAB, pspec);

}

static void
xa_notebook_init(XANotebook *notebook)
{
	g_signal_connect(G_OBJECT(notebook), "switch-page", G_CALLBACK(cb_xa_notebook_page_switched), NULL);
	g_signal_connect(G_OBJECT(notebook), "page-removed", G_CALLBACK(cb_xa_notebook_page_removed), NULL);
	notebook->tool_tips = gtk_tooltips_new();
	gtk_tooltips_enable(notebook->tool_tips);
	gtk_notebook_set_tab_border(GTK_NOTEBOOK(notebook), 0);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
}

static void
xa_notebook_finalize(GObject *object)
{
}

GtkWidget *
xa_notebook_new(XANavigationBar *bar)
{
	XANotebook *notebook;

	notebook = g_object_new(XA_TYPE_NOTEBOOK, NULL);

	notebook->props._up_dir = TRUE;
	if(bar)
	{
		xa_notebook_set_navigation_bar(notebook, bar);
		if(XA_IS_TOOL_BAR(bar) || XA_IS_PATH_BAR(bar))
			notebook->props._up_dir = FALSE;
	}

	notebook->props._show_icons = TRUE;

	return (GtkWidget *)notebook;
}

static void
xa_notebook_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case XA_NOTEBOOK_MULTI_TAB:
			XA_NOTEBOOK(object)->multi_tab = g_value_get_boolean(value);		
			break;
	}
}

static void
xa_notebook_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case XA_NOTEBOOK_MULTI_TAB:
			g_value_set_boolean(value, XA_NOTEBOOK(object)->multi_tab);
			break;
	}
}

void
xa_notebook_set_navigation_bar(XANotebook *notebook, XANavigationBar *bar)
{
	notebook->navigation_bar = bar;
}

void
xa_notebook_add_archive(XANotebook *notebook, LXAArchive *archive, LXAArchiveSupport *support)
{
	GtkWidget *lbl_hbox = gtk_hbox_new(FALSE, 0);
	GtkWidget *label = gtk_label_new(lxa_archive_get_filename(archive));
	GtkWidget *archive_image = gtk_image_new_from_icon_name(lxa_mime_info_get_icon_name(archive->mime_info, notebook->icon_theme), GTK_ICON_SIZE_MENU);
	GtkWidget *close_button = gtk_button_new();
	GtkWidget *close_image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);

	gtk_button_set_image(GTK_BUTTON(close_button), close_image);
	gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);

	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
	gtk_label_set_max_width_chars(GTK_LABEL(label), 20);
	gtk_tooltips_set_tip(notebook->tool_tips, label, lxa_archive_get_filename(archive), NULL);

	GtkWidget *tree_view = gtk_tree_view_new();
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree_view), TRUE);

	GtkTreeModel *tree_model = xa_archive_store_new(archive, notebook->props._show_icons, notebook->props._up_dir, notebook->icon_theme);

	gtk_box_pack_start(GTK_BOX(lbl_hbox), archive_image, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(lbl_hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(lbl_hbox), close_button, FALSE, FALSE, 0);
	gtk_widget_show_all(lbl_hbox);
	gtk_widget_show_all(tree_view);

	g_signal_connect(G_OBJECT(archive), "lxa_status_changed", G_CALLBACK(cb_notebook_archive_status_changed), notebook);
	g_signal_connect(G_OBJECT(archive), "lxa_refreshed", G_CALLBACK(cb_notebook_archive_refreshed), tree_view);

	g_signal_connect(G_OBJECT(close_button), "clicked", G_CALLBACK(cb_notebook_close_archive), tree_view);


	xa_archive_store_set_support(XA_ARCHIVE_STORE(tree_model), support);
	lxa_archive_support_refresh(support, archive);

	xa_archive_store_connect_treeview(XA_ARCHIVE_STORE(tree_model), GTK_TREE_VIEW(tree_view));
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), tree_model);
	if(gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), TRUE);
	else
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tree_view, lbl_hbox);
}

void
cb_notebook_archive_status_changed(LXAArchive *archive, XANotebook *notebook)
{
}

void
cb_notebook_close_archive(GtkButton *button, GtkTreeView *treeview)
{
	GtkNotebook *notebook = GTK_NOTEBOOK(gtk_widget_get_parent(GTK_WIDGET(treeview)));

	gint n = gtk_notebook_page_num(notebook, GTK_WIDGET(treeview));
	gtk_notebook_remove_page(notebook, n);
}

void
cb_notebook_archive_refreshed(LXAArchive *archive, GtkTreeView *treeview)
{
	xa_notebook_treeview_reset_columns(archive, treeview);

	GtkTreeModel *archive_store = gtk_tree_view_get_model(treeview);
	g_object_ref(archive_store);
	gtk_tree_view_set_model(treeview, NULL);
	xa_archive_store_set_archive(XA_ARCHIVE_STORE(archive_store), archive);
	gtk_tree_view_set_model(treeview, archive_store);
}

static void
xa_notebook_treeview_reset_columns(LXAArchive *archive, GtkTreeView *treeview)
{
	GtkCellRenderer *renderer = NULL;
	GtkTreeViewColumn *column = NULL;
	GtkTreeModel *archive_store = gtk_tree_view_get_model(treeview);
	gint x = 0;

	GValue *value = g_new0(GValue, 1);
	value = g_value_init(value, G_TYPE_UINT);
	g_value_set_uint(value, GTK_ICON_SIZE_SMALL_TOOLBAR);

	GList *columns = gtk_tree_view_get_columns(treeview);
	gboolean show_only_filenames = FALSE;

	while(columns)
	{
		gtk_tree_view_remove_column(treeview, columns->data);
		columns = columns->next;
	}
	g_list_free(columns);

	column = gtk_tree_view_column_new();

	if(XA_ARCHIVE_STORE(archive_store)->props._show_icons)
	{
		renderer = gtk_cell_renderer_pixbuf_new();
		g_object_set_property(G_OBJECT(renderer), "stock-size", value);
		gtk_tree_view_column_pack_start(column, renderer, FALSE);
		gtk_tree_view_column_set_attributes(column, renderer, "icon-name", 0, NULL);
	}

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", LXA_ARCHIVE_PROP_FILENAME + 1, NULL);

	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_sort_column_id(column, LXA_ARCHIVE_PROP_FILENAME);
	gtk_tree_view_column_set_title(column, lxa_archive_get_property_name(archive, LXA_ARCHIVE_PROP_FILENAME));
	gtk_tree_view_append_column(treeview, column);

	if(!show_only_filenames)
	{
		for(x = LXA_ARCHIVE_PROP_USER; x < lxa_archive_n_property(archive); ++x)
		{
			switch(lxa_archive_get_property_type(archive, x))
			{
				case(G_TYPE_STRING):
				case(G_TYPE_UINT64):
					renderer = gtk_cell_renderer_text_new();
					column = gtk_tree_view_column_new_with_attributes(lxa_archive_get_property_name(archive, x), renderer, "text", x+1, NULL);
					break;
			}
			gtk_tree_view_column_set_resizable(column, TRUE);
			gtk_tree_view_column_set_sort_column_id(column, x+1);
			gtk_tree_view_append_column(treeview, column);
		}
	}
	gtk_tree_view_set_search_column(treeview, 1);
}

void
xa_notebook_set_icon_theme(XANotebook *notebook, GtkIconTheme *icon_theme)
{
	notebook->icon_theme = icon_theme;
}

static void
cb_xa_notebook_page_switched(XANotebook *notebook, GtkNotebookPage *page, guint page_nr, gpointer data)
{
	GtkWidget *treeview = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page_nr);
	GtkTreeModel *archive_store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	xa_navigation_bar_set_store(notebook->navigation_bar, XA_ARCHIVE_STORE(archive_store));
}

static void
cb_xa_notebook_page_removed(XANotebook *notebook, GtkNotebookPage *page, guint page_nr, gpointer data)
{
	if(!gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
		xa_navigation_bar_set_store(notebook->navigation_bar, NULL);
}

void
xa_notebook_get_active_archive(XANotebook *notebook, LXAArchive **lp_archive, LXAArchiveSupport **lp_support)
{
	gint n = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));

	GtkWidget *treeview = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
	GtkTreeModel *archive_store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

	(*lp_archive) = xa_archive_store_get_archive(XA_ARCHIVE_STORE(archive_store));
	(*lp_support) = xa_archive_store_get_support(XA_ARCHIVE_STORE(archive_store));
}

GtkWidget *
xa_notebook_get_active_child(XANotebook *notebook)
{
	gint n = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));

	return gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
}
