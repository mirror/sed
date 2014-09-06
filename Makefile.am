## Process this file with automake to produce Makefile.in

# Automake requirements
AUTOMAKE_OPTIONS = gnits 1.10 dist-bzip2
ACLOCAL_AMFLAGS = -I m4

PACKAGE = sed

SUBDIRS = lib po sed doc testsuite

noinst_DATA =
noinst_HEADERS = basicdefs.h

EXTRA_DIST = BUGS THANKS COPYING.DOC \
  ChangeLog-2014 po/ChangeLog-2014 \
  bootstrap bootstrap.conf build-aux/texi2dvi build-aux/help2man

# Arrange so that .tarball-version appears only in the distribution
# tarball, and never in a checked-out repository.
dist-hook: gen-ChangeLog
	$(AM_V_GEN)echo $(VERSION) > $(distdir)/.tarball-version
	test $(COPYRIGHT_YEAR) = `date +%Y` || \
	  { echo Copyright year not updated; exit 1; }


gen_start_date = 2014-09-06
.PHONY: gen-ChangeLog
gen-ChangeLog:
	$(AM_V_GEN)if test -d .git; then				\
	  log_fix="$(srcdir)/build-aux/git-log-fix";			\
	  test -e "$$log_fix"						\
	    && amend_git_log="--amend=$$log_fix"			\
	    || amend_git_log=;						\
	  $(top_srcdir)/build-aux/gitlog-to-changelog			\
	    $$amend_git_log --since=$(gen_start_date) > $(distdir)/cl-t && \
	    { rm -f $(distdir)/ChangeLog &&				\
	      mv $(distdir)/cl-t $(distdir)/ChangeLog; }		\
	fi

html:
	cd doc && make html

DISTCHECK_CONFIGURE_FLAGS = XGETTEXT='$(SHELL) -c : dummy' $(EXTRA_DC_FLAGS)
EXTRA_DC_FLAGS =

full-distcheck:
	make distcheck EXTRA_DC_FLAGS='--enable-regex-tests'
	make distcheck EXTRA_DC_FLAGS='--enable-html'
	make distcheck EXTRA_DC_FLAGS='--disable-i18n'
	make distcheck EXTRA_DC_FLAGS='--disable-nls'
	make distcheck EXTRA_DC_FLAGS='--without-included-gettext'
	@case "$(host)" in \
	  *-linux*|*-gnu*) \
	    echo make distcheck EXTRA_DC_FLAGS=\'--without-included-regex\'; \
	    make distcheck EXTRA_DC_FLAGS='--without-included-regex' \
	    ;; \
	  *) \
	    echo Skipping check --without-included-regex \
	    ;; \
	esac

## update-regex:
##	HOST=sources.redhat.com && \
##	BASEURL="http://$$HOST/cgi-bin/cvsweb.cgi/~checkout~/libc/posix" && \
##	QUERY='cvsroot=glibc&content-type=text/plain' && \
##	wget -O lib/regcomp.c "$$BASEURL/regcomp.c?$$QUERY" && \
##	wget -O lib/regexec.c "$$BASEURL/regexec.c?$$QUERY" && \
##	wget -O lib/regex.c "$$BASEURL/regex.c?$$QUERY" && \
##	wget -O lib/regex_.h "$$BASEURL/regex.h?$$QUERY" && \
##	wget -O lib/regex_internal.c "$$BASEURL/regex_internal.c?$$QUERY" && \
##	wget -O lib/regex_internal.h "$$BASEURL/regex_internal.h?$$QUERY" && \
##	wget -O testsuite/BOOST.tests "$$BASEURL/BOOST.tests?$$QUERY" && \
##	wget -O testsuite/PCRE.tests "$$BASEURL/PCRE.tests?$$QUERY" && \
##	wget -O testsuite/SPENCER.tests "$$BASEURL/rxspencer/tests?$$QUERY"
