## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libboing_la_LDFLAGS = $(PFLAGS)
libboing_la_LIBADD = @COMPIZ_LIBS@
nodist_libboing_la_SOURCES = boing_options.c boing_options.h
dist_libboing_la_SOURCES = boing.c

BUILT_SOURCES = $(nodist_libboing_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libboing.la

CLEANFILES = *_options.c *_options.h

boing_options.h: ../../metadata/boing.xml.in
		$(BCOP_BIN) --header $@ $<

boing_options.c: ../../metadata/boing.xml.in boing_options.h
		$(BCOP_BIN) --source $@ $<
