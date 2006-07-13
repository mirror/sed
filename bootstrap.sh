#! /bin/sh

# edit this to taste; note that you can also override via the environment:
case "$CC" in
  "") CC=cc
esac

if test -f config.h; then :; else
  echo "Creating basic config.h..."
  cat >config.h <<'END_OF_CONFIG_H'
/* A bootstrap version of config.h, for systems which can't
   auto-configure due to a lack of a working sed.  If you are on
   a sufficiently odd machine you may need to hand-tweak this file.

   Regardless, once you get a working version of sed you really should
   re-build starting with a run of "configure", as the bootstrap
   version is almost certainly more crippled than it needs to be on
   your machine.
*/

#define PACKAGE "sed"
#define VERSION "3.62-boot"
#define SED_FEATURE_VERSION "4.1"
#define BOOTSTRAP 1

/* Define if your compiler/headers don't support const. */
#undef const

/* Undefine if headers have conflicting definition.  */
#define mbstate_t int

/* Toggle if you encounter errors in lib/mkstemp.c.  */
#define HAVE_UNISTD_H
#define HAVE_FCNTL_H
#undef HAVE_SYS_FILE_H
#undef HAVE_IO_H

/* All other config.h.in options intentionally omitted.  Report as a
   bug if you need extra "#define"s in here. */
END_OF_CONFIG_H

  cat > conftest.c << \EOF
#define size_t unsigned
#include <sys/types.h>
#include <stdio.h>

size_t s;
EOF
  if $CC -c conftest.c -o conftest.o > /dev/null 2>&1 ; then
    echo '#define size_t unsigned' >> config.h
    echo checking for size_t... no
  else
    echo checking for size_t... yes
  fi

  cat > conftest.c << \EOF
#define ssize_t int
#include <sys/types.h>
#include <stdio.h>

ssize_t s;
EOF
  if $CC -c conftest.c -o conftest.o > /dev/null 2>&1 ; then
    echo '#define ssize_t int' >> config.h
    echo checking for ssize_t... no
  else
    echo checking for ssize_t... yes
  fi

  cat > conftest.c << \EOF
void *foo;

EOF
  if $CC -c conftest.c -o conftest.o > /dev/null 2>&1 ; then
    echo checking for void *... yes
  else
    echo '#define VOID char' >> config.h
    echo checking for void *... no
  fi

  cat > conftest.c << \EOF
#include <malloc.h>
int foo;

EOF
  if $CC -c conftest.c -o conftest.o > /dev/null 2>&1 ; then
    echo checking for malloc.h... yes
    echo '#define HAVE_MALLOC_H 1' >> config.h
    echo '#undef STDC_HEADERS' >> config.h
  else
    echo checking for malloc.h... no
    echo '#undef HAVE_MALLOC_H' >> config.h
    echo '#define STDC_HEADERS 1' >> config.h
  fi

  rm -f conftest.*
fi

# tell the user what we're doing from here on...
set -x -e

# the ``|| exit 1''s are for fail-stop; set -e doesn't work on some systems

rm -f lib/*.o sed/*.o sed/sed
cd lib || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c alloca.c
${CC} -DHAVE_CONFIG_H -I.. -I. -c getline.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c getopt.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c getopt1.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c memchr.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c memcmp.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c memmove.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c mkstemp.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c strverscmp.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c obstack.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c strerror.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c utils.c || exit 1

cd ../pcre || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c regdebug.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c regexec.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c regexp.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c reginfo.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c regperl.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c regposix.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c regstudy.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c regsub.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c regtables.c || exit 1

cd ../sed || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -I ../pcre -c sed.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -I ../pcre -c fmt.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -I ../pcre -c compile.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -I ../pcre -c execute.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -I ../pcre -c mbcs.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -I ../pcre -c regexp.c || exit 1

${CC} -o sed *.o ../lib/*.o ../pcre/*.o || exit 1
