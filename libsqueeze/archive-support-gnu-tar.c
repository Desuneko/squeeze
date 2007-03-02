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
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <thunar-vfs/thunar-vfs.h>

#include "libsqueeze.h"
#include "libsqueeze-module.h"
#include "archive-iter.h"
#include "archive-command.h"
#include "archive.h"
#include "archive-support.h"
#include "archive-support-gnu-tar.h"

#include "internals.h"

#ifndef GNU_TAR_APP_NAME
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#define GNU_TAR_APP_NAME "gtar"
#else
#define GNU_TAR_APP_NAME "tar"
#endif
#endif


enum
{
	LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_OVERWRITE = 1,
	LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_TOUCH,
	LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_STRIP,
	LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_KEEP_NEW,
	LSQ_ARCHIVE_SUPPORT_GNU_TAR_ADD_MODE,

	LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_SIZE,
	LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_DATE,
	LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_TIME,
	LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_OWNER,
	LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_RIGHTS
};

#define LSQ_ARCHIVE_TEMP_FILE "gnu_tar_temp_file"
#define LSQ_ARCHIVE_FILES "files"

void
lsq_archive_support_gnu_tar_init(LSQArchiveSupportGnuTar *support);
void
lsq_archive_support_gnu_tar_class_init(LSQArchiveSupportGnuTarClass *supportclass);

gboolean
lsq_archive_support_gnu_tar_refresh_parse_output(LSQArchiveCommand *archive_command);
gboolean
lsq_archive_support_gnu_tar_compress_parse_output(LSQArchiveCommand *archive_command);
gboolean
lsq_archive_support_gnu_tar_decompress_parse_output(LSQArchiveCommand *archive_command);

void
lsq_archive_support_gnu_tar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
void
lsq_archive_support_gnu_tar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gint lsq_archive_support_gnu_tar_add(LSQArchive *, GSList *);
static gint lsq_archive_support_gnu_tar_extract(LSQArchive *, const gchar *, GSList *);
static gint lsq_archive_support_gnu_tar_remove(LSQArchive *, GSList *);
static gint lsq_archive_support_gnu_tar_refresh(LSQArchive *);

GType
lsq_archive_support_gnu_tar_get_type ()
{
	static GType lsq_archive_support_gnu_tar_type = 0;

 	if (!lsq_archive_support_gnu_tar_type)
	{
 		static const GTypeInfo lsq_archive_support_gnu_tar_info = 
		{
			sizeof (LSQArchiveSupportGnuTarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_archive_support_gnu_tar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQArchiveSupportGnuTar),
			0,
			(GInstanceInitFunc) lsq_archive_support_gnu_tar_init,
		};

		lsq_archive_support_gnu_tar_type = g_type_register_static (LSQ_TYPE_ARCHIVE_SUPPORT, "LSQArchiveSupportGnuTar", &lsq_archive_support_gnu_tar_info, 0);
	}
	return lsq_archive_support_gnu_tar_type;
}

void
lsq_archive_support_gnu_tar_init(LSQArchiveSupportGnuTar *support)
{
	/* TODO: free return value of g_find_program_in_path */
	LSQArchiveSupport *archive_support = LSQ_ARCHIVE_SUPPORT(support);
	gchar *program_path = NULL;

	archive_support->id = "Gnu Tar";

	support->_add_mode = g_strdup("");

	program_path = g_find_program_in_path(GNU_TAR_APP_NAME);
	if(program_path)
	{
		lsq_archive_support_add_mime(archive_support, "application/x-tar");
		/* Check for existence of compress -- required for x-tarz */
		program_path = g_find_program_in_path("compress");
		if(program_path)
		{
			lsq_archive_support_add_mime(archive_support, "application/x-tarz");
			g_free(program_path);
		}
		/* Check for existence of gzip -- required for x-compressed-tar*/
		program_path = g_find_program_in_path("gzip");
		if(program_path)
		{
			lsq_archive_support_add_mime(archive_support, "application/x-compressed-tar");
			g_free(program_path);
		}
		/* Check for existence of bzip2 -- required for x-bzip-compressed-tar */
		/*                                          and x-bzip2-compressed-tar */
		program_path = g_find_program_in_path("bzip2");
		if(program_path)
		{
			lsq_archive_support_add_mime(archive_support, "application/x-bzip-compressed-tar");
			lsq_archive_support_add_mime(archive_support, "application/x-bzip2-compressed-tar");
			g_free(program_path);
		}
		/* Check for existence of lzop -- required for x-tzo */
		program_path = g_find_program_in_path("lzop");
		if(program_path)
		{
			lsq_archive_support_add_mime(archive_support, "application/x-tzo");
			g_free(program_path);
		}
	}

	archive_support->add = lsq_archive_support_gnu_tar_add;
	archive_support->extract = lsq_archive_support_gnu_tar_extract;
	archive_support->remove = lsq_archive_support_gnu_tar_remove;
	archive_support->refresh = lsq_archive_support_gnu_tar_refresh;
}

void
lsq_archive_support_gnu_tar_class_init(LSQArchiveSupportGnuTarClass *supportclass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (supportclass);
	GParamSpec *pspec = NULL;

	object_class->set_property = lsq_archive_support_gnu_tar_set_property;
	object_class->get_property = lsq_archive_support_gnu_tar_get_property;

	pspec = g_param_spec_boolean("extract-overwrite",
		_("Overwrite existing files"),
		_("Overwrite existing files on extraction"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_OVERWRITE, pspec);

	pspec = g_param_spec_boolean("extract-touch",
		_("Touch files"),
		_("Touch files"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_TOUCH, pspec);

	pspec = g_param_spec_uint("extract-strip",
		_("Strip directories"),
		_("Strip directories"),
		0,
		128,
		0,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_STRIP, pspec);

	pspec = g_param_spec_boolean("extract-keep-new",
		_("Keep newer files"),
		_("Do not overwrite files newer than those in the archive"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_KEEP_NEW, pspec);

	pspec = g_param_spec_string("add-mode",
		_("Override permissions"),
		_("Override permissions"),
		"",
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_GNU_TAR_ADD_MODE, pspec);

	pspec = g_param_spec_boolean("view-size",
		_("Size"),
		_("View filesize"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_SIZE, pspec);

	pspec = g_param_spec_boolean("view-rights",
		_("Permissions"),
		_("View permissions"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_RIGHTS, pspec);

	pspec = g_param_spec_boolean("view-owner",
		_("Owner/Group"),
		_("View owner/group"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_OWNER, pspec);

	pspec = g_param_spec_boolean("view-date",
		_("Date"),
		_("View date"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_DATE, pspec);

	pspec = g_param_spec_boolean("view-time",
		_("Time"),
		_("View time"),
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_TIME, pspec);

}

LSQArchiveSupport*
lsq_archive_support_gnu_tar_new()
{
	LSQArchiveSupportGnuTar *support;

	support = g_object_new(LSQ_TYPE_ARCHIVE_SUPPORT_GNU_TAR, 
	                       "view-time", TRUE, 
												 "view-date", TRUE,
												 "view-owner", TRUE,
												 "view-rights", TRUE,
												 "view-size", TRUE,
												 NULL);

	return LSQ_ARCHIVE_SUPPORT(support);
}

static gint
lsq_archive_support_gnu_tar_add(LSQArchive *archive, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_GNU_TAR(archive->support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		LSQArchiveCommand *archive_command = NULL;
		gchar *options = NULL;
		gchar *tmp_file = NULL;
		gchar *files = lsq_concat_filenames(filenames);
		gchar *command_skeleton = NULL;
		if(!archive->file_info) /* FIXME */
		{
			command_skeleton = g_strconcat(GNU_TAR_APP_NAME, " %3$s -c -f %1$s %2$s", NULL);

			if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-tarz"))
				options = "-Z";
			if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-compressed-tar"))
				options = "-z";
			if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-bzip-compressed-tar"))
				options = "-j";
			if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-tzo"))
				options = "--use-compress-program=lzop";

			archive_command = lsq_archive_command_new("", archive, command_skeleton, FALSE, TRUE);
			g_object_set_data(G_OBJECT(archive_command), "files", g_strdup(files));
			g_object_set_data(G_OBJECT(archive_command), "options", g_strdup(options));
			g_free(command_skeleton);
		}
		else
		{
			if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-tarz"))
				command_skeleton = g_strdup("uncompress -c %1$s");
			if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-compressed-tar"))
				command_skeleton = g_strdup("gunzip -c %1$s");
			if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-bzip-compressed-tar"))
				command_skeleton = g_strdup("bunzip2 -c %1$s");
			if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-tzo"))
				command_skeleton = g_strdup("lzop -dc %1$s");
			if(command_skeleton)
			{
				archive_command = lsq_archive_command_new("", archive, command_skeleton, FALSE, TRUE);
				lsq_archive_command_set_parse_func(archive_command, 1, lsq_archive_support_gnu_tar_decompress_parse_output);
				tmp_file = g_strconcat(lsq_tmp_dir, "/squeeze-XXXXXX.tar" , NULL);
				g_mkstemp(tmp_file);
				g_object_set_data(G_OBJECT(archive_command), LSQ_ARCHIVE_TEMP_FILE, tmp_file);
				g_free(command_skeleton);
			}

			command_skeleton = g_strconcat(GNU_TAR_APP_NAME, " %3$s -r -f %1$s %2$s", NULL);
			archive_command = lsq_archive_command_new("", archive, command_skeleton, FALSE, TRUE);
			if(tmp_file)
				g_object_set_data(G_OBJECT(archive_command), "archive", g_strdup(tmp_file));
			g_object_set_data(G_OBJECT(archive_command), "files", g_strdup(files));
			g_object_set_data(G_OBJECT(archive_command), "options", g_strdup(options));
			g_free(command_skeleton);
			command_skeleton = NULL;

			if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-tarz"))
				command_skeleton = g_strdup("compress -c %1$s");
			if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-compressed-tar"))
				command_skeleton = g_strdup("gzip -c %1$s");
			if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-bzip-compressed-tar"))
				command_skeleton = g_strdup("bzip2 -c %1$s");
			if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-tzo"))
				command_skeleton = g_strdup("lzop -c %1$s");
			if(command_skeleton)
			{
				archive_command = lsq_archive_command_new("", archive, command_skeleton, FALSE, TRUE);
				lsq_archive_command_set_parse_func(archive_command, 1, lsq_archive_support_gnu_tar_compress_parse_output);
				if(tmp_file)
					g_object_set_data(G_OBJECT(archive_command), "archive", g_strdup(tmp_file));
				g_object_set_data(G_OBJECT(archive_command), LSQ_ARCHIVE_TEMP_FILE, g_strdup(tmp_file));
				g_free(command_skeleton);
			}
		}
		g_free(files);
		archive_command = lsq_archive_get_front_command(archive);
		if(archive_command)
		{
			lsq_archive_command_run(archive_command);
			g_object_unref(archive_command);
		}
		else
			return 1;
	}
	return 0;
}

static gint
lsq_archive_support_gnu_tar_extract(LSQArchive *archive, const gchar *extract_path, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_GNU_TAR(archive->support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		LSQArchiveCommand *archive_command = NULL;
		gchar *options = NULL;
		gchar *files = lsq_concat_filenames(filenames);
		gchar *command_skeleton = g_strconcat(GNU_TAR_APP_NAME, " %3$s -x -f %1$s %2$s", NULL);

		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-tarz"))
			options = "-Z";
		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-compressed-tar"))
			options = "-z";
		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-bzip-compressed-tar"))
			options = "-j";
		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-tzo"))
			options = "--use-compress-program=lzop";

		archive_command = lsq_archive_command_new("", archive, command_skeleton, FALSE, TRUE);
		g_object_set_data(G_OBJECT(archive_command), "files", g_strdup(files));
		g_object_set_data(G_OBJECT(archive_command), "options", g_strdup(options));
		g_free(command_skeleton);
		g_free(files);
	}
	return 0;
}

static gint
lsq_archive_support_gnu_tar_remove(LSQArchive *archive, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_GNU_TAR(archive->support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		LSQArchiveCommand *archive_command = NULL;
		gchar *options = NULL;
		gchar *tmp_file = NULL;
		gchar *files = lsq_concat_filenames(filenames);
		gchar *command_skeleton = NULL;
		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-tarz"))
			command_skeleton = g_strdup("uncompress -c %1$s");
		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-compressed-tar"))
			command_skeleton = g_strdup("gunzip -c %1$s");
		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-bzip-compressed-tar"))
			command_skeleton = g_strdup("bunzip2 -c %1$s");
		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-tzo"))
			command_skeleton = g_strdup("lzop -dc %1$s");
		if(command_skeleton)
		{
			archive_command = lsq_archive_command_new("", archive, command_skeleton, FALSE, TRUE);
			lsq_archive_command_set_parse_func(archive_command, 1, lsq_archive_support_gnu_tar_decompress_parse_output);
			tmp_file = g_strconcat(lsq_tmp_dir, "/squeeze-XXXXXX.tar" , NULL);
			g_mkstemp(tmp_file);
			g_object_set_data(G_OBJECT(archive_command), LSQ_ARCHIVE_TEMP_FILE, tmp_file);
			g_free(command_skeleton);
		}

		command_skeleton = g_strconcat(GNU_TAR_APP_NAME, " %3$s -f %1$s --delete %2$s", NULL);
		archive_command = lsq_archive_command_new("", archive, command_skeleton, FALSE, TRUE);
		if(tmp_file)
			g_object_set_data(G_OBJECT(archive_command), "archive", g_strdup(tmp_file));
		g_object_set_data(G_OBJECT(archive_command), "files", g_strdup(files));
		g_object_set_data(G_OBJECT(archive_command), "options", g_strdup(options));
		g_free(command_skeleton);
		command_skeleton = NULL;

		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-tarz"))
			command_skeleton = g_strdup("compress -c %1$s");
		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-compressed-tar"))
			command_skeleton = g_strdup("gzip -c %1$s");
		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-bzip-compressed-tar"))
			command_skeleton = g_strdup("bzip2 -c %1$s");
		if(!g_strcasecmp(thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-tzo"))
			command_skeleton = g_strdup("lzop -c %1$s");
		if(command_skeleton)
		{
			archive_command = lsq_archive_command_new("", archive, command_skeleton, FALSE, TRUE);
			lsq_archive_command_set_parse_func(archive_command, 1, lsq_archive_support_gnu_tar_compress_parse_output);
			if(tmp_file)
				g_object_set_data(G_OBJECT(archive_command), "archive", g_strdup(tmp_file));
			g_object_set_data(G_OBJECT(archive_command), LSQ_ARCHIVE_TEMP_FILE, g_strdup(tmp_file));
			g_free(command_skeleton);
		}
		g_free(files);
		archive_command = lsq_archive_get_front_command(archive);
		if(archive_command)
		{
			lsq_archive_command_run(archive_command);
			g_object_unref(archive_command);
		}
		else
			return 1;
	}
	return 0;
}

static gint
lsq_archive_support_gnu_tar_refresh(LSQArchive *archive)
{
	guint i = 0;
	LSQArchiveCommand *archive_command = NULL;
	if(!LSQ_IS_ARCHIVE_SUPPORT_GNU_TAR(archive->support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		lsq_archive_clear_entry_property_types(archive);
		i = LSQ_ARCHIVE_PROP_USER;
		if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_rights)
		{
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Permissions"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_owner)
		{
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING,_("Owner/Group"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_size)
		{
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_UINT64, _("Size"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_date)
		{
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Date"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_time)
		{
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Time"));
			i++;
		}

		gchar *command_skeleton = g_strconcat(GNU_TAR_APP_NAME, " -tvvf %1$s", NULL);
		archive_command = lsq_archive_command_new("", archive, command_skeleton, TRUE, TRUE);
		g_free(command_skeleton);
		lsq_archive_command_set_parse_func(archive_command, 1, lsq_archive_support_gnu_tar_refresh_parse_output);
		lsq_archive_command_run(archive_command);
		g_object_unref(archive_command);
	}
	return 0;
}

gboolean
lsq_archive_support_gnu_tar_refresh_parse_output(LSQArchiveCommand *archive_command)
{
	gchar *line = NULL;
	gsize linesize = 0;
	GIOStatus status = G_IO_STATUS_NORMAL;
	LSQArchive *archive = archive_command->archive;
	guint64 size;
	gpointer props[6];
	gint n = 0, a = 0, i = 0;
	gchar *temp_filename = NULL;

	LSQArchiveIter *entry;

	status = lsq_archive_command_read_line(archive_command, 1, &line, &linesize, NULL);
	if (line == NULL)
	{
		if(status == G_IO_STATUS_AGAIN)
			return TRUE;
		else
			return FALSE;
	}

	if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_rights)
	{
		line[10] = '\0';
		props[i] = line;
		i++;
	}
	for(n=13; n < linesize; ++n)
		if(line[n] == ' ') break;

	if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_owner)
	{
		line[n] = '\0';
		props[i] = line+11;
		i++;
	}

	for(++n; n < linesize; ++n)
		if(line[n] >= '0' && line[n] <= '9') break;

	a = n;

	for(; n < linesize; ++n)
		if(line[n] == ' ') break;

	if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_size)
	{
		line[n] = '\0';
		size = g_ascii_strtoull(line + a, NULL, 0);
		props[i] = &size;
		i++;
	}

	a = ++n;

	for(; n < linesize; n++) // DATE
		if(line[n] == ' ') break;

	if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_date)
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}

	a = ++n;
	for (; n < linesize; n++) // TIME
		if (line[n] == ' ') break;

	if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_time)
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}
	n++;

	props[i] = NULL;
	gchar *temp = g_strrstr (&line[n],"->"); 
	if (temp ) 
	{ 
		temp[0] = '\0';
	}
	else
	{
		line[linesize-1] = '\0';
	}
	if(line[0] == 'd')
	{
		/* 1: Work around for gtar, which does not output 
		 * trailing slashes with directories. */
		/* 2: The line includes the newline character, this 
		 * would probably break on platforms that use 
		 * more then one character to indicate a line-end (\r\n) */
		if(line[linesize-2] != '/')
			temp_filename = g_strconcat(line + n, "/", NULL); 
		else
			temp_filename = g_strdup(line + n); 

		entry = lsq_archive_add_file(archive, temp_filename);
		g_free(temp_filename);
	}
	else
	{
		temp_filename = line + n; 

		entry = lsq_archive_add_file(archive, temp_filename);
	}

	lsq_archive_iter_set_propsv(entry, (gconstpointer*)props);
	g_free(line);
	return TRUE;
}

gboolean
lsq_archive_support_gnu_tar_decompress_parse_output(LSQArchiveCommand *archive_command)
{
	GIOStatus status = G_IO_STATUS_NORMAL;
	gchar *buf = g_new0(gchar, 1024);
	guint read = 0;
	GError *error = NULL;
	FILE *out_file;

	const gchar *out_filename = g_object_get_data(G_OBJECT(archive_command), LSQ_ARCHIVE_TEMP_FILE);

	out_file = fopen(out_filename, "ab");
	if(!out_file)
		return FALSE; 
	
	status = lsq_archive_command_read_bytes(archive_command, 1, buf, 1024, (gsize *)&read, &error);
	if(status == G_IO_STATUS_EOF)
	{
		fclose(out_file);
		return TRUE;
	}

	if(read)
	{
		fwrite(buf, 1, read, out_file);
	}
	fclose(out_file);
	g_free(buf);

	return TRUE;
}

gboolean
lsq_archive_support_gnu_tar_compress_parse_output(LSQArchiveCommand *archive_command)
{
	GIOStatus status = G_IO_STATUS_NORMAL;
	gchar *buf = g_new0(gchar, 1024);
	LSQArchive *archive = archive_command->archive;
	guint read = 0;
	GError *error = NULL;
	FILE *out_file;

	const gchar *out_filename = archive->path;
	gboolean remove = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(archive_command), "compressing"));
	if(remove == FALSE)
	{
		g_object_set_data(G_OBJECT(archive_command), "compressing", GUINT_TO_POINTER(TRUE));
		g_unlink(out_filename);
	}

	out_file = fopen(out_filename, "ab");
	if(!out_file)
		return FALSE; 
	
	status = lsq_archive_command_read_bytes(archive_command, 1, buf, 1024, (gsize *)&read, &error);
	if(status == G_IO_STATUS_EOF)
	{
		fclose(out_file);
		return TRUE;
	}

	if(read)
	{
		fwrite(buf, 1, read, out_file);
	}
	fclose(out_file);
	g_free(buf);
	return FALSE;
}

void
lsq_archive_support_gnu_tar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
/* EXTRACT */
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_OVERWRITE:
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_overwrite = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_TOUCH:
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_touch = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_KEEP_NEW:
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_keep_newer = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_STRIP:
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_strip = g_value_get_uint(value);
			break;
/* ADD */
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_ADD_MODE:
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_add_mode = g_value_dup_string(value);
			break;

/* VIEW */
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_SIZE:
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_size = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_DATE:
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_date = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_TIME:
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_time = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_OWNER:
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_owner = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_RIGHTS:
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_rights = g_value_get_boolean(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}

void
lsq_archive_support_gnu_tar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
/* EXTRACT */
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_OVERWRITE:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_overwrite);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_TOUCH:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_touch);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_STRIP:
			g_value_set_uint(value, LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_strip);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_KEEP_NEW:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_keep_newer);
			break;
/* ADD */
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_ADD_MODE:
			g_value_set_string(value, LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_add_mode);
			break;

/* VIEW */
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_SIZE:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_size);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_DATE:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_date);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_TIME:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_time);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_OWNER:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_owner);
			break;
		case LSQ_ARCHIVE_SUPPORT_GNU_TAR_VIEW_RIGHTS:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_rights);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}
