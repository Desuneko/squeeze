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

#include "libsqueeze.h"
#include "support-factory.h"
#include "archive-iter.h"
#include "archive.h"
#include "parser-context.h"
#include "parser.h"
#include "scanf-parser.h"
#include "command-queue.h"
#include "support-reader.h"

#include "internals.h"

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

	xfce_rc_set_group(rc, "Squeeze-Refresh");
  gchar **column_names = xfce_rc_read_list_entry(rc, "X-Squeeze-Headers", ";");
  LSQParser *parser = NULL;
  const gchar *parser_string = xfce_rc_read_entry(rc, "X-Squeeze-Parse", NULL);
  if(parser_string)
  {
    parser = lsq_scanf_parser_new(parser_string);
  }

	gchar **_mime_types = mime_types;
	for(i = 0; _mime_types[i]; ++i)
	{
		LSQSupportTemplate *s_template = g_new(LSQSupportTemplate, 1);

		xfce_rc_set_group(rc, _mime_types[i]);
		/* only add to builder->mime_types if all req. apps are found */
		s_template->required_apps = xfce_rc_read_list_entry(rc, "X-Squeeze-Requires", ";");
		if (s_template->required_apps)
		{
			gchar **_iter = s_template->required_apps;
			s_template->supported = TRUE;
			while(*_iter)
			{
				gchar *path = g_find_program_in_path(*_iter);
				if(path)
				{
					g_free(path);
				}
				else
				{
					s_template->supported = FALSE;
					break;
				}
				_iter++;
			}
		}
		else
			s_template->supported = FALSE;

		s_template->mime_info = thunar_vfs_mime_database_get_info(lsq_mime_database, _mime_types[i]);
		s_template->id = (const gchar *)factory->id;

        const gchar *new_str_queue = xfce_rc_read_entry(rc, "X-Squeeze-New", NULL);
        const gchar *add_str_queue = xfce_rc_read_entry(rc, "X-Squeeze-Add", NULL);
        const gchar *remove_str_queue = xfce_rc_read_entry(rc, "X-Squeeze-Remove", NULL);
        const gchar *extract_str_queue = xfce_rc_read_entry(rc, "X-Squeeze-Extract", NULL);
        const gchar *refresh_str_queue = xfce_rc_read_entry(rc, "X-Squeeze-Refresh", NULL);

	
        if (new_str_queue)    
            s_template->new_cmd_queue     = lsq_command_queue_new(new_str_queue);
        if (add_str_queue)
		    s_template->add_cmd_queue     = lsq_command_queue_new(add_str_queue);
        if (remove_str_queue)
		    s_template->remove_cmd_queue  = lsq_command_queue_new(remove_str_queue);
        if (extract_str_queue)
		    s_template->extract_cmd_queue = lsq_command_queue_new(extract_str_queue);
        if (refresh_str_queue)
		    s_template->refresh_cmd_queue = lsq_command_queue_new(refresh_str_queue);

    s_template->n_properties = g_strv_length(column_names);
    s_template->property_names = column_names;
    s_template->parser = parser;

		if (s_template->supported)
			g_debug("%s supported\n", _mime_types[i]);
		else
			g_debug("%s not supported\n", _mime_types[i]);

		lsq_support_factory_add_template(factory, s_template);
	}
	 
	return factory;
}
