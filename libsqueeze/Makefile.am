lib_LTLIBRARIES = libsqueeze-2.la

libsqueeze_2_la_SOURCES =  \
	archive.c archive.h \
	archive-iter-pool.c archive-iter-pool.h \
	archive-iter.c archive-iter.h \
	archive-tempfs.c archive-tempfs.h \
	command-queue.c command-queue.h \
	command-option.c command-option.h \
	internals.c internals.h \
	libsqueeze.c libsqueeze.h \
	libsqueeze-view.h \
	parser-context.c parser-context.h \
	parser.c parser.h \
	scanf-parser.c scanf-parser.h \
	slist.c slist.h \
	support-reader.c support-reader.h \
	support-template.c support-template.h \
	support-factory.c support-factory.h

# archive-command.c archive-command.h
# spawn-command.c spawn-command.h
# macro-command.c macro-command.h
# remove-command.c remove-command.h
# xfce-launch-command.c xfce-launch-command.h

libsqueeze_2_la_CFLAGS = \
	-DDATADIR=\"$(datadir)\" \
	$(GLIB_CFLAGS)  \
	$(DBUS_CFLAGS)  \
	$(GIO_CFLAGS)   \
	$(LIBXFCE4UTIL_CFLAGS)

libsqueeze_2_la_LDFLAGS = \
	-version-info $(SQUEEZE_VERINFO) \
	$(GLIB_LIBS)  \
	$(DBUS_LIBS)  \
	$(GIO_LIBS)   \
	$(LIBXFCE4UTIL_LIBS)

libsqueeze_includedir= \
	$(includedir)/libsqueeze

INCLUDES = -I$(top_srcdir)
