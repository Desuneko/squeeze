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
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <gettext.h>

#include "mime.h"
#include "archive.h"
#include "archive-support.h"
#include "archive-support-zip.h"

#include "internals.h"

enum
{
	LXA_ARCHIVE_SUPPORT_ZIP_EXTRACT_OVERWRITE = 1,
	LXA_ARCHIVE_SUPPORT_ZIP_ADD_COMPRESSION_LEVEL,
	LXA_ARCHIVE_SUPPORT_ZIP_PASSWORD,

	LXA_ARCHIVE_SUPPORT_ZIP_VIEW_SIZE,
	LXA_ARCHIVE_SUPPORT_ZIP_VIEW_DATE,
	LXA_ARCHIVE_SUPPORT_ZIP_VIEW_TIME,
	LXA_ARCHIVE_SUPPORT_ZIP_VIEW_RATIO,
	LXA_ARCHIVE_SUPPORT_ZIP_VIEW_LENGTH,
	LXA_ARCHIVE_SUPPORT_ZIP_VIEW_METHOD,
	LXA_ARCHIVE_SUPPORT_ZIP_VIEW_CRC_32
};

void
lxa_archive_support_zip_init(LXAArchiveSupportZip *support);
void
lxa_archive_support_zip_class_init(LXAArchiveSupportZipClass *supportclass);

gboolean
lxa_archive_support_zip_refresh_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data);

void
lxa_archive_support_zip_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
void
lxa_archive_support_zip_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

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

	lxa_archive_support_add_mime(archive_support, "application/zip");
	lxa_archive_support_add_mime(archive_support, "application/x-zip");

	archive_support->add = lxa_archive_support_zip_add;
	archive_support->extract = lxa_archive_support_zip_extract;
	archive_support->remove = lxa_archive_support_zip_remove;
	archive_support->refresh = lxa_archive_support_zip_refresh;
}

void
lxa_archive_support_zip_class_init(LXAArchiveSupportZipClass *supportclass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (supportclass);
	GParamSpec *pspec = NULL;

	object_class->set_property = lxa_archive_support_zip_set_property;
	object_class->get_property = lxa_archive_support_zip_get_property;

	pspec = g_param_spec_string("extract-password",
		_("Password"),
		_("Password"),
		"",
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_ZIP_PASSWORD, pspec);

	pspec = g_param_spec_boolean("extract-overwrite",
		_("Overwrite existing files"),
		_("Overwrite existing files on extraction"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_ZIP_EXTRACT_OVERWRITE, pspec);

	pspec = g_param_spec_boolean("view-size",
		"View file-size",
		"View file-size",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_ZIP_VIEW_SIZE, pspec);

	pspec = g_param_spec_boolean("view-time",
		"View time",
		"View time",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_ZIP_VIEW_TIME, pspec);

	pspec = g_param_spec_boolean("view-date",
		"View date",
		"View date",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_ZIP_VIEW_DATE, pspec);

	pspec = g_param_spec_boolean("view-ratio",
		"View ratio",
		"View ratio",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_ZIP_VIEW_RATIO, pspec);

	pspec = g_param_spec_boolean("view-length",
		"View length",
		"View length",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_ZIP_VIEW_LENGTH, pspec);

	pspec = g_param_spec_boolean("view-method",
		"View method",
		"View method",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_ZIP_VIEW_METHOD, pspec);

	pspec = g_param_spec_boolean("view-crc32",
		"View CRC-32",
		"View CRC-32",
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_ZIP_VIEW_CRC_32, pspec);

}

LXAArchiveSupport*
lxa_archive_support_zip_new()
{
	LXAArchiveSupportZip *support;

	support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT_ZIP,
												 "view-length", TRUE,
	                       "view-size", TRUE,
												 "view-time", TRUE,
												 "view-date", TRUE,
												 "view-ratio", TRUE,
												 "view-crc32", TRUE,
												 "view-method", TRUE,
												 NULL);
	
	return LXA_ARCHIVE_SUPPORT(support);
}

gint
lxa_archive_support_zip_add(LXAArchive *archive, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_ZIP(archive->support))
	{
		g_critical("Support is not zip");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(archive->support, lxa_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lxa_concat_filenames(filenames);
		if(!g_strcasecmp((gchar *)lxa_mime_info_get_name(archive->mime_info), "application/x-zip") || 
		   !g_strcasecmp((gchar *)lxa_mime_info_get_name(archive->mime_info), "application/zip"))
		{
			command = g_strconcat("zip -r ", archive->path, " ", files, NULL);
			lxa_execute(command, archive, NULL, NULL, NULL, NULL);
		}
	}
	return 0;
}

gint
lxa_archive_support_zip_extract(LXAArchive *archive, gchar *dest_path, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_ZIP(archive->support))
	{
		g_critical("Support is not Zip");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(archive->support, lxa_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lxa_concat_filenames(filenames);
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp((gchar *)lxa_mime_info_get_name(archive->mime_info), "application/x-zip") || 
		     !g_strcasecmp((gchar *)lxa_mime_info_get_name(archive->mime_info), "application/zip"))
			{
				command = g_strconcat("unzip -o ", archive->path, " -d ", dest_path, " ", files, NULL);
				g_debug("Extracting archive '%s' to '%s'", archive->path, dest_path);
				lxa_execute(command, archive, NULL, NULL, NULL, NULL);
			}	
		} else
			return 1;
	}
	return 0;
}

gint
lxa_archive_support_zip_remove(LXAArchive *archive, GSList *filenames)
{
	if(!LXA_IS_ARCHIVE_SUPPORT_ZIP(archive->support))
	{
		g_critical("Support is not zip");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(archive->support, lxa_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lxa_concat_filenames(filenames);
		if(!g_strcasecmp((gchar *)lxa_mime_info_get_name(archive->mime_info), "application/x-zip") || 
		   !g_strcasecmp((gchar *)lxa_mime_info_get_name(archive->mime_info), "application/zip"))
		{
			command = g_strconcat("zip -d ", archive->path, " ", files, NULL);
			lxa_execute(command, archive, NULL, NULL, NULL, NULL);
		}
	}
	return 0;
}

gint
lxa_archive_support_zip_refresh(LXAArchive *archive)
{
	guint i = 0;
	if(!LXA_IS_ARCHIVE_SUPPORT_ZIP(archive->support))
	{
		g_critical("Support is not Zip");
		return -1;
	}

	if(!lxa_archive_support_mime_supported(archive->support, lxa_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		i = LXA_ARCHIVE_PROP_USER;
		if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_length) {
			lxa_archive_set_property_type(archive, i, G_TYPE_UINT64, _("Length"));
			i++;
		}
		if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_method) {
			lxa_archive_set_property_type(archive, i, G_TYPE_STRING,_("Method"));
			i++;
		}
		if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_size) {
			lxa_archive_set_property_type(archive, i, G_TYPE_UINT64, _("Size"));
			i++;
		}
		if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_ratio) {
			lxa_archive_set_property_type(archive, i, G_TYPE_STRING, _("Ratio"));
			i++;
		}
		if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_date) {
			lxa_archive_set_property_type(archive, i, G_TYPE_STRING, _("Date"));
			i++;
		}
		if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_time) {
			lxa_archive_set_property_type(archive, i, G_TYPE_STRING, _("Time"));
			i++;
		}
		if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_crc_32) {
			lxa_archive_set_property_type(archive, i, G_TYPE_STRING, _("CRC-32"));
			i++;
		}
		gchar *command = g_strconcat("unzip -lv -qq " , archive->path, NULL);
		lxa_execute(command, archive, NULL, NULL, lxa_archive_support_zip_refresh_parse_output, NULL);
		LXA_FREE(command);
	}
	return 0;
}

gboolean
lxa_archive_support_zip_refresh_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	GIOStatus status = G_IO_STATUS_NORMAL;
	LXAArchive *archive = data;
	gchar *line	= NULL;
	LXAEntry *entry;

	guint64 size;
	guint64 length;
	gpointer props[8]; 
	gint n = 0, a = 0, i = 0, o = 0;
	gchar *temp_filename = NULL;
	gint linesize = 0;

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
			/* length, method , size, ratio, date, time, crc-32, filename*/
			linesize = strlen(line);

			for(n=0; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_length)
			{
				line[n]='\0';
				length = g_ascii_strtoull(line + a, NULL, 0);
				props[i] = &length;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_method)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_size)
			{
				line[n]='\0';
				size = g_ascii_strtoull(line + a, NULL, 0);
				props[i] = &size;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_ratio)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_date)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_time)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LXA_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_crc_32)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			temp_filename = g_strchomp(line+n); 

			entry = lxa_archive_add_file(archive, temp_filename);
			lxa_archive_iter_set_propsv(archive, entry, (gconstpointer*)props);
			LXA_FREE(line);
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

void
lxa_archive_support_zip_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case LXA_ARCHIVE_SUPPORT_ZIP_EXTRACT_OVERWRITE:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_ZIP(object)->_extr_overwrite);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_PASSWORD:
			g_value_set_string (value, LXA_ARCHIVE_SUPPORT_ZIP(object)->_extr_password);
			break;

		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_SIZE:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_size);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_DATE:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_date);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_TIME:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_time);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_RATIO:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_ratio);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_LENGTH:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_length);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_METHOD:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_method);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_CRC_32:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_crc_32);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}

void
lxa_archive_support_zip_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case LXA_ARCHIVE_SUPPORT_ZIP_EXTRACT_OVERWRITE:
			LXA_ARCHIVE_SUPPORT_ZIP(object)->_extr_overwrite = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_PASSWORD:
			LXA_ARCHIVE_SUPPORT_ZIP(object)->_extr_password = (gchar *)g_value_get_string(value);
			break;

		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_SIZE:
			LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_size = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_TIME:
			LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_time = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_DATE:
			LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_date = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_RATIO:
			LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_ratio = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_LENGTH:
			LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_length = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_METHOD:
			LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_method = g_value_get_boolean(value);
			break;
		case LXA_ARCHIVE_SUPPORT_ZIP_VIEW_CRC_32:
			LXA_ARCHIVE_SUPPORT_ZIP(object)->_view_crc_32 = g_value_get_boolean(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}
