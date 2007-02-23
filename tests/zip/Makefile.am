INCLUDES =                                                              \
	$(GLIB_CFLAGS)                                                        \
	$(THUNAR_VFS_CFLAGS)

LIBS =                                                                  \
	$(GLIB_LIBS)                                                          \
	$(THUNAR_VFS_LIBS)                                                    \
	$(top_builddir)/libsqueeze/libsqueeze-1.la

TESTS =                                                                 \
	test-zip-add.pl                                                       \
	test-gnu-tar-add.pl                                                       \
	test-zip-extract.pl                                                   \
	test-zip-remove.pl                                                    \
	test-zip-refresh.pl

check_PROGRAMS =                                                    \
	test-add                                                    \
	test-extract                                                \
	test-remove                                                 \
	test-refresh

test_add_SOURCES =                                            \
	test-add.c

test_add_DEPENDENCIES =                                       \
	$(top_builddir)/libsqueeze/libsqueeze-1.la

test_add_LDADD =                                              \
	$(top_builddir)/libsqueeze/libsqueeze-1.la

test_add_LDFLAGS = -static

test_extract_SOURCES =                                              \
	test-extract.c

test_extract_DEPENDENCIES =                                         \
	$(top_builddir)/libsqueeze/libsqueeze-1.la

test_extract_LDADD =                                              \
	$(top_builddir)/libsqueeze/libsqueeze-1.la

test_extract_LDFLAGS = -static

test_remove_SOURCES =                                               \
	test-remove.c

test_remove_DEPENDENCIES =                                          \
	$(top_builddir)/libsqueeze/libsqueeze-1.la

test_remove_LDADD =                                              \
	$(top_builddir)/libsqueeze/libsqueeze-1.la

test_remove_LDFLAGS = -static

test_refresh_SOURCES =                                              \
	test-refresh.c

test_refresh_DEPENDENCIES =                                         \
	$(top_builddir)/libsqueeze/libsqueeze-1.la

test_refresh_LDADD =                                              \
	$(top_builddir)/libsqueeze/libsqueeze-1.la

test_refresh_LDFLAGS = -static
