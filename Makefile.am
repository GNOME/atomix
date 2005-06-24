## Process this file with automake to produce Makefile.in

SUBDIRS = po src level themes

logodir = $(pkgdatadir)
logo_DATA = atomix-logo.png
icondir = $(datadir)/pixmaps
icon_DATA = atomix-icon.png
scoredir=$(localstatedir)/games
Gamesdir = $(datadir)/applications

EXTRA_DIST = \
	atomix.desktop.in   \
        intltool-extract.in \
        intltool-merge.in   \
        intltool-update.in

Games_in_files = atomix.desktop.in
Games_DATA = $(Games_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

install-data-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-touch $(DESTDIR)$(scoredir)/atomix.scores
	-chown games.games $(DESTDIR)$(scoredir)/atomix.scores
	-chmod 664 $(DESTDIR)$(scoredir)/atomix.scores
	-chgrp games $(DESTDIR)$(bindir)/atomix && chmod 2755 $(DESTDIR)$(bindir)/atomix
