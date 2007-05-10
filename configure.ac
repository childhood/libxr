AC_PREREQ(2.57)
AC_INIT([libxr], 0.9.9, [oss@zonio.net], libxr)
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])
AC_CANONICAL_SYSTEM
AC_CANONICAL_BUILD
AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

# for xdl
AM_PROG_LEX
AC_PROG_YACC

# Checks for pkg-config packages
GLIB_REQUIRED="glib-2.0 >= 2.12.0"
GTHREAD_REQUIRED="gthread-2.0 >= 2.12.0"
XMLRPC_REQUIRED="libxml-2.0 >= 2.6.20"
PKG_CHECK_MODULES(XR, [$GLIB_REQUIRED $GTHREAD_REQUIRED $XMLRPC_REQUIRED])

AC_SUBST(XR_CFLAGS)
AC_SUBST(XR_LIBS)

if pkg-config 'glib-2.0 >= 2.13.0' ; then
  AC_DEFINE(HAVE_GLIB_REGEXP, 1, [whether glib supports regexps])
fi

# on win32 we must link in wsock32
WIN32LIBS=
if [[ "$version_type" = windows ]] ; then
  WIN32LIBS="-lwsock32"
fi
AC_SUBST(WIN32LIBS)

# Checks for header files.
AC_HEADER_STDC

CFLAGS="$CFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-pointer-sign -Wno-sign-compare"

# Output
AC_OUTPUT([
	Makefile
	include/Makefile
	lib/Makefile
	xdl-compiler/Makefile
	tests/Makefile
	docs/Makefile
	libxr.pc
	tests/unit/Makefile
])
