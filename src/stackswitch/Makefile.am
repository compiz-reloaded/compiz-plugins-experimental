## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

if STACKSWITCH_PLUGIN
libstackswitch_la_LDFLAGS = $(PFLAGS)
libstackswitch_la_LIBADD = @COMPIZ_LIBS@
nodist_libstackswitch_la_SOURCES = stackswitch_options.c stackswitch_options.h
dist_libstackswitch_la_SOURCES = stackswitch.c
endif

BUILT_SOURCES = $(nodist_libstackswitch_la_SOURCES)

AM_CFLAGS =                                \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

if STACKSWITCH_PLUGIN
module_LTLIBRARIES = libstackswitch.la
endif

CLEANFILES = *_options.c *_options.h

stackswitch_options.h: ../../metadata/stackswitch.xml.in
		$(BCOP_BIN) --header $@ $<

stackswitch_options.c: ../../metadata/stackswitch.xml.in stackswitch_options.h
		$(BCOP_BIN) --source $@ $<
