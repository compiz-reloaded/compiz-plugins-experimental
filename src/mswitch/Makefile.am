## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libmswitch_la_LDFLAGS = $(PFLAGS)
libmswitch_la_LIBADD = @COMPIZ_LIBS@
nodist_libmswitch_la_SOURCES = mswitch_options.c mswitch_options.h
dist_libmswitch_la_SOURCES = mswitch.c

BUILT_SOURCES = $(nodist_libmswitch_la_SOURCES)

INCLUDES =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libmswitch.la

CLEANFILES = *_options.c *_options.h

mswitch_options.h: ../../metadata/mswitch.xml.in
		$(BCOP_BIN) --header $@ $<

mswitch_options.c: ../../metadata/mswitch.xml.in mswitch_options.h
		$(BCOP_BIN) --source $@ $<
