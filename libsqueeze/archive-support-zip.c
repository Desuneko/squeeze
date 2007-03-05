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
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <thunar-vfs/thunar-vfs.h>

#include "libsqueeze.h"
#include "libsqueeze-module.h"
#include "archive-iter.h"
#include "archive-command.h"
#include "archive.h"
#include "archive-support.h"
#include "archive-support-zip.h"

#include "internals.h"

#define XA_TEST_ACTION_ICON "gtk-index"

enum
{
	LSQ_ARCHIVE_SUPPORT_ZIP_EXTRACT_OVERWRITE = 1,
	LSQ_ARCHIVE_SUPPORT_ZIP_ADD_COMPRESSION_LEVEL,
	LSQ_ARCHIVE_SUPPORT_ZIP_PASSWORD,

	LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_SIZE,
	LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_DATE,
	LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_TIME,
	LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_RATIO,
	LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_LENGTH,
	LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_METHOD,
	LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_CRC_32
};

static void
lsq_archive_support_zip_init(LSQArchiveSupportZip *support);
static void
lsq_archive_support_zip_class_init(LSQArchiveSupportZipClass *supportclass);

gboolean
lsq_archive_support_zip_refresh_parse_output(LSQArchiveCommand *archive_command);

static void
lsq_archive_support_zip_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
lsq_archive_support_zip_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gint lsq_archive_support_zip_add(LSQArchive *, GSList *);
static gint lsq_archive_support_zip_extract(LSQArchive *, const gchar *, GSList *);
static gint lsq_archive_support_zip_remove(LSQArchive *, GSList *);
static gint lsq_archive_support_zip_refresh(LSQArchive *);

GType
lsq_archive_support_zip_get_type ()
{
	static GType lsq_archive_support_zip_type = 0;

 	if (!lsq_archive_support_zip_type)
	{
 		static const GTypeInfo lsq_archive_support_zip_info = 
		{
			sizeof (LSQArchiveSupportZipClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_archive_support_zip_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQArchiveSupportZip),
			0,
			(GInstanceInitFunc) lsq_archive_support_zip_init,
		};

		lsq_archive_support_zip_type = g_type_register_static (LSQ_TYPE_ARCHIVE_SUPPORT, "LSQArchiveSupportZip", &lsq_archive_support_zip_info, 0);
	}
	return lsq_archive_support_zip_type;
}

static void
lsq_archive_support_zip_init(LSQArchiveSupportZip *support)
{
	LSQArchiveSupport *archive_support = LSQ_ARCHIVE_SUPPORT(support);

	archive_support->id = "Zip";

	lsq_archive_support_add_mime(archive_support, "application/zip");
	lsq_archive_support_add_mime(archive_support, "application/x-zip");

	archive_support->add = lsq_archive_support_zip_add;
	archive_support->extract = lsq_archive_support_zip_extract;
	archive_support->remove = lsq_archive_support_zip_remove;
	archive_support->refresh = lsq_archive_support_zip_refresh;
}

static void
lsq_archive_support_zip_class_init(LSQArchiveSupportZipClass *supportclass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (supportclass);
	GParamSpec *pspec = NULL;

	object_class->set_property = lsq_archive_support_zip_set_property;
	object_class->get_property = lsq_archive_support_zip_get_property;

	pspec = g_param_spec_string("extract-password",
		_("Password"),
		_("Password"),
		"",
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_ZIP_PASSWORD, pspec);

	pspec = g_param_spec_boolean("extract-overwrite",
		_("Overwrite existing files"),
		_("Overwrite existing files on extraction"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_ZIP_EXTRACT_OVERWRITE, pspec);

	pspec = g_param_spec_boolean("view-compressed-size",
		_("Compressed Size"),
		_("View compressed filesize"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_SIZE, pspec);

	pspec = g_param_spec_boolean("view-time",
		_("Time"),
		_("View time"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_TIME, pspec);

	pspec = g_param_spec_boolean("view-date",
		_("Date"),
		_("View date"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_DATE, pspec);

	pspec = g_param_spec_boolean("view-ratio",
		_("Ratio"),
		_("View ratio"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_RATIO, pspec);

	pspec = g_param_spec_boolean("view-uncompressed-size",
		_("Size"),
		_("View filesize"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_LENGTH, pspec);

	pspec = g_param_spec_boolean("view-method",
		_("Method"),
		_("View method"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_METHOD, pspec);

	pspec = g_param_spec_boolean("view-crc32",
		_("Checksum"),
		_("View Checksum"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_CRC_32, pspec);
}

LSQArchiveSupport*
lsq_archive_support_zip_new()
{
	LSQArchiveSupportZip *support;

	support = g_object_new(LSQ_TYPE_ARCHIVE_SUPPORT_ZIP,
	                       "view-uncompressed-size", TRUE,
	                       "view-compressed-size", TRUE,
	                       "view-time", TRUE,
	                       "view-date", TRUE,
	                       "view-ratio", TRUE,
	                       "view-crc32", TRUE,
	                       "view-method", TRUE,
	                       NULL);
	
	return LSQ_ARCHIVE_SUPPORT(support);
}

static gint
lsq_archive_support_zip_add(LSQArchive *archive, GSList *filenames)
{
	LSQArchiveCommand *archive_command = NULL;
	if(!LSQ_IS_ARCHIVE_SUPPORT_ZIP(archive->support))
	{
		g_critical("Support is not zip");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		if(!g_strcasecmp((gchar *)thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-zip") || 
		   !g_strcasecmp((gchar *)thunar_vfs_mime_info_get_name(archive->mime_info), "application/zip"))
		{
			gchar *files = lsq_concat_filenames(filenames);
			gchar *options = NULL;

			archive_command = lsq_archive_command_new("", archive, "zip %3$s -r %1$s %2$s", FALSE, TRUE);
			g_object_set_data(G_OBJECT(archive_command), "files", g_strdup(files));
			g_object_set_data(G_OBJECT(archive_command), "options", options);
			g_free(files);
			lsq_archive_command_run(archive_command);
			g_object_unref(archive_command);
		}
	}
	return 0;
}

static gint
lsq_archive_support_zip_extract(LSQArchive *archive, const gchar *extract_path, GSList *filenames)
{
	LSQArchiveCommand *archive_command = NULL;
	gchar *dest_path = NULL;
	if(!LSQ_IS_ARCHIVE_SUPPORT_ZIP(archive->support))
	{
		g_critical("Support is not Zip");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		if(!g_strcasecmp((gchar *)thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-zip") || 
	     !g_strcasecmp((gchar *)thunar_vfs_mime_info_get_name(archive->mime_info), "application/zip"))
		{
			gchar *files = lsq_concat_filenames(filenames);
			if(extract_path)
				dest_path = g_shell_quote(extract_path);

			gchar *options = g_strconcat(" -d ", dest_path, NULL);

			archive_command = lsq_archive_command_new("", archive, "unzip -o %1$s %2$s %3$s", TRUE, FALSE);
			g_object_set_data(G_OBJECT(archive_command), "files", files);
			g_object_set_data(G_OBJECT(archive_command), "options", options);
			lsq_archive_command_run(archive_command);
			g_object_unref(archive_command);
			g_free(dest_path);
			g_free(options);
			g_free(files);
		}	
	}
	return 0;
}

static gint
lsq_archive_support_zip_remove(LSQArchive *archive, GSList *file_iters)
{
	LSQArchiveCommand *archive_command = NULL;
	if(!LSQ_IS_ARCHIVE_SUPPORT_ZIP(archive->support))
	{
		g_critical("Support is not zip");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		if(!g_strcasecmp((gchar *)thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-zip") || 
		   !g_strcasecmp((gchar *)thunar_vfs_mime_info_get_name(archive->mime_info), "application/zip"))
		{
			/* FIXME: when directories are removed, append a '*' wildcard to the filenames */
			gchar *files = lsq_concat_iter_filenames(file_iters);

			while(file_iters)
			{
				lsq_archive_iter_remove(file_iters->data);
				file_iters = g_slist_next(file_iters);
			}

			archive_command = lsq_archive_command_new("", archive, "zip -d %1$s %2$s", FALSE, TRUE);
			g_object_set_data(G_OBJECT(archive_command), "files", files);
			lsq_archive_command_run(archive_command);
			g_object_unref(archive_command);
			g_free(files);
		}
	}
	return 0;
}

static gint
lsq_archive_support_zip_refresh(LSQArchive *archive)
{
	guint i = 0;
	LSQArchiveCommand *archive_command = NULL;
	if(!LSQ_IS_ARCHIVE_SUPPORT_ZIP(archive->support))
	{
		g_critical("Support is not Zip");
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
		if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_length) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_UINT64, _("Size"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_method) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING,_("Method"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_size) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_UINT64, _("Compressed Size"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_ratio) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Ratio"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_date) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Date"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_time) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Time"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_crc_32) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Checksum"));
			i++;
		}
		archive_command = lsq_archive_command_new("", archive, "unzip -lv -qq %1$s", TRUE, TRUE);
		lsq_archive_command_set_parse_func(archive_command, 1, lsq_archive_support_zip_refresh_parse_output);
		lsq_archive_command_run(archive_command);
		g_object_unref(archive_command);
	}
	return 0;
}

gboolean
lsq_archive_support_zip_refresh_parse_output(LSQArchiveCommand *archive_command)
{
	gchar *line = NULL;
	gsize linesize = 0;
	GIOStatus status = G_IO_STATUS_NORMAL;
	LSQArchive *archive = archive_command->archive;
	guint64 size;
	guint64 length;
	gpointer props[8]; 
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
	/* length, method , size, ratio, date, time, crc-32, filename*/
	for(n=0; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_length)
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

	if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_method)
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}
	n++;

	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_size)
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

	if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_ratio)
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}
	n++;

	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_date)
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}
	n++;

	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_time)
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}
	n++;

	for(; n < linesize && line[n] == ' '; n++);
	a = n;
	for(; n < linesize && line[n] != ' '; n++);

	if(LSQ_ARCHIVE_SUPPORT_ZIP(archive->support)->_view_crc_32)
	{
		line[n] = '\0';
		props[i] = line + a;
		i++;
	}
	n+=2;

	line[linesize-1] = '\0';
	temp_filename = line+n; 
	props[i] = NULL;

	entry = lsq_archive_add_file(archive, temp_filename);
	lsq_archive_iter_set_propsv(entry, (gconstpointer*)props);
	lsq_archive_iter_unref(entry);
	g_free(line);

	return TRUE;
}

static void
lsq_archive_support_zip_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case LSQ_ARCHIVE_SUPPORT_ZIP_EXTRACT_OVERWRITE:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_ZIP(object)->_extr_overwrite);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_PASSWORD:
			g_value_set_string (value, LSQ_ARCHIVE_SUPPORT_ZIP(object)->_extr_password);
			break;

		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_SIZE:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_size);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_DATE:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_date);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_TIME:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_time);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_RATIO:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_ratio);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_LENGTH:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_length);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_METHOD:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_method);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_CRC_32:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_crc_32);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}

static void
lsq_archive_support_zip_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case LSQ_ARCHIVE_SUPPORT_ZIP_EXTRACT_OVERWRITE:
			LSQ_ARCHIVE_SUPPORT_ZIP(object)->_extr_overwrite = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_PASSWORD:
			LSQ_ARCHIVE_SUPPORT_ZIP(object)->_extr_password = (gchar *)g_value_get_string(value);
			break;

		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_SIZE:
			LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_size = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_TIME:
			LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_time = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_DATE:
			LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_date = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_RATIO:
			LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_ratio = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_LENGTH:
			LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_length = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_METHOD:
			LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_method = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_ZIP_VIEW_CRC_32:
			LSQ_ARCHIVE_SUPPORT_ZIP(object)->_view_crc_32 = g_value_get_boolean(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}
