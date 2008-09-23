AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib xdl-compiler tests docs

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxr.pc

EXTRA_DIST = \
  misc/README \
  misc/xdl.vim

pkginclude_HEADERS = \
  include/xr-lib.h \
  include/xr-value.h \
  include/xr-call.h \
  include/xr-http.h \
  include/xr-client.h \
  include/xr-server.h \
  include/xr-value-utils.h

libxrincludedir = $(includedir)/libxr
#libxrinclude_HEADERS = xr-config.h

DISTCLEANFILES = xr-config.h

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(libxrincludedir)
	$(INSTALL_DATA) xr-config.h $(DESTDIR)$(libxrincludedir)/xr-config.h

uninstall-local:
	rm -f $(DESTDIR)$(libxrincludedir)/xr-config.h
