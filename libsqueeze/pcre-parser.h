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

#ifndef __LIBSQUEEZE_PCRE_PARSER_H__
#define __LIBSQUEEZE_PCRE_PARSER_H__ 

G_BEGIN_DECLS

#define LSQ_TYPE_PCRE_PARSER lsq_pcre_parser_get_type()

#define LSQ_PCRE_PARSER(obj) (		    \
        G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
            LSQ_TYPE_PCRE_PARSER,	    \
            LSQPcreParser))

#define LSQ_IS_PCRE_PARSER(obj) (	    \
        G_TYPE_CHECK_INSTANCE_TYPE ((obj),  \
            LSQ_TYPE_PCRE_PARSER))

#define LSQ_PCRE_PARSER_CLASS(klass) (	    \
        G_TYPE_CHECK_CLASS_CAST ((klass),   \
            LSQ_TYPE_PCRE_PARSER,	    \
            LSQPcreParserClass))

#define LSQ_IS_PCRE_PARSER_CLASS(klass) (   \
        G_TYPE_CHECK_CLASS_TYPE ((klass),   \
            LSQ_TYPE_PCRE_PARSER))

#define LSQ_PCRE_PARSER_GET_CLASS(obj) (    \
        G_TYPE_INSTANCE_GET_CLASS ((obj),   \
            LSQ_TYPE_PCRE_PARSER,	    \
            LSQPcreParserClass))


typedef struct _LSQPcreParser LSQPcreParser;

typedef struct _LSQPcreParserClass LSQPcreParserClass;

GType
lsq_pcre_parser_get_type ( void ) G_GNUC_CONST;

LSQParser *
lsq_pcre_parser_new (
        const gchar *,
        gchar **
    ) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* __LIBSQUEEZE_PCRE_PARSER_H__ */
