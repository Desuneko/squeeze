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
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <glib.h>
#include <glib-object.h>
#include <libintl.h>
#include "archive.h"
#include "archive-support.h"
#include "archive-support-zip.h"

#include "compression-support.h"

#include "internals.h"

#define _(String) gettext(String)

gint
lxa_archive_support_zip_add(LXAArchive *archive);

gint
lxa_archive_support_zip_extract(LXAArchive *archive);

gint
lxa_archive_support_zip_remove(LXAArchive *archive);

gint
lxa_archive_support_zip_view(LXAArchive *archive);

void
lxa_archive_support_zip_child_watch_func(GPid pid, gint status, gpointer data);

gboolean
lxa_archive_support_zip_view_func(GIOChannel *source, GIOCondition condition, gpointer data);

void
lxa_archive_support_zip_init(LXAArchiveSupportZip *support);
void
lxa_archive_support_zip_class_init(LXAArchiveSupportZipClass *supportclass);

GType
lxa_archive_support_zip_get_type ()
{
	static GType lxa_archive_support_zip_type = 0;

 	if (!lxa_archive_support_zip_type)
	{
 		static const GTypeInfo lxa_archive_support_zip_info = 
		{
			sizeof (LXAArchiveSupportZipClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_archive_support_zip_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXAArchiveSupportZip),
			0,
			(GInstanceInitFunc) lxa_archive_support_zip_init,
		};

		lxa_archive_support_zip_type = g_type_register_static (LXA_TYPE_ARCHIVE_SUPPORT, "LXAArchiveSupportZip", &lxa_archive_support_zip_info, 0);
	}
	return lxa_archive_support_zip_type;
}

void
lxa_archive_support_zip_init(LXAArchiveSupportZip *support)
{
	LXAArchiveSupport *archive_support = LXA_ARCHIVE_SUPPORT(support);

	archive_support->id = "Zip";
	archive_support->type = LXA_ARCHIVETYPE_ZIP;

	archive_support->add = lxa_archive_support_zip_add;
	archive_support->extract = lxa_archive_support_zip_extract;
	archive_support->remove = lxa_archive_support_zip_remove;
	archive_support->view = lxa_archive_support_zip_view;
	archive_support->column_nr = 8;
	archive_support->column_names = g_new0(gchar *, archive_support->column_nr);
	archive_support->column_types = g_new0(GType , archive_support->column_nr);
	archive_support->column_names[0] = _("Filename");
	archive_support->column_names[1] = _("Original");
	archive_support->column_names[2] = _("Method");
	archive_support->column_names[3] = _("Compressed");
	archive_support->column_names[4] = _("Ratio");
	archive_support->column_names[5] = _("Date");
	archive_support->column_names[6] = _("Time");
	archive_support->column_names[7] = _("CRC-32");
	archive_support->column_types[0] = G_TYPE_STRING;
	archive_support->column_types[1] = G_TYPE_UINT64;
	archive_support->column_types[2] = G_TYPE_STRING;
	archive_support->column_types[3] = G_TYPE_UINT64;
	archive_support->column_types[4] = G_TYPE_STRING;
	archive_support->column_types[5] = G_TYPE_STRING;
	archive_support->column_types[6] = G_TYPE_STRING;
	archive_support->column_types[7] = G_TYPE_STRING;
}

void
lxa_archive_support_zip_class_init(LXAArchiveSupportZipClass *supportclass)
{
	/*
	GObjectClass *gobject_class = G_OBJECT_CLASS (supportclass);
	LXAArchiveSupportZipClass *klass = LXA_ARCHIVE_SUPPORT_ZIP_CLASS (supportclass);
	*/
}

LXAArchiveSupport*
lxa_archive_support_zip_new()
{
	LXAArchiveSupportZip *support;

	support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT_ZIP, NULL);
	
	return LXA_ARCHIVE_SUPPORT(support);
}

gint
lxa_archive_support_zip_add(LXAArchive *archive)
{
	gchar *command;
	GSList *files = archive->tmp_data;

	command = g_strconcat("zip -r ", archive->path, " ", files->data, NULL);

	if(lxa_execute(command, archive, lxa_archive_support_zip_child_watch_func, NULL, NULL, NULL))
		return 1;

	return 0;
}

gint
lxa_archive_support_zip_extract(LXAArchive *archive)
{
	/*
	 * TODO: use extract- options
	 */
	gchar *command;
	GSList *files = archive->tmp_data;

	if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		command = g_strconcat("unzip -o ", archive->path, " -d ", files->data, NULL);
	else
		return 1;

	if(lxa_execute(command, archive, lxa_archive_support_zip_child_watch_func, NULL, NULL, NULL))
		return 1;
	return 0;
}

gint
lxa_archive_support_zip_remove(LXAArchive *archive)
{
	gchar *command;

	GSList *files = archive->tmp_data;
	if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		command = g_strconcat("zip -d ", archive->path, " ", files->data, NULL);
	else
		return 2;

	if(lxa_execute(command, archive, lxa_archive_support_zip_child_watch_func, NULL, NULL, NULL))
		return 1;

	return 0;

}

gint
lxa_archive_support_zip_view(LXAArchive *archive)
{
	gchar *command;
	if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		command = g_strconcat("unzip -vl -qq ", archive->path, NULL);
	else
		return 2;

	if(lxa_execute(command, archive, lxa_archive_support_zip_child_watch_func, NULL, lxa_archive_support_zip_view_func, NULL))
		return 1;
	return 0;
}

void
lxa_archive_support_zip_child_watch_func(GPid pid, gint status, gpointer data)
{
	GSList *find_result;
	LXACompressionSupport *compression_support;
	LXAArchive *archive = data;

	archive->child_pid = 0;
	if((archive->compression != LXA_COMPRESSIONTYPE_NONE) && (archive->status != LXA_ARCHIVESTATUS_EXTRACT))
	{
		find_result = g_slist_find_custom(lxa_compression_support_list, &(archive->compression), lookup_compression_support);
		if(find_result)
		{
			compression_support = find_result->data;
			compression_support->compress(archive);
		}
	} else
		lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_IDLE);
}

gboolean
lxa_archive_support_zip_view_func(GIOChannel *ioc, GIOCondition condition, gpointer data)
{
	gchar *line = NULL;
	GIOStatus status = 0;
	GError *error = NULL;

	if (condition & (G_IO_IN | G_IO_PRI) )
	{
		status = g_io_channel_read_line(ioc, &line, NULL,NULL,&error);
	}
	else if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		return FALSE;
	}
	return TRUE;
}
