## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libsnow_la_LDFLAGS = $(PFLAGS)
libsnow_la_LIBADD = @COMPIZ_LIBS@
libsnow_la_SOURCES = snow_options.c snow_options.h snow.c

INCLUDES =                              \
	@COMPIZ_CFLAGS@                  \
	-DDATADIR='"$(datadir)"'          \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          \
	-I$(top_srcdir)/include

moduledir = $(plugindir)


module_LTLIBRARIES = libsnow.la

CLEANFILES = *_options.c *_options.h

%_options.h: ../../metadata/%.xml
		$(BCOP_BIN) --header $@ $<

%_options.c: ../../metadata/%.xml %_options.h
		$(BCOP_BIN) --source $@ $<
