lib_LTLIBRARIES = libsqueeze-1.la

libsqueeze_1_la_SOURCES =  \
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

libsqueeze_1_la_CFLAGS = \
	-DDATADIR=\"$(datadir)\" \
	$(GLIB_CFLAGS)  \
	$(DBUS_CFLAGS)  \
	$(THUNAR_VFS_CFLAGS)

libsqueeze_1_la_LIBADD =

libsqueeze_1_la_LDFLAGS = \
  -version-info $(SQUEEZE_VERINFO)

libsqueeze_includedir= \
	$(includedir)/libsqueeze

INCLUDES = -I$(top_srcdir)
