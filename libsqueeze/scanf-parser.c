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

#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "libsqueeze.h"
#include "archive-iter.h"
#include "parser-context.h"
#include "parser.h"
#include "scanf-parser.h"
#include "archive.h"

typedef struct _parse_part parse_part;
typedef struct _LSQScanfParserContext LSQScanfParserContext;
typedef struct _LSQScanfParserContextClass LSQScanfParserContextClass;

typedef guint (*LSQParseFunc)( gchar *, guint, parse_part *, LSQScanfParserContext *, LSQScanfParser * );

struct _parse_part
{
    parse_part *next;
    gchar *delimiter;
    LSQParseFunc function;
    guint index_;
    guint width;
};

struct _LSQScanfParserContext
{
    LSQParserContext parent;
    gchar *filename;
    union _data_store {
        gchar   c;
        gint    i;
        guint   u;
        glong   l;
        gulong  ul;
        gint64  ll;
        guint64 ull;
        gfloat  f;
        gdouble d;
        LSQDateTime dt;
    } *data_store;
    gpointer *props_store;
    parse_part *parse_loc;
    guint parse_line;
};

struct _LSQScanfParserContextClass
{
    LSQParserContextClass parent;
};

GType lsq_scanf_parser_context_get_type ( void );
#define LSQ_IS_SCANF_PARSER_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), lsq_scanf_parser_context_get_type()))

struct _LSQScanfParser
{
    LSQParser parent;

    parse_part *parser;
};

struct _LSQScanfParserClass
{
    LSQParserClass parent;
};

static void
lsq_scanf_parser_finalize ( GObject *object );
static void
parse_part_free ( parse_part *part );

G_DEFINE_TYPE ( LSQScanfParserContext, lsq_scanf_parser_context, LSQ_TYPE_PARSER_CONTEXT );

static void
lsq_scanf_parser_context_init ( LSQScanfParserContext *self )
{
}

static void
lsq_scanf_parser_context_class_init ( LSQScanfParserContextClass *klass )
{
}

static LSQParserContext *
lsq_scanf_parser_context_new ( LSQScanfParser *parser, LSQArchive *archive )
{
    LSQScanfParserContext *ctx;
    guint n_props;

    g_return_val_if_fail( LSQ_IS_SCANF_PARSER( parser ), NULL );
    g_return_val_if_fail( LSQ_IS_ARCHIVE( archive ), NULL );

    ctx = g_object_new( lsq_scanf_parser_context_get_type(), "archive", archive, NULL );
    n_props = lsq_parser_n_properties( LSQ_PARSER(parser) );
    ctx->data_store = g_new0( union _data_store, n_props );
    ctx->props_store = g_new0( gpointer, n_props + 1 );

    return LSQ_PARSER_CONTEXT(ctx);
}

static gboolean
build_parser ( LSQScanfParser *, const gchar * );

static void
lsq_scanf_parser_parse ( LSQScanfParser *, LSQScanfParserContext * );

G_DEFINE_TYPE ( LSQScanfParser, lsq_scanf_parser, LSQ_TYPE_PARSER );

static void
lsq_scanf_parser_init ( LSQScanfParser *self )
{
}

static void
lsq_scanf_parser_class_init ( LSQScanfParserClass *klass )
{
    LSQParserClass *parser_class = LSQ_PARSER_CLASS(klass);

    G_OBJECT_CLASS(klass)->finalize = lsq_scanf_parser_finalize;

    parser_class->get_context = (LSQParserContext *(*)(LSQParser *,LSQArchive *))lsq_scanf_parser_context_new;
    parser_class->parse = (void(*)(LSQParser *,LSQParserContext *))lsq_scanf_parser_parse;
}

static void
lsq_scanf_parser_finalize ( GObject *object )
{
    LSQScanfParser *parser = LSQ_SCANF_PARSER( object );
    parse_part *iter, *next;

    for ( iter = parser->parser; NULL != iter; iter = next )
    {
        next = iter->next;

        parse_part_free( iter );
    }
}

LSQParser *
lsq_scanf_parser_new ( const gchar *parser_string )
{
    LSQScanfParser *parser;

    g_return_val_if_fail( NULL != parser_string, NULL );

    parser = g_object_new( LSQ_TYPE_SCANF_PARSER, NULL );

    if ( FALSE == build_parser( parser, parser_string ) )
    {
        g_object_unref( parser );
        return NULL;
    }

    return LSQ_PARSER(parser);
}

/*{{{ skip functions*/
static guint
skip_byte (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
        LSQScanfParser *parser
    )
{
    if ( 1 > lng )
    {
        return 0;
    }
    
    return 1;
}

static guint
skip_word (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
        LSQScanfParser *parser
    )
{
    if ( 2 > lng )
    {
        return 0;
    }
    
    return 2;
}

static guint
skip_dword (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
        LSQScanfParser *parser
    )
{
    if ( 4 > lng )
    {
        return 0;
    }
    
    return 4;
}

static guint
skip_qword (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
        LSQScanfParser *parser
    )
{
    if ( 8 > lng )
    {
        return 0;
    }
    
    return 8;
}

static guint
skip_char (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
        LSQScanfParser *parser
    )
{
    const gchar *ptr;
    const gchar *delim;

    if ( 0 == lng )
    {
        return 0;
    }

    delim = part->delimiter;
    if ( ( NULL == delim ) && ( NULL == part->next ) )
    {
        delim = "\n";
    }

    if ( '\0' == delim[0] )
    {
        return 1;
    }

    //for(ptr = str; g_ascii_isspace(*ptr); ptr++);

    ptr = g_strstr_len( str + 1, lng - 1, delim );

    if ( NULL == ptr )
    {
        return 0;
    }
    
    return ptr - str;
}

static guint
skip_decimal (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
        LSQScanfParser *parser
    )
{
    gchar *ptr;
#ifdef DO_EXSTENSIVE_CHECKING
    gchar *ptr2;
#endif
    const gchar *delim;

    if ( 0 == lng )
    {
        return 0;
    }

    delim = part->delimiter;
    if ( ( NULL == delim ) && ( NULL == part->next ) )
    {
        delim = "\n";
    }

    if ( '\0' == delim[0] )
    {
        g_ascii_strtoll( str, &ptr, 10 );
        return ptr - str;
    }

    for ( ptr = str; g_ascii_isspace( *ptr ); ++ptr )
    {
        --lng;
        if ( 0 == lng )
        {
            return 0;
        }
    }

    ptr = g_strstr_len( ptr, lng, delim );

    if ( NULL == ptr )
    {
        return 0;
    }

#ifdef DO_EXSTENSIVE_CHECKING
    g_ascii_strtoll( str, &ptr2, 10 );
    if ( ptr > ptr2 )
    {
        return 0;
    }
#endif
    
    return ptr - str;
}

static guint
skip_floatingpoint (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
        LSQScanfParser *parser
    )
{
    gchar *ptr;
#ifdef DO_EXSTENSIVE_CHECKING
    gchar *ptr2;
#endif
    const gchar *delim;

    if ( 0 == lng )
    {
        return 0;
    }

    delim = part->delimiter;
    if ( ( NULL == delim ) && ( NULL == part->next ) )
    {
        delim = "\n";
    }

    if( '\0' == delim[0] )
    {
        g_ascii_strtod( str, &ptr );
        return ptr - str;
    }

    for ( ptr = str; g_ascii_isspace( *ptr ); ++ptr )
    {
        --lng;
        if ( 0 == lng )
        {
            return 0;
        }
    }

    ptr = g_strstr_len( ptr, lng, delim );

    if ( NULL == ptr )
    {
        return 0;
    }
#ifdef DO_EXSTENSIVE_CHECKING
    g_ascii_strtod( str, &ptr2 );
    if ( ptr > ptr2 )
    {
        return 0;
    }
#endif
    
    return ptr - str;
}

static guint
skip_octal (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx ,
        LSQScanfParser *parser
    )
{
    gchar *ptr;
#ifdef DO_EXSTENSIVE_CHECKING
    gchar *ptr2;
#endif
    const gchar *delim;

    if ( 0 == lng )
    {
        return 0;
    }

    delim = part->delimiter;
    if ( ( NULL == delim ) && ( NULL == part->next ) )
    {
        delim = "\n";
    }

    if( '\0' == delim[0] )
    {
        g_ascii_strtoll( str, &ptr, 010 );
        return ptr - str;
    }

    for ( ptr = str; g_ascii_isspace( *ptr ); ++ptr )
    {
        --lng;
        if ( 0 == lng )
        {
            return 0;
        }
    }

    ptr = g_strstr_len( ptr, lng, delim );

    if ( NULL == ptr )
    {
        return 0;
    }

#ifdef DO_EXSTENSIVE_CHECKING
    g_ascii_strtoll( str, &ptr2, 010 );
    if ( ptr > ptr2 )
    {
        return 0;
    }
#endif
    
    return ptr - str;
}

static guint
skip_string (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
        LSQScanfParser *parser
    )
{
    gchar *ptr;
    const gchar *delim;

    if ( 0 == lng )
    {
        return 0;
    }

    delim = part->delimiter;
    if ( ( NULL == delim ) && ( NULL == part->next ) )
    {
        delim = "\n";
    }

    if ( '\0' == delim[0] )
    {
        return 0;
    }

    for ( ptr = str; g_ascii_isspace( *ptr ); ++ptr )
    {
        --lng;
        if ( 0 == lng )
        {
            return 0;
        }
    }

    ptr = g_strstr_len( ptr, lng, delim );

    if ( NULL == ptr )
    {
        return 0;
    }
    
    return ptr - str;
}

static guint
skip_datetime (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
        LSQScanfParser *parser
    )
{
    gchar *ptr;
    gchar *cur;
    const gchar *delim;

    if ( 0 == lng )
    {
        return 0;
    }

    delim = part->delimiter;
    if ( ( NULL == delim ) && ( NULL == part->next ) )
    {
        delim = "\n";
    }

    for ( cur = str; g_ascii_isspace( *cur ); ++cur )
    {
    }

    lsq_datetime_from_string( cur, LSQ_PARSER(parser)->datetime_format, &ptr );
    if ( NULL == ptr )
    {
        return 0;
    }

    if ( ( ptr - str ) > lng )
    {
        return 0;
    }

    if ( '\0' == delim[0] )
    {
        lng -= ptr - str;
        ptr = g_strstr_len( ptr, lng, delim );
        if ( NULL == ptr )
        {
            return 0;
        }
    }

    return ptr - str;
}

static guint
skip_unsigned (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
        LSQScanfParser *parser
    )
{
    gchar *ptr;
#ifdef DO_EXSTENSIVE_CHECKING
    gchar *ptr2;
#endif
    const gchar *delim;

    if ( 0 == lng )
    {
        return 0;
    }

    delim = part->delimiter;
    if ( ( NULL == delim ) && ( NULL == part->next ) )
    {
        delim = "\n";
    }

    if ( '\0' == delim[0] )
    {
        g_ascii_strtoull( str, &ptr, 10);
        return ptr - str;
    }

    for ( ptr = str; g_ascii_isspace( *ptr ); ++ptr )
    {
        --lng;
        if ( 0 == lng )
        {
            return 0;
        }
    }

    ptr = g_strstr_len( ptr, lng, delim );

    if ( NULL == ptr )
    {
        return 0;
    }
#ifdef DO_EXSTENSIVE_CHECKING

    g_ascii_strtoull( str, &ptr2, 10 );

    if ( ptr > ptr2)
    {
        return 0;
    }
#endif
    
    return ptr - str;
}

static guint
skip_hexadecimal (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
        LSQScanfParser *parser
    )
{
    gchar *ptr;
#ifdef DO_EXSTENSIVE_CHECKING
    gchar *ptr2;
#endif
    const gchar *delim;

    if ( 0 == lng )
    {
        return 0;
    }

    delim = part->delimiter;
    if ( ( NULL == delim ) && ( NULL == part->next ) )
    {
        delim = "\n";
    }

    if ( '\0' == delim[0] )
    {
        g_ascii_strtoll( str, &ptr, 0x10 );
        return ptr - str;
    }

    for ( ptr = str; g_ascii_isspace( *ptr ); ++ptr )
    {
        --lng;
        if ( 0 == lng )
        {
            return 0;
        }
    }

    ptr = g_strstr_len( ptr, lng, delim );

    if ( NULL == ptr )
    {
        return 0;
    }
#ifdef DO_EXSTENSIVE_CHECKING
    g_ascii_strtoll( str, &ptr2, 0x10 );
    if ( ptr > ptr2 )
    {
        return 0;
    }
#endif
    
    return ptr - str;
}
/*}}}*/

/*{{{ parse functions*/
#define DEF_PARSE_BIN(func, bytes, type, ptype, pname)    \
static guint parse_##func(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx, LSQScanfParser *parser) { \
    type    val;    \
    ptype *pval;    \
    if(bytes > lng) return 0; \
    val = *((type*)str);    \
    pval = &ctx->data_store[part->index_].pname; \
    *pval = val;    \
    ctx->props_store[part->index_] = pval; \
    return bytes; \
}

#define DEF_PARSE_NUM(func, base, type, ptype, pname) \
static guint parse_##func(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx, LSQScanfParser *parser) {    \
    gchar *ptr;     \
    gchar *ptr2;    \
    type    val;    \
    ptype *pval;    \
    const gchar *delim; \
    if(0 == lng) return 0;   \
    delim = part->delimiter; \
    if(NULL == delim && NULL == part->next) delim = "\n"; \
    pval = &ctx->data_store[part->index_].pname; \
    ctx->props_store[part->index_] = pval; \
    if('\0' == delim[0]) { \
        val = g_ascii_strtoll(str, &ptr, base); \
        *pval = val;    \
        return ptr - str; \
    } \
    for(ptr = str; g_ascii_isspace(*ptr); ++ptr) --lng; \
    ptr = g_strstr_len(ptr, lng, delim);    \
    if(NULL == ptr) return 0;   \
    val = g_ascii_strtoll(str, &ptr2, base);    \
    *pval = val;    \
    if(ptr > ptr2) return 0;    \
    return ptr - str; \
}

#define DEF_PARSE_FLOAT(func, type, pname)    \
static guint parse_##func(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx, LSQScanfParser *parser) {    \
    gchar *ptr;     \
    gchar *ptr2;    \
    type    val;    \
    type  *pval;    \
    const gchar *delim; \
    if(0 == lng) return 0;   \
    delim = part->delimiter; \
    if(NULL == delim && NULL == part->next) delim = "\n"; \
    pval = &ctx->data_store[part->index_].pname; \
    ctx->props_store[part->index_] = pval; \
    if('\0' == delim[0]) { \
        val = g_ascii_strtod(str, &ptr);    \
        *pval = val;    \
        return ptr - str; \
    } \
    for(ptr = str; g_ascii_isspace(*ptr); ++ptr) --lng; \
    ptr = g_strstr_len(ptr, lng, delim);    \
    if(NULL == ptr) return 0;   \
    val = g_ascii_strtod(str, &ptr2); \
    *pval = val;    \
    if(ptr > ptr2) return 0;    \
    return ptr - str; \
}

#define DEF_PARSE_UNS(func, base, type, ptype, pname) \
static guint parse_##func(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx, LSQScanfParser *parser) {    \
    gchar *ptr;     \
    gchar *ptr2;    \
    type    val;    \
    ptype *pval;    \
    const gchar *delim; \
    if(0 == lng) return 0;   \
    delim = part->delimiter; \
    if(NULL == delim && NULL == part->next) delim = "\n"; \
    pval = &ctx->data_store[part->index_].pname; \
    ctx->props_store[part->index_] = pval; \
    if('\0' == delim[0]) { \
        val = g_ascii_strtoull(str, &ptr, base); \
        *pval = val;    \
        return ptr - str; \
    } \
    for(ptr = str; g_ascii_isspace(*ptr); ++ptr) --lng; \
    ptr = g_strstr_len(ptr, lng, delim);    \
    if(NULL == ptr) return 0;   \
    val = g_ascii_strtoull(str, &ptr2, base);    \
    *pval = val;    \
    if(ptr > ptr2) return 0;    \
    return ptr - str; \
}

DEF_PARSE_BIN ( byte, 1, gchar, guint, u )
DEF_PARSE_BIN ( word, 2, gushort, guint, u )
DEF_PARSE_BIN ( dword, 4, gulong, gulong, ul )
DEF_PARSE_BIN ( qword, 8, guint64, guint64, ull )

static guint
parse_char (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
	LSQScanfParser *parser
    )
{
    const gchar *ptr;
    gchar val;
    gchar *pval;
    const gchar *delim;

    if ( 0 == lng )
    {
        return 0;
    }

    delim = part->delimiter;
    if( NULL == delim && NULL == part->next )
    {
        delim = "\n";
    }

    pval = &ctx->data_store[part->index_].c;
    ctx->props_store[part->index_] = pval;

    if ( '\0' == delim[0] )
    {
        val = *str;
        *pval = val;
        return 1;
    }

    //for(ptr = str; g_ascii_isspace(*ptr); ptr++);

    ptr = g_strstr_len( str + 1, lng - 1, delim );

    if ( NULL == ptr )
    {
        return 0;
    }

    //FIXME?
    val = *(ptr-1);
    *pval = val;
    
    return ptr - str;
}

DEF_PARSE_NUM ( decimal, 10, gint, gint, i )
DEF_PARSE_NUM ( decimal16, 10, gshort, gint, i )
DEF_PARSE_NUM ( decimal32, 10, glong, glong, l )
DEF_PARSE_NUM ( decimal64, 10, gint64, gint64, ll )

DEF_PARSE_FLOAT ( floatingpoint, gfloat, f )
DEF_PARSE_FLOAT ( double, gdouble, d )

DEF_PARSE_UNS ( octal, 010, guint, guint, u )
DEF_PARSE_UNS ( octal16, 010, gushort, guint, u )
DEF_PARSE_UNS ( octal32, 010, gulong, gulong, ul )
DEF_PARSE_UNS ( octal64, 010, guint64, guint64, ull )

static guint
parse_string (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
	LSQScanfParser *parser
    )
{
    gchar *ptr;
    gchar *cur;
    const gchar *delim;

    if ( 0 == lng )
    {
        return 0;
    }

    delim = part->delimiter;
    if ( NULL == delim && NULL == part->next )
        delim = "\n";

    if ( '\0' == delim[0] )
        return 0;

    for ( cur = str; g_ascii_isspace( *cur ); ++cur )
    {
        --lng;
        if ( 0 == lng )
        {
            return 0;
        }
    }

    ptr = g_strstr_len( cur, lng, delim );

    if ( NULL == ptr )
        return 0;

    ctx->props_store[part->index_] = g_strndup( str, ptr - str );

    return ptr - str;
}

static guint
parse_datetime (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
        LSQScanfParser *parser
    )
{
    gchar *ptr;
    gchar *cur;
    const gchar *delim;
    LSQDateTime *pval;

    if ( 0 == lng )
    {
        return 0;
    }

    delim = part->delimiter;
    if ( ( NULL == delim ) && ( NULL == part->next ) )
    {
        delim = "\n";
    }

    for ( cur = str; g_ascii_isspace( *cur ); ++cur)
    {
    }

    pval = &ctx->data_store[part->index_].dt;
    ctx->props_store[part->index_] = pval;

    *pval = lsq_datetime_from_string( cur, LSQ_PARSER(parser)->datetime_format, &ptr );
    if ( LSQ_DATETIME_NULL == *pval )
    {
        return 0;
    }

    if ( ( ptr - str ) > lng )
    {
        return 0;
    }

    if ( '\0' == delim[0] )
    {
        lng -= ptr - str;
        ptr = g_strstr_len ( ptr, lng, delim );
        if ( NULL == ptr )
        {
            return 0;
        }
    }

    return ptr - str;
}

DEF_PARSE_UNS(unsigned, 10, guint, guint, u)
DEF_PARSE_UNS(unsigned16, 10, gushort, guint, u)
DEF_PARSE_UNS(unsigned32, 10, gulong, gulong, ul)
DEF_PARSE_UNS(unsigned64, 10, guint64, guint64, ull)

DEF_PARSE_UNS(hexadecimal, 0x10, guint, guint, u)
DEF_PARSE_UNS(hexadecimal16, 0x10, gushort, guint, u)
DEF_PARSE_UNS(hexadecimal32, 0x10, gulong, gulong, ul)
DEF_PARSE_UNS(hexadecimal64, 0x10, guint64, guint64, ull)

static guint
parse_filename (
        gchar *str,
        guint lng,
        parse_part *part,
        LSQScanfParserContext *ctx,
	LSQScanfParser *parser )
{
    gchar *ptr;
    const gchar *delim;

    if ( 0 == lng )
    {
        return 0;
    }

    delim = part->delimiter;

    if ( NULL == delim &&  NULL == part->next )
    {
        delim = "\n";
    }

    if ( '\0' == delim[0] )
    {
        return 0;
    }

    ptr = g_strstr_len(str, lng, delim);

    if ( NULL == ptr )
    {
        return 0;
    }

    ctx->filename = g_strndup(str, ptr-str);

    return ptr - str;
}
/*}}}*/

static gchar *
strdup_escaped (/*{{{*/
        const gchar *str,
        guint lng )
{
    guint i;
    gchar *new_str;
    gchar ch;
    guint size = 0;

    for ( i = 0; i < lng; ++i )
    {
        switch ( str[i] )
        {
            case '%':
                ++i;
            break;
            case '\\':
                ++i;
                switch ( str[i] )
                {
                    case 'x':
                        if ( g_ascii_isxdigit( str[i+1] ) )
                        {
                            ++i;
                            if ( g_ascii_isxdigit( str[i+1] ) )
                            {
                                ++i;
                            }
                        }
                    break;
                    default:
                        ch = str[i+1];
                        if ( '0' <= ch && '8' > ch )
                        {
                            ++i;
                            ch = str[i+1];
                            if ( '0' <= ch && '8' > ch )
                            {
                                ch = str[i];
                                ++i;
                                if ( '4' > ch )
                                {
                                    ch = str[i+1];
                                    if ( '0' <= str[i+1] && '8' > str[i+1] )
                                    {
                                        ++i;
                                    }
                                }
                            }
                        }
                    break;
                }
            break;
        }
        ++size;
    }

    new_str = g_new( gchar, size+1 );
    new_str[size] = '\0';

    size = 0;
    for( i = 0; i < lng; ++i )
    { 
        ch = str[i];
        switch ( ch )
        {
            case '%':
                ++i;
                ch = str[i];
            break;
            case '\\':
                ++i;
                ch = str[i];
                switch ( ch )
                {
                    case 'a':
                        ch = '\a';
                    break;
                    case 'b':
                        ch = '\b';
                    break;
                    case 'f':
                        ch = '\f';
                    break;
                    case 'n':
                        ch = '\n';
                    break;
                    case 'r':
                        ch = '\r';
                    break;
                    case 't':
                        ch = '\t';
                    break;
                    case 'v':
                        ch = '\v';
                    break;
                    case 'x':
                        if ( g_ascii_isxdigit( str[i+1] ) )
                        {
                            ++i;
                            ch = g_ascii_xdigit_value( str[i] );
                            if ( g_ascii_isxdigit( str[i+1] ) )
                            {
                                ++i;
                                ch = ( ch * 0x10 ) + g_ascii_xdigit_value( str[i] );
                            }
                        }
                    break;
                    default:
                        if ( '0' <= str[i+1] && '8' > str[i+1] )
                        {
                            ++i;
                            ch = str[i]-'0';
                            if ( '0' <= str[i+1] && '8' > str[i+1] )
                            {
                                ++i;
                                ch = ( ch * 010 ) + (str[i]-'0');
                                if ( 040 > ch )
                                {
                                    if ( '0' <= str[i+1] && '8' > str[i+1] )
                                    {
                                        ++i;
                                        ch = ( ch * 010 ) + ( str[i] - '0' );
                                    }
                                }
                            }
                        }
                    break;
                }
            break;
        }
        new_str[size++] = ch;
    }

    return new_str;
}/*}}}*/

static gboolean
build_parser (
        LSQScanfParser *parser,
        const gchar *parser_string
    )
{
    const gchar *ptr;
    const gchar *cur;
    gchar *pos;
    gchar ch;
    enum {
        SIZE_NORMAL = FALSE,
        SIZE_SHORT,
        SIZE_LONG,
        SIZE_LONGLONG
    } size_flag;
    gboolean skip_flag;
    guint width_flag;
    guint index_flag;
    guint index_;
    parse_part *part;
    parse_part *parts;
    guint part_count = 0;

    g_return_val_if_fail( LSQ_IS_SCANF_PARSER( parser ), FALSE );
    g_return_val_if_fail( NULL !=  parser_string, FALSE );

    part = g_new0( parse_part, 1 );
    parts = part;

    cur = ptr = parser_string;

    while( '\0' != (ch = *ptr++) )
    {
        if ( '\\' == ch && 'n' == *ptr )
        {
            part->delimiter = strdup_escaped( cur, ptr - cur );
            part->next = g_new0( parse_part, 1 );
            part = part->next;
            cur = ++ptr;
            continue;
        }
        if ( '%' == ch )
        {
            skip_flag = FALSE;
            size_flag = SIZE_NORMAL;
            width_flag = 0;
            index_ = part_count;

            ch = *ptr++;
            if ( '\0' == ch )
            {
                break;
            }

            if ( '%' == ch )
            {
                continue;
            }

            part->delimiter = strdup_escaped( cur, ptr - cur - 2 );

            /*{{{ check differend index_ %.$*/
            if ( g_ascii_isdigit( ch ) )
            {
                index_flag = g_ascii_strtoull( ptr - 1, &pos, 10 );
                if ( '$' == *pos )
                {
                    ptr = pos+1;
                    index_ = index_flag - 1;
                    ch = *ptr++;
                    if ( '\0' == ch )
                    {
                        break;
                    }
                }
            }
            /*}}}*/

            /*{{{ check skip flag %*.*/
            if ( '*' == ch )
            {
                skip_flag = TRUE;
                ch = *ptr++;
                if ( '\0' == ch )
                {
                    break;
                }
            }
            /*}}}*/

            /*{{{ check max characters %.*/
            //ignored for now
            if ( g_ascii_isdigit( ch ) )
            {
                width_flag = g_ascii_strtoull( ptr - 1, (gchar **)&ptr, 10 );
                ch = *ptr++;
                if ( '\0' == ch )
                {
                    break;
                }
            }
            /*}}}*/

            /*{{{ check size flag %[hlL].*/
            switch ( ch )
            {
                case 'h':
                    size_flag = SIZE_SHORT;
                    ch = *ptr++;
                break;
                case 'l':
                    size_flag = SIZE_LONG;
                    ch = *ptr++;
                    if ( 'l' != ch )
                    {
                        break;
                    }
                case 'L':
                    size_flag = SIZE_LONGLONG;
                    ch = *ptr++;
                break;
            }
            if ( '\0' == ch )
            {
                break;
            }
            /*}}}*/

            switch ( ch )
            {
                /*{{{ byte %b*/
                case 'b':
                    if ( 0 != width_flag )
                    {
                        g_warning( "Unexpected %s flag near character %"G_GSIZE_FORMAT" in \'%s\'", "width", (gsize)(ptr - parser_string - 1), parser_string );
                        return FALSE;
                    }
                    part->next = g_new0( parse_part, 1 );
                    part = part->next;
                    if ( TRUE == skip_flag )
                    {
                        switch ( size_flag )
                        {
                            case SIZE_NORMAL:
                                part->function = skip_byte;
                            break;
                            case SIZE_SHORT:
                                part->function = skip_word;
                            break;
                            case SIZE_LONG:
                                part->function = skip_dword;
                            break;
                            case SIZE_LONGLONG:
                                part->function = skip_qword;
                            break;
                        }
                    }
                    else
                    {
                        ++part_count;
                        part->index_ = index_;
                        switch ( size_flag )
                        {
                            case SIZE_NORMAL:
                                part->function = parse_byte;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_UINT );
                            break;
                            case SIZE_SHORT:
                                part->function = parse_word;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_UINT );
                            break;
                            case SIZE_LONG:
                                part->function = parse_dword;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_ULONG );
                            break;
                            case SIZE_LONGLONG:
                                part->function = parse_qword;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_UINT64 );
                            break;
                        }
                    }
                break;
                /*}}}*/
                /*{{{ character %c*/
                case 'c':
                    if ( SIZE_NORMAL != size_flag || 0 != width_flag )
                    {
                        g_warning( "Unexpected %s flag near character %"G_GSIZE_FORMAT" in \'%s\'", ( 0 != width_flag ) ? "width" : "size",  (gsize)(ptr - parser_string - 1), parser_string );
                        return FALSE;
                    }
                    part->next = g_new0( parse_part, 1 );
                    part = part->next;
                    if ( TRUE == skip_flag )
                    {
                        part->function = skip_char;
                    }
                    else
                    {
                        ++part_count;
                        part->index_ = index_;
                        part->function = parse_char;
                        lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_CHAR );
                    }
                break;
                /*}}}*/
                /*{{{ decimal %d*/
                case 'd':
                case 'i':
                    part->next = g_new0( parse_part, 1 );
                    part = part->next;
                    part->width = width_flag;
                    if ( TRUE == skip_flag )
                    {
                        part->function = skip_decimal;
                    }
                    else
                    {
                        ++part_count;
                        part->index_ = index_;
                        switch ( size_flag )
                        {
                            case SIZE_NORMAL:
                                part->function = parse_decimal;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_INT );
                            break;
                            case SIZE_SHORT:
                                part->function = parse_decimal16;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_INT );
                            break;
                            case SIZE_LONG:
                                part->function = parse_decimal32;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_LONG );
                            break;
                            case SIZE_LONGLONG:
                                part->function = parse_decimal64;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_INT64 );
                            break;
                        }
                    }
                break;
                /*}}}*/
                /*{{{ floating point %f*/
                case 'f':
                    if ( SIZE_NORMAL != size_flag && SIZE_LONGLONG != size_flag )
                    {
                        g_warning( "Unexpected %s flag near character %"G_GSIZE_FORMAT" in \'%s\'", "size",  (gsize)(ptr - parser_string - 1), parser_string );
                        return FALSE;
                    }
                    part->next = g_new0( parse_part, 1 );
                    part = part->next;
                    part->width = width_flag;
                    if ( TRUE == skip_flag )
                    {
                        part->function = skip_floatingpoint;
                    }
                    else
                    {
                        ++part_count;
                        part->index_ = index_;
                        switch(size_flag)
                        {
                            case SIZE_NORMAL:
                                part->function = parse_floatingpoint;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_FLOAT );
                            break;
                            case SIZE_LONGLONG:
                                part->function = parse_double;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_DOUBLE );
                            break;
                            default:
                            break;
                        }
                    }
                break;
                /*}}}*/
                /*{{{ octal %o*/
                case 'o':
                    part->next = g_new0( parse_part, 1 );
                    part = part->next;
                    part->width = width_flag;
                    if ( TRUE == skip_flag )
                    {
                        part->function = skip_octal;
                    }
                    else
                    {
                        ++part_count;
                        part->index_ = index_;
                        switch ( size_flag )
                        {
                            case SIZE_NORMAL:
                                part->function = parse_octal;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_UINT );
                            break;
                            case SIZE_SHORT:
                                part->function = parse_octal16;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_UINT );
                            break;
                            case SIZE_LONG:
                                part->function = parse_octal32;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_ULONG );
                            break;
                            case SIZE_LONGLONG:
                                part->function = parse_octal64;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_UINT64 );
                            break;
                        }
                    }
                break;
                /*}}}*/
                /*{{{ string %s*/
                case 's':
                    if ( SIZE_NORMAL != size_flag )
                    {
                        g_warning( "Unexpected %s flag near character %"G_GSIZE_FORMAT" in \'%s\'", "size",  (gsize)(ptr - parser_string - 1), parser_string );
                        return FALSE;
                    }
                    part->next = g_new0( parse_part, 1 );
                    part = part->next;
                    part->width = width_flag;
                    if ( TRUE == skip_flag )
                    {
                        part->function = skip_string;
                    }
                    else
                    {
                        ++part_count;
                        part->index_ = index_;
                        part->function = parse_string;
                        lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_STRING );
                    }
                break;
                /*}}}*/
                /*{{{ datetime %t*/
                case 't':
                    if ( SIZE_NORMAL != size_flag )
                    {
                        g_warning( "Unexpected %s flag near character %"G_GSIZE_FORMAT" in \'%s\'", "size",  (gsize)(ptr - parser_string - 1), parser_string );
                        return FALSE;
                    }
                    part->next = g_new0( parse_part, 1 );
                    part = part->next;
                    part->width = width_flag;
                    if ( TRUE == skip_flag )
                    {
                        part->function = skip_datetime;
                    }
                    else
                    {
                        part_count++;
                        part->index_ = index_;
                        part->function = parse_datetime;
                        lsq_parser_set_property_type( LSQ_PARSER(parser), index_, LSQ_TYPE_DATETIME );
                    }
                break;
                /*}}}*/
                /*{{{ unsigned decimal %u*/
                case 'u':
                    part->next = g_new0( parse_part, 1 );
                    part = part->next;
                    part->width = width_flag;
                    if ( TRUE == skip_flag )
                    {
                        part->function = skip_unsigned;
                    }
                    else
                    {
                        ++part_count;
                        part->index_ = index_;
                        switch ( size_flag )
                        {
                            case SIZE_NORMAL:
                                part->function = parse_unsigned;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_UINT );
                            break;
                            case SIZE_SHORT:
                                part->function = parse_unsigned16;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_UINT );
                            break;
                            case SIZE_LONG:
                                part->function = parse_unsigned32;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_ULONG );
                            break;
                            case SIZE_LONGLONG:
                                part->function = parse_unsigned64;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_UINT64 );
                            break;
                        }
                    }
                break;
                /*}}}*/
                /*{{{ hexadecimal %x %X*/
                case 'x':
                case 'X':
                    part->next = g_new0( parse_part, 1 );
                    part = part->next;
                    part->width = width_flag;
                    if ( TRUE == skip_flag )
                    {
                        part->function = skip_hexadecimal;
                    }
                    else
                    {
                        ++part_count;
                        part->index_ = index_;
                        switch ( size_flag )
                        {
                            case SIZE_NORMAL:
                                part->function = parse_hexadecimal;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_UINT );
                            break;
                            case SIZE_SHORT:
                                part->function = parse_hexadecimal16;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_UINT );
                            break;
                            case SIZE_LONG:
                                part->function = parse_hexadecimal32;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_ULONG );
                            break;
                            case SIZE_LONGLONG:
                                part->function = parse_hexadecimal64;
                                lsq_parser_set_property_type( LSQ_PARSER(parser), index_, G_TYPE_UINT64 );
                            break;
                        }
                    }
                break;
                /*}}}*/
                /*{{{ filename %F*/
                case 'F':
                    if ( TRUE == skip_flag || SIZE_NORMAL != size_flag || 0 != width_flag)
                    {
                        g_warning( "Unexpected %s flag near character %"G_GSIZE_FORMAT" in \'%s\'", ( FALSE == skip_flag ) ? ( 0 != width_flag ) ? "width" : "size" : "skip",  (gsize)(ptr - parser_string - 1), parser_string );
                        return FALSE;
                    }
                    part->next = g_new0( parse_part, 1 );
                    part = part->next;
                    part->function = parse_filename;
                break;
                /*}}}*/
                default:
                    g_warning( "Unexpected type near character %"G_GSIZE_FORMAT" in \'%s\'", (gsize)(ptr - parser_string - 1), parser_string );
                    return FALSE;
            }
            cur = ptr;
        }
    }

    parser->parser = parts;

    return TRUE;
}

static void
parse_part_free ( parse_part *part )
{
    g_return_if_fail( NULL != part );

    g_free( part->delimiter );

    g_free( part );
}

static void
lsq_scanf_parser_parse (
        LSQScanfParser *parser,
        LSQScanfParserContext *ctx
    )
{
    gchar *line, *ptr;
    gsize line_length, lng;
    guint size, line_nr;
    parse_part *part;

#ifdef DEBUG
    g_return_if_fail( LSQ_IS_SCANF_PARSER( parser ) );
    g_return_if_fail( LSQ_IS_SCANF_PARSER_CONTEXT( ctx ) );
#endif

    if ( FALSE == lsq_parser_context_get_line( LSQ_PARSER_CONTEXT(ctx), &line, &line_length ) )
    {
        return;
    }

    ptr = line;
    lng = line_length;
    line_nr = ctx->parse_line;
    part = ctx->parse_loc;

    if ( NULL == part )
    {
        ctx->parse_loc = part = parser->parser;
        ctx->parse_line = line_nr = 0;
    }

    while ( NULL != part )
    {
        //parse
        if ( NULL != part->function )
        {
            size = part->function( ptr, lng, part, ctx, parser );
            if ( 0 == size )
            {
                //no match
                if ( 0 != line_nr )
                {
                    ptr = line;
                    lng = line_length;
                    part = parser->parser;
                    line_nr = 0;
                    continue;
                }
                ctx->parse_loc = parser->parser;
                ctx->parse_line = 0;
                g_free( line );
                return;
            }
            ptr += size;
            lng -= size;
        }

        if ( NULL != part->delimiter )
        {
            if ( FALSE == g_str_has_prefix( ptr, part->delimiter ) )
            {
                //no match
                if ( 0 != line_nr )
                {
                    ptr = line;
                    lng = line_length;
                    part = parser->parser;
                    line_nr = 0;
                    continue;
                }
                ctx->parse_loc = parser->parser;
                ctx->parse_line = 0;
                g_free( line );
                return;
            }

            size = strlen(part->delimiter);
            if ( size > lng )
            {
                size = lng;
            }
            ptr += size;
            lng -= size;
        }

        if ( NULL != part->delimiter )
        {
            if ( TRUE == g_str_has_suffix( part->delimiter, "\n" ) )
            {
                //next line
                if ( NULL != part->next )
                {
                    ctx->parse_loc = part->next;
                    ctx->parse_line = line_nr + 1;
                    g_free( line );
                    return;
                }
            }
        }
        
        part = part->next;
    }

    g_free( line );

    if ( NULL == part )
    {
        LSQArchiveIter *iter = lsq_archive_add_file( LSQ_PARSER_CONTEXT(ctx)->archive, ctx->filename );
        lsq_archive_iter_set_propsv( iter, ctx->props_store );
        lsq_archive_iter_unref( iter );

        ctx->parse_loc = parser->parser;
        ctx->parse_line = 0;
    }
}

