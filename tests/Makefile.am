EXTRA_DIST = \
  bench.h \
  bench.sh \
  test.xdl \
  server.pem

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
  server

server_SOURCES = \
  server.c \
  EEEServer.xrs.c \
  EEEClient.xrs.c \
  EEEServer.stubs.c \
  EEEClient.stubs.c \
  EEEServer.c \
  EEEClient.c \
  EEECommon.c

client_SOURCES = \
  client.c \
  EEEServer.xrc.c \
  EEEClient.xrc.c \
  EEEServer.c \
  EEEClient.c \
  EEECommon.c

client.o: EEECommon.c
server.o: EEECommon.c

EEECommon.c: test.xdl
	../xdl-compiler/xdl-compiler -i test.xdl -o .

CLEANFILES = EEE*.{c,h}
