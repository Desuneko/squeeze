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

#ifndef __SQRCHIVER_EXTRACT_ARCHIVE_DIALOG_H__
#define __SQRCHIVER_EXTRACT_ARCHIVE_DIALOG_H__
G_BEGIN_DECLS

#define SQ_EXTRACT_ARCHIVE_DIALOG(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			sq_extract_archive_dialog_get_type(),      \
			SQExtractArchiveDialog))

#define SQ_IS_EXTRACT_ARCHIVE_DIALOG(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			sq_extract_archive_dialog_get_type()))

#define SQ_EXTRACT_ARCHIVE_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			sq_extract_archive_dialog_get_type(),      \
			SQExtractArchiveDialogClass))

#define SQ_IS_EXTRACT_ARCHIVE_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			sq_extract_archive_dialog_get_type()))

typedef struct _SQExtractArchiveDialog SQExtractArchiveDialog;

struct _SQExtractArchiveDialog
{
	GtkFileChooserDialog parent;
	GtkWidget *l_frame;
	GtkWidget *r_frame;
	GtkWidget *all_files_radio;
	GtkWidget *sel_files_radio;
	LSQArchive *archive;
	LSQArchiveSupport *support;
};

typedef struct _SQExtractArchiveDialogClass SQExtractArchiveDialogClass;

struct _SQExtractArchiveDialogClass
{
	GtkFileChooserDialogClass parent_class;
};

GType      sq_extract_archive_dialog_get_type();
GtkWidget *sq_extract_archive_dialog_new(LSQArchiveSupport *, LSQArchive *, gboolean);

G_END_DECLS
#endif /* __SQRCHIVER_EXTRACT_ARCHIVE_DIALOG_H__ */
