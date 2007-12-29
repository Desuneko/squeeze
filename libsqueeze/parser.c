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

#include <thunar-vfs/thunar-vfs.h>
#include "libsqueeze.h"

#include "parser-context.h"
#include "parser.h"

G_DEFINE_ABSTRACT_TYPE(LSQParser, lsq_parser, G_TYPE_OBJECT);

static void
lsq_parser_init(LSQParser *self)
{
}

static void
lsq_parser_class_init(LSQParserClass *klass)
{
	klass->get_context = NULL;
}

LSQParserContext*
lsq_parser_get_context(LSQParser *self, LSQArchive *archive)
{
	LSQParserClass *klass = LSQ_PARSER_GET_CLASS(self);

	g_return_val_if_fail(klass->get_context, NULL);

	return klass->get_context(self, archive);
}

void
lsq_parser_parse(LSQParser *self, LSQParserContext *ctx)
{
	LSQParserClass *klass = LSQ_PARSER_GET_CLASS(self);

	g_return_if_fail(klass->parse);

	klass->parse(self, ctx);
}

guint
lsq_parser_n_properties(LSQParser *parser)
{
  return parser->n_properties;
}

GType
lsq_parser_get_property_type(LSQParser *parser, guint nr)
{
  g_return_val_if_fail(nr < parser->n_properties, G_TYPE_NONE);
  return parser->property_types[nr];
}

void
lsq_parser_set_property_type(LSQParser *parser, guint nr, GType type)
{
  if(nr >= parser->n_properties)
  {
    GType *new_list = g_new(GType, nr+1);
    guint i;
    for(i=0; i < parser->n_properties; i++)
    {
      new_list[i] = parser->property_types[i];
    }
    while(i<nr)
    {
      new_list[i++] = G_TYPE_NONE;
    }
    g_free(parser->property_types);
    parser->property_types = new_list;
    parser->n_properties = nr+1;
  }
  parser->property_types[nr] = type;
}

