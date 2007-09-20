BUILT_SOURCES = \
	xdl-parser.h \
	xdl-parser.c

EXTRA_DIST = \
	lempar.c \
	lemon.c \
	xdl-compiler.1 \
	xdl-parser.yre \
	$(BUILT_SOURCES)

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

xdl-parser.h xdl-parser.c: xdl-parser.yre
	test -r $(builddir)/lempar.c || test x$(srcdir) = x$(builddir) || cat $(srcdir)/lempar.c > $(builddir)/lempar.c
	gcc -o lemon $(srcdir)/lemon.c
	re2c -s -i -o tmp.y $<
	./lemon -q tmp.y
	rm -f tmp.y
	mv tmp.c xdl-parser.c
	mv tmp.h xdl-parser.h
	test x$(srcdir) = x$(builddir) || rm -f $(builddir)/lempar.c

CLEANFILES = lemon
