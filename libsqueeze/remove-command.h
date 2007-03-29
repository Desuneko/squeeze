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

#ifndef __LIBSQUEEZE_REMOVE_COMMAND_H__
#define __LIBSQUEEZE_REMOVE_COMMAND_H__ 
G_BEGIN_DECLS

#define LSQ_TYPE_REMOVE_COMMAND lsq_remove_command_get_type()

#define LSQ_REMOVE_COMMAND(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_REMOVE_COMMAND,      \
			LSQRemoveCommand))

#define LSQ_IS_REMOVE_COMMAND(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_REMOVE_COMMAND))

#define LSQ_REMOVE_COMMAND_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			LSQ_TYPE_REMOVE_COMMAND,      \
			LSQRemoveCommandClass))

#define LSQ_IS_REMOVE_COMMAND_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			LSQ_TYPE_REMOVE_COMMAND))

typedef struct _LSQRemoveCommand LSQRemoveCommand;

struct _LSQRemoveCommand
{
	LSQArchiveCommand  parent;
};

typedef struct _LSQRemoveCommandClass LSQRemoveCommandClass;

struct _LSQRemoveCommandClass
{
	LSQArchiveCommandClass parent;
}; 

GType lsq_remove_command_get_type();

LSQArchiveCommand * lsq_remove_command_new(const gchar *comment, LSQArchive *archive, GSList *);

G_END_DECLS
#endif /* __LIBSQUEEZE_REMOVE_COMMAND_H__ */
