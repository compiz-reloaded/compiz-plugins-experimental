## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libatlantis_la_LDFLAGS = $(PFLAGS)
libatlantis_la_LIBADD = @COMPIZ_LIBS@
libatlantis_la_SOURCES = atlantis_options.c  \
			 atlantis_options.h  \
			 atlantis.c          \
		         atlantis-internal.h \
		         dolphin.c           \
		         shark.c             \
		         swim.c              \
		         whale.c             \
			 water.c

INCLUDES =                              \
	@COMPIZ_CFLAGS@                  \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          \
	-I$(top_srcdir)/include

moduledir = $(plugindir)


module_LTLIBRARIES = libatlantis.la

CLEANFILES = *_options.c *_options.h

%_options.h: ../../metadata/%.xml
		$(BCOP_BIN) --header $@ $<

%_options.c: ../../metadata/%.xml %_options.h
		$(BCOP_BIN) --source $@ $<
