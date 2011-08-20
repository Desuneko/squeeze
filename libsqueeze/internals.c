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
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "libsqueeze.h"
#include "support-template.h"
#include "archive.h"
#include "archive-iter.h"
#include "support-factory.h"

#include "libsqueeze-view.h"

#include "internals.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

static gint
lsq_opened_archives_lookup_archive(gconstpointer open_archive, gconstpointer path);

gchar *
lsq_concat_filenames(GSList *filenames)
{
	GSList *_filenames = filenames;
	gchar *concat_str = g_strdup(" "), *_concat_str;

	while(_filenames)
	{
		_concat_str = concat_str;
		concat_str = g_strconcat(concat_str, " ", g_shell_quote(_filenames->data) , NULL);
		_filenames = _filenames->next;
		g_free(_concat_str);
	}
	return concat_str;
}

LSQArchive *
lsq_opened_archive_get_archive(gchar *path)
{
	GSList *result = g_slist_find_custom(lsq_opened_archive_list, path, lsq_opened_archives_lookup_archive);
	if(result)
	{
		g_object_ref(result->data);
		return result->data;
	}
	return NULL;
}


static gint
lsq_opened_archives_lookup_archive(gconstpointer open_archive, gconstpointer uri)
{
#ifdef DEBUG
	g_return_val_if_fail(open_archive, 1);
#endif
    GFile *file = g_file_new_for_path (uri);
    
	if(g_file_equal (lsq_archive_get_file(LSQ_ARCHIVE(open_archive)), file))
	{
        g_object_unref (file);
		return 0;
	}
    g_object_unref (file);
    return 1;
}
