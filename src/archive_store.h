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
	guint n_columns;
	GType *column_types;
	LXAArchive *archive;
	GSList *current_entry;
	LXAEntry up_entry;
	struct {
		gboolean _show_icons;
	} props;
};

typedef struct _XAArchiveStoreClass XAArchiveStoreClass;

struct _XAArchiveStoreClass
{
	GObjectClass parent_class;
};


GType
xa_archive_store_get_type();
GtkTreeModel *
xa_archive_store_new(LXAArchive *archive, gboolean show_icons);
void
xa_archive_store_set_contents(XAArchiveStore *archive_store, LXAArchive *archive);

#endif /* __XARCHIVER_ARCHIVE_STORE_H__ */
