dnl Process this file with -*- autoconf -*- to produce a configure script.
AC_INIT(sed, 3.62, bonzini@gnu.org, ssed)
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

AC_MSG_CHECKING([whether -lcP is needed])
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
}], [AC_MSG_RESULT(no)],
    [AC_MSG_RESULT(yes)
     LIBS="-lcP $LIBS"],
    [AC_MSG_RESULT([assuming no])
])

AC_HEADER_DIRENT
AC_CHECK_HEADERS(io.h limits.h locale.h stdarg.h alloca.h stddef.h errno.h \
		 wchar.h wctype.h sys/file.h, [], [], [AC_INCLUDES_DEFAULT])
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
AC_REPLACE_FUNCS(memchr memcmp memmove strerror snprintf mkstemp)
AC_CHECK_FUNCS(isatty bcopy bzero isascii memcpy memset strchr strtoul popen \
		pathconf isblank fchown fchmod setlocale wcrtomb wcscoll btowc)

AC_ARG_ENABLE(pcregrep,
[  --enable-pcregrep       build the pcregrep test program],
PCREGREP=pcregrep,
PCREGREP=)
AC_SUBST(PCREGREP)

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
  sed -e 's/^mkinstalldirs *=.*/mkinstalldirs=$(MKINSTALLDIRS)/' \
      intl/Makefile > intl/Makefile.tmp
  mv intl/Makefile.tmp intl/Makefile
  sed -e 's/^mkinstalldirs *=.*/mkinstalldirs=$(MKINSTALLDIRS)/' \
      po/Makefile > po/Makefile.tmp
  mv po/Makefile.tmp po/Makefile])

AC_CONFIG_FILES([bootstrap.sh], chmod +x bootstrap.sh)
AC_CONFIG_FILES([Makefile doc/Makefile \
lib/Makefile sed/Makefile testsuite/Makefile \
po/Makefile.in intl/Makefile pcre/Makefile])
AC_OUTPUT
