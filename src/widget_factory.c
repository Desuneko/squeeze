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
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h> 
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <gettext.h>

#include "widget_factory.h"

#define XA_PROPERTY_SPEC_DATA "xa-property-spec"
#define XA_PROPERTY_VALUE_DATA "xa-property-value"

static void
xa_widget_factory_class_init(XAWidgetFactoryClass *factory_class);
static void
xa_widget_factory_init(XAWidgetFactory *factory);

static GtkWidget*
xa_widget_factory_create_boolean_widget(XAWidgetFactory *factory, GObject *obj, GParamSpec *pspec, const GValue *value);
static GtkWidget*
xa_widget_factory_create_numeric_widget(XAWidgetFactory *factory, GObject *obj, GParamSpec *pspec, const GValue *value);
static GtkWidget*
xa_widget_factory_create_enum_widget(XAWidgetFactory *factory, GObject *obj, GParamSpec *pspec, const GValue *value);
static GtkWidget*
xa_widget_factory_create_flags_widget(XAWidgetFactory *factory, GObject *obj, GParamSpec *pspec, const GValue *value);
static GtkWidget*
xa_widget_factory_create_string_widget(XAWidgetFactory *factory, GObject *obj, GParamSpec *pspec, const GValue *value);

static void
cb_xa_widget_factory_property_changed(GtkWidget *widget, gpointer user_data);

GType
xa_widget_factory_get_type()
{
	static GType xa_widget_factory_type = 0;

	if(!xa_widget_factory_type)
	{
		static const GTypeInfo xa_widget_factory_info =
		{
			sizeof(XAWidgetFactoryClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) xa_widget_factory_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof(XAWidgetFactory),
			0,
			(GInstanceInitFunc) xa_widget_factory_init,
			NULL
		};

		xa_widget_factory_type = g_type_register_static(G_TYPE_OBJECT, "XAWidgetFactory", &xa_widget_factory_info, 0);
	}
	return xa_widget_factory_type;
}

static void
xa_widget_factory_class_init(XAWidgetFactoryClass *factory_class)
{
/*	GObjectClass *object_class = G_OBJECT_CLASS(factory_class);*/
}

static void
xa_widget_factory_init(XAWidgetFactory *factory)
{
	factory->tips = gtk_tooltips_new();
}

XAWidgetFactory*
xa_widget_factory_new()
{
	XAWidgetFactory *factory;

	factory = g_object_new(xa_widget_factory_get_type(), NULL);

	return factory;
}

static GtkWidget*
xa_widget_factory_create_boolean_widget(XAWidgetFactory *factory, GObject *obj, GParamSpec *pspec, const GValue *value)
{
	GtkWidget *check = gtk_check_button_new_with_label(g_param_spec_get_nick(pspec));

	g_object_set_data(G_OBJECT(check), XA_PROPERTY_SPEC_DATA, pspec);
	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(cb_xa_widget_factory_property_changed), obj);

	const gchar *large_tip = g_param_spec_get_blurb(pspec);
	gchar *small_tip = NULL;
	if(strchr(large_tip, '\n'))
	{
		small_tip = g_strndup(large_tip, strchr(large_tip, '\n') - large_tip);
		large_tip = strchr(large_tip, '\n') + 1;
	}

	gtk_tooltips_set_tip(factory->tips, check, small_tip?small_tip:large_tip, large_tip);

	g_free(small_tip);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), g_value_get_boolean(value));

	gtk_widget_show(check);

	return check;
}

static GtkWidget*
xa_widget_factory_create_numeric_widget(XAWidgetFactory *factory, GObject *obj, GParamSpec *pspec, const GValue *value)
{
	GtkWidget *box = gtk_hbox_new(FALSE, 3);
	GtkWidget *label = gtk_label_new(g_param_spec_get_nick(pspec));
	GValue double_value;
	gdouble min = 0, max = 0, inc = 0, step = 0;
	guint digits = 0;
	GtkAdjustment *adjust;
	GtkWidget *spin;

	memset(&double_value, 0, sizeof(GValue));

	g_value_init(&double_value, G_TYPE_DOUBLE);
	g_return_val_if_fail(g_value_transform(value, &double_value), NULL);

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
			step = ((max-min)/5)>10?10:(gint)((max-min)/5);
			digits = 1;
		break;
		case G_TYPE_UINT:
			min = G_PARAM_SPEC_UINT(pspec)->minimum;
			max = G_PARAM_SPEC_UINT(pspec)->maximum;
			inc = 1;
			step = ((max-min)/5)>10?10:(guint)((max-min)/5);
			digits = 0;
		break;
		case G_TYPE_LONG:
			min = G_PARAM_SPEC_LONG(pspec)->minimum;
			max = G_PARAM_SPEC_LONG(pspec)->maximum;
			inc = 1;
			step = ((max-min)/5)>10?10:(glong)((max-min)/5);
			digits = 0;
		break;
		case G_TYPE_ULONG:
			min = G_PARAM_SPEC_ULONG(pspec)->minimum;
			max = G_PARAM_SPEC_ULONG(pspec)->maximum;
			inc = 1;
			step = ((max-min)/5)>10?10:(gulong)((max-min)/5);
			digits = 0;
		break;
		case G_TYPE_INT64:
			min = G_PARAM_SPEC_INT64(pspec)->minimum;
			max = G_PARAM_SPEC_INT64(pspec)->maximum;
			inc = 1;
			step = ((max-min)/5)>10?10:(gint64)((max-min)/5);
			digits = 0;
		break;
		case G_TYPE_UINT64:
			min = G_PARAM_SPEC_UINT64(pspec)->minimum;
			max = G_PARAM_SPEC_UINT64(pspec)->maximum;
			inc = 1;
			step = ((max-min)/5)>10?10:(guint64)((max-min)/5);
			digits = 0;
		break;
		case G_TYPE_FLOAT:
			min = G_PARAM_SPEC_FLOAT(pspec)->minimum;
			max = G_PARAM_SPEC_FLOAT(pspec)->maximum;
			inc = 0.000001;
			step = ((max-min)/5)>0.1?0.1:(gfloat)((max-min)/5);
			digits = 6;
		break;
		case G_TYPE_DOUBLE:
			min = G_PARAM_SPEC_DOUBLE(pspec)->minimum;
			max = G_PARAM_SPEC_DOUBLE(pspec)->maximum;
			inc = 0.0000000000000000001;
			step = ((max-min)/5)>0.01?0.01:((max-min)/5);
			digits = 20;
		break;
	}

	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(g_value_get_double(&double_value), min, max, inc, step, step));
	spin = gtk_spin_button_new(adjust, step, digits);

	g_object_set_data(G_OBJECT(spin), XA_PROPERTY_SPEC_DATA, pspec);
	g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(cb_xa_widget_factory_property_changed), obj);

	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 3);
	gtk_box_pack_end(GTK_BOX(box), spin, TRUE, TRUE, 3);

	const gchar *large_tip = g_param_spec_get_blurb(pspec);
	gchar *small_tip = NULL;
	if(strchr(large_tip, '\n'))
	{
		small_tip = g_strndup(large_tip, strchr(large_tip, '\n') - large_tip);
		large_tip = strchr(large_tip, '\n') + 1;
	}

	gtk_tooltips_set_tip(factory->tips, label, small_tip?small_tip:large_tip, large_tip);
	gtk_tooltips_set_tip(factory->tips, spin, small_tip?small_tip:large_tip, large_tip);

	g_free(small_tip);

	g_value_unset(&double_value);

	gtk_widget_show_all(box);

	return box;
}

static GtkWidget*
xa_widget_factory_create_enum_widget(XAWidgetFactory *factory, GObject *obj, GParamSpec *pspec, const GValue *value)
{
	GtkWidget *frame = gtk_frame_new(g_param_spec_get_nick(pspec));
	GtkWidget *box = gtk_vbox_new(FALSE, 3);
	GtkWidget *radio = NULL;
	guint i, n = G_PARAM_SPEC_ENUM(pspec)->enum_class->n_values;
	GEnumValue *values = G_PARAM_SPEC_ENUM(pspec)->enum_class->values;

	for(i = 0; i < n; ++i)
	{
		radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio), values[i].value_nick);

		g_object_set_data(G_OBJECT(radio), XA_PROPERTY_SPEC_DATA, pspec);
		g_signal_connect(G_OBJECT(radio), "toggled", G_CALLBACK(cb_xa_widget_factory_property_changed), obj);

		if(g_value_get_enum(value) == values[i].value)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
		gtk_box_pack_start(GTK_BOX(box), radio, FALSE, FALSE, 5);
	}

	gtk_container_add(GTK_CONTAINER(frame), box);

	const gchar *large_tip = g_param_spec_get_blurb(pspec);
	gchar *small_tip = NULL;
	if(strchr(large_tip, '\n'))
	{
		small_tip = g_strndup(large_tip, strchr(large_tip, '\n') - large_tip);
		large_tip = strchr(large_tip, '\n') + 1;
	}

	gtk_tooltips_set_tip(factory->tips, frame, small_tip?small_tip:large_tip, large_tip);

	g_free(small_tip);

	gtk_widget_show_all(frame);

	return frame;
}

static GtkWidget*
xa_widget_factory_create_flags_widget(XAWidgetFactory *factory, GObject *obj, GParamSpec *pspec, const GValue *value)
{
	GtkWidget *frame = gtk_frame_new(g_param_spec_get_nick(pspec));
	GtkWidget *box = gtk_vbox_new(FALSE, 3);
	GtkWidget *check;
	guint i, n = G_PARAM_SPEC_FLAGS(pspec)->flags_class->n_values;
	GFlagsValue *values = G_PARAM_SPEC_FLAGS(pspec)->flags_class->values;

	gtk_container_add(GTK_CONTAINER(frame), box);

	for(i = 0; i < n; ++i)
	{
		check = gtk_check_button_new_with_label(values[i].value_nick);

		g_object_set_data(G_OBJECT(check), XA_PROPERTY_SPEC_DATA, pspec);
		g_object_set_data(G_OBJECT(check), XA_PROPERTY_VALUE_DATA, GINT_TO_POINTER(values[i].value));
		g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(cb_xa_widget_factory_property_changed), obj);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), g_value_get_enum(value) & values[i].value);
		gtk_box_pack_start(GTK_BOX(box), check, FALSE, FALSE, 5);
	}

	const gchar *large_tip = g_param_spec_get_blurb(pspec);
	gchar *small_tip = NULL;
	if(strchr(large_tip, '\n'))
	{
		small_tip = g_strndup(large_tip, strchr(large_tip, '\n') - large_tip);
		large_tip = strchr(large_tip, '\n') + 1;
	}

	gtk_tooltips_set_tip(factory->tips, frame, small_tip?small_tip:large_tip, large_tip);

	g_free(small_tip);

	gtk_widget_show_all(frame);

	return frame;
}

static GtkWidget*
xa_widget_factory_create_string_widget(XAWidgetFactory *factory, GObject *obj, GParamSpec *pspec, const GValue *value)
{
	g_debug("string");
	GtkWidget *box = gtk_hbox_new(FALSE, 3);
	GtkWidget *label = gtk_label_new(g_param_spec_get_nick(pspec));
	GtkWidget *entry = gtk_entry_new();

	g_object_set_data(G_OBJECT(entry), XA_PROPERTY_SPEC_DATA, pspec);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(cb_xa_widget_factory_property_changed), obj);

	gtk_entry_set_text(GTK_ENTRY(entry), g_value_get_string(value));

	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 3);
	gtk_box_pack_end(GTK_BOX(box), entry, TRUE, TRUE, 3);

	const gchar *large_tip = g_param_spec_get_blurb(pspec);
	gchar *small_tip = NULL;
	if(strchr(large_tip, '\n'))
	{
		small_tip = g_strndup(large_tip, strchr(large_tip, '\n') - large_tip);
		large_tip = strchr(large_tip, '\n') + 1;
	}

	gtk_tooltips_set_tip(factory->tips, label, small_tip?small_tip:large_tip, large_tip);
	gtk_tooltips_set_tip(factory->tips, entry, small_tip?small_tip:large_tip, large_tip);

	g_free(small_tip);

	gtk_widget_show_all(box);

	return box;
}

GtkWidget*
xa_widget_factory_create_property_widget(XAWidgetFactory *factory, GObject *obj, const gchar *prop)
{
	GtkWidget *widget = NULL;
	GParamSpec *pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(obj), prop);
	GValue value;

	if(!pspec)
		return NULL;

	memset(&value, 0, sizeof(GValue));

	/* FIXME: object property type string is bugged, in glib? */
	g_value_init(&value, pspec->value_type);
	g_object_get_property(obj, prop, &value);

	switch(pspec->value_type)
	{
		case G_TYPE_BOOLEAN:
			widget = xa_widget_factory_create_boolean_widget(factory, obj, pspec, &value);
		break;
		/*
		case G_TYPE_CHAR:
		case G_TYPE_UCHAR:
		break;*/
		case G_TYPE_INT:
		case G_TYPE_UINT:
		case G_TYPE_LONG:
		case G_TYPE_ULONG:
		case G_TYPE_INT64:
		case G_TYPE_UINT64:
		case G_TYPE_FLOAT:
		case G_TYPE_DOUBLE:
			widget = xa_widget_factory_create_numeric_widget(factory, obj, pspec, &value);
		break;
		case G_TYPE_ENUM:
			widget = xa_widget_factory_create_enum_widget(factory, obj, pspec, &value);
		break;
		case G_TYPE_FLAGS:
			widget = xa_widget_factory_create_flags_widget(factory, obj, pspec, &value);
		break;
		case G_TYPE_STRING:
			widget = xa_widget_factory_create_string_widget(factory, obj, pspec, &value);
		break;
	}

	g_value_unset(&value);

	return widget;
}

static void
cb_xa_widget_factory_property_changed(GtkWidget *widget, gpointer user_data)
{
	GParamSpec *pspec = g_object_get_data(G_OBJECT(widget), XA_PROPERTY_SPEC_DATA);
	GValue value, other_value;
	memset(&value, 0, sizeof(GValue));
	memset(&other_value, 0, sizeof(GValue));

	g_value_init(&value, pspec->value_type);

	switch(pspec->value_type)
	{
		case G_TYPE_BOOLEAN:
			g_value_set_boolean(&value, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
			g_object_set_property(G_OBJECT(user_data), g_param_spec_get_name(pspec), &value);
		break;
		/*
		case G_TYPE_CHAR:
		case G_TYPE_UCHAR:
		break;*/
		case G_TYPE_INT:
		case G_TYPE_UINT:
		case G_TYPE_LONG:
		case G_TYPE_ULONG:
		case G_TYPE_INT64:
		case G_TYPE_UINT64:
		case G_TYPE_FLOAT:
		case G_TYPE_DOUBLE:
			g_value_init(&other_value, G_TYPE_DOUBLE);
			g_value_set_double(&other_value, gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
			g_value_transform(&other_value, &value);
			g_value_unset(&other_value);
			g_object_set_property(G_OBJECT(user_data), g_param_spec_get_name(pspec), &value);
		break;
		case G_TYPE_ENUM:
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
			{
				g_value_set_enum(&value, GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), XA_PROPERTY_VALUE_DATA)));
				g_object_set_property(G_OBJECT(user_data), g_param_spec_get_name(pspec), &value);
			}
		break;
		case G_TYPE_FLAGS:
			// TODO: sync?
			g_object_get_property(G_OBJECT(user_data), g_param_spec_get_name(pspec), &value);
			g_value_set_flags(&value, g_value_get_flags(&value) ^ GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), XA_PROPERTY_VALUE_DATA)));
			g_object_set_property(G_OBJECT(user_data), g_param_spec_get_name(pspec), &value);
		break;
		case G_TYPE_STRING:
			g_value_set_string(&value, gtk_entry_get_text(GTK_ENTRY(widget)));
			g_object_set_property(G_OBJECT(user_data), g_param_spec_get_name(pspec), &value);
		break;
	}

	g_value_unset(&value);
}

