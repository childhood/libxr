SUBDIRS = unit

BUILT_SOURCES = \
  EEECommon.h \
  EEECommon.xrm.h \
  EEECommon.c \
  EEEServer.xrs.c \
  EEEClient.xrs.c \
  EEEServer.xrc.c \
  EEEClient.xrc.c \
  EEEServer.stubs.c \
  EEEClient.stubs.c \
  EEEServer.c \
  EEEClient.c \
  EEEServer.xrs.h \
  EEEClient.xrs.h \
  EEEServer.xrc.h \
  EEEClient.xrc.h \
  EEEServer.xrm.h \
  EEEClient.xrm.h \
  EEEServer.stubs.h \
  EEEClient.stubs.h \
  EEEServer.h \
  EEEClient.h

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
  client \
  tclient \
  server \
  mlserver

client_SOURCES = \
  client.c \
  EEECommon.c \
  EEEServer.xrc.c \
  EEEClient.xrc.c \
  EEEServer.c \
  EEEClient.c

tclient_SOURCES = \
  tclient.c \
  EEECommon.c \
  EEEServer.xrc.c \
  EEEClient.xrc.c \
  EEEServer.c \
  EEEClient.c

server_SOURCES = \
  server.c \
  EEECommon.c \
  EEEServer.xrs.c \
  EEEClient.xrs.c \
  EEEServer.stubs.c \
  EEEClient.stubs.c \
  EEEServer.c \
  EEEClient.c

mlserver_SOURCES = \
  mlserver.c \
  EEECommon.c \
  EEEServer.xrs.c \
  EEEClient.xrs.c \
  EEEServer.stubs.c \
  EEEClient.stubs.c \
  EEEServer.c \
  EEEClient.c

$(BUILT_SOURCES): $(srcdir)/test.xdl
	$(top_builddir)/xdl-compiler/xdl-compiler -i $(srcdir)/test.xdl -o $(top_builddir)/tests/
