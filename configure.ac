AC_PREREQ([2.68])
AC_INIT([bzip3], [1.0.1], [https://github.com/kspalaiologos/bzip3])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects tar-pax color-tests])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([build-aux])

AC_PROG_CC([clang gcc icc])
LT_INIT

AC_ARG_ENABLE([pthread],
			  AS_HELP_STRING([--disable-pthread], [Disable use of pthread library]]))
AM_CONDITIONAL([ENABLE_PTHREAD], [test x"$enable_pthread" != xno])
AM_COND_IF([ENABLE_PTHREAD], [
	AC_CHECK_HEADER(pthread.h,
					[AX_PTHREAD([CFLAGS="$CFLAGS $PTHREAD_CFLAGS -DPTHREAD" LIBS="$LIBS $PTHREAD_LIBS"])],
					[AC_MSG_ERROR([pthread.h not found, use --disable-pthread to skip])])
])

AC_ARG_ENABLE([arch-native],
			  AS_HELP_STRING([--disable-arch-native], [Disable CPU-specific optimizations]))
AM_CONDITIONAL([ENABLE_ARCH_NATIVE], [test x"$enable_arch_native" != xno])
AM_COND_IF([ENABLE_ARCH_NATIVE], [
			AX_CHECK_COMPILE_FLAG([-march=native], [],
								  [AC_MSG_ERROR([Compiler does not support native optimizations, use --disable-arch-native])])
			AX_CHECK_COMPILE_FLAG([-mtune=native], [],
								  [AC_MSG_ERROR([Compiler does not support native optimizations, use --disable-arch-native])])
])

AC_CONFIG_FILES([Makefile])
AC_ARG_PROGRAM
AC_OUTPUT
