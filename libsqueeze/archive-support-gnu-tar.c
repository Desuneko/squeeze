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

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <gettext.h>

#include "mime.h"
#include "archive.h"
#include "archive-support.h"
#include "archive-support-gnu-tar.h"

#include "internals.h"

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

void
lsq_archive_support_gnu_tar_init(LSQArchiveSupportGnuTar *support);
void
lsq_archive_support_gnu_tar_class_init(LSQArchiveSupportGnuTarClass *supportclass);

void
lsq_archive_support_gnu_tar_compress_watch(GPid pid, gint status, gpointer data);
void
lsq_archive_support_gnu_tar_decompress_watch(GPid pid, gint status, gpointer data);

gboolean
lsq_archive_support_gnu_tar_refresh_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data);
gboolean
lsq_archive_support_gnu_tar_compress_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data);
gboolean
lsq_archive_support_gnu_tar_decompress_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data);

void
lsq_archive_support_gnu_tar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
void
lsq_archive_support_gnu_tar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

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

	if(g_find_program_in_path("gtar"))
		support->app_name = "gtar";
	else
		support->app_name = "tar";

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
	program_path = g_find_program_in_path("bzip2");
	if(program_path)
	{
		lsq_archive_support_add_mime(archive_support, "application/x-bzip-compressed-tar");
		g_free(program_path);
	}
	/* Check for existence of lzop -- required for x-tzo */
	program_path = g_find_program_in_path("lzop");
	if(program_path)
	{
		lsq_archive_support_add_mime(archive_support, "application/x-tzo");
		g_free(program_path);
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
		_("Do not overwrite files newer then those in the archive"),
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
	/*
	support = g_object_new(LSQ_TYPE_ARCHIVE_SUPPORT_GNU_TAR, 
	                       "view-time", FALSE, 
												 "view-date", FALSE,
												 "view-owner", FALSE,
												 "view-rights", FALSE,
												 "view-size", FALSE,
												 NULL);
	*/

	return LSQ_ARCHIVE_SUPPORT(support);
}

gint
lsq_archive_support_gnu_tar_add(LSQArchive *archive, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_GNU_TAR(archive->support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, lsq_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		archive->files = lsq_concat_filenames(filenames);
		lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_ADD);
		if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tar"))
				command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name,
				                      " -cf ", archive->path,
															" --mode=", LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_add_mode,
															" ", archive->files, NULL);
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tarz"))
				command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name,
				                      " -Zcf ", archive->path,
															" --mode=", LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_add_mode,
															" ", archive->files, NULL);
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-compressed-tar"))
				command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name,
				                      " -zcf ", archive->path,
															" --mode=", LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_add_mode,
															" ", archive->files, NULL);
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-bzip-compressed-tar"))
				command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name,
				                      " -jcf ", archive->path,
															" --mode=", LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_add_mode,
															" ", archive->files, NULL);
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tzo"))
				command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name,
				                      " --use-compress-program=lzop -cf ", archive->path,
															" --mode=", LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_add_mode,
															" ", archive->files, NULL);
			if(command)
				lsq_execute(command, archive, NULL, NULL, NULL, NULL);
		} else
		{
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tar"))
			{
				command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -rf ", archive->path, " ", archive->files, NULL);
				lsq_execute(command, archive, NULL, NULL, NULL, NULL);
				g_free(command);
				return 0;
			}
			archive->tmp_file = g_strconcat(lsq_tmp_dir, "/squeeze-XXXXXX" , NULL);
			g_mkstemp(archive->tmp_file);
			g_unlink(archive->tmp_file);
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tarz"))
				command = g_strconcat("uncompress -c ", archive->path, NULL);
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-compressed-tar"))
				command = g_strconcat("gunzip -c ", archive->path, NULL);
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-bzip-compressed-tar"))
				command = g_strconcat("bunzip2 -c ", archive->path, NULL);
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tzo"))
				command = g_strconcat("lzop -dc ", archive->path, NULL);
			lsq_execute(command, archive, lsq_archive_support_gnu_tar_decompress_watch, NULL, lsq_archive_support_gnu_tar_decompress_parse_output, NULL);
			g_free(command);
		}
	}
	return 0;
}

gint
lsq_archive_support_gnu_tar_extract(LSQArchive *archive, gchar *dest_path, GSList *filenames)
{
	gchar *command = NULL;
	gchar *command_options = g_strconcat(
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_keep_newer?" --keep-newer-files ":" ",
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_overwrite?" --overwrite ":" ",
			LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_touch?" --touch ":" ",
			NULL
	    );

	if(!LSQ_IS_ARCHIVE_SUPPORT_GNU_TAR(archive->support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, lsq_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		archive->files = lsq_concat_filenames(filenames);
		lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_EXTRACT);
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tar"))
			{
				command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -xf ", archive->path,
						" -C \"", dest_path, "\"", 
						command_options,
						archive->files, NULL);
			}
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tarz"))
			{
				command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -Zxf ", archive->path,
						" -C \"", dest_path, "\"", 
						command_options,
						archive->files, NULL);
			}
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-compressed-tar"))
			{
				command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -zxf ", archive->path,
						" -C \"", dest_path, "\"", 
						command_options,
						archive->files, NULL);
			}
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-bzip-compressed-tar"))
			{
				command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -jxf ", archive->path,
						" -C \"", dest_path, "\"", 
						command_options,
						archive->files, NULL);
			}
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tzo"))
			{
				command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -xf --use-compress-program=lzop ", archive->path,
						" -C \"", dest_path, "\"", 
						command_options,
						archive->files, NULL);
			}
		} else
		{
			g_free(command_options);
			return 1;
		}
		if(command)
		{
			lsq_execute(command, archive, NULL, NULL, NULL, NULL);
			g_free(command_options);
			g_free(command);
		}
	}
	return 0;
}

gint
lsq_archive_support_gnu_tar_remove(LSQArchive *archive, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_GNU_TAR(archive->support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, lsq_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		archive->files = lsq_concat_filenames(filenames);
		lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_REMOVE);
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tar"))
			{
				command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -f ", archive->path, " --delete ", archive->files, NULL);
				lsq_execute(command, archive, NULL, NULL, NULL, NULL);
				g_free(command);
				return 0;
			}
			archive->tmp_file = g_strconcat(lsq_tmp_dir, "/squeeze-XXXXXX" , NULL);
			g_mkstemp(archive->tmp_file);
			g_unlink(archive->tmp_file);
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tarz"))
				command = g_strconcat("uncompress -c ", archive->path, NULL);
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-compressed-tar"))
				command = g_strconcat("gunzip -c ", archive->path, NULL);
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-bzip-compressed-tar"))
				command = g_strconcat("bunzip2 -c ", archive->path, NULL);
			if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tzo"))
				command = g_strconcat("lzop -dc ", archive->path, NULL);
			lsq_execute(command, archive, lsq_archive_support_gnu_tar_decompress_watch, NULL, lsq_archive_support_gnu_tar_decompress_parse_output, NULL);
			g_free(command);
		} else
			return 1;
	}
	return 0;
}

gint
lsq_archive_support_gnu_tar_refresh(LSQArchive *archive)
{
	gchar *command = NULL;
	guint i = 0;
	if(!LSQ_IS_ARCHIVE_SUPPORT_GNU_TAR(archive->support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, lsq_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		i = LSQ_ARCHIVE_PROP_USER;
		if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_rights) {
			lsq_archive_set_property_type(archive, i, G_TYPE_STRING, _("Permissions"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_owner) {
			lsq_archive_set_property_type(archive, i, G_TYPE_STRING,_("Owner/Group"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_size) {
			lsq_archive_set_property_type(archive, i, G_TYPE_UINT64, _("Size"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_date) {
			lsq_archive_set_property_type(archive, i, G_TYPE_STRING, _("Date"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_time) {
			lsq_archive_set_property_type(archive, i, G_TYPE_STRING, _("Time"));
			i++;
		}
		if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tzo"))
			command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " --use-compress-program=lzop -tvf " , archive->path, NULL);
		else
			command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -tvf " , archive->path, NULL);
		lsq_execute(command, archive, NULL, NULL, lsq_archive_support_gnu_tar_refresh_parse_output, NULL);
		g_free(command);
	}
	return 0;
}

void
lsq_archive_support_gnu_tar_decompress_watch(GPid pid, gint status, gpointer data)
{
	LSQArchive *archive = data;
	archive->child_pid = 0;
}

void
lsq_archive_support_gnu_tar_compress_watch(GPid pid, gint status, gpointer data)
{
	LSQArchive *archive = data;
	archive->child_pid = 0;
	gchar *command = NULL;

	if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tarz"))
		command = g_strconcat("compress -c ", archive->tmp_file, NULL);
	if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-compressed-tar"))
		command = g_strconcat("gzip -c ", archive->tmp_file, NULL);
	if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-bzip-compressed-tar"))
		command = g_strconcat("bzip2 -c ", archive->tmp_file, NULL);
	if(!g_strcasecmp(lsq_mime_info_get_name(archive->mime_info), "application/x-tzo"))
		command = g_strconcat("lzop -c ", archive->path, NULL);
	lsq_execute(command, archive, NULL, NULL, lsq_archive_support_gnu_tar_compress_parse_output, NULL);
}

gboolean
lsq_archive_support_gnu_tar_refresh_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	GIOStatus status = G_IO_STATUS_NORMAL;
	LSQArchive *archive = data;
	gchar *line	= NULL;
	LSQEntry *entry;
	gpointer props[6];

	guint64 size;
	guint linesize;
	gint n = 0, a = 0, i = 0, o = 0;
	gchar *temp_filename = NULL;

	if(!LSQ_IS_ARCHIVE(archive))
		return FALSE;


	if(cond & (G_IO_PRI | G_IO_IN))
	{
		for(o = 0; o < 500; o++)
		{
			i = 0;

			status = g_io_channel_read_line(ioc, &line, NULL,NULL,NULL);
			if (line == NULL)
 				break;

			linesize = strlen(line);

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

			gchar *temp = g_strrstr (&line[n],"->"); 
			if (temp ) 
			{ 
				temp[0] = '\0';
			} 

			temp_filename = g_strchomp(line + n); 
 
			entry = lsq_archive_add_file(archive, temp_filename);
			lsq_archive_iter_set_propsv(archive, entry, (gconstpointer*)props);
			g_free(line);
		}
	}
	if(cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_IDLE);
		return FALSE; 
	}
	return TRUE;
}
gboolean
lsq_archive_support_gnu_tar_decompress_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	FILE *out_file = NULL;
	LSQArchive *archive = data;
	gchar *buf = g_new0(gchar, 1024);
	guint read = 0;
	GError *error = NULL;
	gchar *command = NULL;

	if(cond & (G_IO_PRI | G_IO_IN))
	{
		out_file = fopen(archive->tmp_file, "ab");
		if(!out_file)
			g_critical("Could not open file");

		while(g_io_channel_read_chars(ioc, buf, 1024, &read, &error) == G_IO_STATUS_NORMAL)
		{
			if(read)
			{
				fwrite(buf, 1, read, out_file);
			}
			read = 0;
		}
		fclose(out_file);
	}
	g_free(buf);
	if(cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);

		if(!(cond & G_IO_ERR))
		{
			switch(archive->status)
			{
				case(LSQ_ARCHIVESTATUS_ADD):
					g_unlink(archive->path);
					command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -rf ", archive->tmp_file, " ", archive->files, NULL);
					lsq_execute(command, archive, lsq_archive_support_gnu_tar_compress_watch, NULL, NULL, NULL);
					g_free(command);
					break;
				case(LSQ_ARCHIVESTATUS_REMOVE):
					g_unlink(archive->path);
					command = g_strconcat(LSQ_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -f ", archive->tmp_file, " --delete ", archive->files, NULL);
					lsq_execute(command, archive, lsq_archive_support_gnu_tar_compress_watch, NULL, NULL, NULL);
					g_free(command);
					break;
				default:
					break;
			}
		}
		return FALSE; 
	}
	return TRUE;
}

gboolean
lsq_archive_support_gnu_tar_compress_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	FILE *out_file = NULL;
	LSQArchive *archive = data;
	gchar *buf = g_new0(gchar, 1024);
	guint read = 0;
	GError *error = NULL;

	if(cond & (G_IO_PRI | G_IO_IN))
	{
		out_file = fopen(archive->path, "ab");
		if(!out_file)
			g_critical("Could not open file");

		while(g_io_channel_read_chars(ioc, buf, 1024, &read, &error) == G_IO_STATUS_NORMAL)
		{
			if(read)
			{
				fwrite(buf, 1, read, out_file);
			}
			read = 0;
		}
		fclose(out_file);
	}
	g_free(buf);
	if(cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		if(archive->tmp_file)
			g_unlink(archive->tmp_file);
		return FALSE;
	}
	return TRUE;
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

/* */
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

/* */
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
