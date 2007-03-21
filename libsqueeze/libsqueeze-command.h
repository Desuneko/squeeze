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

#ifndef __LIBSQUEEZE_COMMAND_H__
#define __LIBSQUEEZE_COMMAND_H__ 
G_BEGIN_DECLS

#define LSQ_TYPE_ARCHIVE_COMMAND lsq_archive_command_get_type()

#define LSQ_ARCHIVE_COMMAND(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_ARCHIVE_COMMAND,      \
			LSQArchiveCommand))

#define LSQ_IS_ARCHIVE_COMMAND(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_ARCHIVE_COMMAND))

#define LSQ_ARCHIVE_COMMAND_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			LSQ_TYPE_ARCHIVE_COMMAND,      \
			LSQArchiveCommandClass))

#define LSQ_IS_ARCHIVE_COMMAND_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			LSQ_TYPE_ARCHIVE_COMMAND))

typedef struct _LSQArchiveCommand LSQArchiveCommand;


typedef struct _LSQArchiveCommandClass LSQArchiveCommandClass;

GType               lsq_archive_command_get_type(void);

LSQArchive         *lsq_archive_command_get_archive(LSQArchiveCommand *command);

G_END_DECLS
#endif /* __LIBSQUEEZE_COMMAND_H__ */
