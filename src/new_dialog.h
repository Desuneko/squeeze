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

#ifndef __XARCHIVER_NEW_ARCHIVE_DIALOG_H__
#define __XARCHIVER_NEW_ARCHIVE_DIALOG_H__
G_BEGIN_DECLS

#define XA_NEW_ARCHIVE_DIALOG(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			xa_new_archive_dialog_get_type(),      \
			XANewArchiveDialog))

#define XA_IS_NEW_ARCHIVE_DIALOG(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			xa_new_archive_dialog_get_type()))

#define XA_NEW_ARCHIVE_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			xa_new_archive_dialog_get_type(),      \
			XANewArchiveDialogClass))

#define XA_IS_NEW_ARCHIVE_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			xa_new_archive_dialog_get_type()))

typedef struct _XANewArchiveDialog XANewArchiveDialog;

struct _XANewArchiveDialog
{
	GtkFileChooserDialog parent;
};

typedef struct _XANewArchiveDialogClass XANewArchiveDialogClass;

struct _XANewArchiveDialogClass
{
	GtkFileChooserDialogClass parent;
};

GtkWidget *xa_new_archive_dialog_new();

G_END_DECLS
#endif /* __XARCHIVER_NEW_ARCHIVE_DIALOG_H__ */
