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

#include <config.h>
#include <glib.h>
#include <glib-object.h> 
#include <signal.h>

#include "libsqueeze.h"

#include "parser-context.h"

enum {
	LSQ_PARSER_CONTEXT_PROPERTY_ARCHIVE = 1
};

static void lsq_parser_context_set_property(GObject*, guint, const GValue*, GParamSpec*);
static void lsq_parser_context_get_property(GObject*, guint, GValue*, GParamSpec*);

G_DEFINE_TYPE(LSQParserContext, lsq_parser_context, G_TYPE_OBJECT);

static void
lsq_parser_context_init(LSQParser *self)
{
	self->archive = NULL;
}

static void
lsq_parser_context_class_init(LSQParserClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	object_class->set_property = lsq_parser_context_set_property;
	object_class->get_property = lsq_parser_context_get_property;

	pspec = g_param_spec_object("archive", NULL, NULL, LSQ_TYPE_ARCHIVE, G_PARAM_READABLE|G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(object_clas, LSQ_PARSER_CONTEXT_PROPERTY_ARCHIVE, pspec);
}

static void
lsq_parser_context_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch(property_id)
	{
		case LSQ_PARSER_CONTEXT_PROPERTY_ARCHIVE:
			LSQ_PARSER_CONTEXT(object)->archive = g_value_get_object(value);
		break;
	}
}

static void
lsq_parser_context_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch(property_id)
	{
		case LSQ_PARSER_CONTEXT_PROPERTY_ARCHIVE:
			g_value_set_object(value, LSQ_PARSER_CONTEXT(object)->archive);
		break;
	}
}

LSQParserContext*
lsq_parser_context_new(LSQArchive *archive)
{
	LSQParserContext *ctx
	
	g_return_val_if_fail(LSQ_IS_ARCHIVE(archive), NULL);

	ctx = g_object_new(LSQ_PARSER_CONTEXT_TYPE, "archive", archive);

	return ctx;
}

gboolean
lsq_parser_context_get_line(LSQParserContext *ctx, gchar **line, gsize *length)
{
	GIOStatus stat;

	stat = g_io_channel_read_line
}
