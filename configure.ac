AC_PREREQ(2.57)
AC_INIT([xr], 1.0, [ondrej.jirman@zonio.net], xr)
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC

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
])
