lib_LTLIBRARIES = libsqueeze-1.la

libsqueeze_1_la_SOURCES =  \
	internals.c internals.h \
	libsqueeze.c libsqueeze.h \
	libsqueeze-archive.h \
	libsqueeze-view.h \
	libsqueeze-command.h \
	libsqueeze-module.h \
	slist.c slist.h \
	vfs-mime.c vfs-mime.h \
	archive.c archive.h \
	archive-iter.c archive-iter.h \
	archive-command.c archive-command.h \
	spawn-command.c spawn-command.h \
	macro-command.c macro-command.h \
	remove-command.c remove-command.h \
	xfce-launch-command.c xfce-launch-command.h \
	archive-tempfs.c archive-tempfs.h \
	builder-settings.c builder-settings.h \
	command-builder.c command-builder.h \
	command-builder-zip.c command-builder-zip.h \
	command-builder-rar.c command-builder-rar.h \
	command-builder-compr.c command-builder-compr.h \
	command-builder-gnu-tar.c command-builder-gnu-tar.h

libsqueeze_1_la_CFLAGS = \
	$(GLIB_CFLAGS)  \
	$(DBUS_CFLAGS)  \
	$(THUNAR_VFS_CFLAGS)

libsqueeze_1_la_LIBADD =

libsqueeze_1_la_LDFLAGS = \
  -version-info $(SQUEEZE_VERINFO)

libsqueeze_includedir= \
	$(includedir)/libsqueeze
