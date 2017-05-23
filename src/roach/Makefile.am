## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libroach_la_LDFLAGS = $(PFLAGS)
libroach_la_LIBADD = @COMPIZ_LIBS@
nodist_libroach_la_SOURCES = roach_options.c roach_options.h
dist_libroach_la_SOURCES = roach.c

BUILT_SOURCES = $(nodist_libroach_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libroach.la

CLEANFILES = *_options.c *_options.h

roach_options.h: ../../metadata/roach.xml.in
		$(BCOP_BIN) --header $@ $<

roach_options.c: ../../metadata/roach.xml.in roach_options.h
		$(BCOP_BIN) --source $@ $<
