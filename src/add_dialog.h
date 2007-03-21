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

#ifndef __SQRCHIVER_ADD_DIALOG_H__
#define __SQRCHIVER_ADD_DIALOG_H__
G_BEGIN_DECLS

#define SQ_ADD_DIALOG(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			sq_add_dialog_get_type(),      \
			SQAddDialog))

#define SQ_IS_ADD_DIALOG(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			sq_add_dialog_get_type()))

#define SQ_ADD_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			sq_add_dialog_get_type(),      \
			SQAddDialogClass))

#define SQ_IS_ADD_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			sq_add_dialog_get_type()))

typedef struct _SQAddDialog SQAddDialog;

struct _SQAddDialog
{
	GtkDialog parent;
	GtkWidget *scrolled_window;
	GtkWidget *optionframe;
	GtkListStore *file_liststore;
	GtkWidget *file_treeview;
	GtkWidget *files_radio;
	GtkWidget *dirs_radio;
	GtkWidget *add;
	GtkWidget *remove;
};

typedef struct _SQAddDialogClass SQAddDialogClass;

struct _SQAddDialogClass
{
	GtkDialogClass parent;
};

GType      sq_add_dialog_get_type();
GtkWidget *sq_add_dialog_new();
GSList    *sq_add_dialog_get_filenames(SQAddDialog *dialog);

G_END_DECLS
#endif /* __SQRCHIVER_ADD_DIALOG_H__ */
