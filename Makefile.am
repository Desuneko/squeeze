## Process this file with automake to produce Makefile.in

SUBDIRS = libxarchiver src tests po

pixmapsdir       = $(datadir)/pixmaps/xarchiver
pixmaps_DATA     = $(pixmapsdir)/xarchiver.png \
                   $(pixmapsdir)/extract.png \
									 $(pixmapsdir)/extract_button.png \
									 $(pixmapsdir)/add.png \
									 $(pixmapsdir)/add_button.png

wrapperdir = $(libexecdir)/thunar-archive-plugin/
wrapper_SCRIPTS = xarchiver.tap

desktopdir = $(datadir)/applications
desktop_in_files = xarchiver.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	autogen.sh \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	xarchiver.tap \
	$(desktop_in_files) \
	$(pixmaps_DATA) \
	$(wrapper_DATA)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(desktop_DATA)
