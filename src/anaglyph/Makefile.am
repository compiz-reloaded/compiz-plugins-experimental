## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libanaglyph_la_LDFLAGS = $(PFLAGS)
libanaglyph_la_LIBADD = @COMPIZ_LIBS@
nodist_libanaglyph_la_SOURCES = anaglyph_options.c anaglyph_options.h
dist_libanaglyph_la_SOURCES = anaglyph.c

BUILT_SOURCES = $(nodist_libanaglyph_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libanaglyph.la

CLEANFILES = *_options.c *_options.h

anaglyph_options.h: ../../metadata/anaglyph.xml.in
		$(BCOP_BIN) --header $@ $<

anaglyph_options.c: ../../metadata/anaglyph.xml.in anaglyph_options.h
		$(BCOP_BIN) --source $@ $<
