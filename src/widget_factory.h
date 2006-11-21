/*
 *  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#ifndef __XA_WIDGET_FACTORY_H__
#define __XA_WIDGET_FACTORY_H__

G_BEGIN_DECLS

#define XA_WIDGET_FACTORY(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			xa_widget_factory_get_type(),      \
			LXAArchive))

#define LXA_IS_WIDGET_FACTORY(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			xa_widget_factory_get_type()))

#define XA_WIDGET_FACTORY_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			xa_widget_factory_get_type(),      \
			LXAArchiveClass))

#define LXA_IS_WIDGET_FACTORY_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			xa_widget_factory_get_type()))

typedef struct
{
	GObject parent;
	GtkTooltips *tips;
} XAWidgetFactory;

typedef struct
{
	GObjectClass parent;
} XAWidgetFactoryClass;

GType               xa_widget_factory_get_type(void);
XAWidgetFactory    *xa_widget_factory_new();

GtkWidget          *xa_widget_factory_create_property_widget(XAWidgetFactory *, GObject *, const gchar *);
GSList             *xa_widget_factory_create_property_menu(XAWidgetFactory *, GObject *, const gchar *);
GtkWidget          *xa_widget_factory_create_action_widget(XAWidgetFactory *, LXAArchiveSupport *, const gchar *);
GtkWidget          *xa_widget_factory_create_action_menu_item(XAWidgetFactory *, LXAArchiveSupport *, const gchar *);
GtkToolItem        *xa_widget_factory_create_action_bar(XAWidgetFactory *, LXAArchiveSupport *, const gchar *);
GSList             *xa_widget_factory_create_action_menu(XAWidgetFactory *, LXAArchiveSupport *);

#endif /*__XA_WIDGET_FACTORY_H__*/

