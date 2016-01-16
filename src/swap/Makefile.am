## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libswap_la_LDFLAGS = $(PFLAGS)
libswap_la_LIBADD = @COMPIZ_LIBS@
nodist_libswap_la_SOURCES = swap_options.c swap_options.h
dist_libswap_la_SOURCES = swap.c

BUILT_SOURCES = $(nodist_libswap_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libswap.la

CLEANFILES = *_options.c *_options.h

swap_options.h: ../../metadata/swap.xml.in
		$(BCOP_BIN) --header $@ $<

swap_options.c: ../../metadata/swap.xml.in swap_options.h
		$(BCOP_BIN) --source $@ $<
