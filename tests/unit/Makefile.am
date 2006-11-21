EXTRA_DIST = \
  tests.h

CFLAGS= \
  -std=gnu99 \
  $(XR_CFLAGS) \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/lib \
  -D_REENTRANT \
  -D_GNU_SOURCE

AM_LDFLAGS = -static

LDADD = $(XR_LIBS)

noinst_PROGRAMS = \
  t001-http

# http test
t001_http_CFLAGS = \
  -DBIO_read=BIO_read_phony \
  -DBIO_write=BIO_write_phony
t001_http_SOURCES = \
  t001-http.c \
  $(top_srcdir)/lib/xr-http.c
