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

#include <thunar-vfs/thunar-vfs.h>
#include "libsqueeze.h"
#include "archive-iter.h"
#include "parser-context.h"
#include "parser.h"
#include "scanf-parser.h"
#include "archive.h"

typedef struct _parse_part parse_part;
typedef struct _LSQScanfParserContext LSQScanfParserContext;

typedef guint (*LSQParseFunc)(gchar*, guint, parse_part*, LSQScanfParserContext*);

struct _parse_part
{
	gchar *delimiter;
	LSQParseFunc function;
	guint index;
	guint width;
};

struct _LSQScanfParserContext
{
  gchar *filename;
  LSQParserContext parser;
  union {
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
};

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

	if(!lng)
		return 0;

	if(!part->delimiter[0])
		return 1;

	//for(ptr = str; g_ascii_isspace(*ptr); ptr++);

	ptr = g_strstr_len(str, lng, part->delimiter);
	
	return ptr - str;
}

guint skip_decimal(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
	gchar *ptr;
#ifdef DO_EXSTENSIVE_CHECKING
	gchar *ptr2;
#endif

	if(!lng)
		return 0;

	if(!part->delimiter[0])
	{
		g_ascii_strtoll(str, &ptr, 10);
		return ptr - str;
	}

	for(ptr = str; g_ascii_isspace(*ptr); ptr++);

	ptr = g_strstr_len(ptr, lng, part->delimiter);
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

	if(!lng)
		return 0;

	if(!part->delimiter[0])
	{
		g_ascii_strtod(str, &ptr);
		return ptr - str;
	}

	for(ptr = str; g_ascii_isspace(*ptr); ptr++);

	ptr = g_strstr_len(ptr, lng, part->delimiter);
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

	if(!lng)
		return 0;

	if(!part->delimiter[0])
	{
		g_ascii_strtoll(str, &ptr, 010);
		return ptr - str;
	}

	for(ptr = str; g_ascii_isspace(*ptr); ptr++);

	ptr = g_strstr_len(ptr, lng, part->delimiter);
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

	if(!lng)
		return 0;

	if(!part->delimiter[0])
		return 0;

	for(ptr = str; g_ascii_isspace(*ptr); ptr++);

	ptr = g_strstr_len(ptr, lng, part->delimiter);
	
	return ptr - str;
}

guint skip_unsigned(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx)
{
	gchar *ptr;
#ifdef DO_EXSTENSIVE_CHECKING
	gchar *ptr2;
#endif

	if(!lng)
		return 0;

	if(!part->delimiter[0])
	{
		g_ascii_strtoull(str, &ptr, 10);
		return ptr - str;
	}

	ptr = g_strstr_len(str, lng, part->delimiter);
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

	if(!lng)
		return 0;

	if(!part->delimiter[0])
	{
		g_ascii_strtoll(str, &ptr, 0x10);
		return ptr - str;
	}

	ptr = g_strstr_len(str, lng, part->delimiter);
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
  type  val;    \
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
  type  val;    \
  ptype *pval;  \
	if(!lng) return 0;  \
	pval = &ctx->data_store[part->index].pname; \
  ctx->props_store[part->index] = pval; \
	if(!part->delimiter[0]) { \
		val = g_ascii_strtoll(str, &ptr, base); \
    *pval = val;  \
		return ptr - str; \
	} \
	for(ptr = str; g_ascii_isspace(*ptr); ptr++);   \
	ptr = g_strstr_len(ptr, lng, part->delimiter);  \
  val = g_ascii_strtoll(str, &ptr2, base);  \
  *pval = val;  \
	if(ptr > ptr2) return 0;  \
	return ptr - str; \
}

#define DEF_PARSE_FLOAT(func, type, pname)  \
guint parse_##func(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx) {  \
	gchar *ptr;   \
	gchar *ptr2;  \
  type  val;    \
  type  *pval;  \
	if(!lng) return 0;  \
	pval = &ctx->data_store[part->index].pname; \
  ctx->props_store[part->index] = pval; \
	if(!part->delimiter[0]) { \
		val = g_ascii_strtod(str, &ptr);  \
    *pval = val;  \
		return ptr - str; \
	} \
	for(ptr = str; g_ascii_isspace(*ptr); ptr++);   \
	ptr = g_strstr_len(ptr, lng, part->delimiter);  \
  val = g_ascii_strtod(str, &ptr2); \
  *pval = val;  \
	if(ptr > ptr2) return 0;  \
	return ptr - str; \
}

#define DEF_PARSE_UNS(func, base, type, ptype, pname) \
guint parse_##func(gchar *str, guint lng, parse_part *part, LSQScanfParserContext *ctx) {  \
	gchar *ptr;   \
	gchar *ptr2;  \
  type  val;    \
  ptype *pval;  \
	if(!lng) return 0;  \
	pval = &ctx->data_store[part->index].pname; \
  ctx->props_store[part->index] = pval; \
	if(!part->delimiter[0]) { \
		val = g_ascii_strtoull(str, &ptr, base); \
    *pval = val;  \
		return ptr - str; \
	} \
	for(ptr = str; g_ascii_isspace(*ptr); ptr++);   \
	ptr = g_strstr_len(ptr, lng, part->delimiter);  \
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

	if(!lng)
		return 0;

	pval = &ctx->data_store[part->index].c;
  ctx->props_store[part->index] = pval;

	if(!part->delimiter[0])
  {
    val = *str;
    *pval = val;
		return 1;
  }

	//for(ptr = str; g_ascii_isspace(*ptr); ptr++);

	ptr = g_strstr_len(str, lng, part->delimiter);

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

	if(!lng)
		return 0;

	if(!part->delimiter[0])
		return 0;

	for(cur = str; g_ascii_isspace(*cur); cur++);

	ptr = g_strstr_len(cur, lng, part->delimiter);

  ctx->props_store[part->index] = g_strndup(cur, ptr-cur);

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

	if(!lng)
		return 0;

	if(!part->delimiter[0])
		return 0;

	ptr = g_strstr_len(str, lng, part->delimiter);

  ctx->filename = g_strndup(str, ptr-str);

	return ptr - str;
}
/*}}}*/

gchar* strdup_escaped(const gchar *str, guint lng)/*{{{*/
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

void build_parser(LSQArchive *archive, const gchar *parser_string)
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
	GSList *parts = g_slist_prepend(NULL, part);
	guint part_count = 0;

	cur = ptr = parser_string;

	while((ch = *ptr++))
	{
		if(ch == '\\' && *ptr == 'n')
		{
			part->delimiter = strdup_escaped(cur, ptr-cur);
			part = g_new0(parse_part, 1);
			parts = g_slist_prepend(parts, part);
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

			part->delimiter = strdup_escaped(cur, ptr-cur-1);

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
				width_flag = strtoul(ptr-1, &ptr, 10);
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
					if(width_flag)
						return;
					part = g_new0(parse_part, 1);
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
								lsq_archive_set_property_type(archive, index, G_TYPE_UINT);
							break;
							case SIZE_SHORT:
								part->function = parse_word;
								lsq_archive_set_property_type(archive, index, G_TYPE_UINT);
							break;
							case SIZE_LONG:
								part->function = parse_dword;
								lsq_archive_set_property_type(archive, index, G_TYPE_ULONG);
							break;
							case SIZE_LONGLONG:
								part->function = parse_qword;
								lsq_archive_set_property_type(archive, index, G_TYPE_UINT64);
							break;
						}
					}
					parts = g_slist_prepend(parts, part);
				break;
				/*}}}*/
				/*{{{ character %c*/
				case 'c':
					if(size_flag || width_flag)
						return;
					part = g_new0(parse_part, 1);
					if(skip_flag)
					{
						part->function = skip_char;
					}
					else
					{
						part_count++;
						part->index = index;
						part->function = parse_char;
						lsq_archive_set_property_type(archive, index, G_TYPE_CHAR);
					}
					parts = g_slist_prepend(parts, part);
				break;
				/*}}}*/
				/*{{{ decimal %d*/
				case 'd':
				//case 'i':
					part = g_new0(parse_part, 1);
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
								lsq_archive_set_property_type(archive, index, G_TYPE_INT);
							break;
							case SIZE_SHORT:
								part->function = parse_decimal16;
								lsq_archive_set_property_type(archive, index, G_TYPE_INT);
							break;
							case SIZE_LONG:
								part->function = parse_decimal32;
								lsq_archive_set_property_type(archive, index, G_TYPE_LONG);
							break;
							case SIZE_LONGLONG:
								part->function = parse_decimal64;
								lsq_archive_set_property_type(archive, index, G_TYPE_INT64);
							break;
						}
					}
					parts = g_slist_prepend(parts, part);
				break;
				/*}}}*/
				/*{{{ floating point %d*/
				case 'f':
					if(size_flag && size_flag != SIZE_LONGLONG)
						return;
					part = g_new0(parse_part, 1);
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
								lsq_archive_set_property_type(archive, index, G_TYPE_FLOAT);
							break;
							case SIZE_LONGLONG:
								part->function = parse_double;
								lsq_archive_set_property_type(archive, index, G_TYPE_DOUBLE);
							break;
						}
					}
					parts = g_slist_prepend(parts, part);
				break;
				/*}}}*/
				/*{{{ octal %o*/
				case 'o':
					part = g_new0(parse_part, 1);
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
								lsq_archive_set_property_type(archive, index, G_TYPE_UINT);
							break;
							case SIZE_SHORT:
								part->function = parse_octal16;
								lsq_archive_set_property_type(archive, index, G_TYPE_UINT);
							break;
							case SIZE_LONG:
								part->function = parse_octal32;
								lsq_archive_set_property_type(archive, index, G_TYPE_ULONG);
							break;
							case SIZE_LONGLONG:
								part->function = parse_octal64;
								lsq_archive_set_property_type(archive, index, G_TYPE_UINT64);
							break;
						}
					}
					parts = g_slist_prepend(parts, part);
				break;
				/*}}}*/
				/*{{{ string %s*/
				case 's':
					if(size_flag)
						return;
					part = g_new0(parse_part, 1);
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
						lsq_archive_set_property_type(archive, index, G_TYPE_STRING);
					}
					parts = g_slist_prepend(parts, part);
				break;
				/*}}}*/
				/*{{{ unsigned decimal %u*/
				case 'u':
					part = g_new0(parse_part, 1);
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
								lsq_archive_set_property_type(archive, index, G_TYPE_UINT);
							break;
							case SIZE_SHORT:
								part->function = parse_unsigned16;
								lsq_archive_set_property_type(archive, index, G_TYPE_UINT);
							break;
							case SIZE_LONG:
								part->function = parse_unsigned32;
								lsq_archive_set_property_type(archive, index, G_TYPE_ULONG);
							break;
							case SIZE_LONGLONG:
								part->function = parse_unsigned64;
								lsq_archive_set_property_type(archive, index, G_TYPE_UINT64);
							break;
						}
					}
					parts = g_slist_prepend(parts, part);
				break;
				/*}}}*/
				/*{{{ hexadecimal %x %X*/
				case 'x':
				case 'X':
					part = g_new0(parse_part, 1);
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
								lsq_archive_set_property_type(archive, index, G_TYPE_UINT);
							break;
							case SIZE_SHORT:
								part->function = parse_hexadecimal16;
								lsq_archive_set_property_type(archive, index, G_TYPE_UINT);
							break;
							case SIZE_LONG:
								part->function = parse_hexadecimal32;
								lsq_archive_set_property_type(archive, index, G_TYPE_ULONG);
							break;
							case SIZE_LONGLONG:
								part->function = parse_hexadecimal64;
								lsq_archive_set_property_type(archive, index, G_TYPE_UINT64);
							break;
						}
					}
					parts = g_slist_prepend(parts, part);
				break;
				/*}}}*/
				/*{{{ filename %F*/
				case 'F':
					if(skip_flag || size_flag || width_flag)
						return;
					part = g_new0(parse_part, 1);
					part->function = parse_filename;
					parts = g_slist_prepend(parts, part);
				break;
				/*}}}*/
				default:
					return;
			}
			cur = ptr;
		}
	}

}

parse()
{

}

