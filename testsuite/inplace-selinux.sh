#!/bin/sh

# Copyright (C) 2017 Free Software Foundation, Inc.

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
. "${srcdir=.}/testsuite/init.sh"; path_prepend_ ./sed
print_ver_ sed
require_selinux_

fail=0

# Create the first file and symlink pointing at it.
echo "Hello World" > inplace-selinux-file || framework_failure_
ln -s ./inplace-selinux-file inplace-selinux-link || framework_failure_

chcon -h -t user_home_t inplace-selinux-file || framework_failure_
chcon -h -t user_tmp_t inplace-selinux-link || framework_failure_


# Create the second file and symlink pointing at it.
# These will be used with the --follow-symlink option.
echo "Hello World" > inplace-selinux-file2 || framework_failure_
ln -s ./inplace-selinux-file2 inplace-selinux-link2 || framework_failure_

chcon -h -t user_home_t inplace-selinux-file2 || framework_failure_
chcon -h -t user_tmp_t inplace-selinux-link2 || framework_failure_

# Modify prepared files inplace via the symlinks
sed -i -e "s~Hello~Hi~" inplace-selinux-link || fail=1
sed -i --follow-symlinks -e "s~Hello~Hi~" inplace-selinux-link2 || fail=1

# Check selinux context - the first file should be created with the context
# of the symlink...
ls -Z inplace-selinux-link | grep ':user_tmp_t:' || fail=1
# ...the second file should use the context of the file itself.
ls -Z inplace-selinux-file2 | grep ':user_home_t:' || fail=1

Exit $fail
