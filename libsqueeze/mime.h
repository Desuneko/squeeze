/* *  Copyright (c) 2006 Stephan Arts <psybsd@gmail.com>
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

#ifndef __LSQ_MIME_H__
#define __LSQ_MIME_H__

#include <gtk/gtkicontheme.h>

#ifdef HAVE_THUNAR_VFS
typedef struct _ThunarVfsMimeInfo LSQMimeInfo;
#else
typedef struct _LSQMimeInfo LSQMimeInfo;
#endif

void
lsq_mime_init();

void
lsq_mime_destroy();

void
lsq_mime_convert_to_icon_name(GtkIconTheme *icon_theme, GValue *value);

LSQMimeInfo *
lsq_mime_get_mime_info_for_file(const gchar *path);
LSQMimeInfo *
lsq_mime_get_mime_info_for_filename(const gchar *filename);
LSQMimeInfo *
lsq_mime_get_mime_info(const gchar *mime_type);

const gchar *
lsq_mime_info_get_name(const LSQMimeInfo *mime_info);
const gchar *
lsq_mime_info_get_icon_name(const LSQMimeInfo *mime_info, GtkIconTheme *icon_theme);

void
lsq_mime_info_unref(LSQMimeInfo *mime_info);

#endif /* __LSQ_MIME_H__ */
