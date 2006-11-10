EXTRA_DIST = \
  bench.h

AM_CFLAGS= \
  -std=gnu99 \
  $(XR_CFLAGS) \
  -I$(top_srcdir)/include \
  -D_REENTRANT \
  -D_GNU_SOURCE

LDADD = $(XR_LIBS) $(top_builddir)/lib/libxr.la

bin_PROGRAMS = \
  server \
  client \
  test

test_SOURCES = \
  test.c

server_SOURCES = \
  server.c

client_SOURCES = \
  client.c

#CLEANFILES = create.c
#EXTRA_DIST += create.pgc

#%.c: %.pgc
#	$(ECPG) $<
