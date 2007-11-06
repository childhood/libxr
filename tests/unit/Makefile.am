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
  t001-call

noinst_PROGRAMS = \
  $(TESTS)

# t001

t001_call_CFLAGS = \
  $(AM_CFLAGS)

t001_call_SOURCES = \
  t001-call.c \
  phony-lib.c \
  $(top_srcdir)/lib/xr-call.c \
  $(top_srcdir)/lib/xr-value.c
