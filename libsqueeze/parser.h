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

#define LSQ_PARSER(obj) (			   \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
			LSQ_TYPE_PARSER,				  \
			LSQParser))

#define LSQ_IS_PARSER(obj) (			\
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),  \
			LSQ_TYPE_PARSER))

#define LSQ_PARSER_CLASS(class) (	  \
		G_TYPE_CHECK_CLASS_CAST ((class),  \
			LSQ_TYPE_PARSER,				 \
			LSQParserClass))

#define LSQ_IS_PARSER_CLASS(class) (   \
		G_TYPE_CHECK_CLASS_TYPE ((class),  \
			LSQ_TYPE_PARSER))

#define LSQ_PARSER_GET_CLASS(obj) (	\
		G_TYPE_INSTANCE_GET_CLASS ((obj),  \
			LSQ_TYPE_PARSER,				 \
	  LSQParserClass))


typedef struct _LSQParser LSQParser;

struct _LSQParser
{
	GObject parent;
};


typedef struct _LSQParserClass LSQParserClass;

struct _LSQParserClass
{
	GObjectClass parent;

	LSQParserContext*(*get_context)(LSQParser *, LSQArchive *);
  void(*parse)(LSQParser *, LSQParserContext *);
};


GType		   lsq_parser_get_type(void);

LSQParserContext* lsq_parser_get_context(LSQParser *, LSQArchive *);

void              lsq_parser_parse(LSQParser *, LSQParserContext *);

G_END_DECLS

#endif /* __LIBSQUEEZE_PARSER_H__ */

