## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libpcre.a
noinst_HEADERS = internal.h pcre.h regexp.h
noinst_PROGRAMS = @PCREGREP@
EXTRA_PROGRAMS = pcregrep
EXTRA_DIST = AUTHORS ChangeLog ChangeLog.pre-sed LICENCE README regcomp.c

libpcre_a_SOURCES = regtables.c regsub.c regstudy.c regposix.c regperl.c \
	reginfo.c regexec.c regexp.c regdebug.c

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir)/intl -I$(top_srcdir)/pcre \
	-I$(top_srcdir) -I$(top_builddir)/intl

pcregrep_SOURCES = pcregrep.c
pcregrep_LDADD = libpcre.a

