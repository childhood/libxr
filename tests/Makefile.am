SUBDIRS = unit zblok vala

BUILT_SOURCES = \
  TCommon.h \
  TCommon.c \
  TTest1.xrs.c \
  TTest1.xrc.c \
  TTest1.stubs.c \
  TTest1.c \
  TTest1.xrs.h \
  TTest1.xrc.h \
  TTest1.stubs.h \
  TTest1.h \
  TTest2.xrs.c \
  TTest2.xrc.c \
  TTest2.stubs.c \
  TTest2.c \
  TTest2.xrs.h \
  TTest2.xrc.h \
  TTest2.stubs.h \
  TTest2.h

EXTRA_DIST = \
  bench.h \
  bench.sh \
  test.xdl \
  server.pem \
  $(BUILT_SOURCES)

AM_CFLAGS= \
  $(GLIB_CFLAGS) \
  $(XML_CFLAGS) \
  $(JSON_CFLAGS) \
  -I$(top_builddir) \
  -I$(top_srcdir)/include \
  -D_REENTRANT \
  -D_GNU_SOURCE

AM_LDFLAGS = -static

CLEANFILES = $(BUILT_SOURCES)

LDADD = \
  $(GLIB_LIBS) \
  $(XML_LIBS) \
  $(JSON_LIBS) \
  $(top_builddir)/lib/libxr.la

noinst_PROGRAMS = \
  tclient \
  client \
  server

client_SOURCES = \
  client.c \
  $(BUILT_SOURCES)

tclient_SOURCES = \
  tclient.c \
  $(BUILT_SOURCES)

server_SOURCES = \
  server.c \
  $(BUILT_SOURCES)

$(BUILT_SOURCES): $(srcdir)/test.xdl $(top_builddir)/xdl-compiler/xdl-compiler
	$(top_builddir)/xdl-compiler/xdl-compiler -i $(srcdir)/test.xdl -o $(top_builddir)/tests/
