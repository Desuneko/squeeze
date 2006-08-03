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
#include "archive-support-gnu-tar.h"

#include "compression-support.h"

#include "internals.h"

#define _(String) gettext(String)

enum
{
	LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_OVERWRITE = 1,
	LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_TOUCH,
	LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_STRIP
};

gint
lxa_archive_support_gnu_tar_add(LXAArchive *archive);

gint
lxa_archive_support_gnu_tar_extract(LXAArchive *archive);

gint
lxa_archive_support_gnu_tar_remove(LXAArchive *archive);

gint
lxa_archive_support_gnu_tar_view(LXAArchive *archive);

void
lxa_archive_support_gnu_tar_child_watch_func(GPid pid, gint status, gpointer data);

void
lxa_archive_support_gnu_tar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
void
lxa_archive_support_gnu_tar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

gboolean
lxa_archive_support_gnu_tar_view_func(GIOChannel *source, GIOCondition condition, gpointer data);

void
lxa_archive_support_gnu_tar_init(LXAArchiveSupportGnuTar *support);
void
lxa_archive_support_gnu_tar_class_init(LXAArchiveSupportGnuTarClass *supportclass);

GType
lxa_archive_support_gnu_tar_get_type ()
{
	static GType lxa_archive_support_gnu_tar_type = 0;

 	if (!lxa_archive_support_gnu_tar_type)
	{
 		static const GTypeInfo lxa_archive_support_gnu_tar_info = 
		{
			sizeof (LXAArchiveSupportGnuTarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lxa_archive_support_gnu_tar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LXAArchiveSupportGnuTar),
			0,
			(GInstanceInitFunc) lxa_archive_support_gnu_tar_init,
		};

		lxa_archive_support_gnu_tar_type = g_type_register_static (LXA_TYPE_ARCHIVE_SUPPORT, "LXAArchiveSupportGnuTar", &lxa_archive_support_gnu_tar_info, 0);
	}
	return lxa_archive_support_gnu_tar_type;
}

void
lxa_archive_support_gnu_tar_init(LXAArchiveSupportGnuTar *support)
{
	LXAArchiveSupport *archive_support = LXA_ARCHIVE_SUPPORT(support);

	archive_support->id = "Gnu Tar";
	archive_support->type = LXA_ARCHIVETYPE_TAR;

	archive_support->add = lxa_archive_support_gnu_tar_add;
	archive_support->extract = lxa_archive_support_gnu_tar_extract;
	archive_support->remove = lxa_archive_support_gnu_tar_remove;
	archive_support->view = lxa_archive_support_gnu_tar_view;
	archive_support->column_nr = 6;
	archive_support->column_names = g_new0(gchar *, archive_support->column_nr);
	archive_support->column_types = g_new0(GType , archive_support->column_nr);
	archive_support->column_names[0] = _("Filename");
	archive_support->column_names[1] = _("Permissions");
	archive_support->column_names[2] = _("Owner/Group");
	archive_support->column_names[3] = _("Size");
	archive_support->column_names[4] = _("Date");
	archive_support->column_names[5] = _("Time");
	archive_support->column_types[0] = G_TYPE_STRING;
	archive_support->column_types[1] = G_TYPE_STRING;
	archive_support->column_types[2] = G_TYPE_STRING;
	archive_support->column_types[3] = G_TYPE_UINT;
	archive_support->column_types[4] = G_TYPE_STRING;
	archive_support->column_types[5] = G_TYPE_STRING;
}

void
lxa_archive_support_gnu_tar_class_init(LXAArchiveSupportGnuTarClass *supportclass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (supportclass);
	GParamSpec *pspec = NULL;
	LXAArchiveSupportGnuTarClass *klass = LXA_ARCHIVE_SUPPORT_GNU_TAR_CLASS (supportclass);

	object_class->set_property = lxa_archive_support_gnu_tar_set_property;
	object_class->get_property = lxa_archive_support_gnu_tar_get_property;

	pspec = g_param_spec_boolean("extract-overwrite",
		"Overwrite exisiting files",
		"Overwrite existing files on extraction",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_OVERWRITE, pspec);

	pspec = g_param_spec_boolean("extract-touch",
		"Touch files",
		"Touch files",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_TOUCH, pspec);

	pspec = g_param_spec_uint("extract-strip",
		"Strip directories",
		"Strip directories",
		0,
		128,
		0,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_STRIP, pspec);
}

LXAArchiveSupport*
lxa_archive_support_gnu_tar_new()
{
	guint i;
	LXAArchiveSupportGnuTar *support;

	support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT_GNU_TAR, NULL);

	return LXA_ARCHIVE_SUPPORT(support);
}

gint
lxa_archive_support_gnu_tar_add(LXAArchive *archive)
{
	gchar *command;
	GSList *files = archive->tmp_data;

	if(archive->compression == LXA_COMPRESSIONTYPE_NONE)
	{
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar -rf ", archive->path, " ", files->data, NULL);
		else
			command = g_strconcat("tar -cf ", archive->path, " ", files->data, NULL);
	}
	else
	{
		if(g_file_test(archive->tmp_file, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar -rf ", archive->tmp_file, " ", files->data, NULL);
		else
			command = g_strconcat("tar -cf ", archive->tmp_file, " ", files->data, NULL);
	}

	if(lxa_execute(command, archive, lxa_archive_support_gnu_tar_child_watch_func, NULL, NULL, NULL))
		return 1;

	return 0;
}

gint
lxa_archive_support_gnu_tar_extract(LXAArchive *archive)
{
	/*
	 * TODO: use extract- options
	 */
	gchar *command;
	GSList *files = archive->tmp_data;

	if(archive->compression == LXA_COMPRESSIONTYPE_NONE)
	{
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar -xf ", archive->path, " -C ", files->data, NULL);
		else
			return 1;
	}
	else
	{
		if(g_file_test(archive->tmp_file, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar -xf ", archive->tmp_file, " -C ", files->data, NULL);
		else
			return 1;
	}

	if(lxa_execute(command, archive, lxa_archive_support_gnu_tar_child_watch_func, NULL, NULL, NULL))
		return 1;
	return 0;
}

gint
lxa_archive_support_gnu_tar_remove(LXAArchive *archive)
{
	gchar *command;

	GSList *files = archive->tmp_data;

	if(archive->compression == LXA_COMPRESSIONTYPE_NONE)
	{
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar --delete -f ", archive->path, " ", files->data, NULL);
		else
			return 2;
	}
	else
	{
		if(g_file_test(archive->tmp_file, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar --delete -f ", archive->tmp_file, " ", files->data, NULL);
		else
			return 2;
	}

	if(lxa_execute(command, archive, lxa_archive_support_gnu_tar_child_watch_func, NULL, NULL, NULL))
		return 1;

	return 0;

}

gint
lxa_archive_support_gnu_tar_view(LXAArchive *archive)
{
	gchar *command;
	if(archive->compression == LXA_COMPRESSIONTYPE_NONE)
	{
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar --list -f ", archive->path, NULL);
		else
			return 2;
	}
	else
	{
		if(g_file_test(archive->tmp_file, G_FILE_TEST_EXISTS))
			command = g_strconcat("tar --list -f ", archive->tmp_file, NULL);
		else
			return 2;
	}

	if(lxa_execute(command, archive, lxa_archive_support_gnu_tar_child_watch_func, NULL, lxa_archive_support_gnu_tar_view_func, NULL))
		return 1;
	return 0;
}

void
lxa_archive_support_gnu_tar_child_watch_func(GPid pid, gint status, gpointer data)
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

void
lxa_archive_support_gnu_tar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_OVERWRITE:
			LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_overwrite = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_TOUCH:
			LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_touch = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_STRIP:
			LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_strip = g_value_get_uint(value);
			break;
	}
}

void
lxa_archive_support_gnu_tar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{

}

gboolean
lxa_archive_support_gnu_tar_view_func(GIOChannel *ioc, GIOCondition condition, gpointer data)
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
