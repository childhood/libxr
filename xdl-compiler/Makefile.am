EXTRA_DIST = xdl.h xdl-compiler.1

AM_CFLAGS = \
	$(XR_CFLAGS) \
	-D_GNU_SOURCE

AM_YFLAGS = -d
LDADD = $(XR_LIBS)

man_MANS = xdl-compiler.1

bin_PROGRAMS = xdl-compiler

xdl_compiler_SOURCES = xdl.c main.c parser.y scanner.l
