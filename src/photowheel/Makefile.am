## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

if PHOTOWHEEL_PLUGIN
libphoto_la_LDFLAGS = $(PFLAGS)
libphoto_la_LIBADD = @COMPIZ_LIBS@
nodist_libphoto_la_SOURCES = photo_options.c photo_options.h
dist_libphoto_la_SOURCES = photo.c
endif

BUILT_SOURCES = $(nodist_libphoto_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

if PHOTOWHEEL_PLUGIN
module_LTLIBRARIES = libphoto.la
endif

CLEANFILES = *_options.c *_options.h

photo_options.h: ../../metadata/photo.xml.in
		$(BCOP_BIN) --header $@ $<

photo_options.c: ../../metadata/photo.xml.in photo_options.h
		$(BCOP_BIN) --source $@ $<
