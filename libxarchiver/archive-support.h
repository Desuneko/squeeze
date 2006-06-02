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
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __LIBXARCHIVER_ARCHIVE_SUPPORT_H__
#define __LIBXARCHIVER_ARCHIVE_SUPPORT_H__

G_BEGIN_DECLS


#define LXA_TYPE_ARCHIVE_SUPPORT lxa_archive_support_get_type()

#define LXA_ARCHIVE_SUPPORT(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LXA_TYPE_ARCHIVE_SUPPORT,      \
			LXAArchiveSupport))

#define LXA_IS_ARCHIVE_SUPPORT(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LXA_TYPE_ARCHIVE_SUPPORT))

#define LXA_ARCHIVE_SUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LXA_TYPE_ARCHIVE_SUPPORT,      \
			LXAArchiveSupportClass))

#define LXA_IS_ARCHIVE_SUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LXA_TYPE_ARCHIVE_SUPPORT))

typedef struct _LXAArchiveSupport LXAArchiveSupport;

struct _LXAArchiveSupport
{
	GObject parent;
	LXAArchiveType type;
	gchar *id;
	gint (*add)     (LXAArchive *);
	gint (*extract) (LXAArchive *);
};

typedef struct _LXAArchiveSupportClass LXAArchiveSupportClass;

struct _LXAArchiveSupportClass
{
	GObjectClass parent;
}; 

#define              LXA_A_S_ADD_COMPLETE     0
#define              LXA_A_S_EXTRACT_COMPLETE 1

GType                lxa_archive_support_get_type(void);
LXAArchiveSupport *  lxa_archive_support_new();
void                 lxa_archive_support_emit_signal(LXAArchiveSupport *support, guint signal_id, LXAArchive *archive);

G_END_DECLS

#endif /* __LIBXARCHIVER_ARCHIVE_SUPPORT_H__ */
