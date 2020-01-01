# Copyright (C) 2016-2020 Free Software Foundation, Inc.

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

info_TEXINFOS = doc/sed.texi
doc_sed_TEXINFOS = doc/config.texi doc/version.texi doc/fdl.texi
dist_man_MANS = doc/sed.1
dist_noinst_DATA = doc/sed.x doc/sed-dummy.1
HELP2MAN = $(top_srcdir)/build-aux/help2man
SEDBIN = sed/sed

AM_MAKEINFOHTMLFLAGS = --no-split

if BUILD_MAN_PAGE
doc/sed.1: sed/sed$(EXEEXT) .version $(srcdir)/doc/sed.x
	$(AM_V_GEN)$(MKDIR_P) doc
	$(AM_V_at)rm -rf $@ $@-t
	$(AM_V_at)$(HELP2MAN)						\
	    --name 'stream editor for filtering and transforming text'	\
	    -p sed --include $(srcdir)/doc/sed.x			\
	    -o $@-t $(SEDBIN)						\
	  && chmod a-w $@-t						\
	  && mv $@-t $@
else !BUILD_MAN_PAGE

if BUILD_DUMMY_MAN_PAGE
doc/sed.1: doc/sed-dummy.1
	$(AM_V_at)$(SED) 's/VERSION/$(PACKAGE_VERSION)/' $< > $@-t	\
	  && chmod a-w $@-t						\
	  && mv $@-t $@
endif BUILD_DUMMY_MAN_PAGE

endif !BUILD_MAN_PAGE
