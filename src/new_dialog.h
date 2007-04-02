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

#ifndef __SQRCHIVER_NEW_ARCHIVE_DIALOG_H__
#define __SQRCHIVER_NEW_ARCHIVE_DIALOG_H__
G_BEGIN_DECLS

#define SQ_NEW_ARCHIVE_DIALOG(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			sq_new_archive_dialog_get_type(),      \
			SQNewArchiveDialog))

#define SQ_IS_NEW_ARCHIVE_DIALOG(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			sq_new_archive_dialog_get_type()))

#define SQ_NEW_ARCHIVE_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			sq_new_archive_dialog_get_type(),      \
			SQNewArchiveDialogClass))

#define SQ_IS_NEW_ARCHIVE_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			sq_new_archive_dialog_get_type()))

typedef struct _SQNewArchiveDialog SQNewArchiveDialog;

struct _SQNewArchiveDialog
{
	GtkFileChooserDialog parent;
	GtkWidget *archive_types_combo;
	GtkFileFilter *file_filter; /* HACK -- should just be set inside the constructor */
	GSList *supported_mime_types;
};

typedef struct _SQNewArchiveDialogClass SQNewArchiveDialogClass;

struct _SQNewArchiveDialogClass
{
	GtkFileChooserDialogClass parent_class;
};

GType sq_new_archive_dialog_get_type();
GtkWidget *sq_new_archive_dialog_new();
gchar *sq_new_archive_dialog_get_filename(SQNewArchiveDialog *dialog);

G_END_DECLS
#endif /* __SQRCHIVER_NEW_ARCHIVE_DIALOG_H__ */
