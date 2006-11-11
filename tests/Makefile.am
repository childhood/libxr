EXTRA_DIST = \
  bench.h \
  bench.sh \
  test.xdl \
  server.pem \
  t-xdl-compiler1.sh

TESTS = \
  t-xdl-compiler1.sh

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

#  test

test_SOURCES = \
  test.c

server_SOURCES = \
  server.c

client_SOURCES = \
  client.c \
  EEEClient.xrc.c

#CLEANFILES = create.c
#EXTRA_DIST += create.pgc

#%.c: %.pgc
#	$(ECPG) $<
