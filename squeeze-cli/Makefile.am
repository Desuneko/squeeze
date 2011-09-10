check_PROGRAMS = squeeze-cli

squeeze_cli_SOURCES = \
	main.c \
	cli.c cli.h

squeeze_cli_CFLAGS = \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS)
 
squeeze_cli_LDADD = \
	$(top_builddir)/libsqueeze/libsqueeze-2.la

squeeze_cli_LDFLAGS = \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(XFCONF_LIBS) \
	$(LIBXFCE4UTIL_LIBS)


INCLUDES = \
	-I${top_srcdir}

