bin_PROGRAMS = xarchiver

xarchiver_SOURCES = \
	main.c main.h \
	new_dialog.c new_dialog.h \ 
	extract_dialog.c extract_dialog.h

xarchiver_CFLAGS = \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\"
 
xarchiver_LDADD = \
	$(top_srcdir)/libxarchiver/libxarchiver.a \
	@GTK_LIBS@ \
	@GLIB_LIBS@

INCLUDES = \
	-I${top_srcdir}
