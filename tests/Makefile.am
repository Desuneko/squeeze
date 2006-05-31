TESTS = test-libxarchiver-composite-archives

check_PROGRAMS = test-libxarchiver-composite-archives

test_libxarchiver_composite_archives_SOURCES = \
	test-libxarchiver-composite-archives.c

test_libxarchiver_composite_archives_CFLAGS = \
	@GLIB_CFLAGS@

test_libxarchiver_composite_archives_LDADD = \
	$(top_srcdir)/libxarchiver/libxarchiver.a \
	$(GLIB_LIBS) $(GOBJECT_LIBS)

INCLUDES = \
	-I${top_srcdir}
