/*
 *  Copyright (c) 2006 Stephan Arts <stephan@xfce.org>
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
#include <thunar-vfs/thunar-vfs.h>

#include "archive.h"
#include "archive-support.h"
#include "archive-support-rar.h"

#include "internals.h"

enum
{
	LSQ_ARCHIVE_SUPPORT_RAR_EXTRACT_OVERWRITE = 1,
	LSQ_ARCHIVE_SUPPORT_RAR_ADD_COMPRESSION_LEVEL,
	LSQ_ARCHIVE_SUPPORT_RAR_PASSWORD,

	LSQ_ARCHIVE_SUPPORT_RAR_VIEW_SIZE,
	LSQ_ARCHIVE_SUPPORT_RAR_VIEW_DATE,
	LSQ_ARCHIVE_SUPPORT_RAR_VIEW_TIME,
	LSQ_ARCHIVE_SUPPORT_RAR_VIEW_RATIO,
	LSQ_ARCHIVE_SUPPORT_RAR_VIEW_LENGTH,
	LSQ_ARCHIVE_SUPPORT_RAR_VIEW_METHOD,
	LSQ_ARCHIVE_SUPPORT_RAR_VIEW_CRC_32,
	LSQ_ARCHIVE_SUPPORT_RAR_VIEW_VERSION,
	LSQ_ARCHIVE_SUPPORT_RAR_VIEW_RIGHTS
};

static void
lsq_archive_support_rar_init(LSQArchiveSupportRar *support);
static void
lsq_archive_support_rar_class_init(LSQArchiveSupportRarClass *supportclass);

gboolean
lsq_archive_support_rar_refresh_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data);

static void
lsq_archive_support_rar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
lsq_archive_support_rar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gint lsq_archive_support_rar_add(LSQArchive *, GSList *);
static gint lsq_archive_support_rar_extract(LSQArchive *, const gchar *, GSList *);
static gint lsq_archive_support_rar_remove(LSQArchive *, GSList *);
static gint lsq_archive_support_rar_refresh(LSQArchive *);

GType
lsq_archive_support_rar_get_type ()
{
	static GType lsq_archive_support_rar_type = 0;

 	if (!lsq_archive_support_rar_type)
	{
 		static const GTypeInfo lsq_archive_support_rar_info = 
		{
			sizeof (LSQArchiveSupportRarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_archive_support_rar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQArchiveSupportRar),
			0,
			(GInstanceInitFunc) lsq_archive_support_rar_init,
		};

		lsq_archive_support_rar_type = g_type_register_static (LSQ_TYPE_ARCHIVE_SUPPORT, "LSQArchiveSupportRar", &lsq_archive_support_rar_info, 0);
	}
	return lsq_archive_support_rar_type;
}

static void
lsq_archive_support_rar_init(LSQArchiveSupportRar *support)
{
	LSQArchiveSupport *archive_support = LSQ_ARCHIVE_SUPPORT(support);

	archive_support->id = "Rar";


	lsq_archive_support_add_mime(archive_support, "application/x-rar");

	if(g_find_program_in_path("rar"))
	{
		archive_support->add = lsq_archive_support_rar_add;
		archive_support->remove = lsq_archive_support_rar_remove;
	}
	else
	{
		archive_support->add = NULL;
		archive_support->remove = NULL;
	}
	if(g_find_program_in_path("unrar"))
	{
		archive_support->extract = lsq_archive_support_rar_extract;
		archive_support->refresh = lsq_archive_support_rar_refresh;
	}
	else
	{
		archive_support->extract = NULL;
		archive_support->refresh = NULL;
	}

}

static void
lsq_archive_support_rar_class_init(LSQArchiveSupportRarClass *supportclass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (supportclass);
	GParamSpec *pspec = NULL;

	object_class->set_property = lsq_archive_support_rar_set_property;
	object_class->get_property = lsq_archive_support_rar_get_property;

	pspec = g_param_spec_string("extract-password",
		_("Password"),
		_("Password"),
		"",
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_RAR_PASSWORD, pspec);

	pspec = g_param_spec_boolean("extract-overwrite",
		_("Overwrite existing files"),
		_("Overwrite existing files on extraction"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_RAR_EXTRACT_OVERWRITE, pspec);

	pspec = g_param_spec_boolean("view-compressed-size",
		_("Compressed Size"),
		_("View compressed filesize"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_RAR_VIEW_SIZE, pspec);

	pspec = g_param_spec_boolean("view-time",
		_("Time"),
		_("View time"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_RAR_VIEW_TIME, pspec);

	pspec = g_param_spec_boolean("view-date",
		_("Date"),
		_("View date"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_RAR_VIEW_DATE, pspec);

	pspec = g_param_spec_boolean("view-ratio",
		_("Ratio"),
		_("View ratio"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_RAR_VIEW_RATIO, pspec);

	pspec = g_param_spec_boolean("view-uncompressed-size",
		_("Size"),
		_("View filesize"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_RAR_VIEW_LENGTH, pspec);

	pspec = g_param_spec_boolean("view-method",
		_("Method"),
		_("View method"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_RAR_VIEW_METHOD, pspec);

	pspec = g_param_spec_boolean("view-crc32",
		_("Checksum"),
		_("View Checksum"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_RAR_VIEW_CRC_32, pspec);

	pspec = g_param_spec_boolean("view-version",
		_("Compression version"),
		_("View compression version"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_RAR_VIEW_VERSION, pspec);

	pspec = g_param_spec_boolean("view-rights",
		_("Permissions"),
		_("View permissions"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_RAR_VIEW_RIGHTS, pspec);
}

LSQArchiveSupport*
lsq_archive_support_rar_new()
{
	LSQArchiveSupportRar *support;

	support = g_object_new(LSQ_TYPE_ARCHIVE_SUPPORT_RAR,
												 "view-uncompressed-size", TRUE,
	                       "view-compressed-size", TRUE,
												 "view-time", TRUE,
												 "view-date", TRUE,
												 "view-ratio", TRUE,
												 "view-crc32", TRUE,
												 "view-method", TRUE,
												 "view-version", TRUE,
												 "view-rights", TRUE,
												 NULL);
	
	return LSQ_ARCHIVE_SUPPORT(support);
}

static gint
lsq_archive_support_rar_add(LSQArchive *archive, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_RAR(archive->support))
	{
		g_critical("Support is not rar");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lsq_concat_filenames(filenames);
		gchar *archive_path = g_shell_quote(archive->path);
		if(!g_strcasecmp((gchar *)thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-rar"))
		{
			command = g_strconcat("rar a ", archive_path, " ", files, NULL);
			lsq_execute(command, archive, NULL, NULL, NULL, NULL);
		}
		g_free(archive_path);
	}
	return 0;
}

static gint
lsq_archive_support_rar_extract(LSQArchive *archive, const gchar *extract_path, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_RAR(archive->support))
	{
		g_critical("Support is not Rar");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lsq_concat_filenames(filenames);
		gchar *archive_path = g_shell_quote(archive->path);
		gchar *dest_path = g_shell_quote(extract_path);
		if(archive->file_info) /* FIXME */
		{
			if(!g_strcasecmp((gchar *)thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-rar"))
			{
				command = g_strconcat("unrar x ", archive_path, " ", files, " ", dest_path, NULL);
				lsq_execute(command, archive, NULL, NULL, NULL, NULL);
			}	
		} else
		{
			g_free(dest_path);
			g_free(archive_path);
			return 1;
		}
		g_free(dest_path);
		g_free(archive_path);
	}
	return 0;
}

static gint
lsq_archive_support_rar_remove(LSQArchive *archive, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_RAR(archive->support))
	{
		g_critical("Support is not rar");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, thunar_vfs_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lsq_concat_filenames(filenames);
		gchar *archive_path = g_shell_quote(archive->path);
		if(!g_strcasecmp((gchar *)thunar_vfs_mime_info_get_name(archive->mime_info), "application/x-rar"))
		{
			command = g_strconcat("rar d ", archive_path, " ", files, NULL);
			lsq_execute(command, archive, NULL, NULL, NULL, NULL);
		}
		g_free(archive_path);
	}
	return 0;
}

static gint
lsq_archive_support_rar_refresh(LSQArchive *archive)
{
	guint i = 0;
	if(!LSQ_IS_ARCHIVE_SUPPORT_RAR(archive->support))
	{
		g_critical("Support is not Rar");
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
		gchar *archive_path = g_shell_quote(archive->path);
		if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_length) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_UINT64, _("Size"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_size) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_UINT64, _("Compressed Size"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_ratio) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Ratio"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_date) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Date"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_time) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Time"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_rights) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Permissions"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_crc_32) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Checksum"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_method) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING,_("Method"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_version) {
			lsq_archive_set_entry_property_type(archive, i, G_TYPE_STRING, _("Version"));
			i++;
		}
		gchar *command = g_strconcat("unrar v ", archive_path, NULL);
		lsq_execute(command, archive, NULL, NULL, lsq_archive_support_rar_refresh_parse_output, NULL);
		g_free(command);
		g_free(archive_path);
	}
	return 0;
}

gboolean
lsq_archive_support_rar_refresh_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	GIOStatus status = G_IO_STATUS_NORMAL;
	LSQArchive *archive = data;
	gchar *line	= NULL;
	LSQEntry *entry;

	guint64 size;
	guint64 length;
	gpointer props[10]; 
	gint n = 0, a = 0, i = 0, o = 0;
	gint linesize = 0;
	gchar *temp_filename;

	if(!LSQ_IS_ARCHIVE(archive))
		return FALSE;


	if(cond & (G_IO_PRI | G_IO_IN))
	{
		for(o = 0; o < 500; o++)
		{
			i = 0;
			a = 0;

			status = g_io_channel_read_line(ioc, &line, NULL,NULL,NULL);
			if (line == NULL)
 				break; 

			if(line[0] == '\n')
			{
				do
				{
					g_free(line);
					status = g_io_channel_read_line(ioc, &line, NULL,NULL,NULL);
					if (line == NULL)
						break; 
				}
				while(line[0] != '-');
				g_free(line);
				status = g_io_channel_read_line(ioc, &line, NULL,NULL,NULL);
				if (line == NULL)
					break; 
			}
			if(line[0] == '-')
			{
				while(TRUE)
				{
					g_free(line);
					status = g_io_channel_read_line(ioc, &line, NULL,NULL,NULL);
					if (line == NULL)
						break; 
				}
				break;
			}

			temp_filename = line+1;
			line[strlen(temp_filename)] = '\0';
			entry = lsq_archive_add_file(archive, temp_filename);

			status = g_io_channel_read_line(ioc, &line, NULL,NULL,NULL);
			if (line == NULL)
 				break; 
			/* filename, length, size, ratio, date, time, rights, crc-32, method , version*/
			linesize = strlen(line);

			for(n=0; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_length)
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

			if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_size)
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

			if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_ratio)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_date)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_time)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_method)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_crc_32)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_method)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' ' && line[n] != '\n'; n++);

			if(LSQ_ARCHIVE_SUPPORT_RAR(archive->support)->_view_version)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

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

static void
lsq_archive_support_rar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case LSQ_ARCHIVE_SUPPORT_RAR_EXTRACT_OVERWRITE:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_RAR(object)->_extr_overwrite);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_PASSWORD:
			g_value_set_string (value, LSQ_ARCHIVE_SUPPORT_RAR(object)->_extr_password);
			break;

		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_SIZE:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_size);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_DATE:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_date);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_TIME:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_time);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_RATIO:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_ratio);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_LENGTH:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_length);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_METHOD:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_method);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_CRC_32:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_crc_32);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_VERSION:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_version);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_RIGHTS:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_rights);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}

static void
lsq_archive_support_rar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case LSQ_ARCHIVE_SUPPORT_RAR_EXTRACT_OVERWRITE:
			LSQ_ARCHIVE_SUPPORT_RAR(object)->_extr_overwrite = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_PASSWORD:
			LSQ_ARCHIVE_SUPPORT_RAR(object)->_extr_password = (gchar *)g_value_get_string(value);
			break;

		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_SIZE:
			LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_size = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_TIME:
			LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_time = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_DATE:
			LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_date = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_RATIO:
			LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_ratio = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_LENGTH:
			LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_length = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_METHOD:
			LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_method = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_CRC_32:
			LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_crc_32 = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_VERSION:
			LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_version = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_RAR_VIEW_RIGHTS:
			LSQ_ARCHIVE_SUPPORT_RAR(object)->_view_rights= g_value_get_boolean(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}
