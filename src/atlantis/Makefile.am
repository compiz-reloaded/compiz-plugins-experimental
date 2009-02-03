## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libatlantis_la_LDFLAGS = $(PFLAGS)
libatlantis_la_LIBADD = @COMPIZ_LIBS@
nodist_libatlantis_la_SOURCES = atlantis_options.c  \
			        atlantis_options.h
dist_libatlantis_la_SOURCES = \
	atlantis.c  \
	atlantis-internal.h \
	bfish.c \
	bfish.h \
	bubble.c \
	chromis.c \
	chromis.h \
	coral2.c \
	coral2.h \
	coral2_low.h \
	coral.c \
	coral.h \
	coral_low.h \
	crab.c \
	crab.h \
	dolphin.c \
	dolphin.h \
	fish2.c \
	fish2.h \
	fish.c \
	fish.h \
	float.c \
	scuttle.c \
	shark.c \
	shark.h \
	swim.c \
	util.c \
	water.c \
	whale.c \
	whale.h

BUILT_SOURCES = $(nodist_libatlantis_la_SOURCES)

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

atlantis_options.h: ../../metadata/atlantis.xml.in
		$(BCOP_BIN) --header $@ $<

atlantis_options.c: ../../metadata/atlantis.xml.in atlantis_options.h
		$(BCOP_BIN) --source $@ $<
