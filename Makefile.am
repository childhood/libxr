AUTOMAKE_OPTIONS = foreign

SUBDIRS = include lib xdl-compiler tests docs misc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxr.pc

EXTRA_DIST = libxr.pc.in

libxrincludedir = $(includedir)/libxr
#libxrinclude_HEADERS = xr-config.h

DISTCLEANFILES = xr-config.h

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(libxrincludedir)
	$(INSTALL_DATA) xr-config.h $(DESTDIR)$(libxrincludedir)/xr-config.h

uninstall-local:
	rm -f $(DESTDIR)$(libxrincludedir)/xr-config.h
