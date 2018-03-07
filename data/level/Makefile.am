leveldir = $(pkgdatadir)/level

level_in_files = \
	aceticacid.atomix.xml.in \
	acetone.atomix.xml.in \
	butanol.atomix.xml.in\
	cyclobutane.atomix.xml.in \
	dimethylether.atomix.xml.in \
	ethane.atomix.xml.in \
	ethanal.atomix.xml.in \
	ethanol.atomix.xml.in \
	ethylene.atomix.xml.in \
	glycerin.atomix.xml.in \
	lactic-acid.atomix.xml.in \
	methanal.atomix.xml.in \
	methane.atomix.xml.in \
	methanol.atomix.xml.in \
	propanal.atomix.xml.in \
	propylene.atomix.xml.in \
	pyran.atomix.xml.in \
	transbutylen.atomix.xml.in \
	water.atomix.xml.in

level_XML= $(level_in_files:.xml.in=.xml)
$(level_XML): %.xml: %.xml.in
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

level_DATA = $(level_XML) sequence

EXTRA_DIST = $(level_in_files) sequence

CLEANFILES = \
	$(level_XML)
