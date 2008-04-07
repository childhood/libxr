AC_PREREQ(2.57)
AC_INIT([libxr], 0.9.94, [3e@zonio.net], libxr)
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])
AC_CANONICAL_SYSTEM
AC_CANONICAL_BUILD
AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_HEADER_STDC

# Before making a release, the version string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A

LIB_XR_VERSION=1:0:0
AC_SUBST(LIB_XR_VERSION)

# check for dependencies
AC_CHECK_PROG(RE2C, re2c, re2c)

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.12.0 gthread-2.0 >= 2.12.0])
PKG_CHECK_MODULES(XML, [libxml-2.0 >= 2.6.20])
PKG_CHECK_MODULES(JSON, [json >= 0.3], have_json=yes, have_json=no)

AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
AC_SUBST(XML_CFLAGS)
AC_SUBST(XML_LIBS)
AC_SUBST(JSON_CFLAGS)
AC_SUBST(JSON_LIBS)

# use g_regexp_*() on modern glib
if pkg-config 'glib-2.0 >= 2.14.0' ; then
  AC_DEFINE(HAVE_GLIB_REGEXP, 1, [whether glib supports regexps])
fi

# on win32 we must link in wsock32
WIN32LIBS=
if [[ "$version_type" = windows ]] ; then
  WIN32LIBS="-lwsock32"
fi
AC_SUBST(WIN32LIBS)

# enable compiler warnings
CFLAGS="$CFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare"
if $CC --version | head -n1 | grep '(GCC) 4\.' &>/dev/null ; then
  CFLAGS="$CFLAGS -Wno-pointer-sign"
fi

AC_ARG_ENABLE(json,
              [AC_HELP_STRING([--disable-json],
              [disable json transport])],,
              [enable_json=yes])

if test "x$enable_json" = "xyes" ; then
  if test "x$have_json" = "xno" ; then
    enable_json=no
  fi
fi

export enable_json
dnl #AM_CONDITIONAL(ENABLE_JSON, $enable_json)

# generate xr-config.h
AC_CONFIG_COMMANDS([xr-config.h],
[
  outfile=xr-config.h
  cat > $outfile <<\_______EOF
/** xr-config.h
 *
 * This is a generated file.  Please modify 'configure.in'
 */

#ifndef __XR_CONFIG_H__
#define __XR_CONFIG_H__

_______EOF

if test "x$enable_json" = "xyes" ; then
  echo '#define XR_JSON_ENABLED 1' >> $outfile
fi

  cat >> $outfile <<\_______EOF

#endif
_______EOF
])

# write output
AC_OUTPUT([
  Makefile
  include/Makefile
  lib/Makefile
  xdl-compiler/Makefile
  docs/Makefile
  misc/Makefile
  libxr.pc
  tests/Makefile
  tests/unit/Makefile
  tests/zblok/Makefile
  tests/vala/Makefile
])

echo 
echo "libxr is now configured"
echo
echo "  xml-rpc transport: yes"
echo "  json transport:    $enable_json"
echo
