lib_LTLIBRARIES = libxarchiver-1.la

libxarchiver_1_la_SOURCES =  \
	internals.c internals.h \
	libxarchiver.c libxarchiver.h \
	mime.c mime.h \
	slist.c slist.h \
	archive.c archive.h \
	archive-support.c archive-support.h \
	archive-support-zip.c archive-support-zip.h \
	archive-support-gnu-tar.c archive-support-gnu-tar.h

libxarchiver_1_la_CFLAGS = \
	$(GLIB_CFLAGS)  \
	$(THUNAR_VFS_CFLAGS)
libxarchiver_1_la_LIBADD =

libxarchiver_includedir= \
	$(includedir)/libxarchiver
