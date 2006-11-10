EXTRA_DIST = xri.h

AM_CFLAGS = \
	-I$(top_srcdir)/src \
	$(XR_CFLAGS) \
	-D_GNU_SOURCE

AM_YFLAGS = -d
LDADD = $(XR_LIBS)

bin_PROGRAMS = xdl-compiler

xdl_compiler_SOURCES = main.c xri.c parser.y scanner.l
