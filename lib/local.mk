# Copyright 1997-2016 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

include lib/gnulib.mk

lib_libsed_a_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/lib/ \
                        -I$(top_builddir)/lib
lib_libsed_a_CFLAGS = $(AM_CFLAGS) $(GNULIB_WARN_CFLAGS) $(WERROR_CFLAGS)

lib_libsed_a_LIBADD += $(LIBOBJS) $(ALLOCA)
lib_libsed_a_DEPENDENCIES += $(LIBOBJS) $(ALLOCA)
