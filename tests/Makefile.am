TESTS = test_add_gnu_tar

check_PROGRAMS = test_add_gnu_tar

test_add_gnu_tar_SOURCES = \
    gnu_tar_main.c

test_add_gnu_tar_CFLAGS = \
	@GLIB_CFLAGS@

test_add_gnu_tar_LDADD = \
	$(top_srcdir)/libxarchiver/libxarchiver.a \
	$(GLIB_LIBS) $(GOBJECT_LIBS)

INCLUDES = \
	-I${top_srcdir}
