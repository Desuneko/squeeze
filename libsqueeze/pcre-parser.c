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
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h> 
#include <pcre.h>

#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "libsqueeze.h"
#include "archive-iter.h"
#include "parser-context.h"
#include "parser.h"
#include "pcre-parser.h"
#include "archive.h"

typedef struct _type_parser type_parser;
typedef struct _LSQPcreParserContext LSQPcreParserContext;
typedef struct _LSQPcreParserContextClass LSQPcreParserContextClass;

typedef void (*LSQParseFunc)( gchar*, guint, LSQArchiveIter*, guint, LSQPcreParser* );

struct _type_parser
{
    int index;
    LSQParseFunc function;
};

struct _LSQPcreParserContext
{
    LSQParserContext parent;

    gchar *lines;
};

struct _LSQPcreParserContextClass
{
    LSQParserContextClass parent;
};

GType lsq_pcre_parser_context_get_type ( void );

struct _LSQPcreParser
{
    LSQParser parent;

    pcre *parser;
    pcre_extra *study;

    type_parser *types_list;

    int filename_index;

    gboolean multiline;
};

struct _LSQPcreParserClass
{
    LSQParserClass parent;
};

G_DEFINE_TYPE( LSQPcreParserContext, lsq_pcre_parser_context, LSQ_TYPE_PARSER_CONTEXT );

static void
lsq_pcre_parser_context_init ( LSQPcreParserContext *self )
{
}

static void
lsq_pcre_parser_context_class_init ( LSQPcreParserContextClass *klass )
{
}

static LSQParserContext *
lsq_pcre_parser_context_new ( LSQPcreParser *parser, LSQArchive *archive )
{
    LSQPcreParserContext *ctx;

    ctx = g_object_new( lsq_pcre_parser_context_get_type(), "archive", archive, NULL );

    return LSQ_PARSER_CONTEXT( ctx );
}

static void build_parser ( LSQPcreParser *, const gchar *, gchar ** );

static void lsq_pcre_parser_parse ( LSQPcreParser *, LSQPcreParserContext * );

G_DEFINE_TYPE( LSQPcreParser, lsq_pcre_parser, LSQ_TYPE_PARSER );

static void
lsq_pcre_parser_init ( LSQPcreParser *self )
{
}

static void
lsq_pcre_parser_class_init ( LSQPcreParserClass *klass )
{
    LSQParserClass *parser_class = LSQ_PARSER_CLASS( klass );
    parser_class->get_context = (LSQParserContext*(*)(LSQParser*,LSQArchive*))lsq_pcre_parser_context_new;
    parser_class->parse = (void(*)(LSQParser*,LSQParserContext*))lsq_pcre_parser_parse;
}

LSQParser *
lsq_pcre_parser_new ( const gchar *parser_string, gchar **parser_types )
{
    LSQPcreParser *parser;

    parser = g_object_new( LSQ_TYPE_PCRE_PARSER, NULL );

    /* Build the parser base on the provided configuration */
    build_parser( parser, parser_string, parser_types );

    return LSQ_PARSER( parser );
}

#define DEF_PARSE_NUM(func, base, type) \
static void parse_##func(gchar *str, guint lng, LSQArchiveIter *iter, guint n, LSQPcreParser *parser) {  \
    type val; \
    val = g_ascii_strtoll( str, NULL, base ); \
    lsq_archive_iter_set_prop( iter, n, &val ); \
}

#define DEF_PARSE_FLOAT(func, type)  \
static void parse_##func(gchar *str, guint lng, LSQArchiveIter *iter, guint n, LSQPcreParser *parser) {  \
    type  val;  \
    val = g_ascii_strtod(str, NULL);  \
    lsq_archive_iter_set_prop( iter, n, &val ); \
}

#define DEF_PARSE_UNS(func, base, type) \
static void parse_##func(gchar *str, guint lng, LSQArchiveIter *iter, guint n, LSQPcreParser *parser) {  \
    type val; \
    val = g_ascii_strtoull( str, NULL, base ); \
    lsq_archive_iter_set_prop( iter, n, &val ); \
}

static void
parse_char( gchar *str, guint lng, LSQArchiveIter *iter, guint n, LSQPcreParser *parser )
{
    gchar val;

    /* Read a single character in the character parser */
    val = *str;

    lsq_archive_iter_set_prop( iter, n, &val );
}

DEF_PARSE_NUM(decimal, 10, gint)
DEF_PARSE_NUM(decimal16, 10, gint)
DEF_PARSE_NUM(decimal32, 10, glong)
DEF_PARSE_NUM(decimal64, 10, gint64)

DEF_PARSE_FLOAT(floatingpoint, gfloat)
DEF_PARSE_FLOAT(double, gdouble)

DEF_PARSE_UNS(octal, 010, guint)
DEF_PARSE_UNS(octal16, 010, guint)
DEF_PARSE_UNS(octal32, 010, gulong)
DEF_PARSE_UNS(octal64, 010, guint64)

static void
parse_string( gchar *str, guint lng, LSQArchiveIter *iter, guint n, LSQPcreParser *parser )
{
    gchar *val;

    /* Create a copy of the string part */
    val = g_strndup( str, lng );

    lsq_archive_iter_set_prop( iter, n, val );
}

static void
parse_datetime( gchar *str, guint lng, LSQArchiveIter *iter, guint n, LSQPcreParser *parser )
{
    LSQDateTime val;

#ifdef DO_EXSTENSIVE_CHECKING
    gchar *end;
    val = lsq_datetime_from_string( str, LSQ_PARSER(parser)->datetime_format, *end);
    if ( LSQ_DATETIME_NULL != val && ( end - str ) > lng )
    {
        val = LSQ_DATETIME_NULL;
    }
#else
    val = lsq_datetime_from_string( str, LSQ_PARSER(parser)->datetime_format, NULL);
#endif

    lsq_archive_iter_set_prop( iter, n, &val );
}

DEF_PARSE_UNS(unsigned, 10, guint)
DEF_PARSE_UNS(unsigned16, 10, guint)
DEF_PARSE_UNS(unsigned32, 10, gulong)
DEF_PARSE_UNS(unsigned64, 10, guint64)

DEF_PARSE_UNS(hexadecimal, 0x10, guint)
DEF_PARSE_UNS(hexadecimal16, 0x10, guint)
DEF_PARSE_UNS(hexadecimal32, 0x10, gulong)
DEF_PARSE_UNS(hexadecimal64, 0x10, guint64)

static void
build_parser ( LSQPcreParser *parser, const gchar *parser_string, gchar **parser_types )
{
    const char *error;
    int error_pos;
    gint i = 0;
    gchar **iter;

    /* TODO: Should we use g_strstr instead? */
    /* If we want to support multiline matching without the (?m) flag we need to remove the starting lines one by one if no match was found.
     * This is not to difficult, we could just pass the middle of the string to pcre_exec. And this will improve speed. */
    parser->multiline = g_str_has_prefix( parser_string, "(?m)" );

    /* Compile the regex */
    parser->parser = pcre_compile(
            parser_string,
            PCRE_DUPNAMES | PCRE_NO_AUTO_CAPTURE,
            &error,
            &error_pos,
            NULL
        );

    if ( NULL == parser->parser )
    {
        g_error( "%s at %d in '%s'", error, error_pos, parser_string );
        return;
    }

    /* Study the regex for optimizations */
    parser->study = pcre_study(
            parser->parser,
            0,
            &error
        );

    if ( NULL != error )
    {
        g_error( "%s during study of '%s'", error, parser_string );
    }

    parser->filename_index = pcre_get_stringnumber( parser->parser, "F" );

    /* Create a list for type conversion for the found substrings */
    parser->types_list = g_new( type_parser, g_strv_length( parser_types ) );

    for ( iter = parser_types; *iter; ++iter, ++i)
    {
        gchar *name;
        gchar *ptr;
        GType type = G_TYPE_INVALID;
        type_parser *type_iter;
        gchar ch;
        enum {
            SIZE_NORMAL,
            SIZE_SHORT,
            SIZE_LONG,
            SIZE_LONGLONG
        } size_flag = SIZE_NORMAL;

        type_iter = &parser->types_list[i];

        /* The list is has the following syntax :
         * <submatch name>=<scanf type>
         */
        ptr = strchr( *iter, '=' );
        if ( NULL == ptr )
        {
            return;
        }

        /* Store the index of the <submatch name> to retrieve the value during parsing */
        name = g_strndup( *iter, ptr - *iter );
        type_iter->index = pcre_get_stringnumber( parser->parser, name );
        g_free( name );

        ++ptr;          /* Move past the '=' */
        ch = *ptr++;    /* read the first character of the scanf pattern */

        /* Check for size flags */
        switch ( ch )
        {
            case 'h':
                size_flag = SIZE_SHORT;
                ch = *ptr++;
                break;

            case 'l':
                size_flag = SIZE_LONG;
                ch = *ptr++;
                if('l' != ch)   /* ll equals L */
                    break;
            case 'L':
                size_flag = SIZE_LONGLONG;
                ch = *ptr++;
                break;
        }

        /* Check the type flag */
        switch ( ch )
        {
            case 'c':   /* Single character */
                g_return_if_fail( SIZE_NORMAL == size_flag );
                type_iter->function = parse_char;
                type = G_TYPE_CHAR;
                break;

            case 'd':   /* Decimal */
            case 'i':   /* Integer */
                switch( size_flag )
                {
                    case SIZE_NORMAL:
                        type_iter->function = parse_decimal;
                        type = G_TYPE_INT;
                        break;

                    case SIZE_SHORT:
                        type_iter->function = parse_decimal16;
                        type = G_TYPE_INT;
                        break;

                    case SIZE_LONG:
                        type_iter->function = parse_decimal32;
                        type = G_TYPE_LONG;
                        break;

                    case SIZE_LONGLONG:
                        type_iter->function = parse_decimal64;
                        type = G_TYPE_INT64;
                        break;
                }
                break;

            case 'f':   /* Floating point */
                g_return_if_fail( SIZE_NORMAL == size_flag || SIZE_LONGLONG == size_flag );
                switch(size_flag)
                {
                    case SIZE_NORMAL:
                        type_iter->function = parse_floatingpoint;
                        type = G_TYPE_FLOAT;
                        break;

                    case SIZE_LONGLONG:
                        type_iter->function = parse_double;
                        type = G_TYPE_DOUBLE;
                        break;

                    default:
                        break;
                }
                break;

            case 'o':   /* Octal unsigned integer */
                switch(size_flag)
                {
                    case SIZE_NORMAL:
                        type_iter->function = parse_octal;
                        type = G_TYPE_UINT;
                        break;

                    case SIZE_SHORT:
                        type_iter->function = parse_octal16;
                        type = G_TYPE_UINT;
                        break;

                    case SIZE_LONG:
                        type_iter->function = parse_octal32;
                        type = G_TYPE_ULONG;
                        break;

                    case SIZE_LONGLONG:
                        type_iter->function = parse_octal64;
                        type = G_TYPE_UINT64;
                        break;
                }
                break;

            case 's':   /* String */
                type_iter->function = parse_string;
                type = G_TYPE_STRING;
                break;

            case 't':   /* DateTime */
                type_iter->function = parse_datetime;
                type = LSQ_TYPE_DATETIME;
                break;

            case 'u':   /* Unsigned integer */
                switch(size_flag)
                {
                    case SIZE_NORMAL:
                        type_iter->function = parse_unsigned;
                        type = G_TYPE_UINT;
                        break;

                    case SIZE_SHORT:
                        type_iter->function = parse_unsigned16;
                        type = G_TYPE_UINT;
                        break;

                    case SIZE_LONG:
                        type_iter->function = parse_unsigned32;
                        type = G_TYPE_ULONG;
                        break;

                    case SIZE_LONGLONG:
                        type_iter->function = parse_unsigned64;
                        type = G_TYPE_UINT64;
                        break;
                }
                break;

            case 'x':   /* Hexadecimal lowercase */
            case 'X':   /* Hexadecimal uppercase */
                switch(size_flag)
                {
                    case SIZE_NORMAL:
                        type_iter->function = parse_hexadecimal;
                        type = G_TYPE_UINT;
                        break;

                    case SIZE_SHORT:
                        type_iter->function = parse_hexadecimal16;
                        type = G_TYPE_UINT;
                        break;

                    case SIZE_LONG:
                        type_iter->function = parse_hexadecimal32;
                        type = G_TYPE_ULONG;
                        break;

                    case SIZE_LONGLONG:
                        type_iter->function = parse_hexadecimal64;
                        type = G_TYPE_UINT64;
                        break;
                }
                break;

            default:
                g_return_if_reached();
        }

        g_return_if_fail( G_TYPE_INVALID != type );

        lsq_parser_set_property_type( LSQ_PARSER( parser ), i, type );
    }

    g_return_if_fail( lsq_parser_n_properties( LSQ_PARSER( parser ) ) == g_strv_length( parser_types ) );
}

static void
lsq_pcre_parser_parse ( LSQPcreParser *parser, LSQPcreParserContext *ctx )
{
    gchar *line;
    gchar *lines;
    gsize line_length;
    int ovector[60];
    int match_count;
    const char *string;
    guint i = 0;
    int index_;
    LSQArchive *archive;
    LSQArchiveIter *iter;
    int start, end;
    int options = 0;

    if ( FALSE == lsq_parser_context_get_line( LSQ_PARSER_CONTEXT( ctx ), &line, &line_length ) )
    {
        return;
    }

    if ( FALSE != parser->multiline )
    {
        options |= PCRE_PARTIAL_SOFT;

        if ( NULL != ctx->lines )
	{
	    line_length += strlen( ctx->lines );

	    /* TODO: use some big buffer to prevent allocation? */
	    lines = g_strconcat( ctx->lines, line, NULL );
	    g_free (ctx->lines);
	    ctx->lines = NULL;

	    g_free( line );
	    line = lines;
	}
    }

    /* Run the regex */
    /* TODO: Switch to pcre_dfa_exec for better performance? */
    match_count = pcre_exec(
            parser->parser,
            parser->study,
            line,
            line_length,
            0,
            options,
            ovector,
            60
        );

    if ( PCRE_ERROR_PARTIAL == match_count )
    {
	/* TODO: could store the partial match location for speed improvement and decrease memory consumption */
	ctx->lines = line;
	line = NULL;
    }
    else if ( 0 > match_count )
    {
        g_debug( "prce error: %d", match_count );
    }
    else if ( 0 == match_count )
    {
        g_debug( "prce out of match space" );
    }
    else if ( 0 < match_count )
    {
        /* Get the filename */
        pcre_get_substring(
                line,
                ovector,
                match_count,
                parser->filename_index,
                &string
            );

        archive = LSQ_PARSER_CONTEXT( ctx )->archive;

        /* Add the file to the archive */
        iter = lsq_archive_add_file( archive, string );

        pcre_free_substring( string );

        /* Get the values of all the subfields */
        for ( i = 0; i < lsq_parser_n_properties( LSQ_PARSER( parser ) ); ++i )
        {
            index_ = parser->types_list[i].index;

            start = ovector[index_ * 2];
            end = ovector[( index_ * 2 ) + 1];

            /* Parse the subfield */
            parser->types_list[i].function( line + start, end - start, iter, LSQ_ARCHIVE_PROP_USER +  i, parser );
        }

        lsq_archive_iter_unref( iter );
    }

    g_free( line );
}

