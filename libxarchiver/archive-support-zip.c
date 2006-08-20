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

#define EXO_API_SUBJECT_TO_CHANGE

#include <glib.h>
#include <glib-object.h>
#include <thunar-vfs/thunar-vfs.h>

#include "archive.h"
#include "archive-support.h"
#include "archive-support-zip.h"

#include "internals.h"

enum
{
	LXA_ARCHIVE_SUPPORT_ZIP_EXTRACT_OVERWRITE = 1,
	LXA_ARCHIVE_SUPPORT_ZIP_EXTRACT_FRESHEN_EXISTING,
	LXA_ARCHIVE_SUPPORT_ZIP_EXTRACT_UPDATE_EXISTING,
	LXA_ARCHIVE_SUPPORT_ZIP_ADD_COMPRESSION_LEVEL,
	LXA_ARCHIVE_SUPPORT_ZIP_PASSWORD,
};

void
lxa_archive_support_zip_init(LXAArchiveSupportZip *support);
void
lxa_archive_support_zip_class_init(LXAArchiveSupportZipClass *supportclass);

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
}

void
lxa_archive_support_zip_class_init(LXAArchiveSupportZipClass *supportclass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (supportclass);
	GParamSpec *pspec = NULL;

	object_class->set_property = lxa_archive_support_zip_set_property;
	object_class->get_property = lxa_archive_support_zip_get_property;

	pspec = g_param_spec_boolean("extract-overwrite",
		_("Overwrite existing files"),
		_("Overwrite existing files on extraction"),
		FALSE,
		G_PARAM_READWRITE);
	g_object_class_install_property(object_class, LXA_ARCHIVE_SUPPORT_ZIP_EXTRACT_OVERWRITE, pspec);
}

LXAArchiveSupport*
lxa_archive_support_zip_new()
{
	LXAArchiveSupportZip *support;

	support = g_object_new(LXA_TYPE_ARCHIVE_SUPPORT_ZIP, NULL);
	
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

	if(!lxa_archive_support_mime_supported(archive->support, archive->mime))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lxa_concat_filenames(filenames);
		if(!g_strcasecmp((gchar *)archive->mime, "application/x-zip") || 
		   !g_strcasecmp((gchar *)archive->mime, "application/zip"))
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

	if(!lxa_archive_support_mime_supported(archive->support, archive->mime))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lxa_concat_filenames(filenames);
		if(g_file_test(archive->path, G_FILE_TEST_EXISTS))
		{
			if(!g_strcasecmp((gchar *)archive->mime, "application/x-zip") || 
		  	 !g_strcasecmp((gchar *)archive->mime, "application/zip"))
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

	if(!lxa_archive_support_mime_supported(archive->support, archive->mime))
	{
		return 1;
	}
	else
	{
		gchar *command = NULL;
		gchar *files = lxa_concat_filenames(filenames);
		if(!g_strcasecmp((gchar *)archive->mime, "application/x-zip") || 
		   !g_strcasecmp((gchar *)archive->mime, "application/zip"))
		{
			command = g_strconcat("zip -d ", archive->path, " ", files, NULL);
			lxa_execute(command, archive, NULL, NULL, NULL, NULL);
		}
	}
	return 0;
}

void
lxa_archive_support_zip_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch(prop_id)
	{
		case LXA_ARCHIVE_SUPPORT_ZIP_EXTRACT_OVERWRITE:
			g_value_set_boolean(value, LXA_ARCHIVE_SUPPORT_ZIP(object)->_extr_overwrite);
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
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,prop_id,pspec);
			break;
	}
}
