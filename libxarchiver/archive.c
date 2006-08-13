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

#define EXO_API_SUBJECT_TO_CHANGE

#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h> 
#include <thunar-vfs/thunar-vfs.h>
#include "archive.h"
#include "archive-support.h"

#include "internals.h"


static void
lxa_archive_class_init(LXAArchiveClass *archive_class);

static void
lxa_archive_init(LXAArchive *archive);

static void
lxa_archive_finalize(GObject *object);


GType
lxa_archive_get_type ()
{
	static GType lxa_archive_type = 0;

 	if (!lxa_archive_type)
	{
 		static const GTypeInfo lxa_archive_info = 
		{
			sizeof (LXAArchiveClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_archive_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXAArchive),
			0,
			(GInstanceInitFunc) lxa_archive_init,
			NULL
		};

		lxa_archive_type = g_type_register_static (G_TYPE_OBJECT, "LXAArchive", &lxa_archive_info, 0);
	}
	return lxa_archive_type;
}

static void
lxa_archive_class_init(LXAArchiveClass *archive_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(archive_class);

	object_class->finalize = lxa_archive_finalize;
}

static void
lxa_archive_init(LXAArchive *archive)
{
}

static void
lxa_archive_finalize(GObject *object)
{
	LXAArchive *archive = LXA_ARCHIVE(object);
	if(archive->path)
		g_free(archive->path);
}

LXAArchive *
lxa_archive_new(gchar *path, gchar *mime)
{
	LXAArchive *archive;
	ThunarVfsMimeInfo *mime_info;

	archive = g_object_new(lxa_archive_get_type(), NULL);
	if(path)
		archive->path = g_strdup(path);
	else
		archive->path = NULL;

	if(!mime)
		mime_info = thunar_vfs_mime_database_get_info_for_file(lxa_mime_database, archive->path, g_path_get_basename(archive->path));
	else
		mime_info = thunar_vfs_mime_info_new(mime, -1);
	
	archive->mime = g_strdup(thunar_vfs_mime_info_get_name(mime_info));
	g_print("%s\n", archive->mime);

	return archive;
}
