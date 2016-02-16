ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = level themes ui

logodir = $(pkgdatadir)
logo_DATA = atomix-logo.png

icondir = $(datadir)/icons/hicolor/512x512/apps
icon_DATA = atomix.png

@INTLTOOL_DESKTOP_RULE@

appstream_in_files = atomix.appdata.xml.in
appstream_XML = $(appstream_in_files:.xml.in=.xml)

@APPSTREAM_XML_RULES@

@INTLTOOL_XML_RULE@

appdir = $(datadir)/applications
app_in_files = atomix.desktop.in
app_DATA = $(app_in_files:.desktop.in=.desktop)

EXTRA_DIST = \
    $(appstream_in_files) \
	$(app_in_files) \
	$(logo_DATA) \
	$(icon_DATA)

CLEANFILES = \
	$(appstream_XML) \
	$(app_DATA)

check-local: $(desktop_DATA) \
	$(DESKTOP_FILE_VALIDATE) $(desktop_DATA)

DISTCHECK_CONFIGURE_FLAGS = --enable-appdata-check
