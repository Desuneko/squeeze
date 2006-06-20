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
#include <glib.h>
#include <glib-object.h>
#include <libxarchiver/libxarchiver.h>

GMainLoop *loop;
guint max_test = 0;
guint test = 0;

void
test_archive_status_changed(LXAArchive *archive, gpointer data)
{
	switch(archive->status)
	{
		case(LXA_ARCHIVESTATUS_IDLE):
			switch(archive->oldstatus)
			{
				case(LXA_ARCHIVESTATUS_ADD):
					break;
				case(LXA_ARCHIVESTATUS_REMOVE):
					break;
				case(LXA_ARCHIVESTATUS_EXTRACT):
					break;
				case(LXA_ARCHIVESTATUS_IDLE):
				case(LXA_ARCHIVESTATUS_ERROR):
				case(LXA_ARCHIVESTATUS_USERBREAK):
					break;
			}
			test++;
			break;
		case(LXA_ARCHIVESTATUS_ERROR):
			switch(archive->oldstatus)
			{
				case(LXA_ARCHIVESTATUS_ADD):
					break;
				case(LXA_ARCHIVESTATUS_REMOVE):
					break;
				case(LXA_ARCHIVESTATUS_EXTRACT):
					break;
				case(LXA_ARCHIVESTATUS_IDLE):
				case(LXA_ARCHIVESTATUS_ERROR):
				case(LXA_ARCHIVESTATUS_USERBREAK):
					break;
			}
			test++;
			break;
		case(LXA_ARCHIVESTATUS_USERBREAK):
			g_print("User canceled\n");
			test++;
			break;
		case(LXA_ARCHIVESTATUS_ADD):
		case(LXA_ARCHIVESTATUS_REMOVE):
		case(LXA_ARCHIVESTATUS_EXTRACT):
			break;
	}
	if(test == max_test)
	{
		lxa_destroy();
		g_main_loop_quit(loop);
	}
}

int
main(int argc, char **argv)
{
	g_type_init();
	lxa_init();

	GSList *files = NULL;
	files = g_slist_prepend(files, "./composite-archives/content/gpl.txt");

	gint ret = 0;

	LXAArchive *archive_tar  = NULL; 
	LXAArchive *archive_tgz  = NULL; 
	LXAArchive *archive_tbz2 = NULL; 

	lxa_new_archive("./composite-archives/archive.tar", LXA_ARCHIVETYPE_TAR, LXA_COMPRESSIONTYPE_NONE, TRUE, &archive_tar);
	lxa_new_archive("./composite-archives/archive.tar.gz", LXA_ARCHIVETYPE_TAR, LXA_COMPRESSIONTYPE_GZIP, TRUE, &archive_tgz);
	lxa_new_archive("./composite-archives/archive.tar.bz2", LXA_ARCHIVETYPE_TAR, LXA_COMPRESSIONTYPE_BZIP2, TRUE, &archive_tbz2);

	g_print("Creating tar archive:     %s\n", archive_tar ? archive_tar->path : "FAILED");
	g_print("Creating tar.gz archive:  %s\n", archive_tgz ? archive_tgz->path : "FAILED");
	g_print("Creating tar.bz2 archive: %s\n", archive_tbz2? archive_tbz2->path : "FAILED");

	if(archive_tar)
	{
		max_test++;
		g_signal_connect(G_OBJECT(archive_tar), "lxa_status_changed", G_CALLBACK(test_archive_status_changed), NULL);
		ret = lxa_archive_add(archive_tar, files);
	}
	if(archive_tgz)
	{
		max_test++;
		g_signal_connect(G_OBJECT(archive_tgz), "lxa_status_changed", G_CALLBACK(test_archive_status_changed), NULL);
		ret = lxa_archive_add(archive_tgz, files);
	}
	if(archive_tbz2)
	{
		max_test++;
		g_signal_connect(G_OBJECT(archive_tbz2), "lxa_status_changed", G_CALLBACK(test_archive_status_changed), NULL);
		ret = lxa_archive_add(archive_tbz2, files);
	}

	if((!archive_tar) && (!archive_tgz) && (!archive_tbz2))
		return -1;
	loop = g_main_loop_new(NULL, TRUE);
	g_main_loop_run(loop);
	return 0;
}
