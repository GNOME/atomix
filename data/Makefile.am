ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = level themes ui icons

logodir = $(pkgdatadir)
logo_DATA = atomix-logo.png


appstream_in_files = atomix.appdata.xml.in
appstream_XML = $(appstream_in_files:.xml.in=.xml)
$(appstream_XML): $(appstream_in_files)
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

@APPSTREAM_XML_RULES@


appdir = $(datadir)/applications
app_in_files = atomix.desktop.in
app_DATA = $(app_in_files:.desktop.in=.desktop)
$(app_DATA): $(app_in_files)
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = \
    $(appstream_in_files) \
	$(app_in_files) \
	$(logo_DATA)

CLEANFILES = \
	$(appstream_XML) \
	$(app_DATA)

check-local: $(desktop_DATA) \
	$(DESKTOP_FILE_VALIDATE) $(desktop_DATA)

DISTCHECK_CONFIGURE_FLAGS = --enable-appdata-check
