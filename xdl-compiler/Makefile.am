EXTRA_DIST = \
	lempar.c \
	xdl-compiler.1 \
	xdl-parser.yre

AM_CFLAGS = \
	$(XR_CFLAGS) \
	-D_GNU_SOURCE

LDADD = $(XR_LIBS)

man_MANS = xdl-compiler.1
bin_PROGRAMS = xdl-compiler

xdl_compiler_SOURCES = \
	xdl.h \
	xdl.c \
	main.c \
	parser-lib.h \
	parser-lib.c \
	xdl-parser.h \
	xdl-parser.c

# parser

noinst_PROGRAMS = lemon
lemon_SOURCES = lemon.c

xdl-parser.y: xdl-parser.yre
	re2c -s -i -o $@ $<

xdl-parser.c xdl-parser.h: xdl-parser.y lemon
	test -r $(builddir)/lempar.c || cat $(srcdir)/lempar.c > $(builddir)/lempar.c
	./lemon -q $<

BUILT_SOURCES = \
	xdl-parser.h \
	xdl-parser.c \
	xdl-parser.y
