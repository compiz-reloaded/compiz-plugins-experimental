## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libcubemodel_la_LDFLAGS = $(PFLAGS) -pthread
libcubemodel_la_LIBADD = @COMPIZ_LIBS@
nodist_libcubemodel_la_SOURCES = cubemodel_options.c cubemodel_options.h
dist_libcubemodel_la_SOURCES = cubemodel.c \
			cubemodel-internal.h       \
			fileParser.c               \
			loadModel.c

BUILT_SOURCES = $(nodist_libcubemodel_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libcubemodel.la

CLEANFILES = *_options.c *_options.h

cubemodel_options.h: ../../metadata/cubemodel.xml.in
		$(BCOP_BIN) --header $@ $<

cubemodel_options.c: ../../metadata/cubemodel.xml.in cubemodel_options.h
		$(BCOP_BIN) --source $@ $<
