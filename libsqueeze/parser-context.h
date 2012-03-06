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

#ifndef __LIBSQUEEZE_PARSER_CONTEXT_H__
#define __LIBSQUEEZE_PARSER_CONTEXT_H__ 

G_BEGIN_DECLS

#define LSQ_TYPE_PARSER_CONTEXT lsq_parser_context_get_type()

#define LSQ_PARSER_CONTEXT(obj) ( \
        G_TYPE_CHECK_INSTANCE_CAST ((obj), \
            LSQ_TYPE_PARSER_CONTEXT, \
            LSQParserContext))

#define LSQ_IS_PARSER_CONTEXT(obj) ( \
        G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
            LSQ_TYPE_PARSER_CONTEXT))

#define LSQ_PARSER_CONTEXT_CLASS(klass) ( \
        G_TYPE_CHECK_CLASS_CAST ((klass), \
            LSQ_TYPE_PARSER_CONTEXT, \
            LSQParserContextClass))

#define LSQ_IS_PARSER_CONTEXT_CLASS(klass) ( \
        G_TYPE_CHECK_CLASS_TYPE ((klass), \
            LSQ_TYPE_PARSER_CONTEXT))

typedef struct _LSQParserContext LSQParserContext;

struct _LSQParserContext
{
    GObject parent;

    LSQArchive *archive;

    GIOChannel *channel;
    GIOStatus last_stat;
};

typedef struct _LSQParserContextClass LSQParserContextClass;

struct _LSQParserContextClass
{
    GObjectClass parent;
};

GType
lsq_parser_context_get_type ( void ) G_GNUC_CONST;

LSQParserContext *
lsq_parser_context_new ( LSQArchive *archive ) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

gboolean
lsq_parser_context_get_line (
        LSQParserContext *,
        gchar **,
        gsize *
    );

gboolean
lsq_parser_context_is_good ( LSQParserContext * ) G_GNUC_PURE;

gboolean
lsq_parser_context_read_again ( LSQParserContext *) G_GNUC_PURE;

void
lsq_parser_context_set_channel (
        LSQParserContext *,
        GIOChannel *
    );

G_END_DECLS

#endif /* __LIBSQUEEZE_PARSER_CONTEXT_H__ */
