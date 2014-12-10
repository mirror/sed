# Copyright (C) 1990-2014 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

PACKAGE = sed

SUBDIRS = lib po sed doc testsuite gnulib-tests

noinst_DATA =
noinst_HEADERS = basicdefs.h

EXTRA_DIST = BUGS THANKS.in COPYING.DOC \
  .mailmap ChangeLog-2014 po/ChangeLog-2014 \
  bootstrap bootstrap.conf build-aux/help2man \
  thanks-gen

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

# Sort in traditional ASCII order, regardless of the current locale;
# otherwise we may get into trouble with distinct strings that the
# current locale considers to be equal.
ASSORT = LC_ALL=C sort

# Extract all lines up to the first one starting with "##".
prologue = perl -ne '/^\#\#/ and exit; print' $(srcdir)/THANKS.in

THANKS: THANKS.in Makefile.am .mailmap thanks-gen
	$(AM_V_GEN)rm -f $@-t $@;					\
	{								\
	  $(prologue); echo;						\
	  { perl -ne '/^$$/.../^$$/ and print' $(srcdir)/THANKS.in	\
	      | grep -v '^$$' | perl -pe 's/  +/\0/';			\
	    git log --pretty=format:'%aN%x00%aE'			\
	      | $(ASSORT) -u;						\
	  } | $(srcdir)/thanks-gen					\
	    | LC_ALL=en_US.UTF-8 sort -f;				\
	  echo;								\
	  printf ';; %s\n' 'Local Variables:' 'coding: utf-8' End:;	\
	} > $@-t && chmod a-w $@-t && mv $@-t $@

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
