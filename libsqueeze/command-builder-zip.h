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

#ifndef __LIBSQUEEZE_COMMAND_BUILDER_ZIP_H__
#define __LIBSQUEEZE_COMMAND_BUILDER_ZIP_H__

#define LSQ_TYPE_COMMAND_BUILDER_ZIP lsq_command_builder_zip_get_type()

#define LSQ_COMMAND_BUILDER_ZIP(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_COMMAND_BUILDER_ZIP,      \
			LSQCommandBuilderZip))

#define LSQ_IS_COMMAND_BUILDER_ZIP(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_COMMAND_BUILDER_ZIP))

#define LSQ_COMMAND_BUILDER_ZIP_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LSQ_TYPE_COMMAND_BUILDER_ZIP,      \
			LSQCommandBuilderZipClass))

#define LSQ_IS_COMMAND_BUILDER_ZIP_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LSQ_TYPE_COMMAND_BUILDER_ZIP))

typedef struct _LSQCommandBuilderZip LSQCommandBuilderZip;

struct _LSQCommandBuilderZip
{
	LSQCommandBuilder parent;
};

typedef struct _LSQCommandBuilderZipClass LSQCommandBuilderZipClass;

struct _LSQCommandBuilderZipClass
{
	GObjectClass parent;
};

GType                lsq_command_builder_zip_get_type(void);

#endif /* __LIBSQUEEZE_COMMAND_BUILDER_ZIP_H__ */