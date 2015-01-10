## Process this file with automake to produce Makefile.in
scoredir = $(localstatedir)/games
uidir = $(datadir)/gnome-2.0/ui

INCLUDES = -I$(top_srcdir) \
	$(ATOMIX_CFLAGS) \
	$(WARN_CFLAGS) \
	-DPKGDATADIR="\"$(pkgdatadir)\"" \
	-DDATADIR=\""$(datadir)"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DSCORESDIR=\""$(scoredir)"\"

bin_PROGRAMS = atomix

noinst_PROGRAMS = pf-test level-convert

atomix_SOURCES = \
	main.c main.h \
	board.c board.h	\
	undo.c undo.h \
	goal.c goal.h \
	goal-view.c goal-view.h \
	clock.c clock.h \
	games-debug.c games-debug.h \
	games-gtk-compat.h \
	games-profile.c games-profile.h \
	games-score.c games-score.h \
	games-scores.c games-scores.h \
	games-scores-backend.c games-scores-backend.h \
	games-scores-dialog.c games-scores-dialog.h \
	games-scores-dialog-private.h \
	games-show.c games-show.h

atomix_DEPENDENCIES = libatomix.a

atomix_LDADD = \
	libatomix.a \
	$(ATOMIX_LIBS) \
        -lm

noinst_LIBRARIES = libatomix.a

libatomix_a_SOURCES = \
	tile.c tile.h \
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
	pf-test.c

pf_test_LDADD = \
	-L$(top_builddir)/src \
	$(ATOMIX_LIBS)

level_convert_SOURCES = \
	playfield.h playfield.c \
	tile.c tile.h \
	level.c level.h \
	level-convert.c \
	theme.c theme.h

level_convert_LDADD = \
	-L$(top_builddir)/src \
	$(ATOMIX_LIBS)

EXTRA_DIST = $(ui_DATA)

