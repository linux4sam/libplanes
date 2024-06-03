AC_PREREQ([2.63])
AC_INIT([libplanes],[1.0.5])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_SYSTEM
PKG_PROG_PKG_CONFIG

AM_INIT_AUTOMAKE([1.10 foreign])

# Enable quiet compiles on automake 1.11.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

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
-Wpacked -Wswitch-enum \
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

AC_CHECK_FUNCS([memset munmap strdup strerror strtol strtoul])
AC_CHECK_HEADERS([fcntl.h sys/ioctl.h])
AC_CHECK_HEADER_STDBOOL
AC_FUNC_MMAP
AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

AX_PKG_CHECK_MODULES([LIBDRM], [libdrm >= 2.4.0], [], [], [AC_MSG_ERROR([
     libdrm is a hard dependency, but a sufficient version was not found.])])

AX_PKG_CHECK_MODULES([CJSON], [], [libcjson >= 1.6.0], [], [AC_MSG_ERROR([
     cJSON is a hard dependency, but a sufficient version was not found.])])

AX_PKG_CHECK_MODULES([LUA], [], [lua >= 5.3.1], [have_lua=yes], [have_lua=no])
if test "x${have_lua}" = xno; then
    AX_PKG_CHECK_MODULES([LUA], [], [lua5.3 >= 5.3.1], [have_lua=yes], [have_lua=no])
    if test "x${have_lua}" = xno; then
        AC_MSG_ERROR([liblua is a hard dependency, but a sufficient version was
            not found.])
    fi
fi

PKG_CHECK_MODULES([DIRECTFB], [directfb >= 1.7.0], [have_directfb=yes], [have_directfb=no])
AM_CONDITIONAL(HAVE_DIRECTFB, [test "x$have_directfb" != "xno"])

if test "x$have_directfb" = "xyes"; then
   AC_C_INLINE
fi

AX_PKG_CHECK_MODULES([CAIRO], [], [cairo >= 1.14.6], [], [AC_MSG_ERROR([
     cairo is a hard dependency, but a sufficient version was not found.])])

AC_ARG_ENABLE([python],
              [AS_HELP_STRING([--disable-python],
                              [Disable support for python bindings (default: auto)])],
              [PYTHON=$enableval], [PYTHON=yes])

if test "x$PYTHON" = xyes; then
   PKG_CHECK_MODULES(PYTHON, [python >= 2.7], [have_python=yes], [have_python=no])

   if test "$prefix" = "NONE"; then
      prefix=$ac_default_prefix
   fi

   if test "x$have_python" = "xyes"; then
      AC_MSG_CHECKING([for Python site-packages path])
      PYTHON_VERSION=`$PKG_CONFIG --modversion python3`
      PYTHON_SITE_PKG=${prefix}/lib/python${PYTHON_VERSION}/site-packages
      AC_MSG_RESULT([$PYTHON_SITE_PKG])
      AC_SUBST([PYTHON_SITE_PKG])
   fi
fi
AM_CONDITIONAL(HAVE_PYTHON, [test "x$have_python" = xyes])

AC_ARG_ENABLE([examples],
  [AS_HELP_STRING([--enable-examples], [build examples [default=yes]])],
  [enable_examples=$enableval], [enable_examples=yes])
AM_CONDITIONAL([ENABLE_EXAMPLES], [test "x${enable_examples}" = xyes])

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
	docs/Doxyfile])
AC_OUTPUT
