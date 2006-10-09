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
#ifndef __XARCHIVER_ARCHIVE_TREE_STORE_H__
#define __XARCHIVER_ARCHIVE_TREE_STORE_H__

#define XA_TYPE_ARCHIVE_TREE_STORE xa_archive_tree_store_get_type()

#define XA_ARCHIVE_TREE_STORE(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			XA_TYPE_ARCHIVE_TREE_STORE,      \
			XAArchiveTreeStore))

#define XA_IS_ARCHIVE_TREE_STORE(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			XA_TYPE_ARCHIVE_TREE_STORE))

#define XA_ARCHIVE_TREE_STORE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			XA_TYPE_ARCHIVE_TREE_STORE,      \
			XAArchiveTreeStoreClass))

#define XA_IS_ARCHIVE_TREE_STORE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			XA_TYPE_ARCHIVE_TREE_STORE))

typedef struct _XAArchiveTreeStore XAArchiveTreeStore;

struct _XAArchiveTreeStore
{
	GObject parent;
	gint stamp;
	LXAArchive *archive;
	gint sort_column;
	GtkSortType sort_order;
	LXAEntry **sort_list;
	GtkIconTheme *icon_theme;
	struct {
		gboolean _show_icons;
		gboolean _show_only_filename;
		gboolean _show_only_dir;
	} props;
};

typedef struct _XAArchiveTreeStoreClass XAArchiveTreeStoreClass;

struct _XAArchiveTreeStoreClass
{
	GObjectClass parent_class;
};


GType
xa_archive_tree_store_get_type();
GtkTreeModel *
xa_archive_tree_store_new(LXAArchive *archive, gboolean show_icons, gboolean show_only_names, gboolean show_only_dir, GtkIconTheme *icon_theme);
void
xa_archive_tree_store_set_contents(XAArchiveTreeStore *archive_store, LXAArchive *archive);
void
xa_archive_tree_store_connect_treeview(XAArchiveTreeStore *store, GtkTreeView *treeview);
void
xa_archive_tree_store_go_up(XAArchiveTreeStore *store);
gchar *
xa_archive_tree_store_get_pwd(XAArchiveTreeStore *store);
GSList *
xa_archive_tree_store_get_pwd_list(XAArchiveTreeStore *store);
gchar *
xa_archive_tree_store_get_basename(XAArchiveTreeStore *store);
gint
xa_archive_tree_store_set_pwd(XAArchiveTreeStore *store, gchar *path);

#endif /* __XARCHIVER_ARCHIVE_TREE_STORE_H__ */
