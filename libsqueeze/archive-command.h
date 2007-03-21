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

struct _LSQArchiveCommand
{
	GObject      parent;
	GQuark       domain;
	gchar       *comment;
	LSQArchive  *archive;
	GError      *error;
	gboolean     running;
	gboolean     safe;
	gpointer     user_data;
	gboolean   (*execute)(LSQArchiveCommand *);
	gboolean   (*stop)(LSQArchiveCommand *);
};

struct _LSQArchiveCommandClass
{
	GObjectClass parent;
}; 

gboolean
lsq_archive_command_stop(LSQArchiveCommand *command);
gboolean
lsq_archive_command_execute(LSQArchiveCommand *command);
const gchar *
lsq_archive_command_get_comment(LSQArchiveCommand *archive_command);

G_END_DECLS
#endif /* __LIBSQUEEZE_ARCHIVE_COMMAND_H__ */
