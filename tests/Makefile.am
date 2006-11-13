EXTRA_DIST = \
  bench.h \
  bench.sh \
  test.xdl \
  server.pem \
  $(wildcard EEE*)

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

client_SOURCES = \
  client.c \
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

local-all: EEECommon.h

EEECommon.h: test.xdl
	$(top_builddir)/xdl-compiler/xdl-compiler -i test.xdl -o $(top_builddir)/tests/

CLEANFILES = EEE*
