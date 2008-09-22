EXTRA_DIST = \
  xml-priv.h \
  xr-utils.h \
  xr-call-xml-rpc.c \
  xr-call-json-rpc.c

AM_CFLAGS= \
  $(GLIB_CFLAGS) \
  $(XML_CFLAGS) \
  $(SSL_CFLAGS) \
  $(JSON_CFLAGS) \
  -I$(top_srcdir) \
  -I$(top_srcdir)/include \
  -D_REENTRANT \
  -D_GNU_SOURCE

lib_LTLIBRARIES = \
  libxr.la

libxr_la_LIBADD = \
  $(GLIB_LIBS) \
  $(XML_LIBS) \
  $(SSL_LIBS) \
  $(JSON_LIBS) \
  $(WIN32LIBS)

libxr_la_LDFLAGS = -version-info $(LIB_XR_VERSION) -no-undefined

libxr_la_SOURCES = \
  xr-lib.c \
  xr-value.c \
  xr-call.c \
  xr-client.c \
  xr-server.c \
  xr-http.c \
  xr-utils.c \
  xr-value-utils.c
