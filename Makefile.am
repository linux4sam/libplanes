AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src apps python

EXTRA_DIST = README.md COPYING Doxyfile.in

.PHONY: doc

doc: Doxyfile
	@doxygen
