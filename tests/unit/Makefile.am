EXTRA_DIST = \
  tests.h

AM_CFLAGS = \
  $(XR_CFLAGS) \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/lib \
  -D_REENTRANT \
  -D_GNU_SOURCE

AM_LDFLAGS = -static

LDADD = $(XR_LIBS)

TESTS = \
  t001-http \
  t002-call

noinst_PROGRAMS = \
  $(TESTS)

# t001

t001_http_CFLAGS = \
  $(AM_CFLAGS) \
  -DBIO_read=BIO_read_phony \
  -DBIO_write=BIO_write_phony

t001_http_SOURCES = \
  t001-http.c \
  phony-lib.c \
  $(top_srcdir)/lib/xr-http.c

# t002

t002_call_CFLAGS = \
  $(AM_CFLAGS)

t002_call_SOURCES = \
  t002-call.c \
  phony-lib.c \
  $(top_srcdir)/lib/xr-call.c \
  $(top_srcdir)/lib/xr-value.c
