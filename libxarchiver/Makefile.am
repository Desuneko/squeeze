noinst_LIBRARIES = libxarchiver.a

libxarchiver_a_SOURCES =            \
	libxarchiver.c libxarchiver.h     \
	archive.c archive.h \
	internals.c internals.h           \
	archive-support.c archive-support.h \
	archive-support-zip.c archive-support-zip.h \
	archive-support-gnu-tar.c archive-support-gnu-tar.h \
	compression-support.c compression-support.h \
	compression-support-gzip.c compression-support-gzip.h \
	compression-support-bzip2.c compression-support-bzip2.h

libxarchiver_a_CFLAGS = \
	$(GLIB_CFLAGS)
libxarchiver_a_LIBADD =


libxarchiver_includedir= \
	$(includedir)/libxarchiver
