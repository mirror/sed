dnl Process this file with -*- autoconf -*- to produce a configure script.
AC_INIT([super-sed], 3.63, bonzini@gnu.org, ssed)
AC_CONFIG_AUX_DIR(build-aux)
AC_CONFIG_SRCDIR([sed/sed.c])
AM_CONFIG_HEADER(config.h:config_h.in)
AC_PREREQ(2.60)
AM_INIT_AUTOMAKE

SED_FEATURE_VERSION=4.2.2
AC_DEFINE_UNQUOTED(SED_FEATURE_VERSION, "$SED_FEATURE_VERSION",
  [Define to the version of GNU sed whose features are supported by this sed.])
AC_SUBST(SED_FEATURE_VERSION)

AC_PROG_CC
gl_EARLY
gl_INIT
gl_DISABLE_THREADS
AC_CACHE_CHECK([whether "rt" can be used with fopen], [sed_cv_fopen_rt], [
AC_TRY_RUN([
#include <stdio.h>
#include <errno.h>

int main()
{
  FILE *fp;
  int result;
  errno = 0;
  fp = fopen ("conftest.c", "rt");
  if (fp) fclose (fp);
  return fp ? 0 : 1;
}], [sed_cv_fopen_rt=yes],
    [sed_cv_fopen_rt=no],
    [case $host in
       *cygwin* | *mingw*) sed_cv_fopen_rt=yes ;;
       *) sed_cv_fopen_rt='assuming no' ;;
     esac])])
if test "$sed_cv_fopen_rt" = yes; then
  AC_DEFINE([HAVE_FOPEN_RT], [1],
            [Defined if "rt" can be used as a mode to fopen.])
fi

AC_CACHE_CHECK([whether -lcP is needed], [sed_cv_libcp_needed], [
AC_TRY_RUN([
#include <stdio.h>
#include <errno.h>

int main()
{
  FILE *fp;
  int result;
  errno = 0;
  fp = fopen ("conftest.c", "r");
  if (!fp) return 0;	   /* error, assume not needed */
  result = fflush (fp) == EOF && errno == 0;
  fclose (fp);
  return result;
}], [sed_cv_libcp_needed=no],
    [sed_cv_libcp_needed=yes],
    [sed_cv_libcp_needed="assuming no"])
])
if test "$sed_cv_libcp_needed" = yes; then
  LIBS="-lcP $LIBS"
fi

AC_CHECK_HEADERS_ONCE(locale.h errno.h wchar.h wctype.h,
		      [], [], [AC_INCLUDES_DEFAULT])
AC_C_CONST
AC_TYPE_SIZE_T

AM_GNU_GETTEXT_VERSION(0.17)
AM_GNU_GETTEXT([external])

AC_CHECK_FUNCS_ONCE(isatty isascii memcpy strchr strtoul lstat readlink
		    popen pathconf fchown fchmod setlocale)

# Check whether we are able to follow symlinks
if test "$ac_cv_func_lstat:$ac_cv_func_readlink" = yes:yes; then
   AC_DEFINE(ENABLE_FOLLOW_SYMLINKS, ,[Follow symlinks when processing in place])
fi

AC_ARG_ENABLE(pcregrep,
[  --enable-pcregrep       build the pcregrep test program],
PCREGREP=pcregrep,
PCREGREP=)
AC_SUBST(PCREGREP)

# Under MinGW, the bsd.sh test fails because of the EOF character (^Z).
case $host in
  *mingw*) XFAIL_TESTS="$XFAIL_TESTS bsd" ;;
  *) ;;
esac
AC_SUBST([XFAIL_TESTS])

AC_CONFIG_FILES([testsuite/version.good:testsuite/version.gin])
AC_CONFIG_FILES([Makefile doc/Makefile \
lib/Makefile sed/Makefile testsuite/Makefile \
po/Makefile.in pcre/Makefile])
AC_OUTPUT
