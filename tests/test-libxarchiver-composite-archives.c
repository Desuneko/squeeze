#include <glib.h>
#include <glib-object.h>
#include <libxarchiver/libxarchiver.h>

GMainLoop *loop;
guint test = 0;

void
test_archive_status_changed(LXAArchive *archive, gpointer data)
{
	if(archive->status == LXA_ARCHIVESTATUS_IDLE)
	{
		test++;
		if(test == 3)
			g_main_loop_quit(loop);
	}
}

int
main(int argc, char **argv)
{
	g_type_init();
	lxa_init();

	gint ret = 0;

	LXAArchive *archive_tbz2 = lxa_new_archive("./composite-archives/archive.tar.bz2", LXA_ARCHIVETYPE_TAR, LXA_COMPRESSIONTYPE_BZIP2);
	if(archive_tbz2 == NULL)
	{
		g_critical("Could not create a new archive");
		return 1;
	}

	LXAArchive *archive_tgz = lxa_new_archive("./composite-archives/archive.tar.gz", LXA_ARCHIVETYPE_TAR, LXA_COMPRESSIONTYPE_GZIP);
	if(archive_tgz == NULL)
	{
		g_critical("Could not create a new archive");
		return 1;
	}

	LXAArchive *archive_tar = lxa_new_archive("./composite-archives/archive.tar", LXA_ARCHIVETYPE_TAR, LXA_COMPRESSIONTYPE_NONE);
	if(archive_tar == NULL)
	{
		g_critical("Could not create a new archive");
		return 1;
	}

	GSList *files = NULL;
	files = g_slist_prepend(files, "./composite-archives/content");

	ret = lxa_archive_add(archive_tbz2, files);
	ret = lxa_archive_add(archive_tgz, files);
	ret = lxa_archive_add(archive_tar, files);

	g_signal_connect(G_OBJECT(archive_tbz2), "lxa_status_changed", G_CALLBACK(test_archive_status_changed), NULL);
	g_signal_connect(G_OBJECT(archive_tgz), "lxa_status_changed", G_CALLBACK(test_archive_status_changed), NULL);
	g_signal_connect(G_OBJECT(archive_tar), "lxa_status_changed", G_CALLBACK(test_archive_status_changed), NULL);

	loop = g_main_loop_new(NULL, TRUE);
	g_main_loop_run(loop);

	return 0;
}
