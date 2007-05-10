EXTRA_DIST = \
  xml-priv.h \
  xr-http.h \
  xr-utils.h

AM_CFLAGS= \
  -std=gnu99 \
  $(XR_CFLAGS) \
  -I$(top_srcdir)/include \
  -D_REENTRANT \
  -D_GNU_SOURCE

lib_LTLIBRARIES = \
  libxr.la

libxr_la_LIBADD = $(XR_LIBS) -lssl -lcrypto $(WIN32LIBS)
libxr_la_LDFLAGS = -version-info 0:0:0 -no-undefined

libxr_la_SOURCES = \
  xr-lib.c \
  xr-value.c \
  xr-call.c \
  xr-client.c \
  xr-server.c \
  xr-http.c \
  xr-utils.c
