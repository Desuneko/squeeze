/*
 *  Copyright (c) 2006 Stephan Arts <stephan@xfce.org>
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
#include <thunar-vfs/thunar-vfs.h>
#include <libsqueeze/libsqueeze.h>
#include "archive_store.h"
#include "navigation_bar.h"
#include "tool_bar.h"
#include "path_bar.h"
#include "notebook.h"

static void
sq_notebook_class_init(SQNotebookClass *archive_class);

static void
sq_notebook_init(SQNotebook *archive);
static void
sq_notebook_dispose(GObject *object);

static void
sq_notebook_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
sq_notebook_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static void
sq_notebook_treeview_reset_columns(LSQArchive *archive, GtkTreeView *treeview);

static SQArchiveStore *
sq_notebook_get_store(SQNotebook *notebook, gint page_nr);
static GtkTreeView *
sq_notebook_get_tree_view(SQNotebook *notebook, gint page_nr);

static void
cb_notebook_close_archive(GtkButton *button, GtkWidget *child);

static void
cb_notebook_archive_status_changed(LSQArchive *archive, SQNotebook *notebook);
static void
cb_notebook_archive_refreshed(LSQArchive *archive, GtkTreeView *tree_view);
static void
cb_notebook_file_activated(SQArchiveStore *, gchar *, SQNotebook *);

static void
cb_sq_notebook_page_switched(SQNotebook *notebook, GtkNotebookPage *, guint page_nr, gpointer data);
static void
cb_sq_notebook_page_removed(SQNotebook *notebook, gpointer data);

static void
cb_sq_notebook_notify_proxy(GObject *obj, GParamSpec *pspec, gpointer user_data);

enum {
	SQ_NOTEBOOK_MULTI_TAB = 1,
	SQ_NOTEBOOK_STORE_SHOW_ICONS,
	SQ_NOTEBOOK_TREE_RULES_HINT
};

enum
{
	SQ_NOTEBOOK_SIGNAL_ARCHIVE_REMOVED = 0,
	SQ_NOTEBOOK_SIGNAL_PAGE_UP,
	SQ_NOTEBOOK_SIGNAL_PAGE_DOWN,
	SQ_NOTEBOOK_SIGNAL_FILE_ACTIVATED,
	SQ_NOTEBOOK_SIGNAL_ACTIVE_ARCHIVE_STATUS_CHANGED,
	SQ_NOTEBOOK_SIGNAL_COUNT
};

static gint sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_COUNT];

static GObjectClass *parent_class;

GType
sq_notebook_get_type ()
{
	static GType sq_notebook_type = 0;

 	if (!sq_notebook_type)
	{
 		static const GTypeInfo sq_notebook_info = 
		{
			sizeof (SQNotebookClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) sq_notebook_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (SQNotebook),
			0,
			(GInstanceInitFunc) sq_notebook_init,
			NULL
		};

		sq_notebook_type = g_type_register_static (GTK_TYPE_NOTEBOOK, "SQNotebook", &sq_notebook_info, 0);
	}
	return sq_notebook_type;
}

static void
sq_notebook_class_init(SQNotebookClass *notebook_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (notebook_class);
	GParamSpec *pspec = NULL;

	parent_class = gtk_type_class (GTK_TYPE_NOTEBOOK);

	object_class->set_property = sq_notebook_set_property;
	object_class->get_property = sq_notebook_get_property;
	object_class->dispose      = sq_notebook_dispose;

	sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_ARCHIVE_REMOVED] = g_signal_new("archive-removed",
			G_TYPE_FROM_CLASS(notebook_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
			NULL);

	sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_PAGE_UP] = g_signal_new("page-up",
			G_TYPE_FROM_CLASS(notebook_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0,
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0, NULL);

	sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_PAGE_DOWN] = g_signal_new("page-down",
			G_TYPE_FROM_CLASS(notebook_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0,
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0, NULL);

	sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_FILE_ACTIVATED] = g_signal_new("file-activated",
			G_TYPE_FROM_CLASS(notebook_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0,
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1, G_TYPE_STRING, NULL);

	sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_ACTIVE_ARCHIVE_STATUS_CHANGED] = g_signal_new("active-archive-status-changed",
			G_TYPE_FROM_CLASS(notebook_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0,
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1, G_TYPE_OBJECT, NULL);

	pspec = g_param_spec_boolean("multi_tab",
		"",
		"",
		TRUE,
		G_PARAM_READWRITE); g_object_class_install_property(object_class, SQ_NOTEBOOK_MULTI_TAB, pspec); 

	pspec = g_param_spec_boolean("show-icons",
		_("Show mime icons"),
		_("Show the mime type icons for each entry"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_NOTEBOOK_STORE_SHOW_ICONS, pspec);
/*
	pspec = g_param_spec_boolean("sort_folders_first",
		_("Sort folders before files"),
		_("The folders will be put at the top of the list"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_ARCHIVE_STORE_SORT_FOLDERS_FIRST, pspec);

	pspec = g_param_spec_boolean("sort_case_sensitive",
		_("Sort text case sensitive"),
		_("Sort text case sensitive"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_ARCHIVE_STORE_SORT_CASE_SENSITIVE, pspec);
*/
	pspec = g_param_spec_boolean("rules-hint",
		_("Rules hint"),
		_("Make the row background colors alternate"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_NOTEBOOK_TREE_RULES_HINT, pspec);
}

static void
sq_notebook_init(SQNotebook *notebook)
{
	g_signal_connect(G_OBJECT(notebook), "switch-page", G_CALLBACK(cb_sq_notebook_page_switched), NULL);
	g_signal_connect(G_OBJECT(notebook), "archive-removed", G_CALLBACK(cb_sq_notebook_page_removed), NULL);

	g_signal_connect(G_OBJECT(notebook), "page-up", G_CALLBACK(gtk_notebook_next_page),  NULL);
	g_signal_connect(G_OBJECT(notebook), "page-down", G_CALLBACK(gtk_notebook_prev_page), NULL);

	notebook->current_page_fix = 0;
	notebook->props._rules_hint = FALSE;
	notebook->props._show_icons = TRUE;
	notebook->multi_tab = TRUE;
	notebook->accel_group = NULL;

	notebook->tool_tips = gtk_tooltips_new();
	gtk_tooltips_enable(notebook->tool_tips);
	gtk_notebook_set_tab_border(GTK_NOTEBOOK(notebook), 0);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
}

static void
sq_notebook_dispose(GObject *object)
{
	/* TODO: unref archive_stores */
	GtkNotebook *notebook = GTK_NOTEBOOK(object);
	gint n = gtk_notebook_get_n_pages(notebook);
	gint i = 0;
	for(i = 0; i < n; i++)
	{
		GtkWidget *scrolledwindow = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i);
		GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
		GtkTreeModel *archive_store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
		
		LSQArchive *archive = sq_archive_store_get_archive(SQ_ARCHIVE_STORE(archive_store));

		if(archive)
			g_signal_handlers_disconnect_by_func(archive, cb_notebook_archive_refreshed, treeview);
		if(SQ_NOTEBOOK(notebook)->navigation_bar)
			sq_navigation_bar_set_store(((SQNotebook *)notebook)->navigation_bar, NULL);
		g_object_unref(archive_store);

		lsq_close_archive(archive);
	}
	parent_class->dispose(object);
}

GtkWidget *
sq_notebook_new(SQNavigationBar *bar, gboolean use_tabs, GtkAccelGroup *accel_group)
{
	SQNotebook *notebook;

	notebook = g_object_new(SQ_TYPE_NOTEBOOK, NULL);

	sq_notebook_set_navigation_bar(notebook, bar);

	notebook->multi_tab = use_tabs;
	notebook->accel_group = accel_group;

	gtk_widget_add_accelerator(GTK_WIDGET(notebook), "page-up", accel_group, 0xff55, GDK_CONTROL_MASK, 0);
	gtk_widget_add_accelerator(GTK_WIDGET(notebook), "page-down", accel_group, 0xff56, GDK_CONTROL_MASK, 0);

	return (GtkWidget *)notebook;
}

static void
sq_notebook_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case SQ_NOTEBOOK_MULTI_TAB:
			SQ_NOTEBOOK(object)->multi_tab = g_value_get_boolean(value);		
			break;
		case SQ_NOTEBOOK_STORE_SHOW_ICONS:
		{
			SQArchiveStore *store = sq_notebook_get_store(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(store)
				sq_archive_store_set_show_icons(store, g_value_get_boolean(value));
			else
				SQ_NOTEBOOK(object)->props._show_icons = g_value_get_boolean(value);
			break;
		}
		case SQ_NOTEBOOK_TREE_RULES_HINT:
		{
			GtkTreeView *tree = sq_notebook_get_tree_view(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(tree)
				gtk_tree_view_set_rules_hint(tree, g_value_get_boolean(value));
			else
				SQ_NOTEBOOK(object)->props._rules_hint = g_value_get_boolean(value);
			break;
		}
	}
}

static void
sq_notebook_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case SQ_NOTEBOOK_MULTI_TAB:
			g_value_set_boolean(value, SQ_NOTEBOOK(object)->multi_tab);
			break;
		case SQ_NOTEBOOK_STORE_SHOW_ICONS:
		{
			SQArchiveStore *store = sq_notebook_get_store(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(store)
				g_value_set_boolean(value, sq_archive_store_get_show_icons(store));
			else
				g_value_set_boolean(value, SQ_NOTEBOOK(object)->props._show_icons);
			break;
		}
		case SQ_NOTEBOOK_TREE_RULES_HINT:
		{
			GtkTreeView *tree = sq_notebook_get_tree_view(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(tree)
				g_value_set_boolean(value, gtk_tree_view_get_rules_hint(tree));
			else
				g_value_set_boolean(value, SQ_NOTEBOOK(object)->props._rules_hint);
			break;
		}
	}
}

gboolean
sq_notebook_get_multi_tab(SQNotebook *notebook)
{
	return notebook->multi_tab;
}

SQArchiveStore *
sq_notebook_get_active_store(SQNotebook *notebook)
{
	gint page_nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));

	return sq_notebook_get_store(notebook, page_nr);
}

static SQArchiveStore *
sq_notebook_get_store(SQNotebook *notebook, gint page_nr)
{
	GtkWidget *scrolledwindow = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page_nr);
	if(!scrolledwindow)
		return NULL;
	GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
	GtkTreeModel *archive_store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	return SQ_ARCHIVE_STORE(archive_store);
}

GtkTreeView *
sq_notebook_get_active_tree_view(SQNotebook *notebook)
{
	gint page_nr = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));

	return sq_notebook_get_tree_view(notebook, page_nr);
}

static GtkTreeView *
sq_notebook_get_tree_view(SQNotebook *notebook, gint page_nr)
{
	GtkWidget *scrolledwindow = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page_nr);
	if(!scrolledwindow)
		return NULL;
	GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
	return GTK_TREE_VIEW(treeview);
}

void
sq_notebook_set_navigation_bar(SQNotebook *notebook, SQNavigationBar *bar)
{
	if(notebook->navigation_bar)
		sq_navigation_bar_set_store(notebook->navigation_bar, NULL);

	notebook->props._up_dir = TRUE;

	if(bar)
	{
#ifdef ENABLE_TOOLBAR
		if(SQ_IS_TOOL_BAR(bar))
			notebook->props._up_dir = FALSE;
#endif
#ifdef ENABLE_PATHBAR
		if(SQ_IS_PATH_BAR(bar))
			notebook->props._up_dir = FALSE;
#endif
	}

	if(gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
	{
		SQArchiveStore *archive_store = sq_notebook_get_active_store(notebook);
		notebook->navigation_bar = bar;
		if(bar)
			sq_navigation_bar_set_store(notebook->navigation_bar, archive_store);
		if(archive_store)
			g_object_set(G_OBJECT(archive_store), "show_up_dir", notebook->props._up_dir, NULL);
	}
	else
		notebook->navigation_bar = bar;

}

void
sq_notebook_add_archive(SQNotebook *notebook, LSQArchive *archive, LSQArchiveSupport *support, gboolean new_archive)
{
	GtkWidget *lbl_hbox = gtk_hbox_new(FALSE, 0);
	GtkWidget *label = gtk_label_new(lsq_archive_get_filename(archive));
	GtkWidget *archive_image = gtk_image_new_from_icon_name(thunar_vfs_mime_info_lookup_icon_name(archive->mime_info, notebook->icon_theme), GTK_ICON_SIZE_MENU);
	GtkWidget *close_button = gtk_button_new();
	GtkWidget *close_image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	GtkWidget *scroll_window = gtk_scrolled_window_new(NULL, NULL);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_button_set_image(GTK_BUTTON(close_button), close_image);
	gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);

	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
	gtk_label_set_max_width_chars(GTK_LABEL(label), 20);
	gtk_tooltips_set_tip(notebook->tool_tips, label, lsq_archive_get_filename(archive), NULL);

	GtkWidget *tree_view = gtk_tree_view_new();
	g_signal_connect(G_OBJECT(tree_view), "notify", G_CALLBACK(cb_sq_notebook_notify_proxy), notebook);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree_view), TRUE);

	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (tree_view) );
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	GtkTreeModel *tree_model = sq_archive_store_new(archive, notebook->props._show_icons, notebook->props._up_dir, notebook->icon_theme);
	g_signal_connect(G_OBJECT(tree_model), "notify", G_CALLBACK(cb_sq_notebook_notify_proxy), notebook);

	gtk_box_pack_start(GTK_BOX(lbl_hbox), archive_image, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(lbl_hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(lbl_hbox), close_button, FALSE, FALSE, 0);
	gtk_widget_show_all(lbl_hbox);
	gtk_widget_show_all(tree_view);
	gtk_widget_show(scroll_window);

	g_signal_connect(G_OBJECT(archive), "lsq_status_changed", G_CALLBACK(cb_notebook_archive_status_changed), notebook);
	g_signal_connect(G_OBJECT(archive), "lsq_refreshed", G_CALLBACK(cb_notebook_archive_refreshed), tree_view);

	g_signal_connect(G_OBJECT(close_button), "clicked", G_CALLBACK(cb_notebook_close_archive), scroll_window);
	g_signal_connect(G_OBJECT(tree_model), "sq_file_activated", G_CALLBACK(cb_notebook_file_activated), notebook);


	sq_archive_store_set_support(SQ_ARCHIVE_STORE(tree_model), support);
	if(new_archive == FALSE)
		lsq_archive_support_refresh(support, archive);

	sq_archive_store_connect_treeview(SQ_ARCHIVE_STORE(tree_model), GTK_TREE_VIEW(tree_view));
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), tree_model);
	if(gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), TRUE);
	else
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);

	gtk_container_add(GTK_CONTAINER(scroll_window), tree_view);
	
	gint page_nr = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll_window, lbl_hbox);
	if(page_nr >= 0)
	{
		gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_nr);
		gtk_widget_grab_focus(tree_view);
	}
}

void
cb_notebook_archive_status_changed(LSQArchive *archive, SQNotebook *notebook)
{
	GtkWidget *dialog = NULL;

#ifdef DEBUG
	g_debug("NOTEBOOK: Archive status changed");
#endif /* DEBUG */
	if(lsq_archive_get_status(archive) == LSQ_ARCHIVESTATUS_ERROR)
	{
		switch(lsq_archive_get_old_status(archive))
		{
			case LSQ_ARCHIVESTATUS_REFRESH:
				dialog = gtk_message_dialog_new(NULL, 
				                                0, 
																				GTK_MESSAGE_ERROR, 
																				GTK_BUTTONS_OK, 
																				_("Failed to open archive '%s'."), 
																				lsq_archive_get_filename(archive));
				break;
			case LSQ_ARCHIVESTATUS_EXTRACT:
				dialog = gtk_message_dialog_new(NULL, 
				                                0, 
																				GTK_MESSAGE_ERROR, 
																				GTK_BUTTONS_OK, 
																				_("Failed to extract contents of archive '%s'."), 
																				lsq_archive_get_filename(archive));
				break;
			default:
				break;
		}
		gtk_dialog_run((GtkDialog *)dialog);
		gtk_widget_destroy(dialog);
	}
	if(lsq_archive_get_status(archive) == LSQ_ARCHIVESTATUS_IDLE)
	{
		switch(lsq_archive_get_old_status(archive))
		{
			case LSQ_ARCHIVESTATUS_ADD:
				lsq_archive_support_refresh(archive->support, archive);
				break;
			default:break;
		}
	}
	if(sq_notebook_is_active_archive(notebook, archive))
		g_signal_emit(G_OBJECT(notebook), sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_ACTIVE_ARCHIVE_STATUS_CHANGED], 0, archive, NULL);
}

static void
cb_notebook_close_archive(GtkButton *button, GtkWidget *child)
{
	GtkNotebook *notebook = GTK_NOTEBOOK(gtk_widget_get_parent(child));

	GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(child));
	GtkTreeModel *archive_store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

	LSQArchive *archive = sq_archive_store_get_archive(SQ_ARCHIVE_STORE(archive_store));

	if(archive)
		g_signal_handlers_disconnect_by_func(archive, cb_notebook_archive_refreshed, treeview);
	if(SQ_NOTEBOOK(notebook)->navigation_bar)
		sq_navigation_bar_set_store(((SQNotebook *)notebook)->navigation_bar, NULL);
	g_object_unref(archive_store);

	lsq_close_archive(archive);

	gint n = gtk_notebook_page_num(notebook, child);
	gtk_notebook_remove_page(notebook, n);
	g_signal_emit(G_OBJECT(notebook), sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_ARCHIVE_REMOVED], 0, NULL);
}

void
sq_notebook_close_active_archive(SQNotebook *notebook)
{
	gint n = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
	GtkWidget *scrolledwindow = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
	GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
	GtkTreeModel *archive_store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

	LSQArchive *archive = sq_archive_store_get_archive(SQ_ARCHIVE_STORE(archive_store));

	if(archive)
		g_signal_handlers_disconnect_by_func(archive, cb_notebook_archive_refreshed, treeview);
	if(SQ_NOTEBOOK(notebook)->navigation_bar)
		sq_navigation_bar_set_store(notebook->navigation_bar, NULL);
	g_object_unref(archive_store);

	lsq_close_archive(archive);

	gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), n);
	g_signal_emit(G_OBJECT(notebook), sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_ARCHIVE_REMOVED], 0, NULL);
}

void
cb_notebook_archive_refreshed(LSQArchive *archive, GtkTreeView *treeview)
{
	GtkTreeModel *archive_store = gtk_tree_view_get_model(treeview);
	g_object_ref(archive_store);
	gtk_tree_view_set_model(treeview, NULL);
	sq_archive_store_set_archive(SQ_ARCHIVE_STORE(archive_store), archive);
	gtk_tree_view_set_model(treeview, archive_store);
	g_object_unref(archive_store);

	sq_notebook_treeview_reset_columns(archive, treeview);
}

static void
sq_notebook_treeview_reset_columns(LSQArchive *archive, GtkTreeView *treeview)
{
	GtkCellRenderer *renderer = NULL;
	GtkTreeViewColumn *column = NULL;
	gint x = 0;

	GList *columns = gtk_tree_view_get_columns(treeview);
	gboolean show_only_filenames = FALSE;

	while(columns)
	{
		gtk_tree_view_remove_column(treeview, columns->data);
		columns = columns->next;
	}
	g_list_free(columns);

	column = gtk_tree_view_column_new();

	renderer = gtk_cell_renderer_pixbuf_new();
	g_object_set(G_OBJECT(renderer), "stock-size", GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "icon-name", 0, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", LSQ_ARCHIVE_PROP_FILENAME + 1, NULL);

	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_sort_column_id(column, LSQ_ARCHIVE_PROP_FILENAME + 1);
	gtk_tree_view_column_set_title(column, lsq_archive_get_entry_property_name(archive, LSQ_ARCHIVE_PROP_FILENAME));
	gtk_tree_view_append_column(treeview, column);

	if(!show_only_filenames)
	{
		for(x = LSQ_ARCHIVE_PROP_USER; x < lsq_archive_n_property(archive); ++x)
		{
			switch(lsq_archive_get_entry_property_type(archive, x))
			{
				case(G_TYPE_STRING):
				case(G_TYPE_UINT64):
					renderer = gtk_cell_renderer_text_new();
					column = gtk_tree_view_column_new_with_attributes(lsq_archive_get_entry_property_name(archive, x), renderer, "text", x+1, NULL);
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
sq_notebook_set_icon_theme(SQNotebook *notebook, GtkIconTheme *icon_theme)
{
	notebook->icon_theme = icon_theme;
}

static void
cb_sq_notebook_page_switched(SQNotebook *notebook, GtkNotebookPage *page, guint page_nr, gpointer data)
{
	SQArchiveStore *archive_store = sq_notebook_get_store(notebook, page_nr);
	if(notebook->navigation_bar)
		sq_navigation_bar_set_store(notebook->navigation_bar, archive_store);
	if(archive_store)
	{
		g_object_set(G_OBJECT(archive_store), "show_up_dir", notebook->props._up_dir, NULL);
	}
	notebook->current_page_fix = page_nr;
	g_object_notify(G_OBJECT(notebook), "show-icons");
	g_object_notify(G_OBJECT(notebook), "rules-hint");
}

static void
cb_sq_notebook_page_removed(SQNotebook *notebook, gpointer data)
{
	if(!gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
	{
		if(notebook->navigation_bar)
			sq_navigation_bar_set_store(notebook->navigation_bar, NULL);
	}
	if(gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) > 1)
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), TRUE);
	else
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
}

static void
cb_notebook_file_activated(SQArchiveStore *store, gchar *filename, SQNotebook *notebook)
{
	gchar *pwd = sq_archive_store_get_pwd(store);
	gchar *path = g_strconcat(pwd, filename, NULL);
	g_signal_emit(G_OBJECT(notebook), sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_FILE_ACTIVATED], 0, path, NULL);
}

gboolean
sq_notebook_is_active_archive(SQNotebook *notebook, LSQArchive *archive)
{
	SQArchiveStore *archive_store = sq_notebook_get_active_store(notebook);
	if(!archive_store)
		return FALSE;
	LSQArchive * lp_archive = sq_archive_store_get_archive(archive_store);
	if(lp_archive == archive)
		return TRUE;
	return FALSE;
}

void
sq_notebook_get_active_archive(SQNotebook *notebook, LSQArchive **lp_archive, LSQArchiveSupport **lp_support)
{
	gint n = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
		
	sq_notebook_page_get_archive(notebook, lp_archive, lp_support, n);
}

GtkWidget *
sq_notebook_get_active_child(SQNotebook *notebook)
{
	gint n = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));

	return gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
}

void
sq_notebook_page_set_archive(SQNotebook *notebook, LSQArchive *archive, LSQArchiveSupport *support, gint n)
{
	if(gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
	{
		GtkWidget *scrolledwindow = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
		GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
		GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

		sq_archive_store_set_archive(SQ_ARCHIVE_STORE(store), archive);
		sq_archive_store_set_support(SQ_ARCHIVE_STORE(store), support);

		g_signal_connect(G_OBJECT(archive), "lsq_status_changed", G_CALLBACK(cb_notebook_archive_status_changed), notebook);
		g_signal_connect(G_OBJECT(archive), "lsq_refreshed", G_CALLBACK(cb_notebook_archive_refreshed), treeview);

		lsq_archive_support_refresh(support, archive);

		gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), store);
	}
	else
		sq_notebook_add_archive(SQ_NOTEBOOK(notebook), archive, support, FALSE);
}

GSList *
sq_notebook_get_selected_items(SQNotebook *notebook)
{
	GtkWidget *scrolledwindow = sq_notebook_get_active_child(notebook);
	GtkTreeIter iter;
	GValue value;
	memset(&value, 0, sizeof(GValue));
	GSList *filenames = NULL;

	GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
	GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	gchar *pwd = sq_archive_store_get_pwd(SQ_ARCHIVE_STORE(store));
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &store);
	GList *_rows = rows;
	while(_rows)
	{
		gtk_tree_model_get_iter(store, &iter, _rows->data);
		gtk_tree_model_get_value(store, &iter, 1, &value);

		filenames = g_slist_prepend(filenames, g_strconcat(pwd, g_value_get_string(&value), NULL));

		g_value_unset((GValue*)&value);
		_rows = _rows->next;
	}
	g_list_free(rows);
	g_free(pwd);
	
	return filenames;
}


void
sq_notebook_page_get_archive(SQNotebook *notebook, LSQArchive **lp_archive, LSQArchiveSupport **lp_support, gint n)
{
	SQArchiveStore *store = sq_notebook_get_store(notebook, n);
		
	if(lp_archive)
		(*lp_archive) = sq_archive_store_get_archive(store);
	if(lp_support)
		(*lp_support) = sq_archive_store_get_support(store);
}

static void
cb_sq_notebook_notify_proxy(GObject *obj, GParamSpec *pspec, gpointer user_data)
{
	if(strcmp(g_param_spec_get_name(pspec), "show-icons") == 0)
	{
		g_object_notify(user_data, g_param_spec_get_name(pspec));

		GtkTreeView *treeview = sq_notebook_get_active_tree_view(SQ_NOTEBOOK(user_data));
		SQArchiveStore *store = SQ_ARCHIVE_STORE(gtk_tree_view_get_model(treeview));

		sq_notebook_treeview_reset_columns(sq_archive_store_get_archive(store), treeview);
	}
	if(strcmp(g_param_spec_get_name(pspec), "show-icons") == 0)
	{
		g_object_notify(user_data, g_param_spec_get_name(pspec));

		GtkTreeView *treeview = sq_notebook_get_active_tree_view(SQ_NOTEBOOK(user_data));
		SQArchiveStore *store = SQ_ARCHIVE_STORE(gtk_tree_view_get_model(treeview));

		sq_notebook_treeview_reset_columns(sq_archive_store_get_archive(store), treeview);
	}
}

