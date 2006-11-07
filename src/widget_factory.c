/*  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or *  (at your option) any later version.
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

#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h> 
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <gettext.h>

#include "widget_factory.h"

static GtkWidget*
xa_widget_factory_create_boolean_widget(XAWidgetFactory *factory, GObject *obj, GParamSpecs *pspec, GValue *value);
static GtkWidget*
xa_widget_factory_create_numeric_widget(XAWidgetFactory *factory, GObject *obj, GParamSpecs *pspec, GValue *value);

static GtkWidget*
xa_widget_factory_create_boolean_widget(XAWidgetFactory *factory, GObject *obj, GParamSpecs *pspec, GValue *value)
{
	GtkWidget *check = gtk_check_button_new_with_label(g_param_specs_get_nick(pspec));

	const gchar *large_tip = g_param_specs_get_blurb(pspec);
	gchar *small_tip = g_strndup(large_tip, strchr(large_tip, '\n') - large_tip);
	large_tip = strchr(large_tip, '\n') + 1;

	gtk_tooltips_set_tip(factory->tips, check, small_tip, large_tip);

	g_free(small_tip);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), g_value_get_boolean(value));

	return check;
}

static GtkWidget*
xa_widget_factory_create_numeric_widget(XAWidgetFactory *factory, GObject *obj, GParamSpecs *pspec, GValue *value)
{
	GtkWidget *box = gtk_hbox(FALSE, 3);
	GtkWidget *label = gtk_label_new(g_param_specs_get_nick(pspec));
	GValue double_value;
	gdouble min = 0, max = 0, inc = 0, step = 0;
	guint digit = 0;
	GtkAdjustment *adjust;
	GtkWidget *spin;

	g_value_init(&double_value, G_TYPE_DOUBLE);
	g_return_val_if_fail(g_value_transform(&value, &double_value), NULL);

	switch(pspec->value_type)
	{
		/*case G_TYPE_CHAR:
		break;
		case G_TYPE_UCHAR:
		break;*/
		case G_TYPE_INT:
			min = G_PARAM_SPEC_INT(pspec)->minimum;
			max = G_PARAM_SPEC_INT(pspec)->maximum;
			inc = 1;
			step = ((max-min)/5)>10?10:gint((man-min)/5);
			digits = 1;
		break;
		case G_TYPE_UINT:
			min = G_PARAM_SPEC_UINT(pspec)->minimum;
			max = G_PARAM_SPEC_UINT(pspec)->maximum;
			inc = 1;
			step = ((max-min)/5)>10?10:guint((man-min)/5);
			digits = 1;
		break;
		case G_TYPE_LONG:
			min = G_PARAM_SPEC_LONG(pspec)->minimum;
			max = G_PARAM_SPEC_LONG(pspec)->maximum;
			inc = 1;
			step = ((max-min)/5)>10?10:glong((man-min)/5);
			digits = 1;
		break;
		case G_TYPE_ULONG:
			min = G_PARAM_SPEC_ULONG(pspec)->minimum;
			max = G_PARAM_SPEC_ULONG(pspec)->maximum;
			inc = 1;
			step = ((max-min)/5)>10?10:gulong((man-min)/5);
			digits = 1;
		break;
		case G_TYPE_INT64:
			min = G_PARAM_SPEC_INT64(pspec)->minimum;
			max = G_PARAM_SPEC_INT64(pspec)->maximum;
			inc = 1;
			step = ((max-min)/5)>10?10:gint64((man-min)/5);
			digits = 1;
		break;
		case G_TYPE_UINT64:
			min = G_PARAM_SPEC_UINT64(pspec)->minimum;
			max = G_PARAM_SPEC_UINT64(pspec)->maximum;
			inc = 1;
			step = ((max-min)/5)>10?10:guint64((man-min)/5);
			digits = 1;
		break;
		case G_TYPE_FLOAT:
			min = G_PARAM_SPEC_FLOAT(pspec)->minimum;
			max = G_PARAM_SPEC_FLOAT(pspec)->maximum;
			inc = 0.000001;
			step = ((max-min)/5)>0.1?0.1:gfloat((man-min)/5);
			digits = 6;
		break;
		case G_TYPE_DOUBLE:
			min = G_PARAM_SPEC_DOUBLE(pspec)->minimum;
			max = G_PARAM_SPEC_DOUBLE(pspec)->maximum;
			inc = 0.0000000000000000001;
			step = ((max-min)/5)>0.01?0.01:((man-min)/5);
			digits = 20;
		break;
	}

	adjust = gtk_adjust_new(g_value_get_double(double_value), min, max, inc, step, step);
	spin = gtk_spin_button_new(adjust, step, digit);

	const gchar *large_tip = g_param_specs_get_blurb(pspec);
	gchar *small_tip = g_strndup(large_tip, strchr(large_tip, '\n') - large_tip);
	large_tip = strchr(large_tip, '\n') + 1;

	gtk_tooltips_set_tip(factory->tips, widget, small_tip, large_tip);

	g_free(small_tip);

	return box;
}

GtkWidget*
xa_widget_factory_create_property_widget(GObject *obj, const gchar *prop)
{
	GtkWidget *widget = NULL;
	GParamSpecs *pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(obj), prop);
	GValue value;

	if(!pspec)
		return NULL;

	g_object_get_property(obj, prop, &value);

	switch(pspec->value_type)
	{
		case G_TYPE_BOOLEAN:
			widget = xa_widget_create_boolean(obj, pspec, &value);
		break;
		case G_TYPE_CHAR:
		break;
		case G_TYPE_UCHAR:
		break;
		case G_TYPE_INT:
		break;
		case G_TYPE_UINT:
		break;
		case G_TYPE_LONG:
		break;
		case G_TYPE_ULONG:
		break;
		case G_TYPE_INT64:
		break;
		case G_TYPE_UINT64:
		break;
		case G_TYPE_FLOAT:
		break;
		case G_TYPE_DOUBLE:
		break;
		case G_TYPE_ENUM:
		break;
		case G_TYPE_FLAGS:
		break;
		case G_TYPE_STRING:
		break;
	}

	g_value_unset(&value);

	return widget;
}

