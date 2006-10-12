## Process this file with automake to produce Makefile.in

SUBDIRS = libxamime libxarchiver src tests po

pixmapsdir       = $(datadir)/pixmaps
pixmaps_DATA     = pixmaps/xarchiver.png \
                   pixmaps/extract.png \
									 pixmaps/extract_button.png \
									 pixmaps/add.png \
									 pixmaps/add_button.png

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
