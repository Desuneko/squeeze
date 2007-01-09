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

/*
 * The archive properties dialog is written to resemble the thunar file-
 * properties dialog  written by Benedict Meurer for the Thunar file manager.
 */

#ifndef __SQ_PROPERTIES_DIALOG_H__
#define __SQ_PROPERTIES_DIALOG_H__
G_BEGIN_DECLS

#define SQ_PROPERTIES_DIALOG(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			sq_properties_dialog_get_type(),      \
			SQPropertiesDialog))

#define SQ_IS_PROPERTIES_DIALOG(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			sq_properties_dialog_get_type()))

#define SQ_PROPERTIES_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			sq_properties_dialog_get_type(),      \
			SQPropertiesDialogClass))

#define SQ_IS_PROPERTIES_DIALOG_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			sq_properties_dialog_get_type()))

typedef struct _SQPropertiesDialog SQPropertiesDialog;

struct _SQPropertiesDialog
{
	GtkDialog parent;
	GtkTable  *table;
	GtkWidget *icon_image;
};

typedef struct _SQPropertiesDialogClass SQPropertiesDialogClass;

struct _SQPropertiesDialogClass
{
	GtkDialogClass parent;
};

GtkWidget *sq_properties_dialog_new(LSQArchive *, GtkIconTheme *);
void sq_properties_dialog_add_archive_property_str(SQPropertiesDialog *, const gchar *prop_name, const gchar *prop_value);

G_END_DECLS
#endif /* __SQRCHIVER_PROPERTIES_DIALOG_H__ */
