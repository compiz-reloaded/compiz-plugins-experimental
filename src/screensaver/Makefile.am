## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

if SCREENSAVER_PLUGIN
libscreensaver_la_LDFLAGS = $(PFLAGS)
libscreensaver_la_LIBADD = @COMPIZ_LIBS@ -lXss -lstdc++
nodist_libscreensaver_la_SOURCES = screensaver_options.c screensaver_options.h
dist_libscreensaver_la_SOURCES = effect.cpp \
			effect.h                    \
			flyingwindows.cpp           \
			flyingwindows.h             \
			matrix.cpp                  \
			matrix.h                    \
			rotatingcube.cpp            \
			rotatingcube.h              \
			screensaver.cpp             \
			screensaver_internal.h      \
			vector.cpp                  \
			vector.h                    \
			wrapper.cpp                 \
			wrapper.h
endif

BUILT_SOURCES = $(nodist_libscreensaver_la_SOURCES)

AM_CFLAGS =                                \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

if SCREENSAVER_PLUGIN
module_LTLIBRARIES = libscreensaver.la
endif

CLEANFILES = *_options.c *_options.h

screensaver_options.h: ../../metadata/screensaver.xml.in
		$(BCOP_BIN) --header $@ $<

screensaver_options.c: ../../metadata/screensaver.xml.in screensaver_options.h
		$(BCOP_BIN) --source $@ $<
