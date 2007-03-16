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

#ifndef __LIBSQUEEZE_DBUS_COMMAND_H__
#define __LIBSQUEEZE_DBUS_COMMAND_H__ 
G_BEGIN_DECLS

#define LSQ_TYPE_DBUS_COMMAND lsq_dbus_command_get_type()

#define LSQ_DBUS_COMMAND(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LSQ_TYPE_DBUS_COMMAND,      \
			LSQDBusCommand))

#define LSQ_IS_DBUS_COMMAND(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LSQ_TYPE_DBUS_COMMAND))

#define LSQ_DBUS_COMMAND_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			LSQ_TYPE_DBUS_COMMAND,      \
			LSQDBusCommandClass))

#define LSQ_IS_DBUS_COMMAND_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			LSQ_TYPE_DBUS_COMMAND))

typedef struct _LSQDBusCommand LSQDBusCommand;

struct _LSQDBusCommand
{
	LSQArchiveCommand  parent;
	GSList *command_queue;
};

typedef struct _LSQDBusCommandClass LSQDBusCommandClass;

struct _LSQDBusCommandClass
{
	LSQArchiveCommandClass parent;
}; 

LSQArchiveCommand * lsq_dbus_command_new(const gchar *comment, LSQArchive *archive);
void lsq_dbus_command_append(LSQDBusCommand *command, LSQArchiveCommand *sub_command);

G_END_DECLS
#endif /* __LIBSQUEEZE_DBUS_COMMAND_H__ */
