/*
 *  Copyright (c) 2006 Stephan Arts <psyBSD@gmail.com>
 *
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

#ifndef __LIBXARCHIVER_ARCHIVE_H__
#define __LIBXARCHIVER_ARCHIVE_H__ 
G_BEGIN_DECLS

typedef enum
{
	LXA_ARCHIVESTATUS_IDLE = 0,
	LXA_ARCHIVESTATUS_INIT,
	LXA_ARCHIVESTATUS_ADD,
	LXA_ARCHIVESTATUS_EXTRACT,
	LXA_ARCHIVESTATUS_REMOVE,
	LXA_ARCHIVESTATUS_REFRESH,
	LXA_ARCHIVESTATUS_ERROR,
	LXA_ARCHIVESTATUS_USERBREAK
} LXAArchiveStatus;


typedef struct _LXAEntry LXAEntry;

#include "slist.h"

struct _LXAEntry {
	gchar *filename;
	gpointer props;
	gchar *mime_type;
	/* */
	LXAEntry **children;
	LXASList *buffer;
};

#define LXA_ARCHIVE(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			lxa_archive_get_type(),      \
			LXAArchive))

#define LXA_IS_ARCHIVE(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			lxa_archive_get_type()))

#define LXA_ARCHIVE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_CAST ((class),     \
			lxa_archive_get_type(),      \
			LXAArchiveClass))

#define LXA_IS_ARCHIVE_CLASS(class) ( \
		G_TYPE_CHECK_CLASS_TYPE ((class),        \
			lxa_archive_get_type()))


typedef struct _LXAArchive LXAArchive;


struct _LXAArchive
{
	GObject parent;
	gchar              *path;
	gchar              *mime;
	LXAArchiveStatus    status;
	LXAArchiveStatus    old_status;
	GPid                child_pid;
	GIOChannel         *ioc_in;
	GIOChannel         *ioc_out;
	GIOChannel         *ioc_err;
	gpointer            support;
	gchar              *tmp_file;
	gchar              *files;
	gboolean            has_passwd;
	guint               column_number;
	GType              *column_types;
	gchar             **column_names;
	LXAEntry            root_entry;
	gushort             entry_props_size;
};

typedef struct _LXAArchiveClass LXAArchiveClass;

struct _LXAArchiveClass
{
	GObjectClass parent;
}; 

GType              lxa_archive_get_type(void);
LXAArchive        *lxa_archive_new(gchar *, gchar *);

gchar             *lxa_archive_discover_mime(LXAArchive *archive);
void               lxa_archive_set_status(LXAArchive *archive, LXAArchiveStatus status);
gint               lxa_archive_lookup_dir(gpointer entry, gconstpointer filename);
LXAEntry          *lxa_archive_add_file(LXAArchive *archive, gchar *path);
GSList            *lxa_archive_get_children(LXAArchive *archive, gchar *path);
LXAEntry          *lxa_entry_get_child(LXAEntry *, const gchar *);
gboolean           lxa_entry_add_child(LXAEntry *parent, LXAEntry *child);
guint              lxa_entry_children_length(LXAEntry *entry);
LXAEntry          *lxa_entry_children_nth_data(LXAArchive *archive, LXAEntry *entry, guint n);
//gint               lxa_entry_children_index(LXAEntry *entry, LXAEntry *find);


G_END_DECLS

#endif /* __LIBXARCHIVER_ARCHIVE_H__ */
