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

#ifndef __SQRCHIVER_PREFERENCES_DIALOG_H__
#define __SQRCHIVER_PREFERENCES_DIALOG_H__
G_BEGIN_DECLS

#define SQ_PREFERENCES_DIALOG(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			sq_preferences_dialog_get_type(),      \
			SQPreferencesDialog))

#define SQ_IS_PREFERENCES_DIALOG(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			sq_preferences_dialog_get_type()))

#define SQ_PREFERENCES_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			sq_preferences_dialog_get_type(),      \
			SQPreferencesDialogClass))

#define SQ_IS_PREFERENCES_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			sq_preferences_dialog_get_type()))

typedef struct
{
	LSQArchiveSupport *support;
	GtkWidget *box;
} SQSupportTuple;

typedef struct _SQPreferencesDialog SQPreferencesDialog;

struct _SQPreferencesDialog
{
	GtkDialog parent;
	GtkWidget *notebook;
	struct {
		struct {
			GtkWidget *show_icons;
			GtkWidget *rules_hint;
			struct {
				GtkWidget *sort_case;
				GtkWidget *sort_folders;
			} sorting;
		} viewer;
	} general;
	struct {
		GtkWidget *notebook;
		GSList *support_list;
	} support;
};

typedef struct _SQPreferencesDialogClass SQPreferencesDialogClass;

struct _SQPreferencesDialogClass
{
	GtkDialogClass parent;
};

GType      sq_preferences_dialog_get_type ();
GtkWidget *sq_preferences_dialog_new();

G_END_DECLS
#endif /* __SQRCHIVER_PREFERENCES_DIALOG_H__ */
