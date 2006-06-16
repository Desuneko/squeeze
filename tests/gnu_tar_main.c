#include <glib.h>
#include <glib-object.h>
#include <libxarchiver/libxarchiver.h>

GMainLoop *loop;
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
					g_print("Adding to archive %s: SUCCESS\n", archive->path);
					break;
				case(LXA_ARCHIVESTATUS_REMOVE):
					g_print("Remove succeeded\n");
					break;
			}
			lxa_close_archive(archive);
			test++;
			break;
		case(LXA_ARCHIVESTATUS_ERROR):
			switch(archive->oldstatus)
			{
				case(LXA_ARCHIVESTATUS_ADD):
					g_print("Add Failed\n");
					break;
				case(LXA_ARCHIVESTATUS_REMOVE):
					g_print("Remove Failed\n");
					break;
			}
			lxa_close_archive(archive);
			test++;
			break;
		case(LXA_ARCHIVESTATUS_USERBREAK):
			g_print("User canceled\n");
			test++;
			break;
	}
	if(test == 3)
		g_main_loop_quit(loop);
}

int
main(int argc, char **argv)
{
	g_type_init();
	lxa_init();

	GSList *files = NULL;
	files = g_slist_prepend(files, "./composite-archives/content/gpl.txt");

	gint ret = 0;

	LXAArchive *archive_tar  = lxa_new_archive("./composite-archives/archive.tar", LXA_ARCHIVETYPE_TAR, LXA_COMPRESSIONTYPE_NONE);
	LXAArchive *archive_tgz  = lxa_new_archive("./composite-archives/archive.tar.gz", LXA_ARCHIVETYPE_TAR, LXA_COMPRESSIONTYPE_GZIP);
	LXAArchive *archive_tbz2 = lxa_new_archive("./composite-archives/archive.tar.bz2", LXA_ARCHIVETYPE_TAR, LXA_COMPRESSIONTYPE_BZIP2);

	g_print("Creating tar archive:     %s\n", archive_tar ? archive_tar->path : "FAILED");
	g_print("Creating tar.gz archive:  %s\n", archive_tgz ? archive_tgz->path : "FAILED");
	g_print("Creating tar.bz2 archive: %s\n", archive_tbz2? archive_tbz2->path : "FAILED");

	g_signal_connect(G_OBJECT(archive_tar), "lxa_status_changed", G_CALLBACK(test_archive_status_changed), NULL);
	g_signal_connect(G_OBJECT(archive_tgz), "lxa_status_changed", G_CALLBACK(test_archive_status_changed), NULL);
	g_signal_connect(G_OBJECT(archive_tbz2), "lxa_status_changed", G_CALLBACK(test_archive_status_changed), NULL);

	ret = lxa_archive_add(archive_tar, files);
	ret = lxa_archive_add(archive_tgz, files);
	ret = lxa_archive_add(archive_tbz2, files);

	loop = g_main_loop_new(NULL, TRUE);
	g_main_loop_run(loop);
	return 0;
}
