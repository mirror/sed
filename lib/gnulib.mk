# This file is generated automatically by "autoboot".
EXTRA_DIST += acl.h


BUILT_SOURCES += $(ALLOCA_H)
EXTRA_DIST += alloca_.h

# We need the following in order to create <alloca.h> when the system
# doesn't have one that works with the given compiler.
alloca.h: alloca_.h
	cp -f $(srcdir)/alloca_.h $@-t
	mv -f $@-t $@
MOSTLYCLEANFILES += alloca.h alloca.h-t


lib_SOURCES += exit.h

EXTRA_DIST += exitfail.h


EXTRA_DIST += getdelim.h

EXTRA_DIST += getline.h

BUILT_SOURCES += $(GETOPT_H)
EXTRA_DIST += getopt_.h getopt_int.h

# We need the following in order to create <getopt.h> when the system
# doesn't have one that works with the given compiler.
getopt.h: getopt_.h
	cp -f $(srcdir)/getopt_.h $@-t
	mv -f $@-t $@
MOSTLYCLEANFILES += getopt.h getopt.h-t

# This is for those projects which use "gettextize --intl" to put a source-code
# copy of libintl into their package. In such projects, every Makefile.am needs
# -I$(top_builddir)/intl, so that <libintl.h> can be found in this directory.
# For the Makefile.ams in other directories it is the maintainer's
# responsibility; for the one from gnulib we do it here.
# This option has no effect when the user disables NLS (because then the intl
# directory contains no libintl.h file) or when the project does not use
# "gettextize --intl".
# (commented out by autoboot) AM_CPPFLAGS += -I$(top_builddir)/intl

lib_SOURCES += gettext.h



lib_SOURCES += mbchar.h

lib_SOURCES += mbuiter.h






EXTRA_DIST += quote.h

EXTRA_DIST += quotearg.h

EXTRA_DIST += regcomp.c regex.h regex_internal.c regex_internal.h regexec.c


EXTRA_DIST += stat-macros.h

BUILT_SOURCES += $(STDBOOL_H)
EXTRA_DIST += stdbool_.h

# We need the following in order to create <stdbool.h> when the system
# doesn't have one that works.
stdbool.h: stdbool_.h
	rm -f $@-t $@
	sed -e 's/@''HAVE__BOOL''@/$(HAVE__BOOL)/g' < $(srcdir)/stdbool_.h > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += stdbool.h stdbool.h-t

BUILT_SOURCES += $(STDINT_H)
EXTRA_DIST += stdint_.h

# We need the following in order to create <stdint.h> when the system
# doesn't have one that works with the given compiler.
stdint.h: stdint_.h
	rm -f $@-t $@
	sed -e 's/@''HAVE_WCHAR_H''@/$(HAVE_WCHAR_H)/g' \
	    -e 's/@''HAVE_STDINT_H''@/$(HAVE_STDINT_H)/g' \
	    -e 's|@''ABSOLUTE_STDINT_H''@|$(ABSOLUTE_STDINT_H)|g' \
	    -e 's/@''HAVE_SYS_TYPES_H''@/$(HAVE_SYS_TYPES_H)/g' \
	    -e 's/@''HAVE_INTTYPES_H''@/$(HAVE_INTTYPES_H)/g' \
	    -e 's/@''HAVE_SYS_INTTYPES_H''@/$(HAVE_SYS_INTTYPES_H)/g' \
	    -e 's/@''HAVE_SYS_BITYPES_H''@/$(HAVE_SYS_BITYPES_H)/g' \
	    -e 's/@''HAVE_LONG_LONG_INT''@/$(HAVE_LONG_LONG_INT)/g' \
	    -e 's/@''BITSIZEOF_PTRDIFF_T''@/$(BITSIZEOF_PTRDIFF_T)/g' \
	    -e 's/@''PTRDIFF_T_SUFFIX''@/$(PTRDIFF_T_SUFFIX)/g' \
	    -e 's/@''BITSIZEOF_SIG_ATOMIC_T''@/$(BITSIZEOF_SIG_ATOMIC_T)/g' \
	    -e 's/@''HAVE_SIGNED_SIG_ATOMIC_T''@/$(HAVE_SIGNED_SIG_ATOMIC_T)/g' \
	    -e 's/@''SIG_ATOMIC_T_SUFFIX''@/$(SIG_ATOMIC_T_SUFFIX)/g' \
	    -e 's/@''BITSIZEOF_SIZE_T''@/$(BITSIZEOF_SIZE_T)/g' \
	    -e 's/@''SIZE_T_SUFFIX''@/$(SIZE_T_SUFFIX)/g' \
	    -e 's/@''BITSIZEOF_WCHAR_T''@/$(BITSIZEOF_WCHAR_T)/g' \
	    -e 's/@''HAVE_SIGNED_WCHAR_T''@/$(HAVE_SIGNED_WCHAR_T)/g' \
	    -e 's/@''WCHAR_T_SUFFIX''@/$(WCHAR_T_SUFFIX)/g' \
	    -e 's/@''BITSIZEOF_WINT_T''@/$(BITSIZEOF_WINT_T)/g' \
	    -e 's/@''HAVE_SIGNED_WINT_T''@/$(HAVE_SIGNED_WINT_T)/g' \
	    -e 's/@''WINT_T_SUFFIX''@/$(WINT_T_SUFFIX)/g' \
	    < $(srcdir)/stdint_.h > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += stdint.h stdint.h-t

lib_SOURCES += strcase.h


lib_SOURCES += strnlen1.h strnlen1.c

EXTRA_DIST += strverscmp.h

BUILT_SOURCES += $(SYS_STAT_H)
EXTRA_DIST += stat_.h

# We need the following in order to create <sys/stat.h> when the system
# has one that is incomplete.
sys/stat.h: stat_.h
	test -d sys || mkdir sys
	rm -f $@-t $@
	sed -e 's|@''ABSOLUTE_SYS_STAT_H''@|$(ABSOLUTE_SYS_STAT_H)|g' \
	    < $(srcdir)/stat_.h > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += sys/stat.h sys/stat.h-t
MOSTLYCLEANDIRS += sys

BUILT_SOURCES += $(UNISTD_H)

# We need the following in order to create an empty placeholder for
# <unistd.h> when the system doesn't have one.
unistd.h:
	echo '/* Empty placeholder for $@.  */' >$@
MOSTLYCLEANFILES += unistd.h

EXTRA_DIST += unlocked-io.h

lib_SOURCES += verify.h

lib_SOURCES += wcwidth.h

EXTRA_DIST += xalloc.h

lib_SOURCES += xalloc-die.c

