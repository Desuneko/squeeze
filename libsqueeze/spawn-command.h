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

#ifndef __LIBSQUEEZE_SPAWN_COMMAND_H__
#define __LIBSQUEEZE_SPAWN_COMMAND_H__ 
G_BEGIN_DECLS

#define LSQ_TYPE_SPAWN_COMMAND lsq_spawn_command_get_type()

#define LSQ_SPAWN_COMMAND(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_SPAWN_COMMAND,      \
			LSQSpawnCommand))

#define LSQ_IS_SPAWN_COMMAND(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_SPAWN_COMMAND))

#define LSQ_SPAWN_COMMAND_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			LSQ_TYPE_SPAWN_COMMAND,      \
			LSQSpawnCommandClass))

#define LSQ_IS_SPAWN_COMMAND_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			LSQ_TYPE_SPAWN_COMMAND))

typedef struct _LSQSpawnCommand LSQSpawnCommand;

struct _LSQSpawnCommand
{
	LSQArchiveCommand parent;
	gchar            *command;
	GPid              child_pid;
	GIOChannel       *ioc_in;
	GIOChannel       *ioc_out;
	GIOChannel       *ioc_err;

	gchar            *files;
	gchar            *options;
	gchar            *archive_path;

	LSQParseFunc      parse_stdout;
	LSQParseFunc      parse_stderr;
};

typedef struct _LSQSpawnCommandClass LSQSpawnCommandClass;

struct _LSQSpawnCommandClass
{
	LSQArchiveCommandClass parent;
}; 

GType
lsq_spawn_command_get_type();

LSQArchiveCommand *
lsq_spawn_command_new(const gchar *comment, 
                      LSQArchive *archive, 
                      const gchar *command, 
                      const gchar *files, 
                      const gchar *options, 
                      const gchar *archive_path);
gboolean
lsq_spawn_command_set_parse_func(LSQSpawnCommand *spawn_command, guint fd, LSQParseFunc func, gpointer user_data);

G_END_DECLS
#endif /* __LIBSQUEEZE_ARCHIVE_COMMAND_H__ */
