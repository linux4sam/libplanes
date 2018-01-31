AC_PREREQ([2.63])
AC_INIT([planes],[0.0.1])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_SYSTEM
PKG_PROG_PKG_CONFIG

AM_INIT_AUTOMAKE([1.10 foreign])

# Enable quiet compiles on automake 1.11.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_PROG_CC_C99

if test "x$ac_cv_prog_cc_c99" = xno; then
	AC_MSG_ERROR([Building planes requires C99 enabled compiler])
fi

pkgconfigdir=${libdir}/pkgconfig
AC_SUBST(pkgconfigdir)

AC_USE_SYSTEM_EXTENSIONS

dnl ===========================================================================
dnl check compiler flags
AC_DEFUN([LIBDRM_CC_TRY_FLAG], [
  AC_MSG_CHECKING([whether $CC supports $1])

  libdrm_save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $1"

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([ ])], [libdrm_cc_flag=yes], [libdrm_cc_flag=no])
  CFLAGS="$libdrm_save_CFLAGS"

  if test "x$libdrm_cc_flag" = "xyes"; then
    ifelse([$2], , :, [$2])
  else
    ifelse([$3], , :, [$3])
  fi
  AC_MSG_RESULT([$libdrm_cc_flag])
])

MAYBE_WARN="-Wall -Wextra \
-Wsign-compare -Werror-implicit-function-declaration \
-Wpointer-arith -Wwrite-strings -Wstrict-prototypes \
-Wmissing-prototypes -Wmissing-declarations -Wnested-externs \
-Wpacked -Wswitch-enum -Wmissing-format-attribute \
-Wstrict-aliasing=2 -Winit-self \
-Wdeclaration-after-statement -Wold-style-definition \
-Wno-unused-parameter \
-Wno-attributes -Wno-long-long -Winline -Wshadow \
-Wno-missing-field-initializers"

# invalidate cached value if MAYBE_WARN has changed
if test "x$libdrm_cv_warn_maybe" != "x$MAYBE_WARN"; then
	unset libdrm_cv_warn_cflags
fi
AC_CACHE_CHECK([for supported warning flags], libdrm_cv_warn_cflags, [
	echo
	WARN_CFLAGS=""

	# Some warning options are not supported by all versions of
	# gcc, so test all desired options against the current
	# compiler.
	#
	# Note that there are some order dependencies
	# here. Specifically, an option that disables a warning will
	# have no net effect if a later option then enables that
	# warnings, (perhaps implicitly). So we put some grouped
	# options (-Wall and -Wextra) up front and the -Wno options
	# last.

	for W in $MAYBE_WARN; do
		LIBDRM_CC_TRY_FLAG([$W], [WARN_CFLAGS="$WARN_CFLAGS $W"])
	done

	libdrm_cv_warn_cflags=$WARN_CFLAGS
	libdrm_cv_warn_maybe=$MAYBE_WARN

	AC_MSG_CHECKING([which warning flags were supported])])
WARN_CFLAGS="$libdrm_cv_warn_cflags"
AC_SUBST(WARN_CFLAGS)

PKG_CHECK_MODULES(LIBDRM, [libdrm >= 2.4.0], [], [AC_MSG_ERROR([
     libdrm is a hard dependency, but a sufficient version was not found.])])

PKG_CHECK_MODULES(CJSON, [libcjson >= 1.6.0], [], [AC_MSG_ERROR([
     cJSON is a hard dependency, but a sufficient version was not found.])])

PKG_CHECK_MODULES(LUA, [lua >= 5.3.1], [], [AC_MSG_ERROR([
     liblua is a hard dependency, but a sufficient version was not found.])])

PKG_CHECK_MODULES(DIRECTFB, [directfb >= 1.7.0], [have_directfb=yes], [have_directfb=no])
AM_CONDITIONAL(HAVE_DIRECTFB, [test "x$have_directfb" != "xno"])

PKG_CHECK_MODULES(CAIRO, [cairo >= 1.14.6], [], [AC_MSG_ERROR([
     cairo is a hard dependency, but a sufficient version was not found.])])

PKG_CHECK_MODULES(PYTHON, [python >= 2.7], [have_python=yes], [have_python=no])
AM_CONDITIONAL(HAVE_PYTHON, [test "x$have_python" != "xno"])

if test "$prefix" = "NONE"; then
   prefix=$ac_default_prefix
fi

if test "x$have_python" = "xyes"; then
   AC_MSG_CHECKING([for Python site-packages path])
   PYTHON_VERSION='2.7'
   PYTHON_SITE_PKG=${prefix}/lib/python${PYTHON_VERSION}/site-packages
   AC_MSG_RESULT([$PYTHON_SITE_PKG])
   AC_SUBST([PYTHON_SITE_PKG])
fi

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_FUNC_ALLOCA

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT()

AC_CONFIG_FILES([Makefile
	src/Makefile
	apps/Makefile
	python/Makefile
	libplanes.pc
	Doxyfile])
AC_OUTPUT
