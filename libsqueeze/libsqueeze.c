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
#include <glib/gstdio.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "libsqueeze.h"
#include "support-factory.h"
#include "support-reader.h"
#include "archive-iter.h"
#include "archive.h"

#include "internals.h"

void
lsq_init()
{
	support_factory_list = NULL;

	const gchar *filename = NULL;

	lsq_opened_archive_list = NULL;

	gchar *data_squeeze = g_strconcat(DATADIR, "/squeeze", NULL);
	GDir *data_dir = g_dir_open(data_squeeze, 0, NULL);
	if(data_dir)
	{
		while((filename = g_dir_read_name(data_dir)) != NULL)
		{

			if(g_str_has_suffix(filename, ".squeeze"))
			{
                /**
                 * FIXME: factories should be per-mime-type, not per-template
                 */
				gchar *path = g_strconcat(data_squeeze, "/", filename, NULL);
				LSQSupportFactory *factory = lsq_support_reader_parse_file(path);
				if(factory)
				{
					support_factory_list = g_slist_append(support_factory_list, factory);
				}
				g_free(path);
			}
		}

		g_dir_close(data_dir);
	}
}

void
lsq_shutdown()
{
	g_slist_foreach(lsq_opened_archive_list, (GFunc)lsq_close_archive, NULL);
}

/*
 * XAArchive* lsq_new_archive(gchar *path, LSQArchiveType type, gboolean overwrite)
 *
 */
gint
lsq_new_archive(GFile *file, gboolean overwrite, LSQArchive **lp_archive)
{
	if(overwrite)
    {
        g_file_trash (file, NULL, NULL);
    }

	if(g_file_query_exists(file, NULL))
	{
		(*lp_archive) = NULL;
		return 1;
	}

	LSQArchive *archive = lsq_archive_new(file);
	(*lp_archive) = archive;
	if(!archive)
		return 1;
	return 0;
}

/*
 *
 * XAArchive* lsq_open_archive(gchar *path)
 *
 */
gint
lsq_open_archive(GFile *file, LSQArchive **lp_archive)
{
	if(!g_file_query_exists (file, NULL))
	{
		(*lp_archive) = NULL;
		return 1;
	}

	LSQArchive *archive = NULL; /*lsq_opened_archive_get_archive(path); */
	if(!archive)
	{
		archive = lsq_archive_new(file);
		if(archive)
			lsq_opened_archive_list = g_slist_prepend(lsq_opened_archive_list, archive);
	}
	(*lp_archive) = archive;
	if(!archive)
		return 1;
	return 0;
}

gboolean
lsq_is_supported(const gchar *filename)
{
	return FALSE;
}

GSList *
lsq_get_supported_mime_types(LSQCommandType type)
{
	return NULL;
}

const gchar **
lsq_iter_list_to_strv(GSList *list)
{
  guint i;

  const gchar **strv;

  if(!list)
    return NULL;

  strv = g_new(const gchar *, g_slist_length(list)+1);

  i = 0;

  while(list)
  {
    g_debug(list->data);
    strv[i++] = lsq_archive_iter_get_filename(list->data);
    list = g_slist_next(list);
  }

  strv[i] = NULL;

  return strv;
}

