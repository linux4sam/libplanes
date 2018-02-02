AM_CFLAGS = -Wall -I$(top_srcdir) -I$(top_srcdir)/include

if HAVE_PYTHON
SWIG_SRC = planes.i

SWIG=swig
PYTHON=python

SWIG_V_GEN = $(swig_v_GEN_$(V))
swig_v_GEN_ = $(swig_v_GEN_$(AM_DEFAULT_VERBOSITY))
swig_v_GEN_0 = @echo "  SWIG    " $@;

planes_wrap.c: $(SWIG_SRC)
	$(SWIG_V_GEN)$(SWIG) -python -I$(top_srcdir)/include -o $@ $<

PYTHON_BUILT = planes_wrap.c planes.py
BUILT_SOURCES = $(PYTHON_BUILT)

pythonextdir = $(PYTHON_SITE_PKG)/planes
pythonext_SCRIPTS = __init__.pyc
pythonext_LTLIBRARIES = _planes.la
_planes_la_SOURCES = planes_wrap.c $(SWIG_SRC)
_planes_la_CFLAGS = $(AM_CFLAGS) $(PYTHON_CFLAGS) $(LIBDRM_CFLAGS) $(CJSON_CFLAGS) $(CAIRO_CFLAGS) $(LUA_CFLAGS)
_planes_la_LDFLAGS = -module
_planes_la_LIBADD = $(top_builddir)/src/libplanes.la $(LIBDRM_LIBS) $(CAIRO_LIBS) $(CJSON_LIBS) $(LUA_LIBS)


CLEANFILES = $(PYTHON_BUILT) $(pythonext_SCRIPTS)
PYTHONC_V_GEN = $(pythonc_v_GEN_$(V))
pythonc_v_GEN_ = $(pythonc_v_GEN_$(AM_DEFAULT_VERBOSITY))
pythonc_v_GEN_0 = @echo "  PYTHONC " $@;

__init__.pyc: planes.py
	$(PYTHONC_V_GEN)$(PYTHON) -c 'import py_compile, sys; py_compile.compile(sys.argv[1], sys.argv[2])' $< $@
planes.py: planes_wrap.c

EXTRA_DIST = $(PYTHON_BUILT)

endif
