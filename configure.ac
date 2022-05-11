AC_INIT([bzip3], [1.0.1], [https://github.com/kspalaiologos/bzip3])
LT_INIT
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects tar-pax color-tests])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([build-aux])

AC_PROG_CC([clang gcc icc])
LT_INIT

AC_ARG_ENABLE([pthread],
			  AS_HELP_STRING([--enable-pthread], [Enable use of pthread library [default=yes]]),
			  [pthread_enabled=$enableval],
			  [pthread_enabled=yes]
			  )
AM_CONDITIONAL([ENABLE_PTHREAD], [test x"$pthread_enabled" == xyes])
AM_COND_IF([ENABLE_PTHREAD], [
	AC_CHECK_HEADER(pthread.h,
					[AX_PTHREAD([CFLAGS="$CFLAGS $PTHREAD_CFLAGS -DPTHREAD" LIBS="$LIBS $PTHREAD_LIBS"])],
					[AC_MSG_ERROR([pthread.h not found, use --disable-pthread to skip])])
])

AC_CONFIG_FILES([Makefile])
AC_ARG_PROGRAM
AC_OUTPUT
