ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = po src level themes

logodir = $(pkgdatadir)
logo_DATA = atomix-logo.png

icondir = $(datadir)/pixmaps
icon_DATA = atomix-icon.png

scoredir=$(localstatedir)/games

appdir = $(datadir)/applications
app_in_files = atomix.desktop.in
app_DATA = $(app_in_files:.desktop.in=.desktop)

EXTRA_DIST = \
        intltool-extract.in \
        intltool-merge.in   \
        intltool-update.in \
	$(app_in_files) \
	$(logo_DATA) \
	$(icon_DATA)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(app_DATA)

@INTLTOOL_DESKTOP_RULE@

install-data-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-touch $(DESTDIR)$(scoredir)/atomix.scores
	-chown games.games $(DESTDIR)$(scoredir)/atomix.scores
	-chmod 664 $(DESTDIR)$(scoredir)/atomix.scores
	-chgrp games $(DESTDIR)$(bindir)/atomix && chmod 2755 $(DESTDIR)$(bindir)/atomix
