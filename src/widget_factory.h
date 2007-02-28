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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#ifndef __SQ_WIDGET_FACTORY_H__
#define __SQ_WIDGET_FACTORY_H__

G_BEGIN_DECLS

#define SQ_WIDGET_FACTORY(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			sq_widget_factory_get_type(),      \
			LSQArchive))

#define LSQ_IS_WIDGET_FACTORY(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			sq_widget_factory_get_type()))

#define SQ_WIDGET_FACTORY_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			sq_widget_factory_get_type(),      \
			LSQArchiveClass))

#define LSQ_IS_WIDGET_FACTORY_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			sq_widget_factory_get_type()))

typedef struct
{
	GObject parent;
	GtkTooltips *tips;
	// LSQCustomActionCallback *custom_callback;
} SQWidgetFactory;

typedef struct
{
	GObjectClass parent;
} SQWidgetFactoryClass;

GType               sq_widget_factory_get_type(void);
SQWidgetFactory    *sq_widget_factory_new();

GtkWidget          *sq_widget_factory_create_property_widget(SQWidgetFactory *, GObject *, const gchar *);
GSList             *sq_widget_factory_create_property_menu(SQWidgetFactory *, GObject *, const gchar *);
// GtkWidget          *sq_widget_factory_create_action_widget(SQWidgetFactory *, LSQArchiveSupport *, LSQArchive *, const gchar *);
// GtkWidget          *sq_widget_factory_create_action_menu_item(SQWidgetFactory *, LSQArchiveSupport *, LSQArchive *, const gchar *);
// GtkToolItem        *sq_widget_factory_create_action_bar(SQWidgetFactory *, LSQArchiveSupport *, LSQArchive *, const gchar *);
// GSList             *sq_widget_factory_create_action_menu(SQWidgetFactory *, LSQArchiveSupport *, LSQArchive *);

#endif /*__SQ_WIDGET_FACTORY_H__*/

