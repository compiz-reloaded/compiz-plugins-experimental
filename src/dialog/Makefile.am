## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libdialog_la_LDFLAGS = $(PFLAGS)
libdialog_la_LIBADD = @COMPIZ_LIBS@
nodist_libdialog_la_SOURCES = dialog_options.c dialog_options.h
dist_libdialog_la_SOURCES = dialog.c

BUILT_SOURCES = $(nodist_libdialog_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libdialog.la

CLEANFILES = *_options.c *_options.h

dialog_options.h: ../../metadata/dialog.xml.in
		$(BCOP_BIN) --header $@ $<

dialog_options.c: ../../metadata/dialog.xml.in dialog_options.h
		$(BCOP_BIN) --source $@ $<
