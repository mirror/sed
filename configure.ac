dnl Process this file with -*- autoconf -*- to produce a configure script.
AC_INIT(sed, 4.1.5, bonzini@gnu.org, sed)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR([sed/sed.c])
AM_CONFIG_HEADER(config.h:config_h.in)
AC_PREREQ(2.59)
AM_INIT_AUTOMAKE

SED_FEATURE_VERSION=4.1
AC_DEFINE_UNQUOTED(SED_FEATURE_VERSION, "$SED_FEATURE_VERSION",
  [Define to the version of GNU sed whose features are supported by this sed.])
AC_SUBST(SED_FEATURE_VERSION)

AC_PROG_CC
AC_PROG_RANLIB
AC_GNU_SOURCE
AC_AIX
AC_MINIX
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
AC_CHECK_HEADERS(io.h limits.h locale.h stdarg.h alloca.h stddef.h errno.h \
		 wchar.h wctype.h sys/file.h mcheck.h, [], [],
		 [AC_INCLUDES_DEFAULT])
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPE(ssize_t, int)

AC_HEADER_STDBOOL
if test "$ac_cv_type__Bool" = no; then
  HAVE__BOOL=0
else
  HAVE__BOOL=1
fi
AC_SUBST(HAVE__BOOL)
if test "$ac_cv_header_stdbool_h" = no; then
  AC_CONFIG_FILES(lib/stdbool.h:lib/stdbool_.h)
fi

AC_FUNC_ALLOCA
AC_FUNC_VPRINTF
AM_FUNC_GETLINE
AC_FUNC_OBSTACK
AC_FUNC_MBRTOWC
AC_TYPE_MBSTATE_T
gl_FUNC_STRVERSCMP
AC_REPLACE_FUNCS(memchr memcmp memmove strerror mkstemp)
AC_CHECK_FUNCS(isatty bcopy bzero isascii memcpy memset strchr strtoul popen \
		pathconf isblank fchown fchmod setlocale wcrtomb wcscoll btowc)

AC_ARG_ENABLE(i18n,
[  --disable-i18n          disable internationalization (default=yes)], ,
enable_i18n=yes)
if test "x$enable_i18n" = xno; then
  ac_cv_func_wcscoll=false
fi

AC_ARG_WITH(included-regex, 
[  --with-included-regex   use included regex matcher (default=yes)], ,
with_included_regex=yes)

if test "x$with_included_regex" = xno; then
  AC_CHECK_HEADERS(regex.h)
  AC_CHECK_LIB(regex, re_search)
  AC_CHECK_FUNC(re_search)
  if test $ac_cv_header_regex_h = no || test $ac_cv_func_re_search = no; then
    AC_MSG_WARN([GNU regex not found, falling back to the included version])
    with_included_regex=yes
  fi
fi

AC_ARG_ENABLE(regex-tests, 
[  --enable-regex-tests    enable regex matcher regression tests (default=yes)],
[if test "x$with_included_regex" = xno; then
  enable_regex_tests=no
fi],
enable_regex_tests=$with_included_regex)

AM_CONDITIONAL(TEST_REGEX, test "x$enable_regex_tests" != xno)
if test "x$with_included_regex" != xno; then
  AC_CONFIG_LINKS(lib/regex.h:lib/regex_.h)
  AC_LIBOBJ(regex)
fi
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


AC_DEFUN([AM_MKINSTALLDIRS], [MKINSTALLDIRS="$mkdir_p" AC_SUBST(MKINSTALLDIRS)])
AM_GNU_GETTEXT_VERSION(0.14)
AM_GNU_GETTEXT(no-libtool, need-ngettext, ../intl)
AC_CONFIG_COMMANDS([gettext-fix], [
  sed -e '/^mkinstalldirs *=/a\' \
      -e "install_sh=$install_sh" \
      -e 's/^mkinstalldirs *=.*/mkinstalldirs=$(MKINSTALLDIRS)/' \
      intl/Makefile > intl/Makefile.tmp
  mv intl/Makefile.tmp intl/Makefile
  sed -e '/^mkinstalldirs *=/a\' \
      -e "install_sh=$install_sh" \
      -e 's/^mkinstalldirs *=.*/mkinstalldirs=$(MKINSTALLDIRS)/' \
      po/Makefile > po/Makefile.tmp
  mv po/Makefile.tmp po/Makefile], [
  install_sh="$install_sh"])

AC_CONFIG_FILES([bootstrap.sh], chmod +x bootstrap.sh)
AC_CONFIG_FILES([Makefile doc/Makefile \
lib/Makefile sed/Makefile testsuite/Makefile \
po/Makefile.in intl/Makefile])
AC_OUTPUT
