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
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <libxfce4util/libxfce4util.h>

#include <libsqueeze/libsqueeze.h>
#include "settings.h"
#include "archive_store.h"
#include "navigation_bar.h"
#include "tool_bar.h"
#include "path_bar.h"
#include "notebook.h"
#include "throbber.h"

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
cb_notebook_archive_refreshed(LSQArchive *archive, GtkTreeView *tree_view);
static void
cb_notebook_archive_state_changed(LSQArchive *archive, SQNotebook *notebook);
static void
cb_notebook_tab_archive_state_changed(LSQArchive *archive, GtkContainer *widget);
static void
cb_notebook_file_activated(SQArchiveStore *, LSQArchiveIter *, SQNotebook *);

static void
cb_sq_notebook_page_switched(SQNotebook *notebook, GtkNotebookPage *, guint page_nr, gpointer data);
static void
cb_sq_notebook_page_removed(SQNotebook *notebook, gpointer data);

static void
cb_sq_notebook_notify_proxy(GObject *obj, GParamSpec *pspec, gpointer user_data);

enum {
	SQ_NOTEBOOK_MULTI_TAB = 1,
	SQ_NOTEBOOK_STORE_SHOW_FULL_PATH,
	SQ_NOTEBOOK_STORE_SHOW_ICONS,
	SQ_NOTEBOOK_STORE_SORT_FOLDERS_FIRST,
	SQ_NOTEBOOK_STORE_SORT_CASE_SENSITIVE,
	SQ_NOTEBOOK_TREE_RULES_HINT
};

enum
{
	SQ_NOTEBOOK_SIGNAL_ARCHIVE_REMOVED = 0,
	SQ_NOTEBOOK_SIGNAL_PAGE_UP,
	SQ_NOTEBOOK_SIGNAL_PAGE_DOWN,
	SQ_NOTEBOOK_SIGNAL_FILE_ACTIVATED,
	SQ_NOTEBOOK_SIGNAL_STATE_CHANGED, /* is emitted when the state of the active archive changed */
	SQ_NOTEBOOK_SIGNAL_COUNT
};

static gint sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_COUNT];

static GObjectClass *parent_class;

GType
sq_notebook_get_type (void)
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
	object_class->dispose	  = sq_notebook_dispose;

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
			G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);

	sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_STATE_CHANGED] = g_signal_new("archive-state-changed",
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

	pspec = g_param_spec_boolean("show-full-path",
		_("Show full path"),
		_("Show the full path strings for each entry"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_NOTEBOOK_STORE_SHOW_FULL_PATH, pspec);

	pspec = g_param_spec_boolean("show-icons",
		_("Show mime icons"),
		_("Show the mime type icons for each entry"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_NOTEBOOK_STORE_SHOW_ICONS, pspec);

	pspec = g_param_spec_boolean("sort_folders_first",
		_("Sort folders before files"),
		_("The folders will be put at the top of the list"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_NOTEBOOK_STORE_SORT_FOLDERS_FIRST, pspec);

	pspec = g_param_spec_boolean("sort_case_sensitive",
		_("Sort text case sensitive"),
		_("Sort text case sensitive"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, SQ_NOTEBOOK_STORE_SORT_CASE_SENSITIVE, pspec);

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

	notebook->settings = sq_settings_new();

	notebook->current_page_fix = 0;
	notebook->props._rules_hint = sq_settings_read_bool_entry(notebook->settings, "RulesHint", FALSE);
	notebook->props._show_full_path = sq_settings_read_bool_entry(notebook->settings, "ShowFullPath", FALSE);
	notebook->props._show_icons = sq_settings_read_bool_entry(notebook->settings, "ShowIcons", TRUE);
	notebook->props._sort_folders_first = sq_settings_read_bool_entry(notebook->settings, "SortFoldersFirst", TRUE);
	notebook->props._sort_case_sensitive = sq_settings_read_bool_entry(notebook->settings, "SortCaseSensitive", TRUE);
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
	SQNotebook *sq_notebook = SQ_NOTEBOOK(object);
	gint n;
	gint i = 0;

	if(sq_notebook->settings)
	{
		sq_settings_write_bool_entry(sq_notebook->settings, "RulesHint", sq_notebook->props._rules_hint);
		sq_settings_write_bool_entry(sq_notebook->settings, "ShowFullPath", sq_notebook->props._show_full_path);
		sq_settings_write_bool_entry(sq_notebook->settings, "ShowIcons", sq_notebook->props._show_icons);
		sq_settings_write_bool_entry(sq_notebook->settings, "SortFoldersFirst", sq_notebook->props._sort_folders_first);
		sq_settings_write_bool_entry(sq_notebook->settings, "SortCaseSensitive", sq_notebook->props._sort_case_sensitive);

		sq_settings_save(sq_notebook->settings);

		g_object_unref(G_OBJECT(sq_notebook->settings));
		sq_notebook->settings = NULL;
	}

	n = gtk_notebook_get_n_pages(notebook);
	for(i = n-1; i >= 0; --i)
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
		gtk_notebook_remove_page(notebook, i);
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
		case SQ_NOTEBOOK_STORE_SHOW_FULL_PATH:
		{
			SQArchiveStore *store = sq_notebook_get_store(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(store)
				sq_archive_store_set_show_full_path(store, g_value_get_boolean(value));
			SQ_NOTEBOOK(object)->props._show_full_path = g_value_get_boolean(value);
			break;
		}
		case SQ_NOTEBOOK_STORE_SHOW_ICONS:
		{
			SQArchiveStore *store = sq_notebook_get_store(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(store)
				sq_archive_store_set_show_icons(store, g_value_get_boolean(value));
			SQ_NOTEBOOK(object)->props._show_icons = g_value_get_boolean(value);
			break;
		}
		case SQ_NOTEBOOK_STORE_SORT_FOLDERS_FIRST:
		{
			SQArchiveStore *store = sq_notebook_get_store(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(store)
				sq_archive_store_set_sort_folders_first(store, g_value_get_boolean(value));
			SQ_NOTEBOOK(object)->props._sort_folders_first = g_value_get_boolean(value);
			break;
		}
		case SQ_NOTEBOOK_STORE_SORT_CASE_SENSITIVE:
		{
			SQArchiveStore *store = sq_notebook_get_store(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(store)
				sq_archive_store_set_sort_case_sensitive(store, g_value_get_boolean(value));
			SQ_NOTEBOOK(object)->props._sort_case_sensitive = g_value_get_boolean(value);
			break;
		}
		case SQ_NOTEBOOK_TREE_RULES_HINT:
		{
			GtkTreeView *tree = sq_notebook_get_tree_view(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(tree)
				gtk_tree_view_set_rules_hint(tree, g_value_get_boolean(value));
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
		case SQ_NOTEBOOK_STORE_SHOW_FULL_PATH:
		{
			SQArchiveStore *store = sq_notebook_get_store(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(store)
				g_value_set_boolean(value, sq_archive_store_get_show_full_path(store));
			else
				g_value_set_boolean(value, SQ_NOTEBOOK(object)->props._show_full_path);
			break;
		}
		case SQ_NOTEBOOK_STORE_SHOW_ICONS:
		{
			SQArchiveStore *store = sq_notebook_get_store(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(store)
				g_value_set_boolean(value, sq_archive_store_get_show_icons(store));
			else
				g_value_set_boolean(value, SQ_NOTEBOOK(object)->props._show_icons);
			break;
		}
		case SQ_NOTEBOOK_STORE_SORT_FOLDERS_FIRST:
		{
			SQArchiveStore *store = sq_notebook_get_store(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(store)
				g_value_set_boolean(value, sq_archive_store_get_sort_folders_first(store));
			else
				g_value_set_boolean(value, SQ_NOTEBOOK(object)->props._sort_folders_first);
			break;
		}
		case SQ_NOTEBOOK_STORE_SORT_CASE_SENSITIVE:
		{
			SQArchiveStore *store = sq_notebook_get_store(SQ_NOTEBOOK(object), SQ_NOTEBOOK(object)->current_page_fix);
			if(store)
				g_value_set_boolean(value, sq_archive_store_get_sort_case_sensitive(store));
			else
				g_value_set_boolean(value, SQ_NOTEBOOK(object)->props._sort_case_sensitive);
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
	GtkWidget *treeview;
	GtkTreeModel *archive_store;
	if(!scrolledwindow)
		return NULL;
	treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
	archive_store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
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
	GtkWidget *treeview;
	if(!scrolledwindow)
		return NULL;
	treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
	return GTK_TREE_VIEW(treeview);
}

void
sq_notebook_set_navigation_bar(SQNotebook *notebook, SQNavigationBar *bar)
{
	if(notebook->navigation_bar)
	{
		sq_navigation_bar_set_store(notebook->navigation_bar, NULL);
		gtk_widget_unref(GTK_WIDGET(notebook->navigation_bar));
	}

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
	
	if(bar)
		gtk_widget_ref(GTK_WIDGET(bar));

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
sq_notebook_add_archive(SQNotebook *notebook, LSQArchive *archive, gboolean new_archive)
{
	GtkWidget *lbl_hbox = gtk_hbox_new(FALSE, 0);
    gchar *filename = lsq_archive_get_filename(archive);
    gchar *filepath = lsq_archive_get_path(archive);
	GtkWidget *label = gtk_label_new(filename);
	GtkWidget *archive_image = gtk_image_new_from_icon_name("unknown", GTK_ICON_SIZE_MENU);
	GtkWidget *throbber = sq_throbber_new();
	GtkWidget *alignment = gtk_alignment_new(0.5,0.5,1,1);
	GtkWidget *close_button;
	GtkWidget *close_image;
	GtkWidget *scroll_window;
	GtkWidget *tree_view;
	GtkTreeSelection *selection;
	GtkTreeModel *tree_model;
	gint page_nr;
	
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 4, 0, 0, 0);

	gtk_container_add(GTK_CONTAINER(alignment), throbber);

	/*thunar_vfs_mime_info_lookup_icon_name(lsq_archive_get_mimetype(archive), notebook->icon_theme), GTK_ICON_SIZE_MENU);*/
	close_button = gtk_button_new();
	close_image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	scroll_window = gtk_scrolled_window_new(NULL, NULL);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_button_set_image(GTK_BUTTON(close_button), close_image);
	gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);

	gtk_widget_set_size_request(lbl_hbox, -1, 22);

	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
	gtk_label_set_max_width_chars(GTK_LABEL(label), 20);
    
	gtk_tooltips_set_tip(notebook->tool_tips, label, filepath, NULL);

	tree_view = gtk_tree_view_new();
	g_signal_connect(G_OBJECT(tree_view), "notify", G_CALLBACK(cb_sq_notebook_notify_proxy), notebook);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree_view), TRUE);

	selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW (tree_view) );
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	tree_model = sq_archive_store_new(archive, notebook->props._show_icons, notebook->props._up_dir, notebook->icon_theme);
	sq_archive_store_set_show_full_path(SQ_ARCHIVE_STORE(tree_model), notebook->props._show_full_path);
	sq_archive_store_set_sort_folders_first(SQ_ARCHIVE_STORE(tree_model), notebook->props._sort_folders_first);
	sq_archive_store_set_sort_case_sensitive(SQ_ARCHIVE_STORE(tree_model), notebook->props._sort_case_sensitive);
	g_signal_connect(G_OBJECT(tree_model), "notify", G_CALLBACK(cb_sq_notebook_notify_proxy), notebook);

	gtk_box_pack_start(GTK_BOX(lbl_hbox), archive_image, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(lbl_hbox), alignment, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(lbl_hbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(lbl_hbox), close_button, FALSE, FALSE, 0);
	gtk_widget_show_all(lbl_hbox);
	gtk_widget_show_all(tree_view);
	gtk_widget_show(scroll_window);

	g_signal_connect(G_OBJECT(archive), "refreshed", G_CALLBACK(cb_notebook_archive_refreshed), tree_view);
	g_signal_connect(G_OBJECT(archive), "state-changed", G_CALLBACK(cb_notebook_archive_state_changed), notebook);
	g_signal_connect(G_OBJECT(archive), "state-changed", G_CALLBACK(cb_notebook_tab_archive_state_changed), lbl_hbox);

	g_signal_connect(G_OBJECT(close_button), "clicked", G_CALLBACK(cb_notebook_close_archive), scroll_window);
	g_signal_connect(G_OBJECT(tree_model), "file-activated", G_CALLBACK(cb_notebook_file_activated), notebook);


	sq_archive_store_connect_treeview(SQ_ARCHIVE_STORE(tree_model), GTK_TREE_VIEW(tree_view));

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), tree_model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree_view), notebook->props._rules_hint);

  //FIXME: for now it's here, should it be?
	sq_notebook_treeview_reset_columns(archive, GTK_TREE_VIEW(tree_view));

	if(gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), TRUE);
	else
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);

	gtk_container_add(GTK_CONTAINER(scroll_window), tree_view);
	
	page_nr = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll_window, lbl_hbox);
	if(page_nr >= 0)
	{
		gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_nr);
		gtk_widget_grab_focus(tree_view);
	}

	if(new_archive == FALSE)
	{
		lsq_archive_operate(archive, LSQ_COMMAND_TYPE_REFRESH, NULL, NULL);
	}
    g_free(filename);
    g_free(filepath);

}

static void
cb_notebook_close_archive(GtkButton *button, GtkWidget *child)
{
	GtkNotebook *notebook = GTK_NOTEBOOK(gtk_widget_get_parent(child));
	gint n;

	GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(child));
	GtkTreeModel *archive_store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

	LSQArchive *archive = sq_archive_store_get_archive(SQ_ARCHIVE_STORE(archive_store));

	if(archive)
	{
		g_signal_handlers_disconnect_by_func(archive, cb_notebook_archive_refreshed, treeview);
	}
	if(SQ_NOTEBOOK(notebook)->navigation_bar)
		sq_navigation_bar_set_store(((SQNotebook *)notebook)->navigation_bar, NULL);
	g_object_unref(archive_store);

	lsq_close_archive(archive);

	n = gtk_notebook_page_num(notebook, child);
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
	/* 
	 * Some archives are done refreshing so fast there is not even an
	 * archive store to accomodate them.
	 */
	g_object_ref(archive_store);
	gtk_tree_view_set_model(treeview, NULL);
	sq_archive_store_set_archive(SQ_ARCHIVE_STORE(archive_store), archive);
	gtk_tree_view_set_model(treeview, archive_store);
	g_object_unref(archive_store);
	sq_notebook_treeview_reset_columns(archive, treeview);
}

static void
cb_notebook_archive_state_changed(LSQArchive *archive, SQNotebook *notebook)
{
	if(sq_notebook_is_active_archive(notebook, archive))
		g_signal_emit(G_OBJECT(notebook), sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_STATE_CHANGED], 0, archive, NULL);
}

static void
cb_notebook_tab_archive_state_changed(LSQArchive *archive, GtkContainer *widget)
{
	GList *children = gtk_container_get_children(widget);
	switch(lsq_archive_get_state(archive))
	{
        case LSQ_ARCHIVE_STATE_IDLE:
            gtk_widget_show(GTK_WIDGET(children->data));
            sq_throbber_set_animated(SQ_THROBBER(gtk_bin_get_child(GTK_BIN(children->next->data))), FALSE);
            gtk_widget_hide(GTK_WIDGET(children->next->data));
            break;
        default:
            gtk_widget_hide(GTK_WIDGET(children->data));
            sq_throbber_set_animated(SQ_THROBBER(gtk_bin_get_child(GTK_BIN(children->next->data))), TRUE);
            gtk_widget_show(GTK_WIDGET(children->next->data));
            break;
	}

	g_list_free(children);
}

static void
sq_notebook_treeview_reset_columns(LSQArchive *archive, GtkTreeView *treeview)
{
	GtkCellRenderer *renderer = NULL;
	GtkTreeViewColumn *column = NULL;
	guint x = 0;

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
	gtk_tree_view_column_set_attributes(column, renderer, "gicon", SQ_ARCHIVE_STORE_EXTRA_PROP_ICON, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", LSQ_ARCHIVE_PROP_FILENAME + SQ_ARCHIVE_STORE_EXTRA_PROP_COUNT, NULL);

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_sort_column_id(column, LSQ_ARCHIVE_PROP_FILENAME + SQ_ARCHIVE_STORE_EXTRA_PROP_COUNT);
	gtk_tree_view_column_set_title(column, lsq_archive_get_entry_property_name(archive, LSQ_ARCHIVE_PROP_FILENAME));
	gtk_tree_view_append_column(treeview, column);

	if(!show_only_filenames)
	{
		for(x = LSQ_ARCHIVE_PROP_USER; x < lsq_archive_n_entry_properties(archive); ++x)
		{
			switch(lsq_archive_get_entry_property_type(archive, x))
			{
                                default:
                                        if ( LSQ_TYPE_DATETIME != lsq_archive_get_entry_property_type(archive, x) )
                                        {
#ifdef DEBUG
                                            g_debug("Should not be reached");
#endif
                                            continue;
                                        }
        case(G_TYPE_CHAR):
				case(G_TYPE_DOUBLE):
				case(G_TYPE_FLOAT):
				case(G_TYPE_INT):
				case(G_TYPE_INT64):
				case(G_TYPE_LONG):
				case(G_TYPE_STRING):
				case(G_TYPE_UINT):
				case(G_TYPE_UINT64):
				case(G_TYPE_ULONG):
					renderer = gtk_cell_renderer_text_new();
					column = gtk_tree_view_column_new_with_attributes(lsq_archive_get_entry_property_name(archive, x), renderer, "text", x+SQ_ARCHIVE_STORE_EXTRA_PROP_COUNT, NULL);
					break;
			}
			gtk_tree_view_column_set_resizable(column, TRUE);
			gtk_tree_view_column_set_sort_column_id(column, x+SQ_ARCHIVE_STORE_EXTRA_PROP_COUNT);
			gtk_tree_view_append_column(treeview, column);
		}
	}
	gtk_tree_view_set_search_column(treeview, LSQ_ARCHIVE_PROP_FILENAME + SQ_ARCHIVE_STORE_EXTRA_PROP_COUNT);
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
cb_notebook_file_activated(SQArchiveStore *store, LSQArchiveIter *iter, SQNotebook *notebook)
{
	g_signal_emit(G_OBJECT(notebook), sq_notebook_signals[SQ_NOTEBOOK_SIGNAL_FILE_ACTIVATED], 0, iter, NULL);
}

gboolean
sq_notebook_is_active_archive(SQNotebook *notebook, LSQArchive *archive)
{
	SQArchiveStore *archive_store = sq_notebook_get_active_store(notebook);
	LSQArchive * lp_archive;
	if(!archive_store)
		return FALSE;
	lp_archive = sq_archive_store_get_archive(archive_store);
	if(lp_archive == archive)
		return TRUE;
	return FALSE;
}

void
sq_notebook_get_active_archive(SQNotebook *notebook, LSQArchive **lp_archive)
{
	gint n = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
		
	sq_notebook_page_get_archive(notebook, lp_archive, n);
}

GtkWidget *
sq_notebook_get_active_child(SQNotebook *notebook)
{
	gint n = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));

	return gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
}

void
sq_notebook_page_set_archive(SQNotebook *notebook, LSQArchive *archive, gint n)
{
	if(gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
	{
		GtkWidget *scrolledwindow = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n);
		GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
		GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

		sq_archive_store_set_archive(SQ_ARCHIVE_STORE(store), archive);

		g_signal_connect(G_OBJECT(archive), "refreshed", G_CALLBACK(cb_notebook_archive_refreshed), treeview);

		lsq_archive_operate(archive, LSQ_COMMAND_TYPE_REFRESH, NULL, NULL);


		gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), store);
	}
	else
		sq_notebook_add_archive(SQ_NOTEBOOK(notebook), archive, FALSE);
}

GSList *
sq_notebook_get_selected_items(SQNotebook *notebook)
{
	GtkWidget *scrolledwindow = sq_notebook_get_active_child(notebook);
	GtkTreeIter iter;
	GSList *filenames = NULL;

	GtkWidget *treeview = gtk_bin_get_child(GTK_BIN(scrolledwindow));
	GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &store);
	GList *_rows = rows;
	while(_rows)
	{
		LSQArchiveIter *entry;
		gtk_tree_model_get_iter(store, &iter, _rows->data);
		entry = sq_archive_store_get_archive_iter(SQ_ARCHIVE_STORE(store), &iter);

		lsq_archive_iter_ref(entry);
		filenames = g_slist_prepend(filenames, entry);

		_rows = _rows->next;
	}
	g_list_free(rows);
	
	return filenames;
}
		

void
sq_notebook_page_get_archive(SQNotebook *notebook, LSQArchive **lp_archive, gint n)
{
	SQArchiveStore *store = sq_notebook_get_store(notebook, n);
		
	if(lp_archive)
		(*lp_archive) = sq_archive_store_get_archive(store);
}

static void
cb_sq_notebook_notify_proxy(GObject *obj, GParamSpec *pspec, gpointer user_data)
{
	if(strcmp(g_param_spec_get_name(pspec), "show-icons") == 0)
	{
		GtkTreeView *treeview;
		SQArchiveStore *store;

		g_object_notify(user_data, g_param_spec_get_name(pspec));

		treeview = sq_notebook_get_active_tree_view(SQ_NOTEBOOK(user_data));
		store = SQ_ARCHIVE_STORE(gtk_tree_view_get_model(treeview));

		sq_notebook_treeview_reset_columns(sq_archive_store_get_archive(store), treeview);
	}
	if(strcmp(g_param_spec_get_name(pspec), "show-full-path") == 0 || strcmp(g_param_spec_get_name(pspec), "sort-folders-first") == 0 || strcmp(g_param_spec_get_name(pspec), "sort-case-sensitive") == 0)
	{
		g_object_notify(user_data, g_param_spec_get_name(pspec));
	}
	if(strcmp(g_param_spec_get_name(pspec), "rules-hint") == 0)
	{
		g_object_notify(user_data, g_param_spec_get_name(pspec));
	}
}

