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
#include "archive-support-unrar.h"

#include "internals.h"

#define XA_TEST_ACTION_ICON "gtk-index"

enum
{
	LSQ_ARCHIVE_SUPPORT_UNRAR_EXTRACT_OVERWRITE = 1,
	LSQ_ARCHIVE_SUPPORT_UNRAR_ADD_COMPRESSION_LEVEL,
	LSQ_ARCHIVE_SUPPORT_UNRAR_PASSWORD,

	LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_SIZE,
	LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_COMPRESSED,
	LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_DATE,
	LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_TIME,
	LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_RATIO,
	LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_METHOD,
	LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_CHECKSUM,
	LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_VERSION
};

static void
lsq_archive_support_unrar_init(LSQArchiveSupportUnrar *support);
static void
lsq_archive_support_unrar_class_init(LSQArchiveSupportUnrarClass *supportclass);

gboolean
lsq_archive_support_unrar_refresh_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data);

static void
lsq_archive_support_unrar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void
lsq_archive_support_unrar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gint lsq_archive_support_unrar_add(LSQArchive *, GSList *);
static gint lsq_archive_support_unrar_extract(LSQArchive *, gchar *, GSList *);
static gint lsq_archive_support_unrar_remove(LSQArchive *, GSList *);
static gint lsq_archive_support_unrar_refresh(LSQArchive *);

static void lsq_archive_support_unrar_integrity_test(LSQArchiveSupport *, LSQArchive *, gpointer);

GType
lsq_archive_support_unrar_get_type ()
{
	static GType lsq_archive_support_unrar_type = 0;

 	if (!lsq_archive_support_unrar_type)
	{
 		static const GTypeInfo lsq_archive_support_unrar_info = 
		{
			sizeof (LSQArchiveSupportUnrarClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_archive_support_unrar_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQArchiveSupportUnrar),
			0,
			(GInstanceInitFunc) lsq_archive_support_unrar_init,
		};

		lsq_archive_support_unrar_type = g_type_register_static (LSQ_TYPE_ARCHIVE_SUPPORT, "LSQArchiveSupportUnrar", &lsq_archive_support_unrar_info, 0);
	}
	return lsq_archive_support_unrar_type;
}

static void
lsq_archive_support_unrar_init(LSQArchiveSupportUnrar *support)
{
	LSQArchiveSupport *archive_support = LSQ_ARCHIVE_SUPPORT(support);
	LSQCustomAction *custom_action = NULL;

	archive_support->id = "Unrar";

	lsq_archive_support_add_mime(archive_support, "application/unrar");
	lsq_archive_support_add_mime(archive_support, "application/x-unrar");

	archive_support->add = lsq_archive_support_unrar_add;
	archive_support->extract = lsq_archive_support_unrar_extract;
	archive_support->remove = lsq_archive_support_unrar_remove;
	archive_support->refresh = lsq_archive_support_unrar_refresh;
	
	custom_action = lsq_custom_action_new("menu-test",
	                                    _("Test"), 
																			/* TRANSATORS: first line is short comment, after newline is long comment */
																			_("Test archive integrity\nTest the integrity of the archive"),
																			XA_TEST_ACTION_ICON,
																			lsq_archive_support_unrar_integrity_test,
																			archive_support,
																			NULL);
}

static void
lsq_archive_support_unrar_class_init(LSQArchiveSupportUnrarClass *supportclass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (supportclass);
	GParamSpec *pspec = NULL;

	object_class->set_property = lsq_archive_support_unrar_set_property;
	object_class->get_property = lsq_archive_support_unrar_get_property;

	pspec = g_param_spec_string("extract-password",
		_("Password"),
		_("Password"),
		"",
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_UNRAR_PASSWORD, pspec);

	pspec = g_param_spec_boolean("extract-overwrite",
		_("Overwrite existing files"),
		_("Overwrite existing files on extraction"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_UNRAR_EXTRACT_OVERWRITE, pspec);

	pspec = g_param_spec_boolean("view-size",
		_("Filesize"),
		_("View filesize"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_SIZE, pspec);

	pspec = g_param_spec_boolean("view-time",
		_("Time"),
		_("View time"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_TIME, pspec);

	pspec = g_param_spec_boolean("view-date",
		_("Date"),
		_("View date"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_DATE, pspec);

	pspec = g_param_spec_boolean("view-ratio",
		_("Ratio"),
		_("View ratio"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_RATIO, pspec);

	pspec = g_param_spec_boolean("view-length",
		_("Length"),
		_("View length"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_LENGTH, pspec);

	pspec = g_param_spec_boolean("view-method",
		_("Method"),
		_("View method"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_METHOD, pspec);

	pspec = g_param_spec_boolean("view-crc32",
		_("CRC-32"),
		_("View CRC-32"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_CRC_32, pspec);
}

LSQArchiveSupport*
lsq_archive_support_unrar_new()
{
	LSQArchiveSupportUnrar *support;

	support = g_object_new(LSQ_TYPE_ARCHIVE_SUPPORT_UNRAR,
												 "view-length", TRUE,
	                       "view-size", TRUE,
												 "view-time", TRUE,
												 "view-date", TRUE,
												 "view-ratio", TRUE,
												 "view-crc32", TRUE,
												 "view-method", TRUE,
												 NULL);
	
	return LSQ_ARCHIVE_SUPPORT(support);
}

static gint
lsq_archive_support_unrar_add(LSQArchive *archive, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_UNRAR(archive->support))
	{
		g_critical("Support is not unrar");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, lsq_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lsq_concat_filenames(filenames);
		if(!g_strcasecmp((gchar *)lsq_mime_info_get_name(archive->mime_info), "application/x-unrar") || 
		   !g_strcasecmp((gchar *)lsq_mime_info_get_name(archive->mime_info), "application/unrar"))
		{
			command = g_strconcat("unrar -r ", archive->path, " ", files, NULL);
			lsq_execute(command, archive, NULL, NULL, NULL, NULL);
		}
	}
	return 0;
}

static gint
lsq_archive_support_unrar_extract(LSQArchive *archive, gchar *dest_path, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_UNRAR(archive->support))
	{
		g_critical("Support is not Unrar");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, lsq_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lsq_concat_filenames(filenames);
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp((gchar *)lsq_mime_info_get_name(archive->mime_info), "application/x-unrar") || 
		     !g_strcasecmp((gchar *)lsq_mime_info_get_name(archive->mime_info), "application/unrar"))
			{
				command = g_strconcat("ununrar -o ", archive->path, " ", files, " -d ", dest_path, NULL);
#ifdef DEBUG
				g_debug("Extracting archive '%s' to '%s'", archive->path, dest_path);
#endif /* DEBUG */
				lsq_execute(command, archive, NULL, NULL, NULL, NULL);
			}	
		} else
			return 1;
	}
	return 0;
}

static gint
lsq_archive_support_unrar_remove(LSQArchive *archive, GSList *filenames)
{
	if(!LSQ_IS_ARCHIVE_SUPPORT_UNRAR(archive->support))
	{
		g_critical("Support is not unrar");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, lsq_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lsq_concat_filenames(filenames);
		if(!g_strcasecmp((gchar *)lsq_mime_info_get_name(archive->mime_info), "application/x-unrar") || 
		   !g_strcasecmp((gchar *)lsq_mime_info_get_name(archive->mime_info), "application/unrar"))
		{
			command = g_strconcat("unrar -d ", archive->path, " ", files, NULL);
			lsq_execute(command, archive, NULL, NULL, NULL, NULL);
		}
	}
	return 0;
}

static gint
lsq_archive_support_unrar_refresh(LSQArchive *archive)
{
	guint i = 0;
	if(!LSQ_IS_ARCHIVE_SUPPORT_UNRAR(archive->support))
	{
		g_critical("Support is not Unrar");
		return -1;
	}

	if(!lsq_archive_support_mime_supported(archive->support, lsq_mime_info_get_name(archive->mime_info)))
	{
		return 1;
	}
	else
	{
		i = LSQ_ARCHIVE_PROP_USER;
		if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_length) {
			lsq_archive_set_property_type(archive, i, G_TYPE_UINT64, _("Length"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_method) {
			lsq_archive_set_property_type(archive, i, G_TYPE_STRING,_("Method"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_size) {
			lsq_archive_set_property_type(archive, i, G_TYPE_UINT64, _("Size"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_ratio) {
			lsq_archive_set_property_type(archive, i, G_TYPE_STRING, _("Ratio"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_date) {
			lsq_archive_set_property_type(archive, i, G_TYPE_STRING, _("Date"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_time) {
			lsq_archive_set_property_type(archive, i, G_TYPE_STRING, _("Time"));
			i++;
		}
		if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_crc_32) {
			lsq_archive_set_property_type(archive, i, G_TYPE_STRING, _("CRC-32"));
			i++;
		}
		gchar *command = g_strconcat("ununrar -lv -qq " , archive->path, NULL);
		lsq_execute(command, archive, NULL, NULL, lsq_archive_support_unrar_refresh_parse_output, NULL);
		g_free(command);
	}
	return 0;
}

gboolean
lsq_archive_support_unrar_refresh_parse_output(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
	GIOStatus status = G_IO_STATUS_NORMAL;
	LSQArchive *archive = data;
	gchar *line	= NULL;
	LSQEntry *entry;

	guint64 size;
	guint64 length;
	gpointer props[8]; 
	gint n = 0, a = 0, i = 0, o = 0;
	gchar *temp_filename = NULL;
	gint linesize = 0;

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
			/* length, method , size, ratio, date, time, crc-32, filename*/
			linesize = strlen(line);

			for(n=0; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_length)
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

			if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_method)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_size)
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

			if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_ratio)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_date)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_time)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			a = n;
			for(; n < linesize && line[n] != ' '; n++);

			if(LSQ_ARCHIVE_SUPPORT_UNRAR(archive->support)->_view_crc_32)
			{
				line[n] = '\0';
				props[i] = line + a;
				i++;
			}
			n++;

			for(; n < linesize && line[n] == ' '; n++);
			temp_filename = g_strchomp(line+n); 

			entry = lsq_archive_add_file(archive, temp_filename);
			lsq_archive_iter_set_propsv(archive, entry, (gconstpointer*)props);
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
		lsq_archive_set_status(archive, LSQ_ARCHIVESTATUS_IDLE);
		return FALSE; 
	}
	return TRUE;
}

static void
lsq_archive_support_unrar_integrity_test(LSQArchiveSupport *support, LSQArchive *archive, gpointer user_data)
{
#ifdef DEBUG
	g_debug("Custom action %s called", __FUNCTION__);
#endif /* DEBUG */
}

static void
lsq_archive_support_unrar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case LSQ_ARCHIVE_SUPPORT_UNRAR_EXTRACT_OVERWRITE:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_extr_overwrite);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_PASSWORD:
			g_value_set_string (value, LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_extr_password);
			break;

		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_SIZE:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_size);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_DATE:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_date);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_TIME:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_time);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_RATIO:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_ratio);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_LENGTH:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_length);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_METHOD:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_method);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_CRC_32:
			g_value_set_boolean(value, LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_crc_32);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}

static void
lsq_archive_support_unrar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case LSQ_ARCHIVE_SUPPORT_UNRAR_EXTRACT_OVERWRITE:
			LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_extr_overwrite = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_PASSWORD:
			LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_extr_password = (gchar *)g_value_get_string(value);
			break;

		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_SIZE:
			LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_size = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_TIME:
			LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_time = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_DATE:
			LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_date = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_RATIO:
			LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_ratio = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_LENGTH:
			LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_length = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_METHOD:
			LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_method = g_value_get_boolean(value);
			break;
		case LSQ_ARCHIVE_SUPPORT_UNRAR_VIEW_CRC_32:
			LSQ_ARCHIVE_SUPPORT_UNRAR(object)->_view_crc_32 = g_value_get_boolean(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}
