/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/


/* This is a library of functions to support regular expressions whose syntax
   and semantics are as close as possible to those of the Perl 5 language. See
   the file Tech.Notes for some information on the internals.

   Written by: Philip Hazel <ph10@cam.ac.uk>

   Copyright (c) 1997-2000 University of Cambridge

   -----------------------------------------------------------------------------
   Permission is granted to anyone to use this software for any purpose on any
   computer system, and to redistribute it freely, subject to the following
   restrictions:

   1. This software is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   2. The origin of this software must not be misrepresented, either by
   explicit claim or by omission.

   3. Altered versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   4. If PCRE is embedded in any software that is released under the GNU
   General Purpose Licence (GPL), then the terms of that licence shall
   supersede any condition above with which it is incompatible.
   -----------------------------------------------------------------------------
 */

#ifndef PCRE_INTERNAL_H
#define PCRE_INTERNAL_H

/* This header contains definitions that are shared between the different
   modules, but which are not relevant to the outside. */

/* Get the definitions provided by running "configure" */

#include "config.h"

/* #define DEBUG */

/* Standard C headers plus the external interface definition */

#include "basicdefs.h"
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef HAVE_STDDEF_H
# include <stddef.h>
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif

#ifndef HAVE_STRCHR
# define strchr index
# define strrchr rindex
#endif

#include "pcre.h"

#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
#define INT_MAX ((1L<<30)-1+(1L<<30))
#define INT_MIN (-(1L<<30)-(1L<<30))
#endif

#ifndef HAVE_MEMSET
#define memset(s, better_be_zero, n) (better_be_zero ? abort() : bzero(s, n))
#endif

/* In case there is no definition of offsetof() provided - though any proper
   Standard C system should have one. */

#ifndef offsetof
#define offsetof(p_type,field) ((size_t)&(((p_type *)0)->field))
#endif

/* These are the public options that can change during matching. */

#define PCRE_IMS (PCRE_CASELESS|PCRE_MULTILINE|PCRE_DOTALL)

/* Private options flags start at the most significant end of the four bytes,
   but skip the top bit so we can use ints for convenience without getting tangled
   with negative values. The public options defined in pcre.h start at the least
   significant end. Make sure they don't overlap, though now that we have expanded
   to four bytes there is plenty of space. */

#define PCRE_FIRSTSET      0x40000000	/* first_char is set */
#define PCRE_REQCHSET      0x20000000	/* req_char is set */
#define PCRE_STARTLINE     0x10000000	/* start after \n for multiline */
#define PCRE_INGROUP       0x08000000	/* compiling inside a group */
#define PCRE_ICHANGED      0x04000000	/* i option changes within regex */

/* Options for the "extra" block produced by pcre_study(). */

#define PCRE_STUDY_MAPPED        0x01	/* a map of starting chars exists */
#define PCRE_STUDY_BM            0x02	/* a Boyer-Moore map exists */

/* Masks for identifying the public options which are permitted at compile
   time, run time or study time, respectively. */

#define PUBLIC_OPTIONS \
  (PCRE_CASELESS|PCRE_EXTENDED|PCRE_ANCHORED|PCRE_MULTILINE| \
   PCRE_DOTALL|PCRE_DOLLAR_ENDONLY|PCRE_EXTRA|PCRE_UNGREEDY| \
   PCRE_ENGLISH_ERRORS)

#define PUBLIC_EXEC_OPTIONS \
  (PCRE_ANCHORED|PCRE_NOTBOL|PCRE_NOTEOL|PCRE_NOTEMPTY)

#define PUBLIC_STUDY_OPTIONS \
  (PCRE_STUDY_NO_PRUNE|PCRE_STUDY_NO_START)

/* Magic number to provide a small check against being handed junk. */

#define MAGIC_NUMBER  0x50435245UL	/* 'PCRE' */

/* Miscellaneous definitions */

typedef int BOOL;

#define FALSE   0
#define TRUE    1

/* These are escaped items that aren't just an encoding of a particular data
   value such as \n. They must have non-zero values, as check_escape() returns
   their negation. Also, they must appear in the same order as in the opcode
   definitions below, up to ESC_z. The final one must be ESC_REF as subsequent
   values are used for \1, \2, \3, etc. */

enum
  {
    ESC_LESS = 1, ESC_GREATER,
    ESC_G, ESC_B, ESC_b,
    ESC_A, ESC_Z, ESC_z,
    ESC_D, ESC_d, ESC_S, ESC_s, ESC_W, ESC_w,
    ESC_FIRST_CONSUME = ESC_D,
    ESC_LAST_CONSUME = ESC_w,
    ESC_REF,
  };

enum
  {
    KIND_MAX, KIND_MIN, KIND_ONCE
  };

/* Opcode table: OP_BRA must be last, as all values >= it are used for brackets
   that extract substrings. Starting from 1 (i.e. after OP_END), the values up to
   OP_EOD must correspond in order to the list of escapes immediately above. */

enum
  {
    OP_END,			/* End of pattern */

    /* Values corresponding to backslashed assertions */

    OP_BEG_WORD,		/* \< */
    OP_END_WORD,		/* \> */
    OP_ANCHOR_MATCH,		/* \G */
    OP_NOT_WORD_BOUNDARY,	/* \B */
    OP_WORD_BOUNDARY,		/* \b */
    OP_SOD,			/* Start of data: \A */
    OP_EODN,			/* End of data or \n at end of data: \Z. */
    OP_EOD,			/* End of data: \z */

    OP_OPT,			/* Set runtime options */
    OP_CIRC,			/* Start of line - varies with multiline switch */
    OP_DOLL,			/* End of line - varies with multiline switch */
    OP_ANY,			/* Match any character */

    OP_CHARS,			/* Match string of characters */
    OP_MAXSTAR,			/* The maximizing and minimizing versions of */
    OP_MINSTAR,			/* all these opcodes must come in triples, with */
    OP_ONCESTAR,	        /* the minimizing one second and the */
    OP_MAXPLUS,	                /* no-backtracking one third. */
    OP_MINPLUS,			/* This first set applies to single characters */
    OP_ONCEPLUS,
    OP_MAXQUERY,
    OP_MINQUERY,
    OP_ONCEQUERY,
    OP_MAXUPTO,			/* From 0 to n matches */
    OP_MINUPTO,
    OP_ONCEUPTO,
    OP_EXACT,

    OP_NOT,			/* Match anything but the following char */
    OP_NOT_MAXSTAR,		/* The maximizing and minimizing versions of */
    OP_NOT_MINSTAR,		/* all these opcodes must come in triples, with */
    OP_NOT_ONCESTAR,	        /* the minimizing one second and the */
    OP_NOT_MAXPLUS,	        /* no-backtracking one third. */
    OP_NOT_MINPLUS,		/* This set applies to "not" single characters */
    OP_NOT_ONCEPLUS,
    OP_NOT_MAXQUERY,
    OP_NOT_MINQUERY,
    OP_NOT_ONCEQUERY,
    OP_NOT_MAXUPTO,		/* From 0 to n matches */
    OP_NOT_MINUPTO,
    OP_NOT_ONCEUPTO,
    OP_NOTEXACT,		/* Exactly n matches */

    OP_TYPE,
    OP_TYPE_MAXSTAR,		/* The maximizing and minimizing versions of */
    OP_TYPE_MINSTAR,		/* all these opcodes must come in triples, with */
    OP_TYPE_ONCESTAR,		/* the minimizing one second and the */
    OP_TYPE_MAXPLUS,            /* no-backtracking one third. These codes must */            
    OP_TYPE_MINPLUS,		/* be in exactly the same order as those above. */
    OP_TYPE_ONCEPLUS,
    OP_TYPE_MAXQUERY,		/* This set applies to character types such as \d */
    OP_TYPE_MINQUERY,
    OP_TYPE_ONCEQUERY,
    OP_TYPE_MAXUPTO,		/* From 0 to n matches */
    OP_TYPE_MINUPTO,
    OP_TYPE_ONCEUPTO,
    OP_TYPEEXACT,		/* Exactly n matches */

    OP_TYPENOT,
    OP_TYPENOT_MAXSTAR,		/* The maximizing and minimizing versions of */
    OP_TYPENOT_MINSTAR,		/* all these opcodes must come in triples, with */
    OP_TYPENOT_ONCESTAR,	/* the minimizing one second and the */
    OP_TYPENOT_MAXPLUS,         /* no-backtracking one third. These codes must */            
    OP_TYPENOT_MINPLUS,		/* be in exactly the same order as those above. */
    OP_TYPENOT_ONCEPLUS,
    OP_TYPENOT_MAXQUERY,	/* This set applies to character types such as \D */
    OP_TYPENOT_MINQUERY,
    OP_TYPENOT_ONCEQUERY,
    OP_TYPENOT_MAXUPTO,		/* From 0 to n matches */
    OP_TYPENOT_MINUPTO,
    OP_TYPENOT_ONCEUPTO,
    OP_TYPENOTEXACT,		/* Exactly n matches */

    OP_CLASS,			/* Match a character class */
    OP_CL_MAXSTAR,		/* The maximizing and minimizing versions of */
    OP_CL_MINSTAR,		/* all these opcodes must come in pairs, with */
    OP_CL_ONCESTAR,		/* the minimizing one second and the */
    OP_CL_MAXPLUS,		/* no-backtracking one third. These codes must */            
    OP_CL_MINPLUS,		/* be in exactly the same order as those above. */
    OP_CL_ONCEPLUS,
    OP_CL_MAXQUERY,		/* These are for character classes */
    OP_CL_MINQUERY,
    OP_CL_ONCEQUERY,
    OP_CL_MAXRANGE,		/* These are different to the three seta above. */
    OP_CL_MINRANGE,
    OP_CL_ONCERANGE,

    OP_REF,			/* Match a back reference */
    OP_REF_MAXSTAR,		/* The maximizing and minimizing versions of */
    OP_REF_MINSTAR,		/* all these opcodes must come in pairs, with */
    OP_REF_ONCESTAR,		/* the minimizing one second and the */
    OP_REF_MAXPLUS,		/* no-backtracking one third. These codes must */            
    OP_REF_MINPLUS,		/* be in exactly the same order as those above. */
    OP_REF_ONCEPLUS,
    OP_REF_MAXQUERY,		/* These are for character classes */
    OP_REF_MINQUERY,
    OP_REF_ONCEQUERY,
    OP_REF_MAXRANGE,		/* These are different to the three seta above. */
    OP_REF_MINRANGE,
    OP_REF_ONCERANGE,

    OP_RECURSE,			/* Match this pattern recursively */

    OP_ALT,			/* Start of alternation */
    OP_KET,			/* End of group that doesn't have an unbounded repeat */
    OP_KET_MAXSTAR,		/* These three must remain together and in this */
    OP_KET_MINSTAR,		/* order. They are for groups the repeat for ever. */
    OP_KET_ONCESTAR,

    /* The assertions must come before ONCE and COND */

    OP_ASSERT,			/* Positive lookahead */
    OP_ASSERT_NOT,		/* Negative lookahead */
    OP_ASSERTBACK,		/* Positive lookbehind */
    OP_ASSERTBACK_NOT,		/* Negative lookbehind */
    OP_REVERSE,			/* Move pointer back - used in lookbehind assertions */

    /* ONCE and COND must come after the assertions, with ONCE first, as there's
       a test for >= ONCE for a subpattern that isn't an assertion. */

    OP_ONCE,			/* Once matched, don't back up into the subpattern */
    OP_COND,			/* Conditional group */
    OP_CREF,			/* Used to hold an extraction string number (cond ref) */

    OP_BRAZERO,			/* These two must remain together and in this */
    OP_BRAMINZERO,		/* order. */

    OP_BRANUMBER,               /* Used for extracting brackets whose number is greater
	                           than can fit into an opcode. */

    OP_BRA			/* This and greater values are used for brackets that
				   extract substrings up to a basic limit. After that,
                                   use is made of OP_BRANUMBER. */
  };

/* The highest extraction number. This is limited by the number of opcodes
   left after OP_BRA, i.e. 255 - OP_BRA. We actually set it somewhat lower
   to leave room for additional opcodes.. */

#define EXTRACT_BASIC_MAX  150

/* The texts of compile-time error messages are defined as macros here so that
   they can be accessed by the POSIX wrapper and converted into error codes.  Yes,
   I could have used error codes in the first place, but didn't feel like changing
   just to accommodate the POSIX wrapper. */


/* All character handling must be done as unsigned characters. Otherwise there
   are problems with top-bit-set characters and functions such as isspace().
   However, we leave the interface to the outside world as char *, because that
   should make things easier for callers. We define a short type for unsigned char
   to save lots of typing. I tried "uchar", but it causes problems on Digital
   Unix, where it is defined in sys/types, so use "uschar" instead. */

typedef unsigned char uschar;

typedef uschar bitset[32];

/* The real format of the start of the pcre block; the actual code vector
   runs on as long as necessary after the end. */

struct pcre
  {
    unsigned long int magic_number;
    size_t size;
    ssize_t max_match_size;
    const unsigned char *tables;
    unsigned long int options;
    unsigned short int top_bracket;
    unsigned short int top_backref;
    uschar first_char;
    uschar req_char;
    uschar code[1];
  };

/* The real format of the extra block returned by pcre_study(). */

struct pcre_extra
  {
    uschar options;
    union {
      bitset start_bits;
      uschar bmtable[257];
    } data;
  };


/* Structure for passing "static" information around between the functions
   doing the compiling, so that they are thread-safe. */

typedef struct compile_data
  {
    const uschar *lcc;		/* Points to lower casing table */
    const uschar *fcc;		/* Points to case-flipping table */
    const uschar *cbits;	/* Points to character type table */
    const uschar *ctypes;	/* Points to table of type maps */
    const uschar *end;		/* Points to last valid byte */
    ssize_t max_match_size;
  }
compile_data;

/* Structure for passing "static" information around between the functions
   doing the matching, so that they are thread-safe. */

typedef struct match_data
  {
    int errorcode;		/* As it says */
    int *offset_vector;		/* Offset vector */
    int offset_end;		/* One past the end */
    int offset_max;		/* The maximum usable for return data */
    const uschar *lcc;		/* Points to lower casing table */
    const uschar *ctypes;	/* Points to table of type maps */
    BOOL offset_overflow;	/* Set if too many extractions */
    BOOL notbol;		/* NOTBOL flag */
    BOOL noteol;		/* NOTEOL flag */
    BOOL utf8;			/* UTF8 flag */
    BOOL endonly;		/* Dollar not before final \n */
    BOOL notempty;		/* Empty string match not wanted */
    const uschar *start_pattern;	/* For use when recursing */
    const uschar *start_subject;	/* Start of the subject string */
    const uschar *end_subject;	/* End of the subject string */
    const uschar *first_start;	/* Part of subject considered by preg_exec */
    const uschar *start_match;	/* Start of this match attempt */
    const uschar *end_match_ptr;	/* Subject position at end match */
    int end_offset_top;		/* Highwater mark at end of match */
  }
match_data;

/* Bit definitions for entries in the pcre_ctypes table. */

#define ctype_newline 0x01
#define ctype_digit   0x02      /* must follow the other in the escapes above */
#define ctype_space   0x04
#define ctype_word    0x08	/* alphameric or '_' */
#define ctype_letter  0x10
#define ctype_xdigit  0x20
#define ctype_meta    0x80	/* regexp meta char or zero (end pattern) */

/* Offsets for the bitmap tables in pcre_cbits. Each table contains a set
   of bits for a class map. Some classes are built by combining these tables. */

#define cbit_xdigit    0	/* [:xdigit:] */
#define cbit_digit    32	/* [:digit:] */
#define cbit_space    64	/* [:space:] */
#define cbit_word     96	/* [:word:] */
#define cbit_upper   128	/* [:upper:] */
#define cbit_lower   160	/* [:lower:] */
#define cbit_graph   192	/* [:graph:] */
#define cbit_print   224	/* [:print:] */
#define cbit_punct   256	/* [:punct:] */
#define cbit_cntrl   288	/* [:cntrl:] */
#define cbit_blank   320	/* [:blank:] */
#define cbit_length  352	/* Length of the cbits table */

/* Offsets of the various tables from the base tables pointer, and
   total length. */

#define lcc_offset      0
#define fcc_offset    256
#define cbits_offset  512
#define ctypes_offset (cbits_offset + cbit_length)
#define tables_length (ctypes_offset + 256)

#ifdef __STDC__
#define __PCRE_PROTO(a) a
#else
#define __PCRE_PROTO(a) ()
#endif

extern void pcre_debug __PCRE_PROTO ((pcre *));
extern const char *pcre_estrings[];
extern const char *pcre_OP_names[];

#endif
