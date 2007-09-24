## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

lib3d_la_LDFLAGS = $(PFLAGS)
lib3d_la_LIBADD = @COMPIZ_LIBS@
lib3d_la_SOURCES = 3d_options.c 3d_options.h 3d.c

INCLUDES =                              \
	@COMPIZ_CFLAGS@                  \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          \
	-I$(top_srcdir)/include

moduledir = $(plugindir)


module_LTLIBRARIES = lib3d.la

CLEANFILES = *_options.c *_options.h

%_options.h: ../../metadata/%.xml
		$(BCOP_BIN) --header $@ $<

%_options.c: ../../metadata/%.xml %_options.h
		$(BCOP_BIN) --source $@ $<
