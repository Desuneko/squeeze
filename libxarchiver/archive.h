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

enum
{
	LXA_ARCHIVE_PROP_FILENAME = 0,
	LXA_ARCHIVE_PROP_MIME_TYPE,
	LXA_ARCHIVE_PROP_USER
};

typedef struct _LXAEntry LXAEntry;
typedef LXAEntry LXAArchiveIter;


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
	guint               n_property;
	GType              *property_types;
	gchar             **property_names;
	LXAEntry           *root_entry;
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
	gushort             entry_props_size;
};

typedef struct _LXAArchiveClass LXAArchiveClass;

struct _LXAArchiveClass
{
	GObjectClass parent;
}; 

GType           lxa_archive_get_type(void);
LXAArchive     *lxa_archive_new(gchar *, gchar *);

void            lxa_archive_set_status(LXAArchive *archive, LXAArchiveStatus status);

/* new */
gboolean        lxa_archive_iter_is_directory(const LXAArchive *, const LXAArchiveIter *);
guint           lxa_archive_iter_n_children(const LXAArchive *, const LXAArchiveIter *);
LXAArchiveIter *lxa_archive_iter_nth_child(LXAArchive *, LXAArchiveIter *, guint);
LXAArchiveIter *lxa_archive_iter_add_child(LXAArchive *, LXAArchiveIter *, const gchar *);
LXAArchiveIter *lxa_archive_iter_get_child(const LXAArchive *, const LXAArchiveIter *, const gchar *);
gboolean        lxa_archive_iter_del_child(LXAArchive *, LXAArchiveIter *, LXAArchiveIter *);

/* void            lxa_archive_iter_set_filename(LXAArchive *, LXAArchiveIter *, const gchar *); */
void            lxa_archive_iter_set_mime(LXAArchive *, LXAArchiveIter *, const gchar *);
void            lxa_archive_iter_set_prop_str(LXAArchive *, LXAArchiveIter *, guint, const gchar *);
void            lxa_archive_iter_set_prop_uint(LXAArchive *, LXAArchiveIter *, guint, guint);
void            lxa_archive_iter_set_prop_uint64(LXAArchive *, LXAArchiveIter *, guint, guint64);
void            lxa_archive_iter_set_prop_value(LXAArchive *, LXAArchiveIter *, guint, const GValue *);
void            lxa_archive_iter_set_props(LXAArchive *, LXAArchiveIter *, ...);

const gchar    *lxa_archive_iter_get_filename(const LXAArchive *, const LXAArchiveIter *);
const gchar    *lxa_archive_iter_get_mime(const LXAArchive *, const LXAArchiveIter *);
const gchar    *lxa_archive_iter_get_prop_str(const LXAArchive *, const LXAArchiveIter *, guint);
guint           lxa_archive_iter_get_prop_uint(const LXAArchive *, const LXAArchiveIter *, guint);
guint64         lxa_archive_iter_get_prop_uint64(const LXAArchive *, const LXAArchiveIter *, guint);
gboolean        lxa_archive_iter_get_prop_value(const LXAArchive *, const LXAArchiveIter *, guint, GValue *);

LXAArchiveIter *lxa_archive_add_file(LXAArchive *, const gchar *);
GType           lxa_archive_get_property_type(LXAArchive *, guint);

G_END_DECLS

#endif /* __LIBXARCHIVER_ARCHIVE_H__ */
