## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

if WIZARD_PLUGIN
libwizard_la_LDFLAGS = $(PFLAGS)
libwizard_la_LIBADD = @COMPIZ_LIBS@
nodist_libwizard_la_SOURCES = wizard_options.c wizard_options.h
dist_libwizard_la_SOURCES = wizard.c
endif

BUILT_SOURCES = $(nodist_libwizard_la_SOURCES)

AM_CFLAGS =                                \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

if WIZARD_PLUGIN
module_LTLIBRARIES = libwizard.la
endif

CLEANFILES = *_options.c *_options.h

wizard_options.h: ../../metadata/wizard.xml.in
		$(BCOP_BIN) --header $@ $<

wizard_options.c: ../../metadata/wizard.xml.in wizard_options.h
		$(BCOP_BIN) --source $@ $<
