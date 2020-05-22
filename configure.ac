#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([lfq], [v1.0], [peter@people.ops-trust.net])
m4_include([m4/ax_pthread.m4])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
CFLAGS="-g -O3"
AC_PROG_CC([clang gcc])
AX_CHECK_C11
CFLAGS="$CFLAGS -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn -fno-strict-aliasing"

LT_INIT 
LTDL_INIT
AX_PTHREAD

AC_DEFINE([_DEFAULT_SOURCE], 1, [BSD Functions])

CFLAGS="${CFLAGS=}"
if test $ac_cv_prog_gcc = yes -a "x$CFLAGS" = "x-g -O3"; then
    CFLAGS="-g -O3 -std=c11 -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn -fno-strict-aliasing"
fi

# Checks for pthread and args.
AX_PTHREAD([], [AC_MSG_ERROR([pthread library required])])
LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

# Checks for header files.
AC_HEADER_RESOLV
AC_CHECK_HEADERS([inttypes.h stddef.h stdint.h stdlib.h string.h stdatomic.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_SIZEOF(void *)

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for typedefs, structures, and compiler characteristics.

AC_OUTPUT(Makefile src/Makefile example/Makefile)
AC_OUTPUT
