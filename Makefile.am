## Process this file with automake to produce Makefile.in

# Automake requirements
AUTOMAKE_OPTIONS = gnits 1.8
ACLOCAL_AMFLAGS = -I config
DISTCHECK_CONFIGURE_FLAGS = XGETTEXT='$(SHELL) -c : dummy'

PACKAGE = sed

SUBDIRS = intl lib pcre po sed doc testsuite

noinst_DATA = bootstrap.sh
noinst_HEADERS = basicdefs.h

EXTRA_DIST = BUGS THANKS COPYING.DOC README.boot bootstrap.sh 	      \
	config/texi2dvi    config/help2man

html:
	cd doc && make html
