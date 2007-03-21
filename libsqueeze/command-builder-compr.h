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

#ifndef __LIBSQUEEZE_COMMAND_BUILDER_COMPR_H__
#define __LIBSQUEEZE_COMMAND_BUILDER_COMPR_H__

#define LSQ_TYPE_COMMAND_BUILDER_COMPR lsq_command_builder_compr_get_type()

#define LSQ_COMMAND_BUILDER_COMPR(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_COMMAND_BUILDER_COMPR,      \
			LSQCommandBuilderCompr))

#define LSQ_IS_COMMAND_BUILDER_COMPR(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_COMMAND_BUILDER_COMPR))

#define LSQ_COMMAND_BUILDER_COMPR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LSQ_TYPE_COMMAND_BUILDER_COMPR,      \
			LSQCommandBuilderComprClass))

#define LSQ_IS_COMMAND_BUILDER_COMPR_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LSQ_TYPE_COMMAND_BUILDER_COMPR))

typedef struct _LSQCommandBuilderCompr LSQCommandBuilderCompr;

struct _LSQCommandBuilderCompr
{
	LSQCommandBuilder parent;
};

typedef struct _LSQCommandBuilderComprClass LSQCommandBuilderComprClass;

struct _LSQCommandBuilderComprClass
{
	LSQCommandBuilderClass parent;
};

GType                lsq_command_builder_compr_get_type(void);
LSQCommandBuilder   *lsq_command_builder_compr_new();

#endif /* __LIBSQUEEZE_COMMAND_BUILDER_COMPR_H__ */
