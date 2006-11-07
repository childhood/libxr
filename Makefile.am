AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = \
  xmlrpc-value.h \
  xmlrpc-call.h \
  xmlrpc-client.h \
  xmlrpc-server.h \
  xml.h \
  bench.h

AM_CFLAGS= \
  -std=gnu99 \
  $(XR_CFLAGS) \
  -D_REENTRANT \
  -D_GNU_SOURCE

noinst_LIBRARIES = \
  libxr.a

libxr_a_SOURCES = \
  xmlrpc-value.c \
  xmlrpc-call.c \
  xmlrpc-client.c \
  xmlrpc-server.c

bin_PROGRAMS = \
  server \
  client \
  test

LDADD = $(XR_LIBS) libxr.a -lssl -lcrypto

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
