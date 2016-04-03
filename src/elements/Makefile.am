## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libelements_la_LDFLAGS = $(PFLAGS)
libelements_la_LIBADD = @COMPIZ_LIBS@
nodist_libelements_la_SOURCES = elements_options.c  \
			        elements_options.h
dist_libelements_la_SOURCES = \
	elements.c

BUILT_SOURCES = $(nodist_libelements_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)


module_LTLIBRARIES = libelements.la

CLEANFILES = *_options.c *_options.h

elements_options.h: ../../metadata/elements.xml.in
		$(BCOP_BIN) --header $@ $<

elements_options.c: ../../metadata/elements.xml.in elements_options.h
		$(BCOP_BIN) --source $@ $<
