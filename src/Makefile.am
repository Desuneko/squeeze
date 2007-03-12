bin_PROGRAMS = squeeze

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


squeeze_SOURCES = \
	main.c main.h \
	application.c application.h \
	main_window.c main_window.h \
	navigation_bar.c navigation_bar.h \
	notebook.c notebook.h \
	$(TOOLBAR_FILES) \
	$(PATHBAR_FILES) \
	settings.c settings.h \
	archive_store.c archive_store.h \
	new_dialog.c new_dialog.h \
	add_dialog.c add_dialog.h \
	extract_dialog.c extract_dialog.h \
	widget_factory.c widget_factory.h \
	location_entry.c location_entry.h

squeeze_CFLAGS = \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	$(THUNAR_VFS_CFLAGS) \
	$(XFCE4UTIL_CFLAGS) \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\"
 
squeeze_LDADD = \
	$(top_srcdir)/libsqueeze/libsqueeze-1.la \
	@GTK_LIBS@ \
	@GLIB_LIBS@ \
	@THUNAR_VFS_LIBS@

squeeze_LDFLAGS = -static


INCLUDES = \
	-I${top_srcdir}
