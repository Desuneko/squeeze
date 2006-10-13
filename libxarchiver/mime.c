/*
 *  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
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
#include <glib.h>

#ifdef HAVE_THUNAR_VFS
#define EXO_API_SUBJECT_TO_CHANGE
#include <thunar-vfs/thunar-vfs.h>
#else
#include <gettext.h>
#endif

#include "mime.h"

#ifdef HAVE_THUNAR_VFS
ThunarVfsMimeDatabase  *lxa_mime_database;
#endif /* HAVE_THUNAR_VFS */

void
lxa_mime_init()
{
#ifdef HAVE_THUNAR_VFS
	thunar_vfs_init();
	lxa_mime_database = thunar_vfs_mime_database_get_default();
#endif /* HAVE_THUNAR_VFS */
}

void
lxa_mime_destroy()
{
#ifdef HAVE_THUNAR_VFS
	g_object_unref(lxa_mime_database);
#endif /* HAVE_THUNAR_VFS */
}

gchar *
lxa_mime_get_mime_type_for_file(gchar *path)
{
	gchar *result = NULL;
	gchar *base = g_path_get_basename(path);

#ifdef HAVE_THUNAR_VFS
	ThunarVfsMimeInfo *mime_info;

	mime_info = thunar_vfs_mime_database_get_info_for_file(lxa_mime_database, path, base);
	
	result = g_strdup(thunar_vfs_mime_info_get_name(mime_info));

	thunar_vfs_mime_info_unref(mime_info);

#endif /* HAVE_THUNAR_VFS */
	g_free(base);
	return result;
}

gchar *
lxa_mime_get_mime_type_for_filename(gchar *filename)
{
	gchar *result = NULL;

#ifdef HAVE_THUNAR_VFS
	ThunarVfsMimeInfo *mime_info;

	mime_info = thunar_vfs_mime_database_get_info_for_name(lxa_mime_database, filename);
	
	result = g_strdup(thunar_vfs_mime_info_get_name(mime_info));

	thunar_vfs_mime_info_unref(mime_info);

#endif /* HAVE_THUNAR_VFS */
	return result;
}

void
lxa_mime_convert_to_icon_name(GtkIconTheme *icon_theme, GValue *value)
{
	const gchar *mime_type = g_value_get_string(value);
#ifdef HAVE_THUNAR_VFS
	ThunarVfsMimeInfo *mime_info = thunar_vfs_mime_database_get_info(lxa_mime_database, mime_type);
	const gchar *icon_name = thunar_vfs_mime_info_lookup_icon_name(mime_info, icon_theme);
	if(gtk_icon_theme_has_icon(icon_theme, icon_name))
		g_value_set_string(value, icon_name);
	else
#endif
		g_value_set_string(value, NULL);
	/* g_free((gchar *)mime_type); */
}
