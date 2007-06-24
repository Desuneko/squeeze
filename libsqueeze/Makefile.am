lib_LTLIBRARIES = libsqueeze-1.la

libsqueeze_1_la_SOURCES =  \
	internals.c internals.h \
	libsqueeze.c libsqueeze.h \
	libsqueeze-archive.h \
	libsqueeze-view.h \
	libsqueeze-command.h \
	libsqueeze-module.h \
	libsqueeze-mime-support.h \
	slist.c slist.h \
	mime-support.c mime-support.h \
	archive.c archive.h \
	archive-iter.c archive-iter.h \
	archive-tempfs.c archive-tempfs.h \
	support-factory.c support-factory.h \
	support-reader.c support-reader.h

# archive-command.c archive-command.h
# spawn-command.c spawn-command.h
# macro-command.c macro-command.h
# remove-command.c remove-command.h
# xfce-launch-command.c xfce-launch-command.h

libsqueeze_1_la_CFLAGS = \
	$(GLIB_CFLAGS)  \
	$(DBUS_CFLAGS)  \
	$(THUNAR_VFS_CFLAGS)

libsqueeze_1_la_LIBADD =

libsqueeze_1_la_LDFLAGS = \
  -version-info $(SQUEEZE_VERINFO)

libsqueeze_includedir= \
	$(includedir)/libsqueeze

INCLUDES = -I$(top_srcdir)
