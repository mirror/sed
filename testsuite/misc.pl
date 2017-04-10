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



     ['factor',
      # Compute a few common factors for speed.  Clear the subst flag
      # These are placed here to make the flow harder to understand :-)
      # The quotient of dividing by 11 is a limit to the remaining prime factors
      # Pattern space looks like CANDIDATE\nNUMBER.  When a candidate is valid,
      # the number is divided and the candidate is tried again
      # We have a prime factor in CANDIDATE! Print it
      # If NUMBER = 1, we don't have any more factors
      qw(-n -f),
      {IN => q~
s/.*/&;9aaaaaaaaa8aaaaaaaa7aaaaaaa6aaaaaa5aaaaa4aaaa3aaa2aa1a0/
:encode
s/\(a*\)\([0-9]\)\([0-9]*;.*\2\(a*\)\)/\1\1\1\1\1\1\1\1\1\1\4\3/
tencode
s/;.*//

t7a

:2
a\
2
b2a
:3
a\
3
b3a
:5
a\
5
b5a
:7
a\
7

:7a
s/^\(aa*\)\1\{6\}$/\1/
t7
:5a
s/^\(aa*\)\1\{4\}$/\1/
t5
:3a
s/^\(aa*\)\1\1$/\1/
t3
:2a
s/^\(aa*\)\1$/\1/
t2

/^a$/b

s/^\(aa*\)\1\{10\}/\1=&/

:factor
/^\(a\{7,\}\)=\1\1*$/! {
  # Decrement CANDIDATE, and search again if it is still >1
  s/^a//
  /^aa/b factor

  # Print the last remaining factor: since it is stored in the NUMBER
  # rather than in the CANDIDATE, swap 'em: now NUMBER=1
  s/\(.*\)=\(.*\)/\2=\1/
}

h
s/=.*/;;0a1aa2aaa3aaaa4aaaaa5aaaaaa6aaaaaaa7aaaaaaaa8aaaaaaaaa9/

:decode
s/^\(a*\)\1\{9\}\(a\{0,9\}\)\([0-9]*;.*[^a]\2\([0-9]\)\)/\1\4\3/
/^a/tdecode
s/;.*//p

g
:divide
s/^\(a*\)\(=b*\)\1/\1\2b/
tdivide
y/b/a/

/aa$/bfactor
~},

      {IN => "2\n"
           . "3\n"
           . "4\n"
           . "5\n"
           . "8\n"
           . "11\n"
           . "16\n"
           . "143\n"},
      {OUT => "2\n"
           . "3\n"
           . "2\n"
           . "2\n"
           . "5\n"
           . "2\n"
           . "2\n"
           . "2\n"
           . "11\n"
           . "2\n"
           . "2\n"
           . "2\n"
           . "2\n"
           . "13\n"
           . "11\n"}
     ],


     ['flipcase',
      qw(-f),
      {IN => q|s,\([^A-Za-z]*\)\([A-Za-z]*\),\1\L\u\2,g|},
      {IN => "09 - 02 - 2002 00.00 Tg La7 La7 -\n"
           . "09 - 02 - 2002 00.00 Brand New Tmc 2 -\n"
           . "09 - 02 - 2002 00.10 Tg1 Notte Rai Uno -\n"
           . "09 - 02 - 2002 00.15 Tg Parlamento Rai Due -\n"
           . "09 - 02 - 2002 00.15 Kung Fu - La Leggenda Continua La7 -\n"
           . "09 - 02 - 2002 00.20 Berserk - La CoNFESSIONE Di Gatz"
             . " Italia 1 Cartoon\n"
           . "09 - 02 - 2002 00.20 Tg3 - Tg3 Meteo Rai TrE -\n"
           . "09 - 02 - 2002 00.25 Meteo 2 Rai Due -\n"
           . "09 - 02 - 2002 00.30 Appuntamento Al CinEMA RaI Due -\n"
           . "09 - 02 - 2002 00.30 Rai Educational - Mediamente Rai Tre -\n"
           . "09 - 02 - 2002 00.35 Profiler Rai Due -\n"
           . "09 - 02 - 2002 00.35 Stampa OggI - Che Tempo Fa Rai Uno -\n"
           . "09 - 02 - 2002 00.45 Rai Educational - Babele: Euro Rai Uno -\n"
           . "09 - 02 - 2002 00.45 BollettINO Della NEVE RETE 4 News\n"
           . "09 - 02 - 2002 00.50 STUDIO Aperto - La Giornata Italia 1 News\n"
           . "09 - 02 - 2002 00.50 BOCCA A Bocca - 2 Tempo Rete 4 Film\n"
           . "09 - 02 - 2002 01.00 AppuntAMENTO Al Cinema Rai Tre -\n"
           . "09 - 02 - 2002 01.00 Music NoN Stop Tmc 2 -\n"
           . "09 - 02 - 2002 01.00 Studio SpORT Italia 1 SporT\n"
           . "09 - 02 - 2002 01.00 Tg 5 - Notte Canale 5 News\n"
           . "09 - 02 - 2002 01.05 Fuori Orario. CosE (Mai) Viste Rai Tre -\n"
           . "09 - 02 - 2002 01.15 RAINOTTE Rai Due -\n"
           . "09 - 02 - 2002 01.15 Sottovoce Rai Uno -\n"
           . "09 - 02 - 2002 01.15 GiOCHI Olimpici InVERNALI - CERIMONIA"
             . " Di Apertura Rai Tre -\n"
           . "09 - 02 - 2002 01.17 Italia Interroga Rai Due -\n"},
      {OUT => "09 - 02 - 2002 00.00 Tg La7 La7 -\n"
            . "09 - 02 - 2002 00.00 Brand New Tmc 2 -\n"
            . "09 - 02 - 2002 00.10 Tg1 Notte Rai Uno -\n"
            . "09 - 02 - 2002 00.15 Tg Parlamento Rai Due -\n"
            . "09 - 02 - 2002 00.15 Kung Fu - La Leggenda Continua La7 -\n"
            . "09 - 02 - 2002 00.20 Berserk - La Confessione Di Gatz"
              . " Italia 1 Cartoon\n"
            . "09 - 02 - 2002 00.20 Tg3 - Tg3 Meteo Rai Tre -\n"
            . "09 - 02 - 2002 00.25 Meteo 2 Rai Due -\n"
            . "09 - 02 - 2002 00.30 Appuntamento Al Cinema Rai Due -\n"
            . "09 - 02 - 2002 00.30 Rai Educational - Mediamente Rai Tre -\n"
            . "09 - 02 - 2002 00.35 Profiler Rai Due -\n"
            . "09 - 02 - 2002 00.35 Stampa Oggi - Che Tempo Fa Rai Uno -\n"
            . "09 - 02 - 2002 00.45 Rai Educational - Babele: Euro Rai Uno -\n"
            . "09 - 02 - 2002 00.45 Bollettino Della Neve Rete 4 News\n"
            . "09 - 02 - 2002 00.50 Studio Aperto - La Giornata Italia 1 News\n"
            . "09 - 02 - 2002 00.50 Bocca A Bocca - 2 Tempo Rete 4 Film\n"
            . "09 - 02 - 2002 01.00 Appuntamento Al Cinema Rai Tre -\n"
            . "09 - 02 - 2002 01.00 Music Non Stop Tmc 2 -\n"
            . "09 - 02 - 2002 01.00 Studio Sport Italia 1 Sport\n"
            . "09 - 02 - 2002 01.00 Tg 5 - Notte Canale 5 News\n"
            . "09 - 02 - 2002 01.05 Fuori Orario. Cose (Mai) Viste Rai Tre -\n"
            . "09 - 02 - 2002 01.15 Rainotte Rai Due -\n"
            . "09 - 02 - 2002 01.15 Sottovoce Rai Uno -\n"
            . "09 - 02 - 2002 01.15 Giochi Olimpici Invernali - Cerimonia"
              . " Di Apertura Rai Tre -\n"
            . "09 - 02 - 2002 01.17 Italia Interroga Rai Due -\n"}
       ],


     ['inclib',
      # inspired by an autoconf generated configure script.
      qw(-e 's;lib;include;'),
      {IN => "	/usr/X11R6/lib\n"
           . "	/usr/X11R5/lib\n"
           . "	/usr/X11R4/lib\n"
           . "\n"
           . "	/usr/lib/X11R6\n"
           . "	/usr/lib/X11R5\n"
           . "	/usr/lib/X11R4\n"
           . "\n"
           . "	/usr/local/X11R6/lib\n"
           . "	/usr/local/X11R5/lib\n"
           . "	/usr/local/X11R4/lib\n"
           . "\n"
           . "	/usr/local/lib/X11R6\n"
           . "	/usr/local/lib/X11R5\n"
           . "	/usr/local/lib/X11R4\n"
           . "\n"
           . "	/usr/X11/lib\n"
           . "	/usr/lib/X11\n"
           . "	/usr/local/X11/lib\n"
           . "	/usr/local/lib/X11\n"
           . "\n"
           . "	/usr/X386/lib\n"
           . "	/usr/x386/lib\n"
           . "	/usr/XFree86/lib/X11\n"
           . "\n"
           . "	/usr/lib\n"
           . "	/usr/local/lib\n"
           . "	/usr/unsupported/lib\n"
           . "	/usr/athena/lib\n"
           . "	/usr/local/x11r5/lib\n"
           . "	/usr/lpp/Xamples/lib\n"
           . "\n"
           . "	/usr/openwin/lib\n"
           . "	/usr/openwin/share/lib\n"},
      {OUT => "	/usr/X11R6/include\n"
            . "	/usr/X11R5/include\n"
            . "	/usr/X11R4/include\n"
            . "\n"
            . "	/usr/include/X11R6\n"
            . "	/usr/include/X11R5\n"
            . "	/usr/include/X11R4\n"
            . "\n"
            . "	/usr/local/X11R6/include\n"
            . "	/usr/local/X11R5/include\n"
            . "	/usr/local/X11R4/include\n"
            . "\n"
            . "	/usr/local/include/X11R6\n"
            . "	/usr/local/include/X11R5\n"
            . "	/usr/local/include/X11R4\n"
            . "\n"
            . "	/usr/X11/include\n"
            . "	/usr/include/X11\n"
            . "	/usr/local/X11/include\n"
            . "	/usr/local/include/X11\n"
            . "\n"
            . "	/usr/X386/include\n"
            . "	/usr/x386/include\n"
            . "	/usr/XFree86/include/X11\n"
            . "\n"
            . "	/usr/include\n"
            . "	/usr/local/include\n"
            . "	/usr/unsupported/include\n"
            . "	/usr/athena/include\n"
            . "	/usr/local/x11r5/include\n"
            . "	/usr/lpp/Xamples/include\n"
            . "\n"
            . "	/usr/openwin/include\n"
            . "	/usr/openwin/share/include\n"}
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
