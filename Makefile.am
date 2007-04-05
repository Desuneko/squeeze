## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS =							\
	1.8								\
	dist-bzip2

SUBDIRS = pixmaps libsqueeze po docs src icons

wrapperdir = $(libexecdir)/thunar-archive-plugin/
wrapper_SCRIPTS = squeeze.tap

desktopdir = $(datadir)/applications
desktop_in_files = squeeze.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	icons \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	squeeze.tap \
	gettext.h \
	$(desktop_in_files) \
	$(wrapper_DATA)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(desktop_DATA)

ACLOCAL_AMFLAGS = -I m4
