SUBDIRS = unit zblok

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
  TTest1.h

EXTRA_DIST = \
  bench.h \
  bench.sh \
  test.xdl \
  server.pem \
  $(BUILT_SOURCES)

AM_CFLAGS= \
  -std=gnu99 \
  $(XR_CFLAGS) \
  -I$(top_srcdir)/include \
  -D_REENTRANT \
  -D_GNU_SOURCE

AM_LDFLAGS = -static

LDADD = $(XR_LIBS) $(top_builddir)/lib/libxr.la

noinst_PROGRAMS = \
  tclient \
  client \
  server \
  mlserver

client_SOURCES = \
  client.c \
  TCommon.c \
  TTest1.xrc.c \
  TTest1.c

tclient_SOURCES = \
  tclient.c \
  TCommon.c \
  TTest1.xrc.c \
  TTest1.c

server_SOURCES = \
  server.c \
  TCommon.c \
  TTest1.xrs.c \
  TTest1.stubs.c \
  TTest1.c

mlserver_SOURCES = \
  mlserver.c \
  TCommon.c \
  TTest1.xrs.c \
  TTest1.stubs.c \
  TTest1.c

$(BUILT_SOURCES): $(srcdir)/test.xdl
	$(top_builddir)/xdl-compiler/xdl-compiler -i $(srcdir)/test.xdl -o $(top_builddir)/tests/
