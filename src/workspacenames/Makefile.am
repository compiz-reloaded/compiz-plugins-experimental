## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libworkspacenames_la_LDFLAGS = $(PFLAGS)
libworkspacenames_la_LIBADD = @COMPIZ_LIBS@
nodist_libworkspacenames_la_SOURCES = workspacenames_options.c \
			    workspacenames_options.h
dist_libworkspacenames_la_SOURCES = workspacenames.c

BUILT_SOURCES = $(nodist_libworkspacenames_la_SOURCES)

BUILT_SOURCES = $(nodist_libworkspacenames_la_SOURCES)

AM_CFLAGS =                                \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DPREFIX='"$(prefix)"'              \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

if WORKSPACENAMES_PLUGIN
module_LTLIBRARIES = libworkspacenames.la
endif

CLEANFILES = *_options.c *_options.h

workspacenames_options.h: ../../metadata/workspacenames.xml.in
		$(BCOP_BIN) --header $@ $<

workspacenames_options.c: ../../metadata/workspacenames.xml.in workspacenames_options.h
		$(BCOP_BIN) --source $@ $<
