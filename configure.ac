dnl Process this file with -*- autoconf -*- to produce a configure script.
AC_INIT(sed, 3.63, bonzini@gnu.org, ssed)
AC_CONFIG_AUX_DIR(build-aux)
AC_CONFIG_SRCDIR([sed/sed.c])
AM_CONFIG_HEADER(config.h:config_h.in)
AC_PREREQ(2.60)
AM_INIT_AUTOMAKE

SED_FEATURE_VERSION=4.2
AC_DEFINE_UNQUOTED(SED_FEATURE_VERSION, "$SED_FEATURE_VERSION",
  [Define to the version of GNU sed whose features are supported by this sed.])
AC_SUBST(SED_FEATURE_VERSION)

AC_PROG_CC
gl_DISABLE_THREADS
AC_USE_SYSTEM_EXTENSIONS
AC_GNU_SOURCE
gl_EARLY
AC_SYS_LARGEFILE
AC_SYS_LONG_FILE_NAMES

gl_INIT
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

AC_HEADER_DIRENT
AC_CHECK_HEADERS_ONCE(io.h limits.h locale.h stdarg.h alloca.h stddef.h errno.h \
		      wchar.h wctype.h sys/file.h, [], [], [AC_INCLUDES_DEFAULT])
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPE(ssize_t, int)

AC_FUNC_VPRINTF
AM_GNU_GETTEXT_VERSION(0.15)
AM_GNU_GETTEXT(no-libtool, need-ngettext, ../intl)

AC_CHECK_FUNCS_ONCE(isatty bcopy bzero isascii memcpy memset strchr strtoul
		    popen pathconf isblank fchown fchmod setlocale wcrtomb
		    wcscoll btowc)

AC_ARG_ENABLE(pcregrep,
[  --enable-pcregrep       build the pcregrep test program],
PCREGREP=pcregrep,
PCREGREP=)
AC_SUBST(PCREGREP)

# Check whether we are able to follow symlinks
AC_CHECK_FUNC(lstat, have_lstat=yes)
AC_CHECK_FUNC(readlink, have_readlink=yes)
if test "x$have_lstat" = xyes -a "x$have_readlink" = xyes; then
   AC_DEFINE(ENABLE_FOLLOW_SYMLINKS, ,[Follow symlinks when processing in place])
fi

AC_CONFIG_FILES([bootstrap.sh], chmod +x bootstrap.sh)
AC_CONFIG_FILES([testsuite/version.good:testsuite/version.gin])
AC_CONFIG_FILES([Makefile doc/Makefile \
lib/Makefile sed/Makefile testsuite/Makefile \
po/Makefile.in intl/Makefile pcre/Makefile])
AC_OUTPUT
