AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = \
  xmlrpc.h

AM_CFLAGS= \
  -std=gnu99 \
  $(XR_CFLAGS) \
  -D_REENTRANT \
  -D_GNU_SOURCE

bin_PROGRAMS = \
  server \
  client

LDADD = $(XR_LIBS)

server_SOURCES = \
  server.c \
  xmlrpc.c

client_SOURCES = \
  client.c \
  xmlrpc.c

#CLEANFILES = create.c
#EXTRA_DIST += create.pgc

#%.c: %.pgc
#	$(ECPG) $<
