ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = level themes

logodir = $(pkgdatadir)
logo_DATA = atomix-logo.png

icondir = $(datadir)/pixmaps
icon_DATA = atomix-icon.png

@INTLTOOL_DESKTOP_RULE@

appdir = $(datadir)/applications
app_in_files = atomix.desktop.in
app_DATA = $(app_in_files:.desktop.in=.desktop)

EXTRA_DIST = \
	$(app_in_files) \
	$(logo_DATA) \
	$(icon_DATA)

DISTCLEANFILES = \
	$(app_DATA)

