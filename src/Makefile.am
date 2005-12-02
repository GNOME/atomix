## Process this file with automake to produce Makefile.in
scoredir = $(localstatedir)/games
uidir = $(datadir)/gnome-2.0/ui

INCLUDES = -Wall \
	-I$(top_srcdir) \
	$(ATOMIX_CFLAGS) \
	$(EXTRA_WARNING_CFLAGS) \
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
	clock.c clock.h

atomix_DEPENDENCIES = libatomix.a

atomix_LDADD = \
	libatomix.a \
	$(ATOMIX_LIBS)

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

ui_DATA = atomix-ui.xml
EXTRA_DIST = $(ui_DATA)

