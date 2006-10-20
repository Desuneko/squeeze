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
	LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_OVERWRITE = 1,
	LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_TOUCH,
	LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_STRIP,
	LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_SIZE,
	LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_DATE,
	LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_TIME,
	LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_OWNER,
	LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_RIGHTS
};

void
lxa_archive_support_gnu_tar_init(LXAArchiveSupportGnuTar *support);
void
lxa_archive_support_gnu_tar_class_init(LXAArchiveSupportGnuTarClass *supportclass);

void
lxa_archive_support_gnu_tar_compress_watch(GPid pid, gint status, gpointer data);
void
lxa_archive_support_gnu_tar_decompress_watch(GPid pid, gint status, gpointer data);

gboolean
lxa_archive_support_gnu_tar_refresh_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data);
gboolean
lxa_archive_support_gnu_tar_compress_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data);
gboolean
lxa_archive_support_gnu_tar_decompress_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data);

void
lxa_archive_support_gnu_tar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
void
lxa_archive_support_gnu_tar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

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
	/* TODO: free return value of g_find_program_in_path */
	LXAArchiveSupport *archive_support = LXA_ARCHIVE_SUPPORT(support);

	archive_support->id = "Gnu Tar";

	if(g_find_program_in_path("gtar"))
		support->app_name = "gtar";
	else
		support->app_name = "tar";

	lxa_archive_support_add_mime(archive_support, "application/x-tar");
	/* Check for existence of compress -- required for x-tarz */
	if(g_find_program_in_path("compress"))
		lxa_archive_support_add_mime(archive_support, "application/x-tarz");
	/* Check for existence of gzip -- required for x-compressed-tar*/
	if(g_find_program_in_path("gzip"))
		lxa_archive_support_add_mime(archive_support, "application/x-compressed-tar");
	/* Check for existence of bzip2 -- required for x-bzip-compressed-tar */
	if(g_find_program_in_path("bzip2"))
		lxa_archive_support_add_mime(archive_support, "application/x-bzip-compressed-tar");
	/* Check for existence of lzop -- required for x-tzo */
	if(g_find_program_in_path("lzop"))
		lxa_archive_support_add_mime(archive_support, "application/x-tzo");

	archive_support->add = lxa_archive_support_gnu_tar_add;
	archive_support->extract = lxa_archive_support_gnu_tar_extract;
	archive_support->remove = lxa_archive_support_gnu_tar_remove;
	archive_support->refresh = lxa_archive_support_gnu_tar_refresh;
}

void
lxa_archive_support_gnu_tar_class_init(LXAArchiveSupportGnuTarClass *supportclass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (supportclass);
	GParamSpec *pspec = NULL;

	object_class->set_property = lxa_archive_support_gnu_tar_set_property;
	object_class->get_property = lxa_archive_support_gnu_tar_get_property;

	pspec = g_param_spec_boolean("extract-overwrite",
		_("Overwrite existing files"),
		_("Overwrite existing files on extraction"),
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

	pspec = g_param_spec_boolean("view-size",
		"View file-size",
		"View file-size",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_SIZE, pspec);

	pspec = g_param_spec_boolean("view-rights",
		"View permissions",
		"View permissions",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_RIGHTS, pspec);

	pspec = g_param_spec_boolean("view-owner",
		"View owner",
		"View owner",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_OWNER, pspec);

	pspec = g_param_spec_boolean("view-date",
		"View date",
		"View date",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_DATE, pspec);

	pspec = g_param_spec_boolean("view-time",
		"View time",
		"View time",
		TRUE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_TIME, pspec);

}

LXAArchiveSupport*
lxa_archive_support_gnu_tar_new()
{
	LXAArchiveSupportGnuTar *support;

	/**/
	support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT_GNU_TAR, 
	                       "view-time", TRUE, 
												 "view-date", TRUE,
												 "view-owner", TRUE,
												 "view-rights", TRUE,
												 "view-size", TRUE,
												 NULL);
	/*/
	support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT_GNU_TAR, 
	                       "view-time", FALSE, 
												 "view-date", FALSE,
												 "view-owner", FALSE,
												 "view-rights", FALSE,
												 "view-size", FALSE,
												 NULL);
	*/

	return LXA_ARCHIVE_SUPPORT(support);
}

gint
lxa_archive_support_gnu_tar_add(LXAArchive *archive, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_GNU_TAR(archive->support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(archive->support, lxa_mime_info_get_name(archive->mime)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		archive->files = lxa_concat_filenames(filenames);
		lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_ADD);
		if(!g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -cf ", archive->path, " ", archive->files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tarz"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -Zcf ", archive->path, " ", archive->files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-compressed-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -zcf ", archive->path, " ", archive->files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-bzip-compressed-tar"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -jcf ", archive->path, " ", archive->files, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tzo"))
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " --use-compress-program=lzop -cf ", archive->path, " ", archive->files, NULL);
			if(command)
				lxa_execute(command, archive, NULL, NULL, NULL, NULL);
		} else
		{
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tar"))
			{
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -rf ", archive->path, " ", archive->files, NULL);
				lxa_execute(command, archive, NULL, NULL, NULL, NULL);
				g_free(command);
				return 0;
			}
			archive->tmp_file = g_strconcat(lxa_tmp_dir, "/xarchiver-XXXXXX" , NULL);
			g_mkstemp(archive->tmp_file);
			g_unlink(archive->tmp_file);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tarz"))
				command = g_strconcat("uncompress -c ", archive->path, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-compressed-tar"))
				command = g_strconcat("gunzip -c ", archive->path, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-bzip-compressed-tar"))
				command = g_strconcat("bunzip2 -c ", archive->path, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tzo"))
				command = g_strconcat("lzop -dc ", archive->path, NULL);
			lxa_execute(command, archive, lxa_archive_support_gnu_tar_decompress_watch, NULL, lxa_archive_support_gnu_tar_decompress_parse_output, NULL);
			g_free(command);
		}
	}
	return 0;
}

gint
lxa_archive_support_gnu_tar_extract(LXAArchive *archive, gchar *dest_path, GSList *filenames)
{
	gchar *command = NULL;
	if(!LXA_IS_ARCHIVE_SUPPORT_GNU_TAR(archive->support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(archive->support, lxa_mime_info_get_name(archive->mime)))
	{
		return 1;
	}
	else
	{
		archive->files = lxa_concat_filenames(filenames);
		lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_EXTRACT);
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tar"))
			{
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -xf ", archive->path,
						" -C \"", dest_path, "\"", 
						LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_overwrite?" --overwrite ":" ",
						LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_touch?" --touch ":" ",
						archive->files, NULL);
			}
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tarz"))
			{
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -Zxf ", archive->path,
						" -C \"", dest_path, "\"", 
						LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_overwrite?" --overwrite ":" ",
						LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_touch?" --touch ":" ",
						archive->files, NULL);
			}
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-compressed-tar"))
			{
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -zxf ", archive->path,
						" -C \"", dest_path, "\"", 
						LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_overwrite?" --overwrite ":" ",
						LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_touch?" --touch ":" ",
						archive->files, NULL);
			}
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-bzip-compressed-tar"))
			{
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -jxf ", archive->path,
						" -C \"", dest_path, "\"", 
						LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_overwrite?" --overwrite ":" ",
						LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_touch?" --touch ":" ",
						archive->files, NULL);
			}
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tzo"))
			{
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -xf --use-compress-program=lzop ", archive->path,
						" -C \"", dest_path, "\"", 
						LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_overwrite?" --overwrite ":" ",
						LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_extr_touch?" --touch ":" ",
						archive->files, NULL);
			}
		} else
			return 1;
		if(command)
		{
			lxa_execute(command, archive, NULL, NULL, NULL, NULL);
			g_debug("Extracting archive '%s' to '%s'\nUsing command '%s'", archive->path, dest_path, command);
			g_free(command);
		}
	}
	return 0;
}

gint
lxa_archive_support_gnu_tar_remove(LXAArchive *archive, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_GNU_TAR(archive->support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(archive->support, lxa_mime_info_get_name(archive->mime)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		archive->files = lxa_concat_filenames(filenames);
		lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_REMOVE);
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tar"))
			{
				command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -f ", archive->path, " --delete ", archive->files, NULL);
				lxa_execute(command, archive, NULL, NULL, NULL, NULL);
				g_free(command);
				return 0;
			}
			archive->tmp_file = g_strconcat(lxa_tmp_dir, "/xarchiver-XXXXXX" , NULL);
			g_mkstemp(archive->tmp_file);
			g_unlink(archive->tmp_file);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tarz"))
				command = g_strconcat("uncompress -c ", archive->path, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-compressed-tar"))
				command = g_strconcat("gunzip -c ", archive->path, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-bzip-compressed-tar"))
				command = g_strconcat("bunzip2 -c ", archive->path, NULL);
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-tzo"))
				command = g_strconcat("lzop -dc ", archive->path, NULL);
			lxa_execute(command, archive, lxa_archive_support_gnu_tar_decompress_watch, NULL, lxa_archive_support_gnu_tar_decompress_parse_output, NULL);
			g_free(command);
		} else
			return 1;
	}
	return 0;
}

gint
lxa_archive_support_gnu_tar_refresh(LXAArchive *archive)
{
	gchar *command = NULL;
	guint i = 0;
	if(!LXA_IS_ARCHIVE_SUPPORT_GNU_TAR(archive->support))
	{
		g_critical("Support is not GNU TAR");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(archive->support, lxa_mime_info_get_name(archive->mime)))
	{
		return 1;
	}
	else
	{
		i = LXA_ARCHIVE_PROP_USER;
		if(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_rights) {
			lxa_archive_set_property_type(archive, i, G_TYPE_STRING, _("Permissions"));
			i++;
		}
		if(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_owner) {
			lxa_archive_set_property_type(archive, i, G_TYPE_STRING,_("Owner/Group"));
			i++;
		}
		if(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_size) {
			lxa_archive_set_property_type(archive, i, G_TYPE_UINT64, _("Size"));
			i++;
		}
		if(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_date) {
			lxa_archive_set_property_type(archive, i, G_TYPE_STRING, _("Date"));
			i++;
		}
		if(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_time) {
			lxa_archive_set_property_type(archive, i, G_TYPE_STRING, _("Time"));
			i++;
		}
		if(!g_strcasecmp((gchar *)archive->mime, "application/x-tzo"))
			command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " --use-compress-program=lzop -tvf " , archive->path, NULL);
		else
			command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -tvf " , archive->path, NULL);
		lxa_execute(command, archive, NULL, NULL, lxa_archive_support_gnu_tar_refresh_parse_output, NULL);
		g_free(command);
	}
	return 0;
}

void
lxa_archive_support_gnu_tar_decompress_watch(GPid pid, gint status, gpointer data)
{
	LXAArchive *archive = data;
	archive->child_pid = 0;
}

void
lxa_archive_support_gnu_tar_compress_watch(GPid pid, gint status, gpointer data)
{
	LXAArchive *archive = data;
	archive->child_pid = 0;
	gchar *command = NULL;

	if(!g_strcasecmp((gchar *)archive->mime, "application/x-tarz"))
		command = g_strconcat("compress -c ", archive->tmp_file, NULL);
	if(!g_strcasecmp((gchar *)archive->mime, "application/x-compressed-tar"))
		command = g_strconcat("gzip -c ", archive->tmp_file, NULL);
	if(!g_strcasecmp((gchar *)archive->mime, "application/x-bzip-compressed-tar"))
		command = g_strconcat("bzip2 -c ", archive->tmp_file, NULL);
	if(!g_strcasecmp((gchar *)archive->mime, "application/x-tzo"))
		command = g_strconcat("lzop -c ", archive->path, NULL);
	lxa_execute(command, archive, NULL, NULL, lxa_archive_support_gnu_tar_compress_parse_output, NULL);
}

gboolean
lxa_archive_support_gnu_tar_refresh_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	GIOStatus status = G_IO_STATUS_NORMAL;
	LXAArchive *archive = data;
	gchar *line	= NULL;
	LXAEntry *entry;
	gpointer props[6];

	guint64 size;
	guint linesize;
	gint n = 0, a = 0, i = 0, o = 0;
	gchar *temp_filename = NULL;

	if(!LXA_IS_ARCHIVE(archive))
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

			if(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_rights)
			{
				line[10] = '\0';
				props[i] = line;
				i++;
			}


			for(n=13; n < linesize; ++n)
				if(line[n] == ' ') break;

			if(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_owner)
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

			if(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_size)
			{
				line[n] = '\0';
				size = g_ascii_strtoll(line + a, NULL, 0);
				props[i] = &size;
				i++;
			}

			a = ++n;

			for(; n < linesize; n++) // DATE
				if(line[n] == ' ') break;

			if(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_date)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}

			a = ++n;
			for (; n < linesize; n++) // TIME
				if (line[n] == ' ') break;

			if(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->_view_time)
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
 
			entry = lxa_archive_add_file(archive, temp_filename);
			lxa_archive_iter_set_propsv(archive, entry, (gconstpointer*)props);
			g_free(line);
		}
	}
	if(cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
	{
#ifdef DEBUG
		g_debug("shutting down ioc");
#endif
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		lxa_archive_set_status(archive, LXA_ARCHIVESTATUS_IDLE);
		return FALSE; 
	}
	return TRUE;
}
gboolean
lxa_archive_support_gnu_tar_decompress_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	FILE *out_file = NULL;
	LXAArchive *archive = data;
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
#ifdef DEBUG
		g_debug("shutting down ioc");
#endif
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);

		if(!(cond & G_IO_ERR))
		{
			switch(archive->status)
			{
				case(LXA_ARCHIVESTATUS_ADD):
					g_unlink(archive->path);
					command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -rf ", archive->tmp_file, " ", archive->files, NULL);
					lxa_execute(command, archive, lxa_archive_support_gnu_tar_compress_watch, NULL, NULL, NULL);
					g_free(command);
					break;
				case(LXA_ARCHIVESTATUS_REMOVE):
					g_unlink(archive->path);
					command = g_strconcat(LXA_ARCHIVE_SUPPORT_GNU_TAR(archive->support)->app_name, " -f ", archive->tmp_file, " --delete ", archive->files, NULL);
					lxa_execute(command, archive, lxa_archive_support_gnu_tar_compress_watch, NULL, NULL, NULL);
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
lxa_archive_support_gnu_tar_compress_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	FILE *out_file = NULL;
	LXAArchive *archive = data;
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
#ifdef DEBUG
		g_debug("shutting down ioc");
#endif
		g_io_channel_shutdown ( ioc,TRUE,NULL );
		g_io_channel_unref (ioc);
		if(archive->tmp_file)
			g_unlink(archive->tmp_file);
		return FALSE;
	}
	return TRUE;
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

/* */
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_SIZE:
			LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_size = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_DATE:
			LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_date = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_TIME:
			LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_time = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_OWNER:
			LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_owner = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_RIGHTS:
			LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_rights = g_value_get_boolean(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}

void
lxa_archive_support_gnu_tar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_OVERWRITE:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_overwrite);
			break;
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_TOUCH:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_touch);
			break;
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_EXTRACT_STRIP:
			g_value_set_uint(value, LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_extr_strip);
			break;

/* */
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_SIZE:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_size);
			break;
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_DATE:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_date);
			break;
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_TIME:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_time);
			break;
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_OWNER:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_owner);
			break;
		case LXA_ARCHIVE_SUPPORT_GNU_TAR_VIEW_RIGHTS:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_GNU_TAR(object)->_view_rights);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}
