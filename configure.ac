AC_PREREQ(2.57)
AC_INIT([libxr], 0.9.0, [ondrej.jirman@zonio.net], libxr)
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])
AC_CANONICAL_SYSTEM
AC_CANONICAL_BUILD
AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# for xdl
AM_PROG_LEX
AC_PROG_YACC

# Checks for pkg-config packages
GLIB_REQUIRED="glib-2.0 >= 2.6.0"
GTHREAD_REQUIRED="gthread-2.0 >= 2.6.0"
XMLRPC_REQUIRED="libxml-2.0 >= 2.6.20"
PKG_CHECK_MODULES(XR, [$GLIB_REQUIRED $GTHREAD_REQUIRED $XMLRPC_REQUIRED])

AC_SUBST(XR_CFLAGS)
AC_SUBST(XR_LIBS)

# Checks for header files.
AC_HEADER_STDC

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
