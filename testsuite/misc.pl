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

     ['recall',
      # Check that the empty regex recalls the last *executed* regex,
      # not the last *compiled* regex
      qw(-f), {IN => "p;s/e/X/p;:x;s//Y/p;/f/bx"},
      {IN => "eeefff\n" },
      {OUT=> "eeefff\n"
	   . "Xeefff\n"
	   . "XYefff\n"
	   . "XYeYff\n"
	   . "XYeYYf\n"
	   . "XYeYYY\n"
	   . "XYeYYY\n"
      },
      ],

     ['recall2',
      # Starting from sed 4.1.3, regexes are compiled with REG_NOSUB
      # if they are used in an address, so that the matcher does not
      # have to obey leftmost-longest.  The tricky part is to recompile
      # them if they are then used in a substitution.
      qw(-f), {IN => '/\(ab*\)\+/ s//>\1</g'},
      {IN => "ababb||abbbabbbb\n" },
      {OUT=> ">abb<||>abbbb<\n" },
      ],

     ['0range',
      # Test address 0 (GNU extension)
      # FIXME: This test does NOT actually fail if the address is changed to 1.
      qw(-e '0,/aaa/d'),
      {IN => "1\n"
           . "2\n"
           . "3\n"
           . "4\n"
           . "aaa\n"
           . "yes\n"},
      {OUT => "yes\n"}
     ],

     ['amp-escape',
      # Test ampersand as escape sequence (ASCII 0x26), which should
      # not have a special meaning (i.e. the 'matched pattern')
      qw(-e 's/yes/yes\x26/'),
      {IN => "yes\n"},
      {OUT => "yes&\n"}
     ],

     ['appquit',
      # Test 'a'ppend command before 'q'uit
      qw(-f),
      {IN => q(a\
ok
q)},
      {IN => "doh\n"},
      {OUT => "doh\n"
            . "ok\n"}
     ],

     ['bkslashes',
      # Test backslashes in regex
      # bug in sed 4.0b
      qw(-f),
      {IN => q(s/$/\\\\\
/
)},
      {IN => "a\n"},
      {OUT => "a\\\n"
            . "\n"}
     ],

     ['classes',
      # inspired by an autoconf generated configure script.
      qw(-n -f),
      {IN => 's/^\([/[:lower:]A-Z0-9]*_cv_[[:lower:][:upper:]/[:digit:]]*\)'.
             '=\(.*\)/: \${\1=\'\2\'}/p'},
      {IN => "_cv_=emptyvar\n"
           . "ac_cv_prog/RANLIB=/usr/bin/ranlib\n"
           . "ac_cv_prog/CC=/usr/unsupported/\\ \\ /lib/_cv_/cc\n"
           . "a/c_cv_prog/CPP=/usr/bin/cpp\n"
           . "SHELL=bash\n"
           . "GNU=GNU!UNIX\n"},
      {OUT => ": \${_cv_='emptyvar'}\n"
            . ": \${ac_cv_prog/RANLIB='/usr/bin/ranlib'}\n"
            . ": \${ac_cv_prog/CC='/usr/unsupported/\\ \\ /lib/_cv_/cc'}\n"
            . ": \${a/c_cv_prog/CPP='/usr/bin/cpp'}\n"}
     ],


     ['cv-vars',
      # inspired by an autoconf generated configure script.
      qw(-n -f),
      {IN => q|s/^\([a-zA-Z0-9_]*_cv_[a-zA-Z0-9_]*\)=\(.*\)/: \${\1='\2'}/p|},
      {IN => "_cv_=emptyvar\n"
           . "ac_cv_prog_RANLIB=/usr/bin/ranlib\n"
           . "ac_cv_prog_CC=/usr/unsupported/\ \ /lib/_cv_/cc\n"
           . "ac_cv_prog_CPP=/usr/bin/cpp\n"
           . "SHELL=bash\n"
           . "GNU=GNU!UNIX\n"},
      {OUT => ": \${_cv_='emptyvar'}\n"
            . ": \${ac_cv_prog_RANLIB='/usr/bin/ranlib'}\n"
            . ": \${ac_cv_prog_CC='/usr/unsupported/\ \ /lib/_cv_/cc'}\n"
            . ": \${ac_cv_prog_CPP='/usr/bin/cpp'}\n"}
     ],


     ['dollar',
      # Test replacement on the last line (address '$')
      qw(-e '$s/^/space /'),
      {IN => "I can't quite remember where I heard it,\n"
           . "but I can't seem to get out of my head\n"
           . "the phrase\n"
           . "the final frontier\n"},
      {OUT => "I can't quite remember where I heard it,\n"
            . "but I can't seem to get out of my head\n"
            . "the phrase\n"
            . "space the final frontier\n"}
     ],

     ['enable',
      # inspired by an autoconf generated configure script.
      qw(-e 's/-*enable-//;s/=.*//'),
      {IN => "--enable-targets=sparc-sun-sunos4.1.3,srec\n"
           . "--enable-x11-testing=on\n"
           . "--enable-wollybears-in-minnesota=yes-id-like-that\n"},
      {OUT => "targets\n"
            . "x11-testing\n"
            . "wollybears-in-minnesota\n"}
     ],

     ['fasts',
      # test `fast' substitutions
      qw(-f),
      {IN => q(
h
s/a//
p
g
s/a//g
p
g
s/^a//p
g
s/^a//g
p
g
s/not present//g
p
g
s/^[a-z]//g
p
g
s/a$//
p
g

y/a/b/
h
s/b//
p
g
s/b//g
p
g
s/^b//p
g
s/^b//g
p
g
s/^[a-z]//g
p
g
s/b$//
p
g
)},
      {IN => "aaaaaaabbbbbbaaaaaaa\n"},
      {OUT => "aaaaaabbbbbbaaaaaaa\n"
            . "bbbbbb\n"
            . "aaaaaabbbbbbaaaaaaa\n"
            . "aaaaaabbbbbbaaaaaaa\n"
            . "aaaaaaabbbbbbaaaaaaa\n"
            . "aaaaaabbbbbbaaaaaaa\n"
            . "aaaaaaabbbbbbaaaaaa\n"
            . "bbbbbbbbbbbbbbbbbbb\n"
            . "\n"
            . "bbbbbbbbbbbbbbbbbbb\n"
            . "bbbbbbbbbbbbbbbbbbb\n"
            . "bbbbbbbbbbbbbbbbbbb\n"
            . "bbbbbbbbbbbbbbbbbbb\n"
            . "bbbbbbbbbbbbbbbbbbbb\n"}
     ],

     ['xabcx',
      # from the ChangeLog (Fri May 21 1993)
      # Regex address with custom character (\xREGEXx)
      qw(-e '\xfeetxs/blue/too/'),
      {IN => "roses are red\n"
           . "violets are blue\n"
           . "my feet are cold\n"
           . "your feet are blue\n"},
      {OUT => "roses are red\n"
            . "violets are blue\n"
            . "my feet are cold\n"
            . "your feet are too\n"}
     ],


     ['xbxcx',
      # from the ChangeLog (Wed Sep 5 2001)
      qw(-e 's/a*/x/g'),
      {IN => "\n"
           . "b\n"
           . "bc\n"
           . "bac\n"
           . "baac\n"
           . "baaac\n"
           . "baaaac\n"},
      {OUT => "x\n"
            . "xbx\n"
            . "xbxcx\n"
            . "xbxcx\n"
            . "xbxcx\n"
            . "xbxcx\n"
            . "xbxcx\n"}
      ],

     ['xbxcx3',
      # Test s///N replacements (GNU extension)
      qw(-e 's/a*/x/3'),
      {IN => "\n"
           . "b\n"
           . "bc\n"
           . "bac\n"
           . "baac\n"
           . "baaac\n"
           . "baaaac\n"},
      {OUT => "\n"
           . "b\n"
           . "bcx\n"
           . "bacx\n"
           . "baacx\n"
           . "baaacx\n"
           . "baaaacx\n"}
     ],

    );

my $save_temps = $ENV{SAVE_TEMPS};
my $verbose = $ENV{VERBOSE};

my $fail = run_tests ($program_name, $prog, \@Tests, $save_temps, $verbose);
exit $fail;
