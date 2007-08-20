## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libmswitch_la_LDFLAGS = $(PFLAGS)
libmswitch_la_LIBADD = @COMPIZ_LIBS@
libmswitch_la_SOURCES = mswitch_options.c mswitch_options.h mswitch.c

INCLUDES =                              \
	@COMPIZ_CFLAGS@                  \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          \
	-I$(top_srcdir)/include

moduledir = $(plugindir)

module_LTLIBRARIES = libmswitch.la

CLEANFILES = *_options.c *_options.h

%_options.h: ../../metadata/%.xml
		$(BCOP_BIN) --header $@ $<

%_options.c: ../../metadata/%.xml %_options.h
		$(BCOP_BIN) --source $@ $<
