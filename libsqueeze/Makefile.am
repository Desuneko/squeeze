lib_LTLIBRARIES = libsqueeze-1.la

libsqueeze_1_la_SOURCES =  \
	internals.c internals.h \
	libsqueeze.c libsqueeze.h \
	libsqueeze-archive.h \
	libsqueeze-support.h \
	libsqueeze-view.h \
	libsqueeze-module.h \
	slist.c slist.h \
	archive.c archive.h \
	archive-iter.c archive-iter.h \
	archive-command.c archive-command.h \
	spawn-command.c spawn-command.h \
	macro-command.c macro-command.h \
	dbus-command.c dbus-command.h \
	archive-tempfs.c archive-tempfs.h \
	command-builder.c command-builder.h \
	command-builder-zip.c command-builder-zip.h \
	command-builder-rar.c command-builder-rar.h \
	command-builder-compr.c command-builder-compr.h \
	command-builder-gnu-tar.c command-builder-gnu-tar.h
#archive-support-compr.c archive-support-compr.h
#archive-support-zip.c archive-support-zip.h
#archive-support-rar.c archive-support-rar.h
#archive-support-gnu-tar.c archive-support-gnu-tar.h

libsqueeze_1_la_CFLAGS = \
	$(GLIB_CFLAGS)  \
	$(THUNAR_VFS_CFLAGS)

libsqueeze_1_la_LIBADD =

libsqueeze_1_la_LDFLAGS = \
  -version-info $(SQUEEZE_VERINFO)

libsqueeze_includedir= \
	$(includedir)/libsqueeze
