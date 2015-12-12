## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

if SNOWGLOBE_PLUGIN
libsnowglobe_la_LDFLAGS = $(PFLAGS)
libsnowglobe_la_LIBADD = @COMPIZ_LIBS@
nodist_libsnowglobe_la_SOURCES = snowglobe_options.c snowglobe_options.h
dist_libsnowglobe_la_SOURCES = snowglobe.c \
			snowglobe-internal.h       \
			movement.c                 \
			snowflake.c                \
			snowman.c                  \
			water.c
endif

BUILT_SOURCES = $(nodist_libsnowglobe_la_SOURCES)

AM_CFLAGS =                                \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

if SNOWGLOBE_PLUGIN
module_LTLIBRARIES = libsnowglobe.la
endif

CLEANFILES = *_options.c *_options.h

snowglobe_options.h: ../../metadata/snowglobe.xml.in
		$(BCOP_BIN) --header $@ $<

snowglobe_options.c: ../../metadata/snowglobe.xml.in snowglobe_options.h
		$(BCOP_BIN) --source $@ $<
