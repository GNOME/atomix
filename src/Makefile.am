## Process this file with automake to produce Makefile.in
scoredir = $(localstatedir)/games
uidir = $(datadir)/gnome-2.0/ui

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPKGDATADIR="\"$(pkgdatadir)\"" \
	-DDATADIR=\""$(datadir)"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DSCORESDIR=\""$(scoredir)"\"

AM_CFLAGS = \
	$(ATOMIX_CFLAGS) \
	$(WARN_CFLAGS)

AM_LDFLAGS = \
	$(WARN_LDFLAGS)

bin_PROGRAMS = atomix

noinst_PROGRAMS = pf-test

atomix_SOURCES = \
	atomix-application.h \
	main.c main.h \
	board-gtk.c board-gtk.h	\
	undo.c undo.h \
	goal.c goal.h \
	goal-view.c goal-view.h \
	clock.c clock.h \
	games-show.c games-show.h

atomix_DEPENDENCIES = libatomix.a

atomix_LDADD = \
	libatomix.a \
	$(ATOMIX_LIBS) \
        -lm

noinst_LIBRARIES = libatomix.a

libatomix_a_SOURCES = \
	tile.c tile.h \
	xml-util.c xml-util.h \
	theme.c theme.h theme-private.h \
	theme-manager.c theme-manager.h \
	level.c level.h level-private.h \
	level-manager.c level-manager.h \
	canvas_helper.c canvas_helper.h \
	playfield.c playfield.h

pf_test_SOURCES = \
	playfield.h playfield.c \
	tile.c tile.h \
	theme.c theme.h \
	xml-util.c xml-util.h \
	pf-test.c

pf_test_LDADD = \
	-L$(top_builddir)/src \
	$(ATOMIX_LIBS)

EXTRA_DIST = $(ui_DATA)

