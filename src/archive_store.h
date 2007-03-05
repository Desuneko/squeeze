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
#ifndef __SQRCHIVER_ARCHIVE_STORE_H__
#define __SQRCHIVER_ARCHIVE_STORE_H__

#define SQ_TYPE_ARCHIVE_STORE sq_archive_store_get_type()

#define SQ_ARCHIVE_STORE(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			SQ_TYPE_ARCHIVE_STORE,      \
			SQArchiveStore))

#define SQ_IS_ARCHIVE_STORE(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			SQ_TYPE_ARCHIVE_STORE))

#define SQ_ARCHIVE_STORE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			SQ_TYPE_ARCHIVE_STORE,      \
			SQArchiveStoreClass))

#define SQ_IS_ARCHIVE_STORE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			SQ_TYPE_ARCHIVE_STORE))

typedef struct _SQArchiveStore SQArchiveStore;

struct _SQArchiveStore
{
	GObject parent;
	gint stamp;
	LSQArchive *archive;
	LSQArchiveSupport *support;
	gint sort_column;
	GtkSortType sort_order;
	LSQArchiveIter **sort_list;
	guint list_size;
	GtkIconTheme *icon_theme;
	GtkTreeView *treeview;
	struct {
		guint _show_full_path :1;
		guint _show_icons : 1;
		guint _show_up_dir : 1;
		guint _sort_folders_first : 1;
		guint _sort_case_sensitive : 1;
	} props;
	struct {
		GList *history;
		GList *present;
		guint maxhistory;
		LSQArchiveIter *trailing;
	} navigation;
};

typedef struct _SQArchiveStoreClass SQArchiveStoreClass;

struct _SQArchiveStoreClass
{
	GObjectClass parent_class;
};

/* archive iter extra props columns */
enum {
	SQ_ARCHIVE_STORE_EXTRA_PROP_PATH = 0,
	SQ_ARCHIVE_STORE_EXTRA_PROP_ICON,
	SQ_ARCHIVE_STORE_EXTRA_PROP_COUNT
};

GType sq_archive_store_get_type();
GtkTreeModel * sq_archive_store_new(LSQArchive *archive, gboolean show_icons, gboolean show_up_dir, GtkIconTheme *icon_theme);
void sq_archive_store_connect_treeview(SQArchiveStore *store, GtkTreeView *treeview);
void sq_archive_store_connect_iconview(SQArchiveStore *store, GtkIconView *iconview);
void sq_archive_store_go_up(SQArchiveStore *store);
LSQArchiveIter * sq_archive_store_get_pwd(SQArchiveStore *store);
gboolean sq_archive_store_set_pwd(SQArchiveStore *store, LSQArchiveIter *path);
void sq_archive_store_set_icon_theme(SQArchiveStore *store, GtkIconTheme *icon_theme);

gboolean sq_archive_store_get_show_full_path(SQArchiveStore *);
gboolean sq_archive_store_get_show_icons(SQArchiveStore *store);
gboolean sq_archive_store_get_sort_case_sensitive(SQArchiveStore *store);
gboolean sq_archive_store_get_sort_folders_first(SQArchiveStore *store);

void sq_archive_store_set_show_full_path(SQArchiveStore *, gboolean);
void sq_archive_store_set_show_icons(SQArchiveStore *, gboolean);
void sq_archive_store_set_sort_case_sensitive(SQArchiveStore *, gboolean);
void sq_archive_store_set_sort_folders_first(SQArchiveStore *, gboolean);

LSQArchiveIter *sq_archive_store_get_archive_iter(SQArchiveStore *, GtkTreeIter *);

gboolean
sq_archive_store_has_history(SQArchiveStore *store);
gboolean
sq_archive_store_has_future(SQArchiveStore *store);
void
sq_archive_store_go_back(SQArchiveStore *store);
void
sq_archive_store_go_forward(SQArchiveStore *store);
LSQArchiveIter *
sq_archive_store_get_trailing(SQArchiveStore *store);

LSQArchive *
sq_archive_store_get_archive(SQArchiveStore *archive_store);
LSQArchiveSupport *
sq_archive_store_get_support(SQArchiveStore *archive_store);

void sq_archive_store_set_archive(SQArchiveStore *archive_store, LSQArchive *archive);
void sq_archive_store_set_support(SQArchiveStore *archive_store, LSQArchiveSupport *support);

#endif /* __SQRCHIVER_ARCHIVE_STORE_H__ */
