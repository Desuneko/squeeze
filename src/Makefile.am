bin_PROGRAMS = xarchiver

xarchiver_SOURCES = \
	main.c main.h \
	main_window.c main_window.h \
	navigation_bar.c navigation_bar.h \
	tool_bar.c tool_bar.h \
	settings.c settings.h \
	archive_store.c archive_store.h \
	archive_tree_store.c archive_tree_store.h \
	preferences_dialog.c preferences_dialog.h \
	new_dialog.c new_dialog.h \
	add_dialog.c add_dialog.h \
	extract_dialog.c extract_dialog.h

xarchiver_CFLAGS = \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	@THUNAR_VFS_CFLAGS@ \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\"
 
xarchiver_LDADD = \
	$(top_srcdir)/libxarchiver/libxarchiver.a \
	@GTK_LIBS@ \
	@GLIB_LIBS@ \
	@THUNAR_VFS_LIBS@

INCLUDES = \
	-I${top_srcdir}
