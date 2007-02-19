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
#include <glib.h>
#include <glib-object.h>
#include <thunar-vfs/thunar-vfs.h>
#include <libsqueeze/libsqueeze.h>

GMainLoop *loop = NULL;
gint ret_val = 0;
gchar *filename = NULL;

static GOptionEntry entries[] =
{
	{	"new", 'n', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &filename,
		NULL,
		NULL
	},
	{ NULL }
};

void
cb_command_terminated(LSQArchive *archive)
{
	if(loop)
		g_main_loop_quit(loop);
	else
		ret_val = 1;
}

int main(int argc, char **argv)
{
	g_type_init();
	thunar_vfs_init();
	lsq_init();

	LSQArchive *archive = NULL;
	LSQArchiveSupport *archive_support = NULL;
	GSList *files = NULL;
	gint i = 0;

	GOptionContext *opt_context = g_option_context_new("test-add -n <archive> [filename ...]");
	g_option_context_add_main_entries(opt_context, entries, NULL);
	g_option_context_parse (opt_context, &argc, &argv, NULL);

	if(filename == NULL)
	{
		g_print("Filename is not specified\n");
		return 1;
	}

	lsq_new_archive(filename, TRUE, NULL, &archive);
	archive_support = lsq_get_support_for_mimetype(lsq_archive_get_mimetype(archive));

	g_signal_connect(G_OBJECT(archive), "command-terminated", G_CALLBACK(cb_command_terminated), NULL);

	for(i = 1; i < argc; i++)
	{
		files = g_slist_prepend(files, argv[i]);
	}

	if(lsq_archive_support_add(archive_support, archive, files))
		ret_val = 1;

	if(ret_val == 0)
	{
		loop = g_main_loop_new(NULL, FALSE);
		g_main_loop_run(loop);
	}

	lsq_close_archive(archive);

	lsq_shutdown();
	thunar_vfs_shutdown();

	return ret_val;
}
