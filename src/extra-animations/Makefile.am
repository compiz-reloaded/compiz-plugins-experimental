## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined -lGLU

if ANIMATIONPLUS_PLUGIN
libanimationplus_la_LDFLAGS = $(PFLAGS)
libanimationplus_la_LIBADD = @COMPIZ_LIBS@ @COMPIZANIMATION_LIBS@
nodist_libanimationplus_la_SOURCES = animationplus_options.c animationplus_options.h
dist_libanimationplus_la_SOURCES = animationplus.c \
			animationplus.h                    \
			animation_tex.h                    \
			blinds.c                           \
			bonanza.c                          \
			compiz-animationplus.h             \
			helix.c                            \
			shatter.c
endif

BUILT_SOURCES = $(nodist_libanimationplus_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	@COMPIZANIMATION_CFLAGS@            \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

if ANIMATIONPLUS_PLUGIN
module_LTLIBRARIES = libanimationplus.la
endif

CLEANFILES = *_options.c *_options.h

animationplus_options.h: ../../metadata/animationplus.xml.in
		$(BCOP_BIN) --header $@ $<

animationplus_options.c: ../../metadata/animationplus.xml.in animationplus_options.h
		$(BCOP_BIN) --source $@ $<
