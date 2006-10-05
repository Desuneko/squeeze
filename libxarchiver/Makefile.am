noinst_LIBRARIES = libxarchiver.a

libxarchiver_a_SOURCES =            \
	libxarchiver.c libxarchiver.h     \
	internals.c internals.h \
	slist.c slist.h \
	archive.c archive.h \
	archive-support.c archive-support.h \
  archive-support-zip.c archive-support-zip.h \
	archive-support-gnu-tar.c archive-support-gnu-tar.h

libxarchiver_a_CFLAGS = \
	$(GLIB_CFLAGS)  \
	$(THUNARVFS_CFLAGS)
libxarchiver_a_LIBADD =


libxarchiver_includedir= \
	$(includedir)/libxarchiver
