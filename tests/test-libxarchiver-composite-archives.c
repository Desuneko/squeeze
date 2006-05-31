#include <glib.h>
#include <glib-object.h>
#include <libxarchiver/libxarchiver.h>

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
	gchar *files[] = {"./composite-archives/content", NULL};
	ret = lxa_archive_add(archive, files);
	if(ret)
	{
		g_critical("Could not add content");
		return 1;
	}
	return 0;
}
