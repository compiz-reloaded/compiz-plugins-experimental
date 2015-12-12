## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

if GHOST_PLUGIN
libghost_la_LDFLAGS = $(PFLAGS)
libghost_la_LIBADD = @COMPIZ_LIBS@
nodist_libghost_la_SOURCES = ghost_options.c ghost_options.h
dist_libghost_la_SOURCES = ghost.c
endif

BUILT_SOURCES = $(nodist_libghost_la_SOURCES)

AM_CFLAGS =                                \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

if GHOST_PLUGIN
module_LTLIBRARIES = libghost.la
endif

CLEANFILES = *_options.c *_options.h

ghost_options.h: ../../metadata/ghost.xml.in
		$(BCOP_BIN) --header $@ $<

ghost_options.c: ../../metadata/ghost.xml.in ghost_options.h
		$(BCOP_BIN) --source $@ $<
