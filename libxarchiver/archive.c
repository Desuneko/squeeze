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

#include <stdio.h>
#include <signal.h>
#include <glib.h>
#include <glib-object.h> 
#include "archive.h"
#include "archive-support.h"
#include "compression-support.h"

#include "internals.h"


static void
lxa_archive_class_init(LXAArchiveClass *archive_class);

static void
lxa_archive_init(LXAArchive *archive);

static void
lxa_archive_finalize(GObject *object);


static guint lxa_archive_signals[1];

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

	lxa_archive_signals[0] = g_signal_new("lxa_status_changed",
			G_TYPE_FROM_CLASS(archive_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER,
			NULL);
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
lxa_archive_new(gchar *path, LXAArchiveType type, LXACompressionType compression)
{
	LXAArchive *archive;

	archive = g_object_new(lxa_archive_get_type(), NULL);
	if(path)
		archive->path = g_strdup(path);
	else
		archive->path = NULL;

	archive->compression = compression;
	archive->type = type;
	
	return archive;
}

void
lxa_archive_set_status(LXAArchive *archive, LXAArchiveStatus status)
{
	archive->status = status;
	g_signal_emit(G_OBJECT(archive), lxa_archive_signals[0], 0, archive);
}

gint
lxa_archive_compress(LXAArchive *archive)
{
}

gint
lxa_archive_set_compression(LXAArchive *archive, LXACompressionType compression)
{
	if(archive->compression != LXA_COMPRESSIONTYPE_NONE)
	{
		lxa_archive_decompress(archive);
		archive->compression = compression;
	}
	lxa_archive_compress(archive);
}

gint
lxa_archive_decompress(LXAArchive *archive)
{
}

gint
lxa_archive_add(LXAArchive *archive, GSList *files)
{
	GSList *find_result;
	LXACompressionSupport *compression_support;
	LXAArchiveSupport *archive_support;

	if(archive->status != LXA_ARCHIVESTATUS_IDLE)
	{
		g_debug("archive is buzy...");
		return 1;
	}
	lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_ADD);
	
	archive->tmp_data = files;

	if(archive->compression != LXA_COMPRESSIONTYPE_NONE)
	{
		find_result = g_slist_find_custom(lxa_compression_support_list, &(archive->compression), lookup_compression_support);
		if(find_result)
		{
			compression_support = find_result->data;
			archive->tmp_file = g_strconcat(lxa_tmp_dir, "/xarchiver-XXXXXX" , NULL);
			g_mkstemp(archive->tmp_file);
			/* since we only need the filename: we unlink it */
			g_unlink(archive->tmp_file);

			/* Check if the archive already exists */
			if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
				compression_support->decompress(archive);

			else
			{
				find_result = g_slist_find_custom(lxa_archive_support_list, &(archive->type), lookup_archive_support);
				if(find_result)
				{
					archive_support = find_result->data;
					archive_support->add(archive);
				} else
					return 2;
			}
		} else
		{
			g_critical("Could not find compression-support");
			return 2;
		}
	}
	else
	{
		find_result = g_slist_find_custom(lxa_archive_support_list, &(archive->type), lookup_archive_support);
		if(find_result)
		{
			archive_support = find_result->data;
			archive_support->add(archive);
		} else
			return 2;
	}
}

gint
lxa_archive_extract(LXAArchive *archive, GSList *files, gchar *destination)
{
	if(!destination)
		return 1;
	GSList *find_result;
	LXACompressionSupport *compression_support;
	LXAArchiveSupport *archive_support;

	if(archive->status != LXA_ARCHIVESTATUS_IDLE)
	{
		g_debug("archive is buzy...");
		return 1;
	}
	lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_EXTRACT);
	
	archive->tmp_data = g_slist_prepend(files, destination);

	if(archive->compression != LXA_COMPRESSIONTYPE_NONE)
	{
		find_result = g_slist_find_custom(lxa_compression_support_list, &(archive->compression), lookup_compression_support);
		if(find_result)
		{
			compression_support = find_result->data;
			archive->tmp_file = g_strconcat(lxa_tmp_dir, "/xarchiver-XXXXXX" , NULL);
			g_mkstemp(archive->tmp_file);
			/* since we only need the filename: we unlink it */
			g_unlink(archive->tmp_file);

			/* Check if the archive already exists */
			if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
				compression_support->decompress(archive);

			else
			{
				find_result = g_slist_find_custom(lxa_archive_support_list, &(archive->type), lookup_archive_support);
				if(find_result)
				{
					archive_support = find_result->data;
					archive_support->extract(archive);
				} else
					return 2;
			}
		} else
		{
			g_critical("Could not find compression-support");
			return 2;
		}
	}
	else
	{
		find_result = g_slist_find_custom(lxa_archive_support_list, &(archive->type), lookup_archive_support);
		if(find_result)
		{
			archive_support = find_result->data;
			archive_support->extract(archive);
		} else
			return 2;
	}

}

gint
lxa_archive_remove(LXAArchive *archive, GSList *files)
{
	GSList *find_result;
	LXACompressionSupport *compression_support;
	LXAArchiveSupport *archive_support;

	if(archive->status != LXA_ARCHIVESTATUS_IDLE)
	{
		g_debug("archive is buzy...");
		return 1;
	}
	lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_REMOVE);
	
	archive->tmp_data = files;

	if(archive->compression != LXA_COMPRESSIONTYPE_NONE)
	{
		find_result = g_slist_find_custom(lxa_compression_support_list, &(archive->compression), lookup_compression_support);
		if(find_result)
		{
			compression_support = find_result->data;
			archive->tmp_file = g_strconcat(lxa_tmp_dir, "/xarchiver-XXXXXX" , NULL);
			g_mkstemp(archive->tmp_file);
			/* since we only need the filename: we unlink it */
			g_unlink(archive->tmp_file);

			/* Check if the archive already exists */
			if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
				compression_support->decompress(archive);

			else
			{
				find_result = g_slist_find_custom(lxa_archive_support_list, &(archive->type), lookup_archive_support);
				if(find_result)
				{
					archive_support = find_result->data;
					archive_support->remove(archive);
				} else
					return 2;
			}
		} else
		{
			g_critical("Could not find compression-support");
			return 2;
		}
	}
	else
	{
		find_result = g_slist_find_custom(lxa_archive_support_list, &(archive->type), lookup_archive_support);
		if(find_result)
		{
			archive_support = find_result->data;
			archive_support->remove(archive);
		} else
			return 2;
	}
}

gint
lxa_archive_stop(LXAArchive *archive)
{
	if(kill(archive->child_pid, SIGABRT) < 0)
		return 1;
	lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_USERBREAK);
	archive->child_pid = 0;
}
