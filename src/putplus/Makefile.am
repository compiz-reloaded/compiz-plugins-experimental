
PFLAGS=-module -avoid-version -no-undefined

libputplus_la_LDFLAGS = $(PFLAGS)
libputplus_la_LIBADD = @COMPIZ_LIBS@
nodist_libputplus_la_SOURCES = putplus_options.c putplus_options.h
dist_libputplus_la_SOURCES = putplus.c

BUILT_SOURCES = $(nodist_libputplus_la_SOURCES)

AM_CFLAGS =                                \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libputplus.la

CLEANFILES = *_options.c *_options.h

putplus_options.h: ../../metadata/putplus.xml.in
		$(BCOP_BIN) --header $@ $<

putplus_options.c: ../../metadata/putplus.xml.in putplus_options.h
		$(BCOP_BIN) --source $@ $<
