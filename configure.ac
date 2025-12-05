AC_PREREQ([2.69])
AC_INIT([kuaf], [1.0])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE([1.10 no-define])
AM_PROG_AS([AS])

AC_CONFIG_MACRO_DIR([build/m4])
AC_CONFIG_HEADERS([build/config.h])

AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL

AC_ARG_ENABLE([cover],
    [AS_HELP_STRING([--enable-cover], [enable coverage])],
    [enable_cover=yes],
    [enable_cover=no]
)
AM_CONDITIONAL([COVER], [test "$enable_cover" = "yes"])

AC_CONFIG_FILES([
	src/Makefile
    ])
AC_OUTPUT