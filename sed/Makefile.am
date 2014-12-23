## Process this file with automake to produce Makefile.in
bin_PROGRAMS = sed

localedir = $(datadir)/locale

sed_SOURCES = sed.c compile.c execute.c regexp.c mbcs.c utils.c
noinst_HEADERS = sed.h utils.h

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir) -I$(top_builddir)/lib \
	-DLOCALEDIR=\"$(localedir)\"
AM_CFLAGS = $(WARN_CFLAGS) $(WERROR_CFLAGS)

sed_LDADD = libver.a ../lib/libsed.a $(INTLLIBS) $(LIB_ACL) $(LIB_SELINUX)

sed_DEPENDENCIES = ../lib/libsed.a libver.a

$(PROGRAMS): $(LDADD)

BUILT_SOURCES =
DISTCLEANFILES =

BUILT_SOURCES += version.c
DISTCLEANFILES += version.c
version.c: Makefile
	$(AM_V_GEN)rm -f $@
	$(AM_V_at)printf '#include <config.h>\n' > $@t
	$(AM_V_at)printf 'char const *Version = "$(PACKAGE_VERSION)";\n' >> $@t
	$(AM_V_at)chmod a-w $@t
	$(AM_V_at)mv $@t $@

BUILT_SOURCES += version.h
DISTCLEANFILES += version.h
version.h: Makefile
	$(AM_V_GEN)rm -f $@
	$(AM_V_at)printf 'extern char const *Version;\n' > $@t
	$(AM_V_at)chmod a-w $@t
	$(AM_V_at)mv $@t $@

noinst_LIBRARIES = libver.a
nodist_libver_a_SOURCES = version.c version.h
