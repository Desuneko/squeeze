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
#ifndef __XARCHIVER_ARCHIVE_STORE_H__
#define __XARCHIVER_ARCHIVE_STORE_H__

#define XA_TYPE_ARCHIVE_STORE xa_archive_store_get_type()

#define XA_ARCHIVE_STORE(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			XA_TYPE_ARCHIVE_STORE,      \
			XAArchiveStore))

#define XA_IS_ARCHIVE_STORE(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			XA_TYPE_ARCHIVE_STORE))

#define XA_ARCHIVE_STORE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			XA_TYPE_ARCHIVE_STORE,      \
			XAArchiveStoreClass))

#define XA_IS_ARCHIVE_STORE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			XA_TYPE_ARCHIVE_STORE))

typedef struct _XAArchiveStore XAArchiveStore;

struct _XAArchiveStore
{
	GObject parent;
	gint stamp;
	LXAArchive *archive;
	LXAArchiveSupport *support;
	GSList *current_entry;
	gint sort_column;
	GtkSortType sort_order;
	LXAEntry **sort_list;
	GtkIconTheme *icon_theme;
	struct {
		guint _show_icons : 1;
		guint _show_up_dir : 1;
		guint _sort_folders_first : 1;
		guint _sort_case_sensitive : 1;
	} props;
	struct {
		GList *history;
		GList *pwd;
	} navigation;
};

typedef struct _XAArchiveStoreClass XAArchiveStoreClass;

struct _XAArchiveStoreClass
{
	GObjectClass parent_class;
};


GType xa_archive_store_get_type();
GtkTreeModel * xa_archive_store_new(LXAArchive *archive, gboolean show_icons, gboolean show_up_dir, GtkIconTheme *icon_theme);
void xa_archive_store_connect_treeview(XAArchiveStore *store, GtkTreeView *treeview);
void xa_archive_store_go_up(XAArchiveStore *store);
gchar * xa_archive_store_get_pwd(XAArchiveStore *store);
GSList * xa_archive_store_get_pwd_list(XAArchiveStore *store);
gchar * xa_archive_store_get_basename(XAArchiveStore *store);
gboolean xa_archive_store_set_pwd(XAArchiveStore *store, const gchar *path);
gboolean xa_archive_store_set_pwd_silent(XAArchiveStore *store, const gchar *path);
void xa_archive_store_set_icon_theme(XAArchiveStore *store, GtkIconTheme *icon_theme);

gboolean xa_archive_store_get_show_icons(XAArchiveStore *store);
gboolean xa_archive_store_get_sort_case_sensitive(XAArchiveStore *store);
gboolean xa_archive_store_get_sort_folders_first(XAArchiveStore *store);

void xa_archive_store_set_show_icons(XAArchiveStore *, gboolean);
void xa_archive_store_set_sort_case_sensitive(XAArchiveStore *, gboolean);
void xa_archive_store_set_sort_folders_first(XAArchiveStore *, gboolean);

gchar *
xa_archive_store_get_filename(XAArchiveStore *store, GtkTreeIter *iter);

void
xa_archive_store_set_history(XAArchiveStore *store, GList *history, GList *pwd);
void
xa_archive_store_get_history(XAArchiveStore *store, GList **history, GList **pwd);

LXAArchive *
xa_archive_store_get_archive(XAArchiveStore *archive_store);
LXAArchiveSupport *
xa_archive_store_get_support(XAArchiveStore *archive_store);

void xa_archive_store_set_archive(XAArchiveStore *archive_store, LXAArchive *archive);
void xa_archive_store_set_support(XAArchiveStore *archive_store, LXAArchiveSupport *support);

#endif /* __XARCHIVER_ARCHIVE_STORE_H__ */
