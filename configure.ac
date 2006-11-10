dnl Process this file with -*- autoconf -*- to produce a configure script.
AC_INIT(sed, 4.1a, bonzini@gnu.org, sed)
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
gl_EARLY
AC_ISC_POSIX
AC_SYS_LARGEFILE
AC_SYS_LONG_FILE_NAMES

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
AC_CHECK_HEADERS_ONCE(io.h limits.h locale.h stdarg.h alloca.h stddef.h
		      errno.h wchar.h wctype.h sys/file.h mcheck.h, [], [],
		 [AC_INCLUDES_DEFAULT])
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPE(ssize_t, int)

AC_FUNC_VPRINTF

AC_DEFUN([AM_MKINSTALLDIRS], [MKINSTALLDIRS="$mkdir_p" AC_SUBST(MKINSTALLDIRS)])
AM_GNU_GETTEXT_VERSION(0.15)
AM_GNU_GETTEXT(no-libtool, need-ngettext, ../intl)

gl_INIT
AC_CHECK_FUNCS_ONCE(isatty bcopy bzero isascii memcpy memset strchr strtoul
		    popen pathconf isblank fchown fchmod setlocale wcrtomb
		    wcscoll btowc)

AC_ARG_ENABLE(i18n,
[  --disable-i18n          disable internationalization (default=enabled)], ,
enable_i18n=yes)
if test "x$enable_i18n" = xno; then
  ac_cv_func_wcscoll=false
fi

AC_ARG_ENABLE(regex-tests, 
[  --enable-regex-tests    enable regex matcher regression tests (default=yes)],
[if test "x$with_included_regex" = xno; then
  enable_regex_tests=no
fi],
enable_regex_tests=$with_included_regex)

AM_CONDITIONAL(TEST_REGEX, test "x$enable_regex_tests" = xyes)
if test "x$enable_regex_tests" = xyes; then
  AC_DEFINE_UNQUOTED(_REGEX_RE_COMP, 1,
    [Include BSD functions in regex, used by the testsuite])
fi

AC_ARG_ENABLE(html, 
[  --enable-html           build HTML manual (default=no)], ,
enable_html=no)

AM_CONDITIONAL(BUILD_HTML, test "x$enable_html" != xno)

: ${TEXI2HTML=texi2html -monolithic}
AC_SUBST(TEXI2HTML)

AC_MSG_CHECKING(how to build HTML documentation)
if eval $am_missing_run makeinfo --help 2>&1 | grep .-html > /dev/null; then
  AC_MSG_RESULT(with makeinfo)
  enable_html=makeinfo
else
  if $TEXI2HTML --help 2>&1 | grep monolithic > /dev/null; then
    AC_MSG_RESULT(with texi2html)
    enable_html=texi2html
  else
    AC_MSG_RESULT(not built)
    if test "x$enable_html" != xno; then
      AC_MSG_ERROR(cannot build HTML documentation, install makeinfo 4.0 or texi2html)
    fi
    enable_html=no
  fi
fi 

AM_CONDITIONAL(MAKEINFO_HTML, test "x$enable_html" = xmakeinfo)
AM_CONDITIONAL(TEXI2HTML_HTML, test "x$enable_html" = xtexi2html)


AC_CONFIG_FILES([bootstrap.sh], chmod +x bootstrap.sh)
AC_CONFIG_FILES([testsuite/version.good:testsuite/version.gin])
AC_CONFIG_FILES([Makefile doc/Makefile \
lib/Makefile sed/Makefile testsuite/Makefile \
po/Makefile.in intl/Makefile])
AC_OUTPUT
