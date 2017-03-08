#!/usr/bin/perl
# Test misc.

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

use strict;
use File::stat;

(my $program_name = $0) =~ s|.*/||;

# Turn off localization of executable's output.
@ENV{qw(LANGUAGE LANG LC_ALL)} = ('C') x 3;

my $prog = 'sed';

print "PATH = $ENV{PATH}\n";

my @Tests =
    (
     ['empty', qw(-e ''), {IN=>''}, {OUT=>''}],
     ['empty2', q('s/^ *//'), {IN=>"x\n\n"}, {OUT=>"x\n\n"}],

     ['head', qw(3q), {IN=>"1\n2\n3\n4\n"}, {OUT=>"1\n2\n3\n"}],
     ['space', q('s/_\S/XX/g;s/\s/_/g'),
      {IN=>  "Hello World\t!\nSecond_line_ of tests\n" },
      {OUT=> "Hello_World_!\nSecondXXine__of_tests\n" }],

     ['zero-anchor', qw(-z), q('N;N;s/^/X/g;s/^/X/mg;s/$/Y/g;s/$/Y/mg'),
      {IN=>"a\0b\0c\0" },
      {OUT=>"XXaY\0XbY\0XcYY\0" }],

     ['case-insensitive', qw(-n), q('h;s/Version: *//p;g;s/version: *//Ip'),
      {IN=>"Version: 1.2.3\n" },
      {OUT=>"1.2.3\n1.2.3\n" },
      ],

     ['preserve-missing-EOL-at-EOF', q('s/$/x/'),
      {IN=> "a\nb" },
      {OUT=>"ax\nbx" },
      ],

     ['y-bracket', q('y/[/ /'),
      {IN => "Are you sure (y/n)? [y]\n" },
      {OUT=> "Are you sure (y/n)?  y]\n" },
      ],

     ['y-zero', q('y/b/\x00/'),
      {IN => "abc\n" },
      {OUT=> "a\0c\n" },
      ],

     ['y-newline', q('H
G
y/Ss\nYy/yY$sS/'),
      {IN => "Are you sure (y/n)? [y]\n" },
      {OUT=> 'Are Sou Yure (S/n)? [S]$$Are Sou Yure (S/n)? [S]'."\n"},
      ],

     ['allsub', q('s/foo/bar/g'),
      {IN => "foo foo fo oo f oo foo foo foo foo foo foo foo foo foo\n"},
      {OUT=> "bar bar fo oo f oo bar bar bar bar bar bar bar bar bar\n"},
      ],

     ['insert-nl', qw(-f), {IN => "/foo/i\\\n"},
      {IN => "bar\nfoo\n" },
      {OUT=> "bar\n\nfoo\n" },
      ],

    );

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;
