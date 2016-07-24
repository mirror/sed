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
SEDBIN = $(top_builddir)/sed/sed

AM_MAKEINFOHTMLFLAGS = --no-split

# To produce better quality output, in the example sed
# scripts we group comments with lines following them;
# since mantaining the "@group...@end group" manually
# is a burden, we do this automatically
$(top_srcdir)/doc/sed.texi: $(top_srcdir)/doc/s-texi
$(top_srcdir)/doc/s-texi: doc/sed-in.texi $(top_srcdir)/doc/groupify.sed
	sed -nf $(top_srcdir)/doc/groupify.sed \
	  < $(top_srcdir)/doc/sed-in.texi > $(top_srcdir)/doc/sed-tmp.texi
	if cmp $(top_srcdir)/doc/sed.texi $(top_srcdir)/doc/sed-tmp.texi; then \
	  rm -f $(top_srcdir)/doc/sed-tmp.texi; \
	else \
	  mv -f $(top_srcdir)/doc/sed-tmp.texi $(top_srcdir)/doc/sed.texi; \
	fi
	echo stamp > $(top_srcdir)/doc/s-texi

doc/sed.1: $(top_srcdir)/sed/sed.c $(top_srcdir)/configure.ac \
           $(top_srcdir)/doc/sed.x
	$(HELP2MAN) --name "stream editor for filtering and transforming text" \
	  -p sed --include $(top_srcdir)/doc/sed.x \
	  -o $(top_srcdir)/doc/sed.1 $(SEDBIN)

dist-hook-man-page:
	touch $(distdir)/doc/sed.1
