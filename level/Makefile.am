leveldir = $(pkgdatadir)/level

level_old_files = \
	methane.old \
	methanol.old \
	water.old

level_DATA = \
	$(level_old_files:.old=.atomix) \
	sequence

EXTRA_DIST = $(level_old_files) sequence

SUFFIXES = .old .atomix

CLEANFILES = $(level_old_files:.old=.atomix)

.old.atomix:
	$(top_builddir)/src/level-convert $< > $@

