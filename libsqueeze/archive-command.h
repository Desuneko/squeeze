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

#ifndef __LIBSQUEEZE_ARCHIVE_COMMAND_H__
#define __LIBSQUEEZE_ARCHIVE_COMMAND_H__ 
G_BEGIN_DECLS

#define LSQ_ARCHIVE_COMMAND(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			lsq_archive_command_get_type(),      \
			LSQArchiveCommand))

#define LSQ_IS_ARCHIVE_COMMAND(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			lsq_archive_command_get_type()))

#define LSQ_ARCHIVE_COMMAND_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			lsq_archive_command_get_type(),      \
			LSQArchiveCommandClass))

#define LSQ_IS_ARCHIVE_COMMAND_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			lsq_archive_command_get_type()))

typedef struct _LSQArchiveCommand LSQArchiveCommand;

typedef gboolean (*LSQParseFunc) (LSQArchiveCommand *archive_command);

struct _LSQArchiveCommand
{
	GObject      parent;
	GQuark       domain;
	gchar       *comment;
	gchar       *command;
	LSQArchive  *archive;
	GPid         child_pid;
	GIOChannel  *ioc_in;
	GIOChannel  *ioc_out;
	GIOChannel  *ioc_err;
	gboolean     safe;
	gboolean     refresh;
	GError      *error;
	LSQParseFunc parse_stdout;
};

typedef struct _LSQArchiveCommandClass LSQArchiveCommandClass;

struct _LSQArchiveCommandClass
{
	GObjectClass parent;
}; 

GType               lsq_archive_command_get_type(void) G_GNUC_INTERNAL;
LSQArchiveCommand  *lsq_archive_command_new(const gchar *comment, 
                                            LSQArchive *archive,
                                            const gchar *command,
																						gboolean safe,
																						gboolean change) G_GNUC_INTERNAL;

gboolean            lsq_archive_command_run(LSQArchiveCommand *archive_command) G_GNUC_INTERNAL;

gboolean            lsq_archive_command_stop(LSQArchiveCommand *archive_command) G_GNUC_INTERNAL;
GIOStatus           lsq_archive_command_read_line(LSQArchiveCommand *archive_command,
                                                  guint fd,
                                                  gchar **lines,
                                                  gsize *length,
																									GError **error) G_GNUC_INTERNAL;
GIOStatus           lsq_archive_command_read_bytes(LSQArchiveCommand *archive_command, 
                                                  guint fd,
                                                  gchar *buf,
                                                  gsize max_length,
                                                  gsize *length,
                                                  GError **error) G_GNUC_INTERNAL;
gboolean            lsq_archive_command_set_parse_func(LSQArchiveCommand *archive_command,
                                                  guint fd,
                                                  LSQParseFunc func);
const gchar        *lsq_archive_command_get_comment(LSQArchiveCommand *archive_command);

G_END_DECLS
#endif /* __LIBSQUEEZE_ARCHIVE_COMMAND_H__ */
