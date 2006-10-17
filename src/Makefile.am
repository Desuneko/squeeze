bin_PROGRAMS = xarchiver

if ENABLE_PATHBAR
PATHBAR_FILES = path_bar.c path_bar.h
else
PATHBAR_FILES = 
endif

if ENABLE_TOOLBAR
TOOLBAR_FILES = tool_bar.c tool_bar.h
else
TOOLBAR_FILES =
endif


xarchiver_SOURCES = \
	main.c main.h \
	main_window.c main_window.h \
	navigation_bar.c navigation_bar.h \
	$(TOOLBAR_FILES) \
	$(PATHBAR_FILES) \
	settings.c settings.h \
	archive_store.c archive_store.h \
	preferences_dialog.c preferences_dialog.h \
	new_dialog.c new_dialog.h \
	add_dialog.c add_dialog.h \
	extract_dialog.c extract_dialog.h

xarchiver_CFLAGS = \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	$(THUNAR_VFS_CFLAGS) \
	$(XFCE4UTIL_CFLAGS) \
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
