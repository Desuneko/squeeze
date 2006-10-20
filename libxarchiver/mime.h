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

#ifndef __LXA_MIME_H__
#define __LXA_MIME_H__

#include <gtk/gtkicontheme.h>

#ifdef HAVE_THUNAR_VFS
typedef struct _ThunarVfsMimeInfo LXAMimeInfo;
#else
typedef struct _LXAMimeInfo LXAMimeInfo;
#endif

void
lxa_mime_init();

void
lxa_mime_destroy();

void
lxa_mime_convert_to_icon_name(GtkIconTheme *icon_theme, GValue *value);

LXAMimeInfo *
lxa_mime_get_mime_info_for_file(const gchar *path);
LXAMimeInfo *
lxa_mime_get_mime_info_for_filename(const gchar *filename);
LXAMimeInfo *
lxa_mime_get_mime_info(const gchar *mime_type);

const gchar *
lxa_mime_info_get_name(const LXAMimeInfo *mime_info);

void
lxa_mime_info_unref(LXAMimeInfo *mime_info);

#endif /* __LXA_MIME_H__ */
