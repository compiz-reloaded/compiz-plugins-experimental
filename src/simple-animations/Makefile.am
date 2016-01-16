## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined -lGLU

if ANIMATIONSIM_PLUGIN
libanimationsim_la_LDFLAGS = $(PFLAGS)
libanimationsim_la_LIBADD = @COMPIZ_LIBS@ @COMPIZANIMATION_LIBS@
nodist_libanimationsim_la_SOURCES = animationsim_options.c animationsim_options.h
dist_libanimationsim_la_SOURCES = animationsim.c \
			animationsim.h                   \
			bounce.c                         \
			expand.c                         \
			expand-piecewise.c               \
			explode3d.c                      \
			flyin.c                          \
			rotatein.c                       \
			sheet.c
endif

BUILT_SOURCES = $(nodist_libanimationsim_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	@COMPIZANIMATION_CFLAGS@            \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

if ANIMATIONSIM_PLUGIN
module_LTLIBRARIES = libanimationsim.la
endif

CLEANFILES = *_options.c *_options.h

animationsim_options.h: ../../metadata/animationsim.xml.in
		$(BCOP_BIN) --header $@ $<

animationsim_options.c: ../../metadata/animationsim.xml.in animationsim_options.h
		$(BCOP_BIN) --source $@ $<
