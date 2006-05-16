## Process this file with automake to produce Makefile.in

# Automake requirements
AUTOMAKE_OPTIONS = gnits 1.8
ACLOCAL_AMFLAGS = -I config

PACKAGE = sed

SUBDIRS = intl lib pcre po sed doc testsuite

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
        make distcheck EXTRA_DC_FLAGS='--without-included-gettext'
