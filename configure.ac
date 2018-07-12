AC_PREREQ(2.57)

AC_INIT([compiz-plugins-experimental], [0.8.15], [chat-to-me@raveit.de])
AM_INIT_AUTOMAKE([1.9 foreign dist-xz no-dist-gzip check-news])
AC_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
LT_INIT
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h sys/time.h unistd.h])
IT_PROG_INTLTOOL([0.35.0])
AC_CONFIG_MACRO_DIR([m4])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19])
GETTEXT_PACKAGE=compiz-plugins-experimental
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])
AC_SUBST(GETTEXT_PACKAGE)

if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[[\ \	]]-Wall[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wpointer-arith[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wpointer-arith" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wstrict-prototypes[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wstrict-prototypes" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wmissing-prototypes[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wmissing-prototypes" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wmissing-declarations[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wmissing-declarations" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wnested-externs[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wnested-externs" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-fno-strict-aliasing[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -fno-strict-aliasing" ;;
  esac

  if test "x$enable_ansi" = "xyes"; then
    case " $CFLAGS " in
    *[[\ \	]]-ansi[[\ \	]]*) ;;
    *) CFLAGS="$CFLAGS -ansi" ;;
    esac

    case " $CFLAGS " in
    *[[\ \	]]-pedantic[[\ \	]]*) ;;
    *) CFLAGS="$CFLAGS -pedantic" ;;
    esac
  fi
fi

AC_C_BIGENDIAN

plugindir=$libdir/compiz
AC_SUBST(plugindir)

imagedir=$datadir/compiz
AC_SUBST(imagedir)

metadatadir=$datadir/compiz
AC_SUBST(metadatadir)

compdatadir=$datadir/compiz
AC_SUBST(compdatadir)

pluginiconsdir=$datadir/compiz/icons/hicolor
AC_SUBST(pluginiconsdir)

dnl ============================================================
dnl Check for the pkg-config path.
if test x"$PKG_CONFIG_PATH" = x; then
    PKG_CONFIG_PATH=${prefix}/lib/pkgconfig:${prefix}/share/pkgconfig
        else
    PKG_CONFIG_PATH=${prefix}/lib/pkgconfig:${prefix}/share/pkgconfig:${PKG_CONFIG_PATH}
fi
export PKG_CONFIG_PATH
AC_SUBST(PKG_CONFIG_PATH)
AC_MSG_NOTICE([Using PKG_CONFIG_PATH=$PKG_CONFIG_PATH])

PKG_CHECK_MODULES(COMPIZ, compiz)
PKG_CHECK_MODULES(BCOP, bcop >= 0.7.3, [bcop_found=yes])
if test "$bcop_found" = yes; then
  if test -z "$PKG_CONFIG"; then
      AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  fi
  if test "$PKG_CONFIG" != "no" ; then
      BCOP_BIN=`$PKG_CONFIG --variable=bin bcop`
      AC_SUBST(BCOP_BIN)
  fi
fi

AC_MSG_CHECKING(for GL_CFLAGS)
AC_ARG_WITH(gl-cflags, [  --with-gl-cflags=CFLAGS ],
		       [GL_CFLAGS="$withval"],
		       [GL_CFLAGS=""])

AC_MSG_RESULT($GL_CFLAGS)
AC_MSG_CHECKING(for GL_LIBS)
AC_ARG_WITH(gl-libs, [  --with-gl-libs=LIBS ],
		     [GL_LIBS="$withval"],
		     [GL_LIBS="-lGL"])
AC_MSG_RESULT($GL_LIBS)

AC_SUBST(GL_CFLAGS)
AC_SUBST(GL_LIBS)


PKG_CHECK_MODULES(CAIRO, cairo-xlib cairo >= 1.0, [have_cairo=yes], [have_cairo=no])
AM_CONDITIONAL(FREEWINS_PLUGIN, test "x$have_cairo" = "xyes")

PKG_CHECK_MODULES(COMPIZTEXT, compiz-text, [have_compiz_text=yes], [have_compiz_text=no])
AM_CONDITIONAL(ELEMENTS_PLUGIN, test "x$have_compiz_text" = "xyes")
AM_CONDITIONAL(STACKSWITCH_PLUGIN, test "x$have_compiz_text" = "xyes")
AM_CONDITIONAL(WORKSPACENAMES_PLUGIN, test "x$have_compiz_text" = "xyes")

PKG_CHECK_MODULES(COMPIZCUBE, compiz-cube, [have_compiz_cube=yes], [have_compiz_cube=no])
AM_CONDITIONAL(ATLANTIS_PLUGIN, test "x$have_compiz_cube" = "xyes")
AM_CONDITIONAL(CUBEMODEL_PLUGIN, test "x$have_compiz_cube" = "xyes")
AM_CONDITIONAL(PHOTOWHEEL_PLUGIN, test "x$have_compiz_cube" = "xyes")
AM_CONDITIONAL(SNOWGLOBE_PLUGIN, test "x$have_compiz_cube" = "xyes")

PKG_CHECK_MODULES(COMPIZMOUSEPOLL, compiz-mousepoll, [have_compiz_mousepoll=yes], [have_compiz_mousepoll=no])
AM_CONDITIONAL(GHOST_PLUGIN, test "x$have_compiz_mousepoll" = "xyes")
AM_CONDITIONAL(WIZARD_PLUGIN, test "x$have_compiz_mousepoll" = "xyes")

PKG_CHECK_MODULES(XSCRNSAVER, xscrnsaver, [have_xscrnsaver=yes], [have_xscrnsaver=no])
AM_CONDITIONAL(SCREENSAVER_PLUGIN, test "x$have_compiz_text" = "xyes" && test "x$have_xscrnsaver" = "xyes")

PKG_CHECK_MODULES(COMPIZANIMATION, compiz-animation, [have_compiz_animation=yes], [have_compiz_animation=no])
AM_CONDITIONAL(ANIMATIONSIM_PLUGIN, test "x$have_compiz_animation" = "xyes")

PKG_CHECK_MODULES(COMPIZANIMATIONADDON, compiz-animationaddon, [have_compiz_animationaddon=yes], [have_compiz_animationaddon=no])
AM_CONDITIONAL(ANIMATIONPLUS_PLUGIN, test "x$have_compiz_animationaddon" = "xyes")

PKG_CHECK_MODULES(GLEW, glew, [have_glew=yes], [have_glew=no])
PKG_CHECK_MODULES(LIBCURL, libcurl, [have_libcurl=yes], [have_libcurl=no])
AM_CONDITIONAL(EARTH_PLUGIN, test "x$have_glew" = "xyes" && test "x$have_libcurl" = "xyes")

AC_PATH_PROG(UPDATE_ICON_CACHE, gtk-update-icon-cache)

AC_OUTPUT([
Makefile
icons/Makefile
data/Makefile
data/earth/Makefile
images/Makefile
images/earth/Makefile
images/elements/Makefile
images/fireflies/Makefile
images/snow/Makefile
images/stars/Makefile
include/Makefile
metadata/Makefile
src/Makefile
src/anaglyph/Makefile
src/atlantis/Makefile
src/cubemodel/Makefile
src/dialog/Makefile
src/earth/Makefile
src/elements/Makefile
src/extra-animations/Makefile
src/fakeargb/Makefile
src/fireflies/Makefile
src/freewins/Makefile
src/ghost/Makefile
src/mswitch/Makefile
src/photowheel/Makefile
src/putplus/Makefile
src/screensaver/Makefile
src/simple-animations/Makefile
src/smartput/Makefile
src/snowglobe/Makefile
src/snow/Makefile
src/stackswitch/Makefile
src/stars/Makefile
src/static/Makefile
src/swap/Makefile
src/throw/Makefile
src/tile/Makefile
src/toggle-decoration/Makefile
src/wizard/Makefile
src/workspacenames/Makefile
po/Makefile.in
])
