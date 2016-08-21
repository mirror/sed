# Copyright (C) 2016 Free Software Foundation, Inc.

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

info_TEXINFOS = doc/sed.texi
sed_TEXINFOS = doc/config.texi doc/version.texi
dist_man_MANS = doc/sed.1
dist_noinst_DATA = doc/config.texi doc/sed.x doc/sed-in.texi doc/s-texi
dist_noinst_SCRIPTS = doc/groupify.sed
HELP2MAN = $(top_srcdir)/build-aux/help2man
SEDBIN = sed/sed

AM_MAKEINFOHTMLFLAGS = --no-split

# To produce better quality output, in the example sed
# scripts we group comments with lines following them;
# since mantaining the "@group...@end group" manually
# is a burden, we do this automatically
doc/sed.texi: $(srcdir)/doc/s-texi
doc/s-texi: doc/sed-in.texi $(srcdir)/doc/groupify.sed
	sed -nf $(srcdir)/doc/groupify.sed \
	  < $(srcdir)/doc/sed-in.texi > $(srcdir)/doc/sed-tmp.texi
	if cmp $(srcdir)/doc/sed.texi $(srcdir)/doc/sed-tmp.texi; then \
	  rm -f $(srcdir)/doc/sed-tmp.texi; \
	else \
	  mv -f $(srcdir)/doc/sed-tmp.texi $(srcdir)/doc/sed.texi; \
	fi
	echo stamp > $(srcdir)/doc/s-texi

doc/sed.1: sed/sed .version $(srcdir)/doc/sed.x
	$(AM_V_GEN)$(MKDIR_P) doc
	$(AM_V_at)rm -rf $@ $@-t
	$(AM_V_at)$(HELP2MAN)						\
	    --name 'stream editor for filtering and transforming text'	\
	    -p sed --include $(srcdir)/doc/sed.x			\
	    -o $@-t $(SEDBIN)						\
	  && chmod a-w $@-t						\
	  && mv $@-t $@
