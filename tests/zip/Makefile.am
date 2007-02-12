INCLUDES =                                                              \
	$(GLIB_CFLAGS)                                                        \
	$(THUNAR_VFS_CFLAGS)

LIBS =                                                                  \
	$(GLIB_LIBS)                                                          \
	$(THUNAR_VFS_LIBS)                                                    \
	$(top_builddir)/libsqueeze/libsqueeze-1.la

TESTS =                                                                 \
	test-zip-add                                                          \
	test-zip-extract                                                      \
	test-zip-remove                                                       \
	test-zip-refresh

check_PROGRAMS =                                                        \
	test-zip-add                                                          \
	test-zip-extract                                                      \
	test-zip-remove                                                       \
	test-zip-refresh

test_zip_add_SOURCES =                                                  \
	test-zip-add.c

test_zip_add_DEPENDENCIES =                                             \
	$(top_builddir)/libsqueeze/libsqueeze-1.la

test_zip_extract_SOURCES =                                              \
	test-zip-extract.c

test_zip_extract_DEPENDENCIES =                                         \
	$(top_builddir)/libsqueeze/libsqueeze-1.la

test_zip_remove_SOURCES =                                               \
	test-zip-remove.c

test_zip_remove_DEPENDENCIES =                                          \
	$(top_builddir)/libsqueeze/libsqueeze-1.la


test_zip_refresh_SOURCES =                                              \
	test-zip-refresh.c

test_zip_refresh_DEPENDENCIES =                                         \
	$(top_builddir)/libsqueeze/libsqueeze-1.la
