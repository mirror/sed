# Copyright (C) 2016-2017 Free Software Foundation, Inc.

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

CLEANFILES += tmp* core *.core $(EXTRA_PROGRAMS) *.*out *.log eval.in2

TEST_EXTENSIONS = .sh
SH_LOG_COMPILER = $(SHELL)

# Put new, init.sh-using tests here, so that each name
# is listed in only one place.

T =					\
  testsuite/cmd-l.sh			\
  testsuite/cmd-R.sh			\
  testsuite/colon-with-no-label.sh	\
  testsuite/comment-n.sh		\
  testsuite/compile-errors.sh		\
  testsuite/compile-tests.sh		\
  testsuite/convert-number.sh		\
  testsuite/execute-tests.sh		\
  testsuite/help-version.sh		\
  testsuite/in-place-hyphen.sh		\
  testsuite/in-place-suffix-backup.sh	\
  testsuite/invalid-mb-seq-UMR.sh	\
  testsuite/mb-bad-delim.sh		\
  testsuite/mb-charclass-non-utf8.sh	\
  testsuite/mb-match-slash.sh		\
  testsuite/mb-y-translate.sh		\
  testsuite/newline-dfa-bug.sh		\
  testsuite/normalize-text.sh		\
  testsuite/nulldata.sh			\
  testsuite/panic-tests.sh		\
  testsuite/posix-char-class.sh		\
  testsuite/posix-mode-addr.sh		\
  testsuite/posix-mode-bad-ref.sh	\
  testsuite/posix-mode-s.sh		\
  testsuite/posix-mode-N.sh		\
  testsuite/range-overlap.sh		\
  testsuite/recursive-escape-c.sh	\
  testsuite/regex-errors.sh		\
  testsuite/sandbox.sh			\
  testsuite/stdin-prog.sh		\
  testsuite/subst-options.sh		\
  testsuite/subst-mb-incomplete.sh	\
  testsuite/subst-replacement.sh	\
  testsuite/temp-file-cleanup.sh	\
  testsuite/title-case.sh		\
  testsuite/unbuffered.sh

if TEST_SYMLINKS
T += testsuite/follow-symlinks.sh		\
     testsuite/follow-symlinks-stdin.sh
endif



TESTS = $(check_PROGRAMS) $(SEDTESTS) $(T)

SEDTESTS =

noinst_HEADERS += testsuite/testcases.h testsuite/ptestcases.h

check_PROGRAMS = testsuite/get-mb-cur-max testsuite/test-mbrtowc
testsuite_get_mb_cur_max_LDADD = lib/libsed.a $(INTLLIBS)
testsuite_test_mbrtowc_LDADD = lib/libsed.a $(INTLLIBS)

if TEST_REGEX
check_PROGRAMS += testsuite/bug-regex7 \
  testsuite/bug-regex8 testsuite/bug-regex9 testsuite/bug-regex10 \
  testsuite/bug-regex11 testsuite/bug-regex12 testsuite/bug-regex13 \
  testsuite/bug-regex14 testsuite/bug-regex15 testsuite/bug-regex16 \
  testsuite/bug-regex21 testsuite/bug-regex27 testsuite/bug-regex28 \
  testsuite/tst-pcre testsuite/tst-boost testsuite/runtests \
  testsuite/runptests testsuite/tst-rxspencer testsuite/tst-regex2

SEDTESTS += space
endif

SEDTESTS += testsuite/appquit testsuite/enable testsuite/sep		\
        testsuite/inclib testsuite/8bit testsuite/newjis		\
        testsuite/xabcx testsuite/dollar testsuite/noeol		\
        testsuite/noeolw testsuite/modulo testsuite/numsub		\
        testsuite/numsub2 testsuite/numsub3 testsuite/numsub4		\
        testsuite/numsub5 testsuite/0range testsuite/bkslashes		\
        testsuite/head testsuite/madding testsuite/mac-mf		\
        testsuite/empty testsuite/xbxcx testsuite/xbxcx3		\
        testsuite/recall testsuite/recall2 testsuite/xemacs		\
        testsuite/fasts testsuite/uniq testsuite/manis			\
        testsuite/khadafy testsuite/linecnt testsuite/eval		\
        testsuite/distrib testsuite/8to7 testsuite/y-bracket		\
        testsuite/y-newline testsuite/y-zero testsuite/allsub		\
        testsuite/cv-vars testsuite/classes testsuite/middle		\
        testsuite/bsd testsuite/stdin testsuite/flipcase		\
        testsuite/insens testsuite/subwrite testsuite/writeout		\
        testsuite/readin testsuite/insert testsuite/utf8-1		\
        testsuite/utf8-2 testsuite/utf8-3 testsuite/utf8-4		\
        testsuite/badenc testsuite/inplace-hold testsuite/brackets	\
        testsuite/amp-escape testsuite/help testsuite/file		\
        testsuite/quiet testsuite/factor testsuite/binary3		\
        testsuite/binary2 testsuite/binary testsuite/dc			\
        testsuite/newline-anchor testsuite/zero-anchor

# Note that the first lines are statements.  They ensure that environment
# variables that can perturb tests are unset or set to expected values.
# The rest are envvar settings that propagate build-related Makefile
# variables to test scripts.
TESTS_ENVIRONMENT =				\
  tmp__=$${TMPDIR-/tmp};			\
  test -d "$$tmp__" && test -w "$$tmp__" || tmp__=.;	\
  . $(srcdir)/testsuite/envvar-check;		\
  TMPDIR=$$tmp__; export TMPDIR;		\
						\
  if test -n "$$BASH_VERSION" || (eval "export v=x") 2>/dev/null; then \
    export_with_values () { export "$$@"; };		\
  else							\
    export_with_values ()				\
    {							\
      sed_extract_var='s/=.*//';			\
      sed_quote_value="s/'/'\\\\''/g;s/=\\(.*\\)/='\\1'/";\
      for arg in "$$@"; do				\
        var=`echo "$$arg" | sed "$$sed_extract_var"`;	\
        arg=`echo "$$arg" | sed "$$sed_quote_value"`;	\
        eval "$$arg";					\
        export "$$var";					\
      done;						\
    };							\
  fi;							\
						\
  export_with_values				\
  VERSION='$(VERSION)'				\
  LOCALE_FR='$(LOCALE_FR)'			\
  LOCALE_FR_UTF8='$(LOCALE_FR_UTF8)'		\
  LOCALE_JA='$(LOCALE_JA)'			\
  AWK=$(AWK)					\
  LC_ALL=C					\
  abs_top_builddir='$(abs_top_builddir)'	\
  abs_top_srcdir='$(abs_top_srcdir)'		\
  abs_srcdir='$(abs_srcdir)'			\
  built_programs=sed;				\
  srcdir='$(srcdir)'				\
  top_srcdir='$(top_srcdir)'			\
  CC='$(CC)'					\
  SED_TEST_NAME=`echo $$tst|sed 's,^\./,,;s,/,-,g'` \
  MAKE=$(MAKE)					\
  MALLOC_PERTURB_=$(MALLOC_PERTURB_)		\
  PACKAGE_BUGREPORT='$(PACKAGE_BUGREPORT)'	\
  PACKAGE_VERSION=$(PACKAGE_VERSION)		\
  PERL='$(PERL)'				\
  SHELL='$(SHELL)'				\
  PATH='$(abs_top_builddir)/src$(PATH_SEPARATOR)'"$$PATH" \
  $(LOCALCHARSET_TESTS_ENVIRONMENT)		\
  ; 9>&2

LOG_COMPILER = $(top_srcdir)/testsuite/runtest


EXTRA_DIST += \
	$(T) \
	testsuite/init.sh init.cfg \
	testsuite/envvar-check \
	testsuite/PCRE.tests testsuite/BOOST.tests testsuite/SPENCER.tests \
	testsuite/runtest testsuite/Makefile.tests \
	testsuite/0range.good \
	testsuite/0range.inp \
	testsuite/0range.sed \
	testsuite/8bit.good \
	testsuite/8bit.inp \
	testsuite/8bit.sed \
	testsuite/8to7.good \
	testsuite/8to7.inp \
	testsuite/8to7.sed \
	testsuite/allsub.good \
	testsuite/allsub.inp \
	testsuite/allsub.sed \
	testsuite/amp-escape.good \
	testsuite/amp-escape.inp \
	testsuite/amp-escape.sed \
	testsuite/appquit.good \
	testsuite/appquit.inp \
	testsuite/appquit.sed \
	testsuite/binary.good \
	testsuite/binary.inp \
	testsuite/binary.sed \
	testsuite/binary2.sed \
	testsuite/binary3.sed \
	testsuite/bkslashes.good \
	testsuite/bkslashes.inp \
	testsuite/bkslashes.sed \
	testsuite/brackets.good \
	testsuite/brackets.inp \
	testsuite/brackets.sed \
	testsuite/bsd.good \
	testsuite/bsd.sh \
	testsuite/cv-vars.good \
	testsuite/cv-vars.inp \
	testsuite/cv-vars.sed \
	testsuite/classes.good \
	testsuite/classes.inp \
	testsuite/classes.sed \
	testsuite/dc.good \
	testsuite/dc.inp \
	testsuite/dc.sed \
	testsuite/distrib.good \
	testsuite/distrib.inp \
	testsuite/distrib.sed \
	testsuite/distrib.sh \
	testsuite/dollar.good \
	testsuite/dollar.inp \
	testsuite/dollar.sed \
	testsuite/empty.good \
	testsuite/empty.inp \
	testsuite/empty.sed \
	testsuite/enable.good \
	testsuite/enable.inp \
	testsuite/enable.sed \
	testsuite/eval.good \
	testsuite/eval.inp \
	testsuite/eval.sed \
	testsuite/factor.good \
	testsuite/factor.inp \
	testsuite/factor.sed \
	testsuite/fasts.good \
	testsuite/fasts.inp \
	testsuite/fasts.sed \
	testsuite/flipcase.good \
	testsuite/flipcase.inp \
	testsuite/flipcase.sed \
	testsuite/head.good \
	testsuite/head.inp \
	testsuite/head.sed \
	testsuite/inclib.good \
	testsuite/inclib.inp \
	testsuite/inclib.sed \
	testsuite/insens.good \
	testsuite/insens.inp \
	testsuite/insens.sed \
	testsuite/insert.good \
	testsuite/insert.inp \
	testsuite/insert.sed \
	testsuite/khadafy.good \
	testsuite/khadafy.inp \
	testsuite/khadafy.sed \
	testsuite/linecnt.good \
	testsuite/linecnt.inp \
	testsuite/linecnt.sed \
	testsuite/space.good \
	testsuite/space.inp \
	testsuite/space.sed \
	testsuite/mac-mf.good \
	testsuite/mac-mf.inp \
	testsuite/mac-mf.sed \
	testsuite/madding.good \
	testsuite/madding.inp \
	testsuite/madding.sed \
	testsuite/manis.good \
	testsuite/manis.inp \
	testsuite/manis.sed \
	testsuite/middle.good \
	testsuite/middle.sed \
	testsuite/middle.inp \
	testsuite/modulo.good \
	testsuite/modulo.sed \
	testsuite/modulo.inp \
	testsuite/newjis.good \
	testsuite/newjis.inp \
	testsuite/newjis.sed \
	testsuite/newline-anchor.good \
	testsuite/newline-anchor.inp \
	testsuite/newline-anchor.sed \
	testsuite/noeol.good \
	testsuite/noeol.inp \
	testsuite/noeol.sed \
	testsuite/noeolw.good \
	testsuite/noeolw.1good \
	testsuite/noeolw.2good \
	testsuite/noeolw.sed \
	testsuite/numsub.good \
	testsuite/numsub.inp \
	testsuite/numsub.sed \
	testsuite/numsub2.good \
	testsuite/numsub2.inp \
	testsuite/numsub2.sed \
	testsuite/numsub3.good \
	testsuite/numsub3.inp \
	testsuite/numsub3.sed \
	testsuite/numsub4.good \
	testsuite/numsub4.inp \
	testsuite/numsub4.sed \
	testsuite/numsub5.good \
	testsuite/numsub5.inp \
	testsuite/numsub5.sed \
	testsuite/readin.good \
	testsuite/readin.inp \
	testsuite/readin.sed \
	testsuite/recall.good \
	testsuite/recall.inp \
	testsuite/recall.sed \
	testsuite/recall2.good \
	testsuite/recall2.inp \
	testsuite/recall2.sed \
	testsuite/sep.good \
	testsuite/sep.inp \
	testsuite/sep.sed \
	testsuite/subwrite.inp \
	testsuite/subwrite.sed \
	testsuite/subwrt1.good \
	testsuite/subwrt2.good \
	testsuite/uniq.good \
	testsuite/uniq.inp \
	testsuite/uniq.sed \
	testsuite/utf8-1.good \
	testsuite/utf8-1.inp \
	testsuite/utf8-1.sed \
	testsuite/utf8-2.good \
	testsuite/utf8-2.inp \
	testsuite/utf8-2.sed \
	testsuite/utf8-3.good \
	testsuite/utf8-3.inp \
	testsuite/utf8-3.sed \
	testsuite/utf8-4.good \
	testsuite/utf8-4.inp \
	testsuite/utf8-4.sed \
	testsuite/badenc.good \
	testsuite/badenc.inp \
	testsuite/badenc.sed \
	testsuite/writeout.inp \
	testsuite/writeout.sed \
	testsuite/wrtout1.good \
	testsuite/wrtout2.good \
	testsuite/xabcx.good \
	testsuite/xabcx.inp \
	testsuite/xabcx.sed \
	testsuite/xbxcx.good \
	testsuite/xbxcx.inp \
	testsuite/xbxcx.sed \
	testsuite/xbxcx3.good \
	testsuite/xbxcx3.inp \
	testsuite/xbxcx3.sed \
	testsuite/xemacs.good \
	testsuite/xemacs.inp \
	testsuite/xemacs.sed \
	testsuite/y-bracket.good \
	testsuite/y-bracket.sed \
	testsuite/y-bracket.inp \
	testsuite/y-zero.good \
	testsuite/y-zero.sed \
	testsuite/y-zero.inp \
	testsuite/y-newline.good \
	testsuite/y-newline.sed \
	testsuite/y-newline.inp \
	testsuite/zero-anchor.good \
	testsuite/zero-anchor.sed \
	testsuite/zero-anchor.inp

# automake makes `check' depend on $(TESTS).  Declare
# dummy targets for $(TESTS) so that make does not complain.

.PHONY: $(SEDTESTS)
$(SEDTESTS):
