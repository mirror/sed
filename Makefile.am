## Process this file with automake to produce Makefile.in

# Automake requirements
AUTOMAKE_OPTIONS = gnits 1.10
ACLOCAL_AMFLAGS = -I m4

PACKAGE = sed

SUBDIRS = lib pcre po sed doc testsuite

noinst_DATA = bootstrap.sh
noinst_HEADERS = basicdefs.h

EXTRA_DIST = BUGS THANKS COPYING.DOC README.boot \
	autoboot build-aux/texi2dvi build-aux/help2man

html:
	cd doc && make html

DISTCHECK_CONFIGURE_FLAGS = XGETTEXT='$(SHELL) -c : dummy' $(EXTRA_DC_FLAGS)
EXTRA_DC_FLAGS =

dist-hook:
	cmp bootstrap.sh $(srcdir)/bootstrap.sh > /dev/null 2>&1 || \
	  cp -p bootstrap.sh $(srcdir)/bootstrap.sh
	cp -p $(srcdir)/bootstrap.sh $(distdir)/bootstrap.sh

full-distcheck:
        make distcheck EXTRA_DC_FLAGS='--enable-html'
        make distcheck EXTRA_DC_FLAGS='--disable-i18n'
        make distcheck EXTRA_DC_FLAGS='--disable-nls'
        make distcheck EXTRA_DC_FLAGS='--without-included-gettext'
