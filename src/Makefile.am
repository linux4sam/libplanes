AM_CFLAGS = $(WARN_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/include

lib_LTLIBRARIES = libplanes.la
libplanes_la_CFLAGS = $(AM_CFLAGS) $(LIBDRM_CFLAGS) $(CJSON_CFLAGS) $(CAIRO_CFLAGS) $(LUA_CFLAGS)
libplanes_la_LIBADD = $(CJSON_LIBS) $(LUA_LIBS)
libplanes_la_SOURCES = kms-crtc.c \
kms-device.c \
kms-framebuffer.c \
kms-plane.c \
kms-screen.c \
engine.c \
draw.c \
plane.c \
fb.c \
common.h \
common.c \
script.h \
script.c \
p_kms.h

libplanesincludedir = ${includedir}/planes
libplanesinclude_HEADERS = $(top_srcdir)/include/planes/engine.h \
	$(top_srcdir)/include/planes/draw.h \
	$(top_srcdir)/include/planes/kms.h \
	$(top_srcdir)/include/planes/plane.h \
	$(top_srcdir)/include/planes/fb.h
libplanes_la_LDFLAGS = -version-info 1:1:0 -no-undefined

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = $(top_builddir)/libplanes.pc
