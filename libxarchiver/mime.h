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

#ifndef __LXA_MIME_H__
#define __LXA_MIME_H__

#include <gtk/gtkicontheme.h>

void
lxa_mime_init();

void
lxa_mime_destroy();

void
lxa_mime_convert_to_icon_name(GtkIconTheme *icon_theme, GValue *value);

gchar *
lxa_mime_get_mime_type_for_file(gchar *path);
gchar *
lxa_mime_get_mime_type_for_filename(gchar *filename);

#endif /* __LXA_MIME_H__ */
