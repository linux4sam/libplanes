AM_CFLAGS = $(WARN_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/include

bin_PROGRAMS = planes render grab

if HAVE_DIRECTFB
bin_PROGRAMS += dfblayers
endif

planes_SOURCES = planes.c
planes_CFLAGS = $(AM_CFLAGS) $(LIBDRM_CFLAGS) $(CAIRO_CFLAGS) $(CAIRO_CFLAGS)
planes_LDADD = $(top_builddir)/src/libplanes.la $(LIBDRM_LIBS) $(CAIRO_LIBS) $(CJSON_LIBS)
planesdir = $(prefix)/share/@PACKAGE@
planes_DATA = $(wildcard $(top_srcdir)/configs/*.png) \
	$(wildcard $(top_srcdir)/configs/*.config)
planes_LDFLAGS = $(AM_LDFLAGS)

render_SOURCES = render.c
render_CFLAGS = $(AM_CFLAGS) $(LIBDRM_CFLAGS) $(CJSON_CFLAGS) $(CAIRO_CFLAGS)
render_LDADD = $(top_builddir)/src/libplanes.la $(LIBDRM_LIBS) $(CJSON_LIBS) $(CAIRO_LIBS)
render_LDFLAGS = $(AM_LDFLAGS)

grab_SOURCES = grab.c
grab_CFLAGS = $(AM_CFLAGS) $(LIBDRM_CFLAGS) $(CJSON_CFLAGS) $(CAIRO_CFLAGS)
grab_LDADD = $(top_builddir)/src/libplanes.la $(LIBDRM_LIBS) $(CJSON_LIBS) $(CAIRO_LIBS)
grab_LDFLAGS = $(AM_LDFLAGS)

dfblayers_SOURCES = dfblayers.c
dfblayers_CFLAGS = $(AM_CFLAGS) $(DIRECTFB_CFLAGS)
dfblayers_LDADD = $(DIRECTFB_LIBS)
