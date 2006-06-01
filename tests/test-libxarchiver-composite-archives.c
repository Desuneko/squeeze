#include <glib.h>
#include <glib-object.h>
#include <libxarchiver/libxarchiver.h>

GMainLoop *loop;

void
test_archive_status_changed(LXAArchive *archive, gpointer data)
{
	if(archive->status == LXA_ARCHIVESTATUS_IDLE)
		g_main_loop_quit(loop);

}

int
main(int argc, char **argv)
{
	g_type_init();
	lxa_init();

	gint ret = 0;

	LXAArchive *archive = lxa_new_archive("./composite-archives/archive.tar.bz2", LXA_ARCHIVETYPE_TAR, LXA_COMPRESSIONTYPE_BZIP2);
	if(archive == NULL)
	{
		g_critical("Could not create a new archive");
		return 1;
	}
	GSList *files = NULL;
	files = g_slist_prepend(files, "./composite-archives/content");

	ret = lxa_archive_add(archive, files);

	g_signal_connect(G_OBJECT(archive), "lxa_status_changed", G_CALLBACK(test_archive_status_changed), NULL);

	loop = g_main_loop_new(NULL, TRUE);
	g_main_loop_run(loop);

	return 0;
}
