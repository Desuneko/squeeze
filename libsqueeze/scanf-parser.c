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

typedef guint (*LSQParseFunc)(gchar*, guint, parse_part*, LSQScanfParserContext*);

struct _parse_part
{
  parse_part *next;
  gchar *delimiter;
  LSQParseFunc function;
  guint index;
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
  } *data_store;
  gpointer *props_store;
  parse_part *parse_loc;
  guint parse_line;
};

struct _LSQScanfParserContextClass
{
  LSQParserContextClass parent;
};

struct _LSQScanfParser
{
  LSQParser parent;

  parse_part *parser;
};

struct _LSQScanfParserClass
{
  LSQParserClass parent;
};

G_DEFINE_TYPE(LSQScanfParserContext, lsq_scanf_parser_context, LSQ_TYPE_PARSER_CONTEXT);

static void
lsq_scanf_parser_context_init(LSQScanfParserContext *self)
{
}

static void
lsq_scanf_parser_context_class_init(LSQScanfParserContextClass *klass)
{
}

static LSQParserContext *lsq_scanf_parser_context_new(LSQScanfParser *parser, LSQArchive *archive)
{
  LSQScanfParserContext *ctx;

	ctx = g_object_new(lsq_scanf_parser_context_get_type(), "archive", archive, NULL);
  guint n_props = lsq_parser_n_properties(LSQ_PARSER(parser));
  ctx->data_store = g_new0(union _data_store, n_props);
  ctx->props_store = g_new0(gpointer, n_props+1);

  return LSQ_PARSER_CONTEXT(ctx);
}

static void build_parser(LSQScanfParser *, const gchar *);

static void lsq_scanf_parser_parse(LSQScanfParser *, LSQScanfParserContext *);

G_DEFINE_TYPE(LSQScanfParser, lsq_scanf_parser, LSQ_TYPE_PARSER);

static void
lsq_scanf_parser_init(LSQScanfParser *self)
{
}

static void
lsq_scanf_parser_class_init(LSQScanfParserClass *klass)
{
  LSQParserClass *parser_class = LSQ_PARSER_CLASS(klass);
  parser_class->get_context = (LSQParserContext*(*)(LSQParser*,LSQArchive*))lsq_scanf_parser_context_new;
  parser_class->parse = (void(*)(LSQParser*,LSQParserContext*))lsq_scanf_parser_parse;
}

LSQParser *lsq_scanf_parser_new(const gchar *parser_string)
{
  LSQScanfParser *parser;

  parser = g_object_new(LSQ_TYPE_SCANF_PARSER, NULL);

  build_parser(parser, parser_string);

  return LSQ_PARSER(parser);
}

/*{{{ skip functions*/
guint skip_byte(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  if(lng < 1)
    return 0;
  
  return 1;
}

guint skip_word(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  if(lng < 2)
    return 0;
  
  return 2;
}

guint skip_dword(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  if(lng < 4)
    return 0;
  
  return 4;
}

guint skip_qword(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  if(lng < 8)
    return 0;
  
  return 8;
}

guint skip_char(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  const gchar *ptr;
  const gchar *delim;

  if(!lng)
    return 0;

  delim = part->delimiter;
  if(!delim && !part->next)
    delim = "\n";

  if(!delim)
    return 1;

  //for(ptr = str; g_ascii_isspace(*ptr); ptr++);

  ptr = g_strstr_len(str, lng, delim);

  if(!ptr)
    return 0;
  
  return ptr - str;
}

guint skip_decimal(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  gchar *ptr;
#ifdef DO_EXSTENSIVE_CHECKING
  gchar *ptr2;
#endif
  const gchar *delim;

  if(!lng)
    return 0;

  delim = part->delimiter;
  if(!delim && !part->next)
    delim = "\n";

  if(!delim[0])
  {
    g_ascii_strtoll(str, &ptr, 10);
    return ptr - str;
  }

  for(ptr = str; g_ascii_isspace(*ptr); ptr++);

  ptr = g_strstr_len(ptr, lng, delim);

  if(!ptr)
    return 0;
#ifdef DO_EXSTENSIVE_CHECKING
  g_ascii_strtoll(str, &ptr2, 10);
  if(ptr > ptr2)
    return 0;
#endif
  
  return ptr - str;
}

guint skip_floatingpoint(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  gchar *ptr;
#ifdef DO_EXSTENSIVE_CHECKING
  gchar *ptr2;
#endif
  const gchar *delim;

  if(!lng)
    return 0;

  delim = part->delimiter;
  if(!delim && !part->next)
    delim = "\n";

  if(!delim[0])
  {
    g_ascii_strtod(str, &ptr);
    return ptr - str;
  }

  for(ptr = str; g_ascii_isspace(*ptr); ptr++);

  ptr = g_strstr_len(ptr, lng, delim);

  if(!ptr)
    return 0;
#ifdef DO_EXSTENSIVE_CHECKING
  g_ascii_strtod(str, &ptr2);
  if(ptr > ptr2)
    return 0;
#endif
  
  return ptr - str;
}

guint skip_octal(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  gchar *ptr;
#ifdef DO_EXSTENSIVE_CHECKING
  gchar *ptr2;
#endif
  const gchar *delim;

  if(!lng)
    return 0;

  delim = part->delimiter;
  if(!delim && !part->next)
    delim = "\n";

  if(!delim[0])
  {
    g_ascii_strtoll(str, &ptr, 010);
    return ptr - str;
  }

  for(ptr = str; g_ascii_isspace(*ptr); ptr++);

  ptr = g_strstr_len(ptr, lng, delim);

  if(!ptr)
    return 0;
#ifdef DO_EXSTENSIVE_CHECKING
  g_ascii_strtoll(str, &ptr2, 010);
  if(ptr > ptr2)
    return 0;
#endif
  
  return ptr - str;
}

guint skip_string(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  gchar *ptr;
  const gchar *delim;

  if(!lng)
    return 0;

  delim = part->delimiter;
  if(!delim && !part->next)
    delim = "\n";

  if(!delim[0])
    return 0;

  for(ptr = str; g_ascii_isspace(*ptr); ptr++);

  ptr = g_strstr_len(ptr, lng, delim);

  if(!ptr)
    return 0;
  
  return ptr - str;
}

guint skip_unsigned(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  gchar *ptr;
#ifdef DO_EXSTENSIVE_CHECKING
  gchar *ptr2;
#endif
  const gchar *delim;

  if(!lng)
    return 0;

  delim = part->delimiter;
  if(!delim && !part->next)
    delim = "\n";

  if(!delim[0])
  {
    g_ascii_strtoull(str, &ptr, 10);
    return ptr - str;
  }

  ptr = g_strstr_len(str, lng, delim);

  if(!ptr)
    return 0;
#ifdef DO_EXSTENSIVE_CHECKING
  g_ascii_strtoull(str, &ptr2, 10);
  if(ptr > ptr2)
    return 0;
#endif
  
  return ptr - str;
}

guint skip_hexadecimal(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  gchar *ptr;
#ifdef DO_EXSTENSIVE_CHECKING
  gchar *ptr2;
#endif
  const gchar *delim;

  if(!lng)
    return 0;

  delim = part->delimiter;
  if(!delim && !part->next)
    delim = "\n";

  if(!delim[0])
  {
    g_ascii_strtoll(str, &ptr, 0x10);
    return ptr - str;
  }

  ptr = g_strstr_len(str, lng, delim);

  if(!ptr)
    return 0;
#ifdef DO_EXSTENSIVE_CHECKING
  g_ascii_strtoll(str, &ptr2, 0x10);
  if(ptr > ptr2)
    return 0;
#endif
  
  return ptr - str;
}
/*}}}*/

/*{{{ parse functions*/
#define DEF_PARSE_BIN(func, bytes, type, ptype, pname)  \
guint parse_##func(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx) { \
  type  val;  \
  ptype *pval;  \
  if(lng < bytes) return 0; \
  val = *((type*)str);  \
  pval = &ctx->data_store[part->index].pname; \
  *pval = val;  \
  ctx->props_store[part->index] = pval; \
  return bytes; \
}

#define DEF_PARSE_NUM(func, base, type, ptype, pname) \
guint parse_##func(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx) {  \
  gchar *ptr;   \
  gchar *ptr2;  \
  type  val;  \
  ptype *pval;  \
  const gchar *delim; \
  if(!lng) return 0;  \
  delim = part->delimiter; \
  if(!delim && !part->next) delim = "\n"; \
  pval = &ctx->data_store[part->index].pname; \
  ctx->props_store[part->index] = pval; \
  if(!delim[0]) { \
    val = g_ascii_strtoll(str, &ptr, base); \
    *pval = val;  \
    return ptr - str; \
  } \
  for(ptr = str; g_ascii_isspace(*ptr); ptr++);   \
  ptr = g_strstr_len(ptr, lng, delim);  \
  if(!ptr) return 0; \
  val = g_ascii_strtoll(str, &ptr2, base);  \
  *pval = val;  \
  if(ptr > ptr2) return 0;  \
  return ptr - str; \
}

#define DEF_PARSE_FLOAT(func, type, pname)  \
guint parse_##func(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx) {  \
  gchar *ptr;   \
  gchar *ptr2;  \
  type  val;  \
  type  *pval;  \
  const gchar *delim; \
  if(!lng) return 0;  \
  delim = part->delimiter; \
  if(!delim && !part->next) delim = "\n"; \
  pval = &ctx->data_store[part->index].pname; \
  ctx->props_store[part->index] = pval; \
  if(!delim[0]) { \
    val = g_ascii_strtod(str, &ptr);  \
    *pval = val;  \
    return ptr - str; \
  } \
  for(ptr = str; g_ascii_isspace(*ptr); ptr++);   \
  ptr = g_strstr_len(ptr, lng, delim);  \
  if(!ptr) return 0; \
  val = g_ascii_strtod(str, &ptr2); \
  *pval = val;  \
  if(ptr > ptr2) return 0;  \
  return ptr - str; \
}

#define DEF_PARSE_UNS(func, base, type, ptype, pname) \
guint parse_##func(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx) {  \
  gchar *ptr;   \
  gchar *ptr2;  \
  type  val;  \
  ptype *pval;  \
  const gchar *delim; \
  if(!lng) return 0;  \
  delim = part->delimiter; \
  if(!delim && !part->next) delim = "\n"; \
  pval = &ctx->data_store[part->index].pname; \
  ctx->props_store[part->index] = pval; \
  if(!delim[0]) { \
    val = g_ascii_strtoull(str, &ptr, base); \
    *pval = val;  \
    return ptr - str; \
  } \
  for(ptr = str; g_ascii_isspace(*ptr); ptr++);   \
  ptr = g_strstr_len(ptr, lng, delim);  \
  if(!ptr) return 0; \
  val = g_ascii_strtoull(str, &ptr2, base);  \
  *pval = val;  \
  if(ptr > ptr2) return 0;  \
  return ptr - str; \
}

DEF_PARSE_BIN(byte, 1, gchar, guint, u)
DEF_PARSE_BIN(word, 2, gushort, guint, u)
DEF_PARSE_BIN(dword, 4, gulong, gulong, ul)
DEF_PARSE_BIN(qword, 8, guint64, guint64, ull)

guint parse_char(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  const gchar *ptr;
  gchar val;
  gchar *pval;
  const gchar *delim;

  if(!lng)
    return 0;

  delim = part->delimiter;
  if(!delim && !part->next)
    delim = "\n";

  pval = &ctx->data_store[part->index].c;
  ctx->props_store[part->index] = pval;

  if(!delim[0])
  {
    val = *str;
    *pval = val;
    return 1;
  }

  //for(ptr = str; g_ascii_isspace(*ptr); ptr++);

  ptr = g_strstr_len(str, lng, delim);

  if(!ptr)
    return 0;

  //FIXME?
  val = *(ptr-1);
  *pval = val;
  
  return ptr - str;
}

DEF_PARSE_NUM(decimal, 10, gint, gint, i)
DEF_PARSE_NUM(decimal16, 10, gshort, gint, i)
DEF_PARSE_NUM(decimal32, 10, glong, glong, l)
DEF_PARSE_NUM(decimal64, 10, gint64, gint64, ll)

DEF_PARSE_FLOAT(floatingpoint, gfloat, f)
DEF_PARSE_FLOAT(double, gdouble, d)

DEF_PARSE_UNS(octal, 010, guint, guint, u)
DEF_PARSE_UNS(octal16, 010, gushort, guint, u)
DEF_PARSE_UNS(octal32, 010, gulong, gulong, ul)
DEF_PARSE_UNS(octal64, 010, guint64, guint64, ull)

guint parse_string(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  gchar *ptr;
  gchar *cur;
  const gchar *delim;

  if(!lng)
    return 0;

  delim = part->delimiter;
  if(!delim && !part->next)
    delim = "\n";

  if(!delim[0])
    return 0;

  for(cur = str; g_ascii_isspace(*cur); cur++);

  ptr = g_strstr_len(cur, lng, delim);

  if(!ptr)
    return 0;

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

guint parse_filename(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
  gchar *ptr;
  const gchar *delim;

  if(!lng)
    return 0;

  delim = part->delimiter;

  if(!delim && !part->next)
    delim = "\n";

  if(!delim[0])
    return 0;

  ptr = g_strstr_len(str, lng, delim);

  if(!ptr)
    return 0;

  ctx->filename = g_strndup(str, ptr-str);

  return ptr - str;
}
/*}}}*/

static gchar* strdup_escaped(const gchar *str, guint lng)/*{{{*/
{
  guint i;
  gchar *new_str;
  gchar ch;
  guint size = 0;

  for(i = 0; i < lng; i++)
  {
    switch(str[i])
    {
      case '%':
        i++;
      break;
      case '\\':
        i++;
        switch(str[i])
        {
          case 'x':
            if(g_ascii_isxdigit(str[i+1]))
            {
              i++;
              if(g_ascii_isxdigit(str[i+1]))
              {
                i++;
              }
            }
          break;
          default:
            ch = str[i+1];
            if(ch>='0' && ch < '8')
            {
              i++;
              ch = str[i+1];
              if(ch>='0' && ch < '8')
              {
                ch = str[i];
                i++;
                if(ch < '4')
                {
                  ch = str[i+1];
                  if(str[i+1]>='0' && str[i+1] < '8')
                  {
                    i++;
                  }
                }
              }
            }
          break;
        }
      break;
    }
    size++;
  }

  new_str = g_new(gchar, size+1);
  new_str[size] = '\0';

  size = 0;
  for(i = 0; i < lng; i++)
  {
    switch(ch = str[i])
    {
      case '%':
        i++;
        ch = str[i];
      break;
      case '\\':
        i++;
        switch(ch = str[i])
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
            if(g_ascii_isxdigit(str[i+1]))
            {
              i++;
              ch = g_ascii_xdigit_value(str[i]);
              if(g_ascii_isxdigit(str[i+1]))
              {
                i++;
                ch = (ch*0x10) + g_ascii_xdigit_value(str[i]);
              }
            }
          break;
          default:
            if(str[i+1]>='0' && str[i+1] < '8')
            {
              i++;
              ch = str[i]-'0';
              if(str[i+1]>='0' && str[i+1] < '8')
              {
                i++;
                if((ch = (ch*010) + (str[i]-'0')) < 040)
                {
                  if(str[i+1]>='0' && str[i+1] < '8')
                  {
                    i++;
                    ch = (ch*010) + (str[i]-'0');
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

static void build_parser(LSQScanfParser *parser, const gchar *parser_string)
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
  guint index;

  parse_part *part = g_new0(parse_part, 1);
  parse_part *parts = part;
  guint part_count = 0;

  cur = ptr = parser_string;

  while((ch = *ptr++))
  {
    if(ch == '\\' && *ptr == 'n')
    {
      part->delimiter = strdup_escaped(cur, ptr-cur);
      part->next = g_new0(parse_part, 1);
      part = part->next;
      cur = ++ptr;
      continue;
    }
    if(ch == '%')
    {
      skip_flag = FALSE;
      size_flag = SIZE_NORMAL;
      width_flag = 0;
      index = part_count;

      if(!(ch = *ptr++))
        break;

      if(ch == '%')
        continue;

      part->delimiter = strdup_escaped(cur, ptr-cur-2);

      /*{{{ check differend index %.$*/
      if(g_ascii_isdigit(ch))
      {
        index_flag = g_ascii_strtoull(ptr-1, &pos, 10);
        if(*pos == '$')
        {
          ptr = pos+1;
          index = index_flag-1;
          if(!(ch = *ptr++))
            break;
        }
      }
      /*}}}*/

      /*{{{ check skip flag %*.*/
      if(ch == '*')
      {
        skip_flag = TRUE;
        if(!(ch = *ptr++))
          break;
      }
      /*}}}*/

      /*{{{ check max characters %.*/
      //ignored for now
      if(g_ascii_isdigit(ch))
      {
        width_flag = g_ascii_strtoull(ptr-1, (gchar **)&ptr, 10);
        if(!(ch = *ptr++))
          break;
      }
      /*}}}*/

      /*{{{ check size flag %[hlL].*/
      switch(ch)
      {
        case 'h':
          size_flag = SIZE_SHORT;
          ch = *ptr++;
        break;
        case 'l':
          size_flag = SIZE_LONG;
          ch = *ptr++;
          if(ch != 'l')
            break;
        case 'L':
          size_flag = SIZE_LONGLONG;
          ch = *ptr++;
        break;
      }
      if(!ch)
        break;
      /*}}}*/

      switch(ch)
      {
        /*{{{ byte %b*/
        case 'b':
          g_return_if_fail(!width_flag);
          part->next = g_new0(parse_part, 1);
          part = part->next;
          if(skip_flag)
          {
            switch(size_flag)
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
            part_count++;
            part->index = index;
            switch(size_flag)
            {
              case SIZE_NORMAL:
                part->function = parse_byte;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_UINT);
              break;
              case SIZE_SHORT:
                part->function = parse_word;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_UINT);
              break;
              case SIZE_LONG:
                part->function = parse_dword;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_ULONG);
              break;
              case SIZE_LONGLONG:
                part->function = parse_qword;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_UINT64);
              break;
            }
          }
        break;
        /*}}}*/
        /*{{{ character %c*/
        case 'c':
          g_return_if_fail(!size_flag && !width_flag);
          part->next = g_new0(parse_part, 1);
          part = part->next;
          if(skip_flag)
          {
            part->function = skip_char;
          }
          else
          {
            part_count++;
            part->index = index;
            part->function = parse_char;
            lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_CHAR);
          }
        break;
        /*}}}*/
        /*{{{ decimal %d*/
        case 'd':
        case 'i':
          part->next = g_new0(parse_part, 1);
          part = part->next;
          part->width = width_flag;
          if(skip_flag)
          {
            part->function = skip_decimal;
          }
          else
          {
            part_count++;
            part->index = index;
            switch(size_flag)
            {
              case SIZE_NORMAL:
                part->function = parse_decimal;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_INT);
              break;
              case SIZE_SHORT:
                part->function = parse_decimal16;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_INT);
              break;
              case SIZE_LONG:
                part->function = parse_decimal32;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_LONG);
              break;
              case SIZE_LONGLONG:
                part->function = parse_decimal64;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_INT64);
              break;
            }
          }
        break;
        /*}}}*/
        /*{{{ floating point %d*/
        case 'f':
          g_return_if_fail(!size_flag || size_flag == SIZE_LONGLONG);
          part->next = g_new0(parse_part, 1);
          part = part->next;
          part->width = width_flag;
          if(skip_flag)
          {
            part->function = skip_floatingpoint;
          }
          else
          {
            part_count++;
            part->index = index;
            switch(size_flag)
            {
              case SIZE_NORMAL:
                part->function = parse_floatingpoint;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_FLOAT);
              break;
              case SIZE_LONGLONG:
                part->function = parse_double;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_DOUBLE);
              break;
              default:
              break;
            }
          }
        break;
        /*}}}*/
        /*{{{ octal %o*/
        case 'o':
          part->next = g_new0(parse_part, 1);
          part = part->next;
          part->width = width_flag;
          if(skip_flag)
          {
            part->function = skip_octal;
          }
          else
          {
            part_count++;
            part->index = index;
            switch(size_flag)
            {
              case SIZE_NORMAL:
                part->function = parse_octal;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_UINT);
              break;
              case SIZE_SHORT:
                part->function = parse_octal16;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_UINT);
              break;
              case SIZE_LONG:
                part->function = parse_octal32;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_ULONG);
              break;
              case SIZE_LONGLONG:
                part->function = parse_octal64;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_UINT64);
              break;
            }
          }
        break;
        /*}}}*/
        /*{{{ string %s*/
        case 's':
          g_return_if_fail(!size_flag);
          part->next = g_new0(parse_part, 1);
          part = part->next;
          part->width = width_flag;
          if(skip_flag)
          {
            part->function = skip_string;
          }
          else
          {
            part_count++;
            part->index = index;
            part->function = parse_string;
            lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_STRING);
          }
        break;
        /*}}}*/
        /*{{{ unsigned decimal %u*/
        case 'u':
          part->next = g_new0(parse_part, 1);
          part = part->next;
          part->width = width_flag;
          if(skip_flag)
          {
            part->function = skip_unsigned;
          }
          else
          {
            part_count++;
            part->index = index;
            switch(size_flag)
            {
              case SIZE_NORMAL:
                part->function = parse_unsigned;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_UINT);
              break;
              case SIZE_SHORT:
                part->function = parse_unsigned16;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_UINT);
              break;
              case SIZE_LONG:
                part->function = parse_unsigned32;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_ULONG);
              break;
              case SIZE_LONGLONG:
                part->function = parse_unsigned64;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_UINT64);
              break;
            }
          }
        break;
        /*}}}*/
        /*{{{ hexadecimal %x %X*/
        case 'x':
        case 'X':
          part->next = g_new0(parse_part, 1);
          part = part->next;
          part->width = width_flag;
          if(skip_flag)
          {
            part->function = skip_hexadecimal;
          }
          else
          {
            part_count++;
            part->index = index;
            switch(size_flag)
            {
              case SIZE_NORMAL:
                part->function = parse_hexadecimal;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_UINT);
              break;
              case SIZE_SHORT:
                part->function = parse_hexadecimal16;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_UINT);
              break;
              case SIZE_LONG:
                part->function = parse_hexadecimal32;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_ULONG);
              break;
              case SIZE_LONGLONG:
                part->function = parse_hexadecimal64;
                lsq_parser_set_property_type(LSQ_PARSER(parser), index, G_TYPE_UINT64);
              break;
            }
          }
        break;
        /*}}}*/
        /*{{{ filename %F*/
        case 'F':
          g_return_if_fail(!skip_flag && !size_flag && !width_flag);
          part->next = g_new0(parse_part, 1);
          part = part->next;
          part->function = parse_filename;
        break;
        /*}}}*/
        default:
          g_return_if_reached();
      }
      cur = ptr;
    }
  }

  parser->parser = parts;
}

static void lsq_scanf_parser_parse(LSQScanfParser *parser, LSQScanfParserContext *ctx)
{
  gchar *line, *ptr;
  gsize line_length, lng;
  guint size, line_nr;
  parse_part *part;
  if(!lsq_parser_context_get_line(LSQ_PARSER_CONTEXT(ctx), &line, &line_length))
    return;


  ptr = line;
  lng = line_length;
  line_nr = ctx->parse_line;
  part = ctx->parse_loc;

  if(!part)
  {
    ctx->parse_loc = part = parser->parser;
    ctx->parse_line = line_nr = 0;
  }

  while(part)
  {
    //parse
    if(part->function)
    {
      size = part->function(ptr, lng, part, ctx);
      if(!size)
      {
        //no match
        if(line_nr)
        {
          ptr = line;
          lng = line_length;
          part = parser->parser;
          line_nr = 0;
          continue;
        }
        ctx->parse_loc = parser->parser;
        ctx->parse_line = 0;
        g_free(line);
        return;
      }
      ptr += size;
      lng -= size;
    }

    if(part->delimiter)
    {
      if(!g_str_has_prefix(ptr, part->delimiter))
      {
        //no match
        if(line_nr)
        {
          ptr = line;
          lng = line_length;
          part = parser->parser;
          line_nr = 0;
          continue;
        }
        ctx->parse_loc = parser->parser;
        ctx->parse_line = 0;
        g_free(line);
        return;
      }

      size = strlen(part->delimiter);
      if(size > lng)
        size = lng;
      ptr += size;
      lng -= size;
    }

    if(part->delimiter)
    {
      if(g_str_has_suffix(part->delimiter, "\n"))
      {
        //next line
        if(part->next)
        {
          ctx->parse_loc = part->next;
          ctx->parse_line = line_nr+1;
          g_free(line);
          return;
        }
      }
    }
    
    part = part->next;
  }

  g_free(line);

  if(!part)
  {
    LSQArchiveIter *iter = lsq_archive_add_file(LSQ_PARSER_CONTEXT(ctx)->archive, ctx->filename);
    lsq_archive_iter_set_propsv(iter, ctx->props_store);
    lsq_archive_iter_unref(iter);

    ctx->parse_loc = parser->parser;
    ctx->parse_line = 0;
  }
}

