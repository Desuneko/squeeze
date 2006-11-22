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

static void
cb_notebook_close_archive(GtkButton *button, GtkWidget *child);

static void
cb_notebook_archive_status_changed(LXAArchive *archive, XANotebook *notebook);
static void
cb_notebook_archive_refreshed(LXAArchive *archive, GtkTreeView *tree_view);
static void
cb_notebook_file_activated(XAArchiveStore *, gchar *, XANotebook *);

static void
cb_xa_notebook_page_switched(XANotebook *notebook, GtkNotebookPage *, guint page_nr, gpointer data);
static void
cb_xa_notebook_page_removed(XANotebook *notebook, gpointer data);

enum {
	XA_NOTEBOOK_MULTI_TAB = 1
};

static gint xa_notebook_signals[4];

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

	xa_notebook_signals[0] = g_signal_new("archive-removed",
			G_TYPE_FROM_CLASS(notebook_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
			NULL);

	xa_notebook_signals[1] = g_signal_new("page-up",
			G_TYPE_FROM_CLASS(notebook_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0,
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0, NULL);

	xa_notebook_signals[2] = g_signal_new("page-down",
			G_TYPE_FROM_CLASS(notebook_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0,
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0, NULL);

	xa_notebook_signals[3] = g_signal_new("xa_file_activated",
			G_TYPE_FROM_CLASS(notebook_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0,
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1, G_TYPE_STRING, NULL);

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
	g_signal_connect(G_OBJECT(notebook), "archive-removed", G_CALLBACK(cb_xa_notebook_page_removed), NULL);

	g_signal_connect(G_OBJECT(notebook), "page-up", G_CALLBACK(gtk_notebook_next_page),  NULL);
	g_signal_connect(G_OBJECT(notebook), "page-down", G_CALLBACK(gtk_notebook_prev_page), NULL);

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
xa_notebook_new(XANavigationBar *bar, gboolean use_tabs, GtkAccelGroup *accel_group)
{
	XANotebook *notebook;

	notebook = g_object_new(XA_TYPE_NOTEBOOK, NULL);

	notebook->props._up_dir = TRUE;
	if(bar)
	{
		xa_notebook_set_navigation_bar(notebook, bar);
#ifdef ENABLE_TOOLBAR
		if(XA_IS_TOOL_BAR(bar))
			notebook->props._up_dir = FALSE;
#endif
#ifdef ENABLE_PATHBAR
		if(XA_IS_PATH_BAR(bar))
			notebook->props._up_dir = FALSE;
#endif
	}

	notebook->props._show_icons = TRUE;
	notebook->multi_tab = use_tabs;
	notebook->accel_group = accel_group;

	gtk_widget_add_accelerator(GTK_WIDGET(notebook), "page-up", accel_group, 0xff55, GDK_CONTROL_MASK, 0);
	gtk_widget_add_accelerator(GTK_WIDGET(notebook), "page-down", accel_group, 0xff56, GDK_CONTROL_MASK, 0);

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

gboolean
xa_notebook_get_multi_tab(XANotebook *notebook)
{
	return notebook->multi_tab;
}

void
xa_notebook_set_navigation_bar(XANotebook *notebook, XANavigationBar *bar)
{
	if(notebook->navigation_bar)
		xa_navigation_bar_set_store(notebook->navigation_bar, NULL);

	if(gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
	{
		gint page_nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
		GtkWidget *scrolledwindow = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page_nr);
		GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
		GtkTreeModel *archive_store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
		notebook->navigation_bar = bar;
		xa_navigation_bar_set_store(notebook->navigation_bar, XA_ARCHIVE_STORE(archive_store));
	}
	else
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
	GtkWidget *scroll_window = gtk_scrolled_window_new(NULL, NULL);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_button_set_image(GTK_BUTTON(close_button), close_image);
	gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);

	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
	gtk_label_set_max_width_chars(GTK_LABEL(label), 20);
	gtk_tooltips_set_tip(notebook->tool_tips, label, lxa_archive_get_filename(archive), NULL);

	GtkWidget *tree_view = gtk_tree_view_new();
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree_view), TRUE);

	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (tree_view) );
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	GtkTreeModel *tree_model = xa_archive_store_new(archive, notebook->props._show_icons, notebook->props._up_dir, notebook->icon_theme);

	gtk_box_pack_start(GTK_BOX(lbl_hbox), archive_image, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(lbl_hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(lbl_hbox), close_button, FALSE, FALSE, 0);
	gtk_widget_show_all(lbl_hbox);
	gtk_widget_show_all(tree_view);
	gtk_widget_show(scroll_window);

	g_signal_connect(G_OBJECT(archive), "lxa_status_changed", G_CALLBACK(cb_notebook_archive_status_changed), notebook);
	g_signal_connect(G_OBJECT(archive), "lxa_refreshed", G_CALLBACK(cb_notebook_archive_refreshed), tree_view);

	g_signal_connect(G_OBJECT(close_button), "clicked", G_CALLBACK(cb_notebook_close_archive), scroll_window);
	g_signal_connect(G_OBJECT(tree_model), "xa_file_activated", G_CALLBACK(cb_notebook_file_activated), notebook);


	xa_archive_store_set_support(XA_ARCHIVE_STORE(tree_model), support);
	lxa_archive_support_refresh(support, archive);

	xa_archive_store_connect_treeview(XA_ARCHIVE_STORE(tree_model), GTK_TREE_VIEW(tree_view));
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), tree_model);
	if(gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), TRUE);
	else
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);

	gtk_container_add(GTK_CONTAINER(scroll_window), tree_view);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll_window, lbl_hbox);
}

void
cb_notebook_archive_status_changed(LXAArchive *archive, XANotebook *notebook)
{
#ifdef DEBUG
	g_debug("NOTEBOOK: Archive status changed");
#endif /* DEBUG */
}

static void
cb_notebook_close_archive(GtkButton *button, GtkWidget *child)
{
	GtkNotebook *notebook = GTK_NOTEBOOK(gtk_widget_get_parent(child));

	GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(child));
	GtkTreeModel *archive_store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

	LXAArchive *archive = xa_archive_store_get_archive(XA_ARCHIVE_STORE(archive_store));

	g_signal_handlers_disconnect_by_func(archive, cb_notebook_archive_refreshed, treeview);

	lxa_close_archive(archive);

	gint n = gtk_notebook_page_num(notebook, child);
	gtk_notebook_remove_page(notebook, n);
	g_signal_emit(G_OBJECT(notebook), xa_notebook_signals[0], 0, NULL);
}

void
xa_notebook_close_active_archive(XANotebook *notebook)
{
	GtkNotebook *_notebook = GTK_NOTEBOOK(notebook);
	gint n = gtk_notebook_get_current_page(_notebook);
	gtk_notebook_remove_page(_notebook, n);
	g_signal_emit(G_OBJECT(notebook), xa_notebook_signals[0], 0, NULL);
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
	GtkWidget *scrolledwindow = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page_nr);
	GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
	GtkTreeModel *archive_store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	xa_navigation_bar_set_store(notebook->navigation_bar, XA_ARCHIVE_STORE(archive_store));
}

static void
cb_xa_notebook_page_removed(XANotebook *notebook, gpointer data)
{
	if(!gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
		xa_navigation_bar_set_store(notebook->navigation_bar, NULL);
}

static void
cb_notebook_file_activated(XAArchiveStore *store, gchar *filename, XANotebook *notebook)
{
	gchar *pwd = xa_archive_store_get_pwd(store);
	gchar *path = g_strconcat(pwd, filename, NULL);
	g_signal_emit(G_OBJECT(notebook), xa_notebook_signals[3], 0, path, NULL);
}


void
xa_notebook_get_active_archive(XANotebook *notebook, LXAArchive **lp_archive, LXAArchiveSupport **lp_support)
{
	gint n = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));

	GtkWidget *scrolledwindow = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
	GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
	GtkTreeModel *archive_store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

	if(lp_archive)
		(*lp_archive) = xa_archive_store_get_archive(XA_ARCHIVE_STORE(archive_store));
	if(lp_support)
		(*lp_support) = xa_archive_store_get_support(XA_ARCHIVE_STORE(archive_store));
}

GtkWidget *
xa_notebook_get_active_child(XANotebook *notebook)
{
	gint n = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));

	return gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
}

void
xa_notebook_page_set_archive(XANotebook *notebook, LXAArchive *archive, LXAArchiveSupport *support, gint n)
{
	if(gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
	{
		GtkWidget *scrolledwindow = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
		GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
		GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

		xa_archive_store_set_archive(XA_ARCHIVE_STORE(store), archive);
		xa_archive_store_set_support(XA_ARCHIVE_STORE(store), support);

		g_signal_connect(G_OBJECT(archive), "lxa_status_changed", G_CALLBACK(cb_notebook_archive_status_changed), notebook);
		g_signal_connect(G_OBJECT(archive), "lxa_refreshed", G_CALLBACK(cb_notebook_archive_refreshed), treeview);

		lxa_archive_support_refresh(support, archive);

		gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), store);
	}
	else
		xa_notebook_add_archive(XA_NOTEBOOK(notebook), archive, support);
}

GSList *
xa_notebook_get_selected_items(XANotebook *notebook)
{
	GtkWidget *scrolledwindow = xa_notebook_get_active_child(notebook);
	GtkTreeIter iter;
	GValue *value = g_new0(GValue, 1);
	GSList *filenames = NULL;

	GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
	GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	gchar *pwd = xa_archive_store_get_pwd(XA_ARCHIVE_STORE(store));
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &store);
	GList *_rows = rows;
	while(_rows)
	{
		gtk_tree_model_get_iter(store, &iter, _rows->data);
		if(xa_archive_store_get_show_icons(XA_ARCHIVE_STORE(store)))
			gtk_tree_model_get_value(store, &iter, 1, value);
		else
			gtk_tree_model_get_value(store, &iter, 0, value);

		filenames = g_slist_prepend(filenames, g_strconcat(pwd, g_value_get_string(value), NULL));

		g_value_unset(value);
		_rows = _rows->next;
	}
	g_list_free(rows);
	g_free(pwd);
	
	return filenames;
}


void
xa_notebook_page_get_archive(XANotebook *notebook, LXAArchive **lp_archive, LXAArchiveSupport **lp_support, gint n)
{
	GtkWidget *scrolledwindow = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
	GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
	GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
		
	if(lp_archive)
		(*lp_archive) = xa_archive_store_get_archive(XA_ARCHIVE_STORE(store));
	if(lp_support)
		(*lp_support) = xa_archive_store_get_support(XA_ARCHIVE_STORE(store));
}
