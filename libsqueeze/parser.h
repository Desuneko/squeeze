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

#ifndef __LIBSQUEEZE_PARSER_H__
#define __LIBSQUEEZE_PARSER_H__ 

G_BEGIN_DECLS

#define LSQ_TYPE_PARSER lsq_parser_get_type()

#define LSQ_PARSER(obj) ( \
        G_TYPE_CHECK_INSTANCE_CAST ((obj), \
            LSQ_TYPE_PARSER, \
            LSQParser))

#define LSQ_IS_PARSER(obj) ( \
        G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
            LSQ_TYPE_PARSER))

#define LSQ_PARSER_CLASS(klass) ( \
        G_TYPE_CHECK_CLASS_CAST ((klass), \
            LSQ_TYPE_PARSER, \
            LSQParserClass))

#define LSQ_IS_PARSER_CLASS(klass) ( \
        G_TYPE_CHECK_CLASS_TYPE ((klass), \
            LSQ_TYPE_PARSER))

#define LSQ_PARSER_GET_CLASS(obj) ( \
        G_TYPE_INSTANCE_GET_CLASS ((obj), \
            LSQ_TYPE_PARSER, \
            LSQParserClass))


#if 0
typedef struct _LSQParser LSQParser;
#endif

struct _LSQParser
{
    GObject parent;

    guint n_properties;
    GType *property_types;
    guint *property_offset;
    guint properties_size;

    gchar *datetime_format;
};


typedef struct _LSQParserClass LSQParserClass;

struct _LSQParserClass
{
    GObjectClass parent;

    LSQParserContext *
    (*get_context)(
            LSQParser *,
            LSQArchive * );

    void
    (*parse)(
            LSQParser *,
            LSQParserContext *);
};


GType
lsq_parser_get_type ( void ) G_GNUC_CONST;

LSQParserContext *
lsq_parser_get_context (
        LSQParser *,
        LSQArchive *
    ) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

void
lsq_parser_parse (
        LSQParser *,
        LSQParserContext *
    );

guint
lsq_parser_n_properties ( LSQParser * ) G_GNUC_PURE;

GType
lsq_parser_get_property_type (
        LSQParser *,
        guint
    ) G_GNUC_PURE;

void
lsq_parser_set_property_type (
        LSQParser *,
        guint,
        GType
    );

gsize
lsq_parser_get_property_offset (
        LSQParser *,
        guint
    ) G_GNUC_PURE;

gsize
lsq_parser_get_properties_size ( LSQParser * ) G_GNUC_PURE;

void
lsq_parser_set_datetime_format (
        LSQParser *,
        const gchar *
    );

G_END_DECLS

#endif /* __LIBSQUEEZE_PARSER_H__ */

