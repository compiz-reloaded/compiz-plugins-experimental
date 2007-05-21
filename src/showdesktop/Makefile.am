## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libshowdesktop_la_LDFLAGS = $(PFLAGS)
libshowdesktop_la_LIBADD = @COMPIZ_LIBS@
libshowdesktop_la_SOURCES = showdesktop_options.c showdesktop.c

INCLUDES =                              \
	@COMPIZ_CFLAGS@                  \
	-DDATADIR='"$(datadir)"'          \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          \
	-I$(top_srcdir)/include

moduledir = $(plugindir)


module_LTLIBRARIES = libshowdesktop.la

CLEANFILES = *_options.c *_options.h

%_options.h: ../../metadata/%.xml
		$(BCOP_BIN) --header $@ $<

%_options.c: ../../metadata/%.xml %_options.h
		$(BCOP_BIN) --source $@ $<
