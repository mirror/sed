# Customize maint.mk                           -*- makefile -*-
# Copyright (C) 2009-2018 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# When running a 'check' target, adjust the $PATH to use the newly built
# sed binary - expanding coverage testing, and enabling sophisticated
# rules in syntax-check.
_is-check-target ?= $(filter-out install%, \
                         $(filter %check check%,$(MAKECMDGOALS)))
ifneq (,$(_is-check-target))
  export PATH := $(srcdir)/sed:${PATH}
endif

# Used in maint.mk's web-manual rule
manual_title = GNU Sed: a stream editor

# Use the direct link.  This is guaranteed to work immediately, while
# it can take a while for the faster mirror links to become usable.
url_dir_list = http://ftp.gnu.org/gnu/$(PACKAGE)

# Tests not to run as part of "make distcheck".
local-checks-to-skip =			\
  sc_GPL_version			\
  sc_bindtextdomain			\
  sc_error_message_uppercase		\
  sc_preprocessor_indentation		\
  sc_prohibit_atoi_atof			\
  sc_prohibit_magic_number_exit		\
  sc_prohibit_strcmp			\
  sc_unmarked_diagnostics		\
  sc_useless_cpp_parens

# Tools used to bootstrap this package, used for "announcement".
bootstrap-tools = autoconf,automake,gnulib

# Override the default Cc: used in generating an announcement.
announcement_Cc_ = $(translation_project_), $(PACKAGE)-devel@gnu.org

# Now that we have better tests, make this the default.
export VERBOSE = yes

# Comparing tarball sizes compressed using different xz presets, we see
# that -6e adds only 60 bytes to the size of the tarball, yet reduces
# (from -9) the decompression memory requirement from 64 MiB to 9 MiB.
# Don't be tempted by -5e, since -6 and -5 use the same dictionary size.
# $ for i in {4,5,6,7,8,9}{e,}; do
#   (n=$(xz -$i < sed-4.2.2.tar |wc -c);echo $n $i) & done |sort -nr
# 900032 4
# 854932 5
# 844572 4e
# 843780 9
# 843780 8
# 843780 7
# 843780 6
# 837892 5e
# 836832 9e
# 836832 8e
# 836832 7e
# 836832 6e
export XZ_OPT = -6e

old_NEWS_hash = 3f590fab578ab7756a49f98c25767fe4

# Many m4 macros names once began with 'jm_'.
# Make sure that none are inadvertently reintroduced.
sc_prohibit_jm_in_m4:
	@grep -nE 'jm_[A-Z]'						\
		$$($(VC_LIST) m4 |grep '\.m4$$'; echo /dev/null) &&	\
	    { echo '$(ME): do not use jm_ in m4 macro names'		\
	      1>&2; exit 1; } || :

sc_prohibit_echo_minus_en:
	@prohibit='\<echo -[en]'					\
	halt='do not use echo ''-e or echo ''-n; use printf instead'	\
	  $(_sc_search_regexp)

# Look for lines longer than 80 characters, except omit:
# - program-generated long lines in diff headers,
# - the help2man script copied from upstream,
# - tests involving long checksum lines, and
# - the 'pr' test cases.
LINE_LEN_MAX = 80
FILTER_LONG_LINES =						\
  /^[^:]*\.diff:[^:]*:@@ / d;					\
  \|^[^:]*man/help2man:| d;			\
  \|^[^:]*tests/misc/sha[0-9]*sum.*\.pl[-:]| d;			\
  \|^[^:]*tests/pr/|{ \|^[^:]*tests/pr/pr-tests:| !d; };
sc_long_lines:
	@files=$$($(VC_LIST_EXCEPT))					\
	halt='line(s) with more than $(LINE_LEN_MAX) characters; reindent'; \
	for file in $$files; do						\
	  expand $$file | grep -nE '^.{$(LINE_LEN_MAX)}.' |		\
	  sed -e "s|^|$$file:|" -e '$(FILTER_LONG_LINES)';		\
	done | grep . && { msg="$$halt" $(_sc_say_and_exit) } || :

# Indent only with spaces.
sc_prohibit_tab_based_indentation:
	@prohibit='^ *	'						\
	halt='TAB in indentation; use only spaces'			\
	  $(_sc_search_regexp)

# Don't use "indent-tabs-mode: nil" anymore.  No longer needed.
sc_prohibit_emacs__indent_tabs_mode__setting:
	@prohibit='^( *[*#] *)?indent-tabs-mode:'			\
	halt='use of emacs indent-tabs-mode: setting'			\
	  $(_sc_search_regexp)

# Enforce recommended preprocessor indentation style.
sc_preprocessor_indentation:
	@if cppi --version >/dev/null 2>&1; then			\
	  $(VC_LIST_EXCEPT) | grep '\.[ch]$$' | xargs cppi -a -c	\
	    || { echo '$(ME): incorrect preprocessor indentation' 1>&2;	\
		exit 1; };						\
	else								\
	  echo '$(ME): skipping test $@: cppi not installed' 1>&2;	\
	fi

# THANKS.in is a list of name/email pairs for people who are mentioned in
# commit logs (and generated ChangeLog), but who are not also listed as an
# author of a commit.  Name/email pairs of commit authors are automatically
# extracted from the repository.  As a very minor factorization, when
# someone who was initially listed only in THANKS.in later authors a commit,
# this rule detects that their pair may now be removed from THANKS.in.
sc_THANKS_in_duplicates:
	@{ git log --pretty=format:%aN | sort -u;			\
	    cut -b-36 THANKS.in | sed '/^$$/d;s/  *$$//'; }		\
	  | sort | uniq -d | grep .					\
	    && { echo '$(ME): remove the above names from THANKS.in'	\
		  1>&2; exit 1; } || :

# Ensure the contributor list stays sorted.  However, if the system's
# en_US.UTF-8 locale data is erroneous, give a diagnostic and skip
# this test.  This affects OS X up to at least 10.11.6.
sc_THANKS_in_sorted:
	@printf '%s\n' ja j.b| LC_ALL=en_US.UTF-8 sort -c 2> /dev/null	\
	  && {								\
	    sed '/^$$/,/^$$/!d;/^$$/d' $(srcdir)/THANKS.in > $@.1 &&	\
	    LC_ALL=en_US.UTF-8 sort -f -k1,1 $@.1 > $@.2 &&		\
	    diff -u $@.1 $@.2; diff=$$?;				\
	    rm -f $@.1 $@.2;						\
	    test "$$diff" = 0						\
	      || { echo '$(ME): THANKS.in is unsorted' 1>&2; exit 1; };	\
	    }								\
	  || { echo '$(ME): this system has erroneous locale data;'	\
		    'skipping $@' 1>&2; }

###########################################################
_p0 = \([^"'/]\|"\([^\"]\|[\].\)*"\|'\([^\']\|[\].\)*'
_pre = $(_p0)\|[/][^"'/*]\|[/]"\([^\"]\|[\].\)*"\|[/]'\([^\']\|[\].\)*'\)*
_pre_anchored = ^\($(_pre)\)
_comment_and_close = [^*]\|[*][^/*]\)*[*][*]*/
# help font-lock mode: '

# A sed expression that removes ANSI C and ISO C99 comments.
# Derived from the one in GNU gettext's 'moopp' preprocessor.
_sed_remove_comments =					\
/[/][/*]/{						\
  ta;							\
  :a;							\
  s,$(_pre_anchored)//.*,\1,;				\
  te;							\
  s,$(_pre_anchored)/[*]\($(_comment_and_close),\1 ,;	\
  ta;							\
  /^$(_pre)[/][*]/{					\
    s,$(_pre_anchored)/[*].*,\1 ,;			\
    tu;							\
    :u;							\
    n;							\
    s,^\($(_comment_and_close),,;			\
    tv;							\
    s,^.*$$,,;						\
    bu;							\
    :v;							\
  };							\
  :e;							\
}
# Quote all single quotes.
_sed_rm_comments_q = $(subst ','\'',$(_sed_remove_comments))
# help font-lock mode: '

_space_before_paren_exempt =? \\n\\$$
_space_before_paren_exempt = \
  (^ *\#|\\n\\$$|%s\(to %s|(date|group|character)\(s\))
# Ensure that there is a space before each open parenthesis in C code.
sc_space_before_open_paren:
	@if $(VC_LIST_EXCEPT) | grep -l '\.[ch]$$' > /dev/null; then	\
	  fail=0;							\
	  for c in $$($(VC_LIST_EXCEPT) | grep '\.[ch]$$'); do		\
	    sed '$(_sed_rm_comments_q)' $$c 2>/dev/null			\
	      | grep -i '[[:alnum:]]('					\
	      | grep -vE '$(_space_before_paren_exempt)'		\
	      | grep . && { fail=1; echo "*** $$c"; };			\
	  done;								\
	  test $$fail = 1 &&						\
	    { echo '$(ME): the above files lack a space-before-open-paren' \
		1>&2; exit 1; } || :;					\
	else :;								\
	fi
###########################################################

sc_prohibit-form-feed:
	@prohibit=$$'\f' \
	in_vc_files='\.[chly]$$' \
	halt='Form Feed (^L) detected' \
	  $(_sc_search_regexp)


# Ensure gnulib generated files are ignored
# TODO: Perhaps augment gnulib-tool to do this in lib/.gitignore?
sc_gitignore_missing:
	@{ sed -n '/^\/lib\/.*\.h$$/{p;p}' $(srcdir)/.gitignore;	\
	    find lib -name '*.in*' ! -name '*~' ! -name 'sys_*' |	\
	      sed 's|^|/|; s|_\(.*in\.h\)|/\1|; s/\.in//'; } |		\
	      sort | uniq -u | grep . && { echo '$(ME): Add above'	\
		'entries to .gitignore' >&2; exit 1; } || :


# Similar to the gnulib maint.mk rule for sc_prohibit_strcmp
# Use STREQ_LEN or STRPREFIX rather than comparing strncmp == 0, or != 0.
sc_prohibit_strncmp:
	@prohibit='^[^#].*str''ncmp *\('				\
	halt='use STREQ_LEN or STRPREFIX instead of str''ncmp'		\
	  $(_sc_search_regexp)

# Ensure that tests don't include a redundant fail=0.
sc_prohibit_fail_0:
	@prohibit='\<fail=0\>'						\
	halt='fail=0 initialization'					\
	  $(_sc_search_regexp)

# Ensure that tests don't use `cmd ... && fail=1` as that hides crashes.
# The "exclude" expression allows common idioms like `test ... && fail=1`
# and the 2>... portion allows commands that redirect stderr and so probably
# independently check its contents and thus detect any crash messages.
sc_prohibit_and_fail_1:
	@prohibit='&& fail=1'						\
	exclude='(returns_|stat|kill|test |EGREP|grep|compare|2> *[^/])' \
	halt='&& fail=1 detected. Please use: returns_ 1 ... || fail=1'	\
	in_vc_files='^tests/'						\
	  $(_sc_search_regexp)

# Ensure that env vars are not passed through returns_ as
# that was seen to fail on FreeBSD /bin/sh at least
sc_prohibit_env_returns:
	@prohibit='=[^ ]* returns_ '					\
	exclude='_ returns_ '						\
	halt='Passing env vars to returns_ is non portable'		\
	in_vc_files='^tests/'						\
	  $(_sc_search_regexp)

sc_prohibit_perl_hash_quotes:
	@prohibit="\{'[A-Z_]+' *[=}]"					\
	halt="in Perl code, write \$$hash{KEY}, not \$$hash{'K''EY'}"	\
	  $(_sc_search_regexp)

# Use print_ver_ (from init.cfg), not open-coded $VERBOSE check.
sc_prohibit_verbose_version:
	@prohibit='test "\$$VERBOSE" = yes && .* --version'		\
	halt='use the print_ver_ function instead...'			\
	  $(_sc_search_regexp)


# Use framework_failure_, not the old name without the trailing underscore.
sc_prohibit_framework_failure:
	@prohibit='\<framework_''failure\>'				\
	halt='use framework_failure_ instead'				\
	  $(_sc_search_regexp)

# Prohibit the use of `...` in tests/.  Use $(...) instead.
sc_prohibit_test_backticks:
	@prohibit='`' in_vc_files='^tests/'				\
	halt='use $$(...), not `...` in tests/'				\
	  $(_sc_search_regexp)

# Ensure that compare is used to check empty files
# so that the unexpected contents are displayed
sc_prohibit_test_empty:
	@prohibit='test -s.*&&' in_vc_files='^tests/'			\
	halt='use `compare /dev/null ...`, not `test -s ...` in tests/'	\
	  $(_sc_search_regexp)

# With split lines, don't leave an operator at end of line.
# Instead, put it on the following line, where it is more apparent.
# Don't bother checking for "*" at end of line, since it provokes
# far too many false positives, matching constructs like "TYPE *".
# Similarly, omit "=" (initializers).
binop_re_ ?= [-/+^!<>]|[-/+*^!<>=]=|&&?|\|\|?|<<=?|>>=?
sc_prohibit_operator_at_end_of_line:
	@prohibit='. ($(binop_re_))$$'					\
	in_vc_files='\.[chly]$$'					\
	halt='found operator at end of line'				\
	  $(_sc_search_regexp)

update-copyright-env = \
  UPDATE_COPYRIGHT_USE_INTERVALS=2 \
  UPDATE_COPYRIGHT_MAX_LINE_LENGTH=79

config_h_header ?= (<config\.h>|"sed\.h")

exclude_file_name_regexp--sc_long_lines = ^tests/.*$$
exclude_file_name_regexp--sc_prohibit_doubled_word = \
  ^testsuite/(mac-mf|uniq)\.(good|inp)$$

exclude_file_name_regexp--sc_program_name = ^testsuite/.*\.c$$

exclude_file_name_regexp--sc_space_tab = ^testsuite/.*$$
exclude_file_name_regexp--sc_prohibit_always_true_header_tests = \
  ^configure\.ac$$

tbi_1 = (^testsuite/.*|^gl/lib/reg.*\.c\.diff|\.mk|/help2man)$$
tbi_2 = (GNU)?[Mm]akefile(\.am)?$$
exclude_file_name_regexp--sc_prohibit_tab_based_indentation = \
  $(tbi_1)|$(tbi_2)

exclude_file_name_regexp--sc_prohibit_empty_lines_at_EOF = \
  ^testsuite/(bkslashes.good|(noeolw?|empty|zero-anchor)\.(2?good|inp))$$

# Exempt test-related files from our 80-column limitation, for now.
exclude_file_name_regexp--sc_long_lines = ^testsuite/

# Exempt test-related files from space-before-parens requirements.
exclude_file_name_regexp--sc_space_before_open_paren = ^testsuite/

# Exempt makefiles from 'fail=0' detection (it is only relevant to shell
# script tests).
exclude_file_name_regexp--sc_prohibit_fail_0 = cfg\.mk


# static analysis
STAN_CFLAGS ?= "-g -O0"

.PHONY: static-analysis-init
static-analysis-init:
	type scan-build 1>/dev/null 2>&1 || \
	    { echo "scan-build program not found" >&2; exit 1; }
	$(MAKE) clean

.PHONY: static-analysis-config
static-analysis-config:
	test -x ./configure || \
	    { echo "./configure script not found" >&2; exit 1; }
	scan-build ./configure CFLAGS=$(STAN_CFLAGS)

.PHONY: static-analysis-make
static-analysis-make:
	scan-build $(MAKE)

.PHONY: static-analysis
static-analysis: static-analysis-init static-analysis-config \
                 static-analysis-make

ASAN_FLAGS=-fsanitize=address -fno-omit-frame-pointer
ASAN_CFLAGS=-O0 -g -Dlint $(ASAN_FLAGS)
ASAN_LDFLAGS=$(ASAN_FLAGS)

.PHONY: build-asan
build-asan:
	test -x ./configure || \
	    { echo "./configure script not found" >&2; exit 1; }
	./configure CFLAGS="$(ASAN_CFLAGS)" LDFLAGS="$(ASAN_LDFLAGS)"
	make

# NOTE: These options require a recent gcc (tested with gcc 8.2)
UBSAN_FLAGS=-fsanitize=undefined \
	-fsanitize=signed-integer-overflow \
	-fsanitize=bounds-strict \
	-fsanitize=shift \
	-fsanitize=shift-exponent \
	-fsanitize=shift-base \
	-fsanitize=integer-divide-by-zero \
	-fsanitize=null \
	-fsanitize=return \
	-fsanitize=alignment \
	-fsanitize=object-size \
	-fsanitize=nonnull-attribute \
	-fsanitize=returns-nonnull-attribute \
	-fsanitize=bool \
	-fsanitize=enum \
	-fsanitize=pointer-overflow \
	-fsanitize=builtin
UBSAN_CFLAGS=-O0 -g -Dlint $(UBSAN_FLAGS)
UBSAN_LDFLAGS=$(UBSAN_FLAGS)

.PHONY: build-ubsan
build-ubsan:
	test -x ./configure || \
	    { echo "./configure script not found" >&2; exit 1; }
	./configure CFLAGS="$(UBSAN_CFLAGS)" LDFLAGS="$(UBSAN_LDFLAGS)"
	make
