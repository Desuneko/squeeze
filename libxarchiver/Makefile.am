noinst_LIBRARIES = libxarchiver.a

libxarchiver_a_SOURCES =            \
	libxarchiver.c libxarchiver.h     \
	internals.c internals.h \
	archive.c archive.h \
	archive-support.c archive-support.h \
  archive-support-zip.c archive-support-zip.h \
  archive-support-rar.c archive-support-rar.h \
  archive-support-unrar.c archive-support-unrar.h \
	archive-support-gnu-tar.c archive-support-gnu-tar.h

libxarchiver_a_CFLAGS = \
	$(GLIB_CFLAGS)  \
	$(THUNARVFS_CFLAGS)
libxarchiver_a_LIBADD =


libxarchiver_includedir= \
	$(includedir)/libxarchiver
