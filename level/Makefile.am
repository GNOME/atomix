leveldir = $(pkgdatadir)/level

level_old_files = \
	aceticacid.old \
	acetone.old \
	buthanol.old \
	cyclo-buthane.old \
	dimethylether.old \
	ethan.old \
	ethanal.old \
	ethanol.old \
	ethylene.old \
	formaldehyd.old \
	glycerin.old \
	lactic-acid.old \
	methane.old \
	methanol.old \
	propanal.old \
	propylen.old \
	pyran.old \
	transbutylen.old \
	water.old

level_DATA = \
	$(level_old_files:.old=.atomix) \
	sequence

EXTRA_DIST = $(level_old_files) sequence

SUFFIXES = .old .atomix

CLEANFILES = $(level_old_files:.old=.atomix)

.old.atomix:
	$(top_builddir)/src/level-convert $< > $@

