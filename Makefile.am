## Process this file with automake to produce Makefile.in

# Automake requirements
AUTOMAKE_OPTIONS = gnits 1.8
ACLOCAL_AMFLAGS = -I config

PACKAGE = sed

SUBDIRS = intl lib po sed doc testsuite

noinst_DATA = bootstrap.sh
noinst_HEADERS = basicdefs.h

EXTRA_DIST = BUGS THANKS COPYING.DOC README.boot bootstrap.sh 	      \
	config/texi2dvi    config/help2man

html:
	cd doc && make html

DISTCHECK_CONFIGURE_FLAGS = XGETTEXT='$(SHELL) -c : dummy' $(EXTRA_DC_FLAGS)
EXTRA_DC_FLAGS = 

full-distcheck:
	make distcheck EXTRA_DC_FLAGS='--enable-html'
	make distcheck EXTRA_DC_FLAGS='--disable-i18n'
	make distcheck EXTRA_DC_FLAGS='--disable-nls'
	make distcheck EXTRA_DC_FLAGS='--without-included-regex'
	make distcheck EXTRA_DC_FLAGS='--without-included-gettext'

update-regex:
	HOST=sources.redhat.com && \
	BASEURL="http://$$HOST/cgi-bin/cvsweb.cgi/~checkout~/libc/posix" && \
	QUERY='cvsroot=glibc&content-type=text/plain' && \
	wget -O lib/regcomp.c "$$BASEURL/regcomp.c?$$QUERY" && \
	wget -O lib/regexec.c "$$BASEURL/regexec.c?$$QUERY" && \
	wget -O lib/regex.c "$$BASEURL/regex.c?$$QUERY" && \
	wget -O lib/regex_.h "$$BASEURL/regex.h?$$QUERY" && \
	wget -O lib/regex_internal.c "$$BASEURL/regex_internal.c?$$QUERY" && \
	wget -O lib/regex_internal.h "$$BASEURL/regex_internal.h?$$QUERY" && \
	wget -O testsuite/BOOST.tests "$$BASEURL/BOOST.tests?$$QUERY" && \
	wget -O testsuite/PCRE.tests "$$BASEURL/PCRE.tests?$$QUERY" && \
	wget -O testsuite/SPENCER.tests "$$BASEURL/rxspencer/tests?$$QUERY"
