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

#ifndef __SQRCHIVER_MESSAGE_DIALOG_H__
#define __SQRCHIVER_MESSAGE_DIALOG_H__
G_BEGIN_DECLS

#define SQ_MESSAGE_DIALOG(obj)		 ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),	\
			sq_message_dialog_get_type(),	  \
			SQMessageDialog))

#define SQ_IS_MESSAGE_DIALOG(obj)	  ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),	\
			sq_message_dialog_get_type()))

#define SQ_MESSAGE_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),	 \
			sq_message_dialog_get_type(),	  \
			SQMessageDialogClass))

#define SQ_IS_MESSAGE_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),		\
			sq_message_dialog_get_type()))

typedef struct _SQMessageDialog SQMessageDialog;

struct _SQMessageDialog
{
	GtkWindow parent;
	LSQArchive *archive;
	GtkWidget *message_label;
	GtkWidget *progress_bar;
};

typedef struct _SQMessageDialogClass SQMessageDialogClass;

struct _SQMessageDialogClass
{
	GtkWindowClass parent;
};

GType	  sq_message_dialog_get_type();
GtkWidget *sq_message_dialog_new();

G_END_DECLS
#endif /* __SQRCHIVER_MESSAGE_DIALOG_H__ */
