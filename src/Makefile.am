## Process this file with automake to produce Makefile.in
scoredir = $(localstatedir)/games
uidir = $(pkgdatadir)/ui

INCLUDES = -Wall \
	-I$(top_srcdir)/intl \
	-DDEBUG \
	$(ATOMIX_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

bin_PROGRAMS = atomix

noinst_PROGRAMS = pf-test level-convert

atomix_SOURCES = \
	main.c main.h \
	board.c board.h	\
	undo.c undo.h \
	goal.c goal.h \
	goal-view.c goal-view.h

ui_DATA = atomix-ui.xml

atomix_DEPENDENCIES = libatomix.a

atomix_LDADD = \
	-L$(top_builddir)/src \
	$(ATOMIX_LIBS) \
	-latomix

noinst_LIBRARIES = libatomix.a

libatomix_a_SOURCES = \
	tile.c tile.h \
	theme.c theme.h theme-private.h \
	theme-manager.c theme-manager.h \
	level.c level.h level-private.h \
	level-manager.c level-manager.h \
	canvas_helper.c canvas_helper.h \
	playfield.c playfield.h \
	util.c util.h

pf_test_SOURCES = \
	playfield.h playfield.c \
	tile.c tile.h \
	pf-test.c

pf_test_LDADD = \
	-L$(top_builddir)/src \
	$(ATOMIX_LIBS)

level_convert_SOURCES = \
	playfield.h playfield.c \
	tile.c tile.h \
	level.c level.h \
	level-convert.c

level_convert_LDADD = \
	-L$(top_builddir)/src \
	$(ATOMIX_LIBS)

EXTRA_DIST = atomix-ui.xml

#install-data-local:
#	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
#	-touch $(DESTDIR)$(scoredir)/atomix.scores
#	-chown games.games $(DESTDIR)$(scoredir)/atomix.scores
#	-chmod 664 $(DESTDIR)$(scoredir)/atomix.scores
#	-chgrp games $(DESTDIR)$(bindir)/atomix && chmod 2111 $(DESTDIR)$(bindir)/atomix
