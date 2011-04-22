AC_INIT([ibus-zinnia],
        [0.0.1],
        [http://github.com/yusukes/ibus-zinnia])

AC_PREREQ([2.50])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.10])
AC_GNU_SOURCE

AM_DISABLE_STATIC
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_ISC_POSIX
AC_HEADER_STDC

PKG_PROG_PKG_CONFIG([0.20])

AC_SUBST([datarootdir])

# check ibus
PKG_CHECK_MODULES(IBUS, [ibus-1.0 >= 1.3.99])

# OUTPUT files
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/zinnia.xml.in
                 ])

AC_OUTPUT
