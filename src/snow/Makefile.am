## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libsnow_la_LDFLAGS = $(PFLAGS)
libsnow_la_LIBADD = @COMPIZ_LIBS@
nodist_libsnow_la_SOURCES = snow_options.c snow_options.h
dist_libsnow_la_SOURCES = snow.c

BUILT_SOURCES = $(nodist_libsnow_la_SOURCES)

INCLUDES =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)


module_LTLIBRARIES = libsnow.la

CLEANFILES = *_options.c *_options.h

snow_options.h: ../../metadata/snow.xml.in
		$(BCOP_BIN) --header $@ $<

snow_options.c: ../../metadata/snow.xml.in snow_options.h
		$(BCOP_BIN) --source $@ $<
