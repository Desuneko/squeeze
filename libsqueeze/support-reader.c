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
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <thunar-vfs/thunar-vfs.h>
#include <libxfce4util/libxfce4util.h>

#include "libsqueeze-archive.h"
#include "support-factory.h"
#include "archive-iter.h"
#include "archive.h"
#include "support-reader.h"

static void
lsq_support_reader_class_init(LSQSupportReaderClass *);
static void
lsq_support_reader_init(LSQSupportReader *);
static void
lsq_support_reader_dispose(GObject *object);
static void
lsq_support_reader_finalize(GObject *object);

static GObjectClass *parent_class;

GType
lsq_support_reader_get_type ()
{
	static GType lsq_support_reader_type = 0;

 	if (!lsq_support_reader_type)
	{
 		static const GTypeInfo lsq_support_reader_info = 
		{
			sizeof (LSQSupportReaderClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) lsq_support_reader_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (LSQSupportReader),
			0,
			(GInstanceInitFunc) lsq_support_reader_init,
			NULL
		};

		lsq_support_reader_type = g_type_register_static (G_TYPE_OBJECT, "LSQSupportReader", &lsq_support_reader_info, 0);
	}
	return lsq_support_reader_type;
}

static void
lsq_support_reader_class_init(LSQSupportReaderClass *support_reader_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(support_reader_class);

	object_class->dispose = lsq_support_reader_dispose;
	object_class->finalize = lsq_support_reader_finalize;
	
	parent_class = g_type_class_peek(G_TYPE_OBJECT); 
}

static void
lsq_support_reader_init(LSQSupportReader *support)
{

}

static void
lsq_support_reader_dispose(GObject *object)
{

}

static void
lsq_support_reader_finalize(GObject *object)
{

}

LSQSupportReader *
lsq_support_reader_new()
{
	LSQSupportReader *reader;

	reader = g_object_new(LSQ_TYPE_SUPPORT_READER, NULL);

	return reader;
}

LSQSupportFactory *
lsq_support_reader_parse_file(const gchar *filename)
{
	gint i = 0;
	LSQSupportFactory *factory = g_object_new(LSQ_TYPE_SUPPORT_FACTORY, NULL);

	XfceRc *rc = xfce_rc_simple_open(filename, TRUE);

	xfce_rc_set_group(rc, "Desktop Entry");

	const gchar *type = xfce_rc_read_entry(rc, "Type", "");
	if(strcmp(type, "X-Squeeze-Archiver"))
	{
		g_object_unref(factory);
		return NULL;
	}

	const gchar *name = xfce_rc_read_entry(rc, "Name", NULL);
	if(name)
		factory->id = g_strdup(name);
	else
	{
		g_object_unref(factory);
		return NULL;
	}

	gchar **mime_types = xfce_rc_read_list_entry(rc, "MimeType", ";");

	gchar **_mime_types = mime_types;
	for(i = 0; _mime_types[i]; ++i)
	{
		LSQMimeSupport *mime_support = g_new(LSQMimeSupport, 1);

		xfce_rc_set_group(rc, _mime_types[i]);
		/* only add to builder->mime_types if all req. apps are found */
		mime_support->required_apps = xfce_rc_read_list_entry(rc, "X-Squeeze-Requires", ";");
		gchar **_iter = mime_support->required_apps;
		mime_support->supported = TRUE;
		while(_iter)
		{
			gchar *path = g_find_program_in_path(*_iter);
			if(path)
				g_free(path);
			else
			{
				mime_support->supported = FALSE;
				break;
			}
			_iter++;
		}

		mime_support->mime = g_strdup(_mime_types[i]);

		mime_support->new_cmd_queue     = xfce_rc_read_list_entry(rc, "X-Squeeze-New", ";");
		mime_support->add_cmd_queue     = xfce_rc_read_list_entry(rc, "X-Squeeze-Add", ";");
		mime_support->remove_cmd_queue  = xfce_rc_read_list_entry(rc, "X-Squeeze-Remove", ";");
		mime_support->extract_cmd_queue = xfce_rc_read_list_entry(rc, "X-Squeeze-Extract", ";");
		mime_support->refresh_cmd_queue = xfce_rc_read_list_entry(rc, "X-Squeeze-Refresh", ";");

		factory->mime_support = g_slist_prepend(factory->mime_support, mime_support);
	}
	 
	return factory;
}
