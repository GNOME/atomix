## Process this file with automake to produce Makefile.in
scoredir = $(localstatedir)/games

INCLUDES = -Wall \
	-DDEBUG	\
	-I$(top_srcdir)/intl \
	$(ATOMIX_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

bin_PROGRAMS = atomix

noinst_PROGRAMS = pf-test

atomix_SOURCES = \
	main.c main.h global.h \
	board.c board.h	\
	undo.c undo.h \
	goal.c goal.h \
	goal-view.c goal-view.h

#	preferences.c preferences.h \
#	callbacks.c callbacks.h \
#	gtk-time-limit.c gtk-time-limit.h

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
	level.c level.h \
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


install-data-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-touch $(DESTDIR)$(scoredir)/atomix.scores
	-chown games.games $(DESTDIR)$(scoredir)/atomix.scores
	-chmod 664 $(DESTDIR)$(scoredir)/atomix.scores
	-chgrp games $(DESTDIR)$(bindir)/atomix && chmod 2111 $(DESTDIR)$(bindir)/atomix
