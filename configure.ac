#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([bzip3], [0.1], [kspalaiologos@gmail.com])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AC_COPYRIGHT([Copyright (C) Kamila Szewczyk, 2022. Licensed under the terms of the GNU General Public License v3.])
AC_LANG([C])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

AC_CHECK_LIB([pthread], [pthread_create])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h stdint.h unistd.h])

# Checks for typedefs, structures, and compiler/platform characteristics.
AC_C_INLINE
AC_C_RESTRICT
AC_C_CONST
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])
AC_C_BIGENDIAN

# Checks for library functions.
AC_CHECK_FUNCS([ntohl htonl isatty fileno])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
