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
#ifndef __LIBXARCHIVER_ARCHIVESUPPORT_H__
#define __LIBXARCHIVER_ARCHIVESUPPORT_H__

G_BEGIN_DECLS


#define LXA_TYPE_ARCHIVESUPPORT lxa_archivesupport_get_type()

#define LXA_ARCHIVESUPPORT(obj)         ( \
		G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
			LXA_TYPE_ARCHIVESUPPORT,      \
			LXAArchiveSupport))

#define LXA_IS_ARCHIVESUPPORT(obj)      ( \
		G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
			LXA_TYPE_ARCHIVESUPPORT))

#define LXA_ARCHIVESUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_CAST ((klass),     \
			LXA_TYPE_ARCHIVESUPPORT,      \
			LXAArchiveSupportClass))

#define LXA_IS_ARCHIVESUPPORT_CLASS(klass) ( \
		G_TYPE_CHECK_CLASS_TYPE ((klass),        \
			LXA_TYPE_ARCHIVESUPPORT))

typedef struct _LXAArchiveSupport LXAArchiveSupport;

struct _LXAArchiveSupport
{
	GObject parent;
	LXAArchiveType type;
};

typedef struct _LXAArchiveSupportClass LXAArchiveSupportClass;

struct _LXAArchiveSupportClass
{
	GObjectClass parent;
}; 

GType                lxa_archivesupport_get_type(void);
LXAArchiveSupport *  lxa_archivesupport_new();

G_END_DECLS

#endif /* __LIBXARCHIVER_ARCHIVESUPPORT_H__ */
