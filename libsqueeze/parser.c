/*
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or 
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
 *    GNU Library General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <glib.h>
#include <glib-object.h> 
#include <signal.h>

#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "libsqueeze.h"

#include "parser-context.h"
#include "parser.h"

G_DEFINE_ABSTRACT_TYPE(LSQParser, lsq_parser, G_TYPE_OBJECT);

struct _LSQTypeStorageGroup
{
    guint size_of;
    guint n_types;
    GType *types;
};
typedef struct _LSQTypeStorageGroup LSQTypeStorageGroup;
static LSQTypeStorageGroup *storage_groups = NULL;
static guint storage_group_count = 0;

static void
lsq_storage_group_add(GType type, guint size_of)
{
    guint i;
    LSQTypeStorageGroup *_storage_groups;
    GType *_types;

    for (i = 0; i < storage_group_count; ++i)
    {
        if (storage_groups[i].size_of <= size_of)
        {
            break;
        }
    }

    g_return_if_fail(i <= storage_group_count);

    if (i == storage_group_count ||
            storage_groups[i].size_of < size_of)
    {
        _storage_groups = g_new0(LSQTypeStorageGroup, storage_group_count+1);

        memcpy(_storage_groups, storage_groups, sizeof(LSQTypeStorageGroup) * i);
        memcpy(_storage_groups + i + 1, storage_groups + i, sizeof(LSQTypeStorageGroup) * (storage_group_count - i));

        g_free(storage_groups);
        storage_groups = _storage_groups;
        ++storage_group_count;

        storage_groups[i].size_of = size_of;
        storage_groups[i].n_types = 0;
        storage_groups[i].types = NULL;
    }

    _types = g_new(GType, storage_groups[i].n_types+1);

    memcpy(_types, storage_groups[i].types, sizeof(GType) * storage_groups[i].n_types);

    g_free(storage_groups[i].types);
    storage_groups[i].types = _types;
    _types[storage_groups[i].n_types] = type;
    ++storage_groups[i].n_types;
}

static gboolean
lsq_storage_group_has_type (
        LSQTypeStorageGroup *storage_group,
        GType type )
{
    guint i;
    for (i = 0; i < storage_group->n_types; ++i)
    {
        if (storage_group->types[i] == type)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void
lsq_parser_init ( LSQParser *self )
{
}

static void
lsq_parser_class_init ( LSQParserClass *klass )
{
    klass->get_context = NULL;

    lsq_storage_group_add(G_TYPE_CHAR, sizeof(gchar));
    lsq_storage_group_add(G_TYPE_DOUBLE, sizeof(gdouble));
    lsq_storage_group_add(G_TYPE_FLOAT, sizeof(gfloat));
    lsq_storage_group_add(G_TYPE_INT, sizeof(gint));
    lsq_storage_group_add(G_TYPE_INT64, sizeof(gint64));
    lsq_storage_group_add(G_TYPE_LONG, sizeof(glong));
    lsq_storage_group_add(G_TYPE_STRING, sizeof(gchar*));
    lsq_storage_group_add(G_TYPE_UINT, sizeof(guint));
    lsq_storage_group_add(G_TYPE_UINT64, sizeof(guint64));
    lsq_storage_group_add(G_TYPE_ULONG, sizeof(gulong));
}

LSQParserContext*
lsq_parser_get_context (
        LSQParser *self,
        LSQArchive *archive )
{
    LSQParserClass *klass = LSQ_PARSER_GET_CLASS(self);

    g_return_val_if_fail(klass->get_context, NULL);

    return klass->get_context(self, archive);
}

void
lsq_parser_parse (
        LSQParser *self,
        LSQParserContext *ctx )
{
    LSQParserClass *klass = LSQ_PARSER_GET_CLASS(self);

    g_return_if_fail(klass->parse);

    klass->parse(self, ctx);
}

guint
lsq_parser_n_properties ( LSQParser *parser )
{
    return parser->n_properties;
}

GType
lsq_parser_get_property_type (
        LSQParser *parser,
        guint nr )
{
    g_return_val_if_fail(nr < parser->n_properties, G_TYPE_NONE);
    return parser->property_types[nr];
}

guint
lsq_parser_get_property_offset (
        LSQParser *parser,
        guint nr )
{
    g_return_val_if_fail(nr < parser->n_properties, 0);
    return parser->property_offset[nr];
}

void
lsq_parser_set_property_type (
        LSQParser *parser,
        guint nr,
        GType type )
{
    guint i, j;
    guint size_of, offset = 0;

    if(nr >= parser->n_properties)
    {
        GType *new_list = g_new(GType, nr+1);
        for(i=0; i < parser->n_properties; ++i)
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

        g_free(parser->property_offset);
        parser->property_offset = g_new(guint, nr+1);
    }

    parser->property_types[nr] = type;

    for (i = 0; i < storage_group_count; ++i)
    {
        size_of = storage_groups[i].size_of;
        for (j = 0; j < parser->n_properties; ++j)
        {
            if (lsq_storage_group_has_type(&storage_groups[i], parser->property_types[j]))
            {
                guint align = offset % size_of;
                if ( 0 != align )
                {
                    offset += size_of - align;
                }

                /* Store the offset as count of block sized size_of */
                parser->property_offset[j] = offset / size_of;
                offset += size_of;
            }
        }
    }

    parser->properties_size = offset;
}

guint
lsq_parser_get_properties_size ( LSQParser *parser )
{
    return parser->properties_size;
}

