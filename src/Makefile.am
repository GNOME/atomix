## Process this file with automake to produce Makefile.in
scoredir = $(localstatedir)/games

INCLUDES = \
	-DDEBUG	\
	-I$(top_srcdir)/intl \
	$(ATOMIX_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

bin_PROGRAMS = atomix

atomix_SOURCES = \
	main.c main.h global.h \
	callbacks.c callbacks.h \
	preferences.c preferences.h \
	board.c board.h	\
	undo.c undo.h \
	goal.c goal.h

#	gtk-time-limit.c gtk-time-limit.h

atomix_DEPENDENCIES = libatomix.a

atomix_LDADD = \
	-L$(top_builddir)/src \
	$(ATOMIX_LIBS) \
	-latomix

noinst_LIBRARIES = libatomix.a

libatomix_a_SOURCES = \
	tile.c tile.h \
	theme.c theme.h \
	level.c level.h \
	canvas_helper.c canvas_helper.h \
	playfield.c playfield.h \
	util.c util.h

install-data-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-touch $(DESTDIR)$(scoredir)/atomix.scores
	-chown games.games $(DESTDIR)$(scoredir)/atomix.scores
	-chmod 664 $(DESTDIR)$(scoredir)/atomix.scores
	-chgrp games $(DESTDIR)$(bindir)/atomix && chmod 2111 $(DESTDIR)$(bindir)/atomix
