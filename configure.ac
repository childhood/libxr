AC_PREREQ(2.62)
AC_INIT([libxr],[0.9.96],[3e@zonio.net],[libxr])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([dist-bzip2 foreign])
AM_MAINTAINER_MODE

AC_PROG_CC
AM_PROG_CC_C_O
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

GLIB_REQUIRES="glib-2.0 >= 2.12.0 gthread-2.0 >= 2.12.0"
XML_REQUIRES="libxml-2.0 >= 2.6.20"
SSL_REQUIRES="libssl libcrypto"
JSON_REQUIRES="json >= 0.3"

PKG_CHECK_MODULES(GLIB, [$GLIB_REQUIRES])
PKG_CHECK_MODULES(XML, [$XML_REQUIRES])
PKG_CHECK_MODULES(SSL, [$SSL_REQUIRES])
PKG_WITH_MODULES(JSON, [$JSON_REQUIRES], [have_json=yes], [have_json=no], [build JSON transport], [yes])

AC_SUBST(GLIB_REQUIRES)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
AC_SUBST(XML_REQUIRES)
AC_SUBST(XML_CFLAGS)
AC_SUBST(XML_LIBS)
AC_SUBST(SSL_REQUIRES)
AC_SUBST(SSL_CFLAGS)
AC_SUBST(SSL_LIBS)
AC_SUBST(JSON_REQUIRES)
AC_SUBST(JSON_CFLAGS)
AC_SUBST(JSON_LIBS)

# use g_regexp_*() on modern glib
AS_IF([pkg-config 'glib-2.0 >= 2.14.0'], [
  AC_DEFINE(HAVE_GLIB_REGEXP, 1, [whether glib supports regexps])
])

# on win32 we must link in wsock32
AS_IF([test "x$version_type" = xwindows], [WIN32LIBS="-lwsock32"], [WIN32LIBS=])
AC_SUBST(WIN32LIBS)

# enable compiler warnings
CFLAGS="$CFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare"
AS_IF([$CC --version | head -n1 | grep '(GCC) 4\.' &>/dev/null],
  [CFLAGS="$CFLAGS -Wno-pointer-sign"])

AS_IF([test "x$have_json" != "xyes"], [JSON_REQUIRES=""])

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

AS_IF([test "x$have_json" = "xyes"], [
  echo '#define XR_JSON_ENABLED 1' >> $outfile
])

  cat >> $outfile <<\_______EOF

#endif
_______EOF
])

# write output
AC_CONFIG_FILES([
  Makefile
  lib/Makefile
  xdl-compiler/Makefile
  docs/Makefile
  libxr.pc
  tests/Makefile
  tests/unit/Makefile
])
AC_OUTPUT

echo 
echo "libxr is now configured"
echo
echo "  xml-rpc transport: yes"
echo "  json transport:    $have_json"
echo
