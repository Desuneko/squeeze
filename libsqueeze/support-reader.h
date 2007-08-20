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

#ifndef __LIBSQUEEZE_SUPPORT_READER_H__
#define __LIBSQUEEZE_SUPPORT_READER_H__ 

G_BEGIN_DECLS

#define LSQ_TYPE_SUPPORT_READER lsq_support_reader_get_type()

#define LSQ_SUPPORT_READER(obj)		 ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),	\
			LSQ_TYPE_SUPPORT_READER,	  \
			LSQSupportReader))

#define LSQ_IS_SUPPORT_READER(obj)	  ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),	\
			LSQ_TYPE_SUPPORT_READER))

#define LSQ_SUPPORT_READER_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),	 \
			LSQ_TYPE_SUPPORT_READER,	  \
			LSQSupportReaderClass))

#define LSQ_IS_SUPPORT_READER_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),		\
			LSQ_TYPE_SUPPORT_READER))

typedef struct _LSQSupportReader LSQSupportReader;

struct _LSQSupportReader
{
	GObject		parent;
};

typedef struct _LSQSupportReaderClass LSQSupportReaderClass;

struct _LSQSupportReaderClass
{
	GObjectClass parent;
}; 

GType			  lsq_support_reader_get_type(void);
LSQSupportReader  *lsq_support_reader_new();
LSQSupportFactory *lsq_support_reader_parse_file(const gchar *filename);


G_END_DECLS
#endif /* __LIBSQUEEZE_SUPPORT_READER_H__ */

