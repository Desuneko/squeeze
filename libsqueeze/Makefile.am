lib_LTLIBRARIES = libsqueeze-1.la

libsqueeze_1_la_SOURCES =  \
	internals.c internals.h \
	libsqueeze.c libsqueeze.h \
	mime.c mime.h \
	slist.c slist.h \
	archive.c archive.h \
	archive-support.c archive-support.h \
	archive-support-zip.c archive-support-zip.h \
	archive-support-gnu-tar.c archive-support-gnu-tar.h

libsqueeze_1_la_CFLAGS = \
	$(GLIB_CFLAGS)  \
	$(THUNAR_VFS_CFLAGS)

libsqueeze_1_la_LIBADD =

libsqueeze_1_la_LDFLAGS = \
  -version-info $(SQUEEZE_VERINFO)

libsqueeze_includedir= \
	$(includedir)/libsqueeze