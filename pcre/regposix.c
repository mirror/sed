
/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/*
   This is a library of functions to support regular expressions whose syntax
   and semantics are as close as possible to those of the Perl 5 language. See
   the file Tech.Notes for some information on the internals.

   Written by: Philip Hazel <ph10@cam.ac.uk>

   Copyright (c) 1997-2000 University of Cambridge

   ----------------------------------------------------------------------------
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


/* Include the internals header, which itself includes Standard C headers plus
   the external pcre header. */

#include "internal.h"

/* Use a macro for debugging printing, 'cause that eliminates the use of #ifdef
   inline, and there are *still* stupid compilers about that don't like indented
   pre-processor statements. I suppose it's only been 10 years... */

#ifdef DEBUG
#define DPRINTF(p) printf p
#define DFWRITE(p) fwrite p
#else
#define DPRINTF(p)		/*nothing */
#define DFWRITE(p)		/*nothing */
#endif


/* Allow compilation as C++ source code, should anybody want to do that. */

#ifdef __cplusplus
#define class pcre_class
#endif


/* Maximum number of items on the nested bracket stacks at compile time. This
   applies to the nesting of all kinds of parentheses. It does not limit
   un-nested, non-capturing parentheses. This number can be made bigger if
   necessary - it is used to dimension one int and one unsigned char vector at
   compile time. */

#define BRASTACK_SIZE 200


/* The number of bytes in a literal character string above which we can't
   add any more. */

#define MAXLIT 255

/* Text forms of OP_ values and things, for debugging (not all used) */

/* Table for handling escaped characters in the range ' '-'z'. Positive returns
   are simple data values; negative values are for special things like \d and so
   on. Zero means further processing is needed (for things like \x), or the escape
   is invalid. */

static const short int escapes[] = {
  0, 0, 0, 0, 0, 0, 0, -ESC_z,	/* space - ' */
  0, 0, 0, 0, 0, 0, 0, 0,	/* ( - / */
  0, -ESC_REF - 1, -ESC_REF - 2,	/* 0 - 8 */
  -ESC_REF - 3, -ESC_REF - 4, -ESC_REF - 5,
  -ESC_REF - 6, -ESC_REF - 7, -ESC_REF - 8,
  -ESC_REF - 9, ':', ';', -ESC_LESS, '=', -ESC_GREATER, '?',	/* 9 - ? */
  '@', 0, -ESC_B, 0, 0, 0, 0, -ESC_G,	/* @ - G */
  0, 0, 0, 0, 0, 0, 0, 0,	/* H - O */
  0, 0, 0, -ESC_S, 0, 0, 0, -ESC_W,	/* P - W */
  0, 0, 0, '[', '\\', ']', '^', '_',	/* X - _ */
  -ESC_A, 7, -ESC_b, 0, 0, 27, '\f', 0,	/* ` - g */
  0, 0, 0, 0, 0, 0, '\n', 0,	/* h - o */
  0, 0, '\r', -ESC_s, '\t', 0, '\v', -ESC_w,	/* p - w */
  0, 0, 0			/* x - z */
};

#include "regcomp.c"


/*************************************************
*            Handle escapes                      *
*************************************************/

/* This function is called when a \ has been encountered. It either returns a
   positive value for a simple escape such as \n, or a negative value which
   encodes one of the more complicated things such as \d. On entry, ptr is 
   pointing at the \. On exit, it is on the final character of the escape
   sequence.

   Arguments:
   ptrptr     points to the pattern position pointer
   errorptr   points to the pointer to the error message
   bracount   number of previous capturing brackets
   options    the options bits
   isclass    TRUE if inside a character class
   cd         pointer to char tables block

   Returns:     zero or positive => a data character
   negative => a special escape sequence
   on error, errorptr is set
 */

static int
check_escape (ptrptr, errorptr, bracount, options, isclass, cd)
     const uschar **ptrptr;
     const char **errorptr;
     int bracount;
     int options;
     BOOL isclass;
     compile_data *cd;
{
  const uschar *ptr = *ptrptr;
  int c, i;

  c = *(++ptr);

  /* Digits or letters may have special meaning; all others are literals. */
  if (c < ' ' || c > 'z')
    {
      if (isclass) {
	c = '\\';
	--ptr;
      }
    }

  /* Do an initial lookup in a table. A > zero result is something that can be
     returned immediately. A < zero is a literal inside a class, a special
     operator outside brackets.  Otherwise further processing may be required. */
  else if (((i = escapes[c - ' ']) < 0 || i == c) && isclass)
    {
      c = '\\';
      --ptr;
    }

  else if (i != 0)
    c = i;

  /* Escapes that need further processing, or are illegal. */
  else
    {
      const uschar *oldptr;
      switch (c)
	{
	case 'o':
	  if (ptr == cd->end || (cd->ctypes[ptr[1]] & ctype_digit) == 0 ||
	      ptr[1] == '8' || ptr[1] == '9')
	    break;

	  c = 0;
	  do
	    c = c * 8 + *(++ptr) - '0';
	  while (++i < 3 && ptr < cd->end
		 && (cd->ctypes[ptr[1]] & ctype_digit) != 0 &&
		 ptr[1] != '8' && ptr[1] != '9');

	  c &= 255;		/* Take least significant 8 bits */
	  break;

	case 'd':
	  if (ptr == cd->end || (cd->ctypes[ptr[1]] & ctype_digit) == 0)
	    break;

	  c = 0;
	  do
	    c = c * 10 + *(++ptr) - '0';
	  while (++i < 3 && ptr < cd->end
		 && (cd->ctypes[ptr[1]] & ctype_digit) != 0);

	  c &= 255;		/* Take least significant 8 bits */
	  break;

	case 'x':
	  /* Read just a single hex char */
	  if (ptr == cd->end || (cd->ctypes[ptr[1]] & ctype_xdigit) == 0)
	    break;

	  c = 0;
	  do
	    {
	      ptr++;
	      c = c * 16 + cd->lcc[*ptr] -
		(((cd->ctypes[*ptr] & ctype_digit) != 0) ? '0' : 'a' - 10);
	    }
	  while (++i < 2 && ptr < cd->end
		 && (cd->ctypes[ptr[1]] & ctype_xdigit) != 0);
	  break;

	case 'c':
	  if ((++ptr) > cd->end || (c = *ptr) == 0)
	    {
	      *errorptr = pcre_estrings[2];
	      return 0;
	    }

	  /* A letter is upper-cased; then the 0x40 bit is flipped */

	  if (c >= 'a' && c <= 'z')
	    c = cd->fcc[c];
	  c ^= 0x40;
	  break;

	default:
	  if (isclass)
	    {
	      c = '\\';
	      --ptr;
	    }

	  break;
	}
    }

  *ptrptr = ptr;
  return c;
}



/*************************************************
*           Compile one branch                   *
*************************************************/

/* Scan the pattern, compiling it into the code vector.

   Arguments:
   options      the option bits
   brackets     points to number of capturing brackets used
   code         points to the pointer to the current code point
   ptrptr       points to the current pattern pointer
   errorptr     points to pointer to error message
   optchanged   set to the value of the last OP_OPT item compiled
   reqchar      set to the last literal character required, else -1
   countlits    set to count of mandatory literal characters
   cd           contains pointers to tables

   Returns:       TRUE on success
   FALSE, with *errorptr set on error
 */

static BOOL
compile_branch_posix (options, brackets, codeptr, ptrptr, errorptr, optchanged,
		      reqchar, countlits, cd)
     int options;
     int *brackets;
     uschar **codeptr;
     const uschar **ptrptr;
     const char **errorptr;
     int *optchanged;
     int *reqchar;
     int *countlits;
     compile_data *cd;
{
  int repeat_min, repeat_max;
  int bravalue, length;
  int greedy_default;
  int prevreqchar;
  int condcount = 0;
  int subcountlits = 0;
  BOOL first = TRUE;
  int c;
  uschar *code = *codeptr;
  const uschar *ptr = *ptrptr;
  const uschar *tempptr;
  uschar *previous = NULL;
  uschar class[32];

  /* Set up the settings for greediness */
  greedy_default = ((options & PCRE_UNGREEDY) != 0);

  /* Initialize no required char, and count of literals */
  *reqchar = prevreqchar = -1;
  *countlits = 0;

  /* Initialize standard {1,1} repeats */
  repeat_min = repeat_max = 1;

  /* Switch on next character until the end of the branch */
  for (;; first = FALSE, ptr++)
    {
      BOOL negate_class;
      int class_charcount;
      int class_lastchar;
      int newoptions;
      int skipbytes;
      int subreqchar;

      c = (ptr > cd->end) ? 0 : *ptr;

      if ((repeat_min != 1 || repeat_max != 1) &&
	  (c != '*' && c != '+' && c != '?' && c != '{'))
	{
	  int op_type;
	  int repeat_type;

	  if (*previous == OP_CIRC || *previous == OP_DOLL ||
	      (*previous >= ESC_LESS && *previous < ESC_FIRST_CONSUME) ||
	      (*previous > ESC_LAST_CONSUME && *previous <= ESC_z))
	    {
	      /* Handle assertions: ^* --> <nothing>,  ^{1,3} --> ^ */
	      if (repeat_min == 0)
		code = previous;
	      goto END_REPEAT;
	    }

	  repeat_type = greedy_default;

	  /* If previous was a string of characters, chop off the last one and use it
	     as the subject of the repeat. If there was only one character, we can
	     abolish the previous item altogether. A repeat with a zero minimum wipes
	     out any reqchar setting, backing up to the previous value. We must also
	     adjust the countlits value. */

	  if (*previous == OP_CHARS)
	    {
	      int len = previous[1];

	      if (repeat_min == 0)
		*reqchar = prevreqchar;
	      *countlits += repeat_min - 1;

	      if (len == 1)
		{
		  c = previous[2];
		  code = previous;
		}
	      else
		{
		  c = previous[len + 1];
		  previous[1]--;
		  code--;
		}
	      op_type = 0;	/* Use single-char op codes */
	      goto OUTPUT_SINGLE_REPEAT;	/* Code shared with single character types */
	    }

	  /* If previous was a single negated character ([^a] or similar), we use
	     one of the special opcodes, replacing it. The code is shared with single-
	     character repeats by adding a suitable offset into repeat_type. */

	  else if ((int) *previous == OP_NOT)
	    {
	      op_type = OP_NOT - OP_CHARS;	/* Use "not" opcodes */
	      c = previous[1];
	      code = previous;
	      goto OUTPUT_SINGLE_REPEAT;
	    }

	  /* If previous was a character type match (\d or similar), abolish it and
	     create a suitable repeat item. The code is shared with single-character
	     repeats by adding a suitable offset into repeat_type. */

	  else if (*previous == OP_TYPE || *previous == OP_TYPENOT ||
		   *previous == OP_ANY)
	    {
	      if (*previous == OP_ANY)
		{
		  c = 0;
		  *previous = OP_TYPENOT;
		}
	      else
		c = previous[1];

	      op_type = *previous - OP_CHARS;
	      code = previous;

	    OUTPUT_SINGLE_REPEAT:
	      cd->max_match_size += repeat_max - 1;

	      /* If the maximum is zero then the minimum must also be zero; Perl allows
	         this case, so we do too - by simply omitting the item altogether. */

	      if (repeat_max == 0)
		goto END_REPEAT;

	      /* Combine the op_type with the repeat_type */

	      repeat_type += op_type;

	      /* A minimum of zero is handled either as the special case * or ?, or as
	         an UPTO, with the maximum given. */

	      if (repeat_min == 0)
		{
		  if (repeat_max == -1)
		    *code++ = OP_MAXSTAR + repeat_type;
		  else if (repeat_max == 1)
		    *code++ = OP_MAXQUERY + repeat_type;
		  else
		    {
		      *code++ = OP_MAXUPTO + repeat_type;
		      *code++ = repeat_max >> 8;
		      *code++ = (repeat_max & 255);
		    }
		}

	      /* The case {1,} is handled as the special case + */

	      else if (repeat_min == 1 && repeat_max == -1)
		*code++ = OP_MAXPLUS + repeat_type;

	      /* The case {n,n} is just an EXACT, while the general case {n,m} is
	         handled as an EXACT followed by an UPTO. An EXACT of 1 is optimized. */

	      else
		{
		  if (repeat_min != 1)
		    {
		      *code++ = OP_EXACT + op_type;	/* NB EXACT doesn't have repeat_type */
		      *code++ = repeat_min >> 8;
		      *code++ = (repeat_min & 255);
		    }

		  /* If the mininum is 1 and the previous item was a character string,
		     we either have to put back the item that got cancelled if the string
		     length was 1, or add the character back onto the end of a longer
		     string. For a character type nothing need be done; it will just get
		     put back naturally. Note that the final character is always going to
		     get added below. */

		  else if (*previous == OP_CHARS)
		    {
		      if (code == previous)
			code += 2;
		      else
			previous[1]++;
		    }

		  /*  For negated characters or classes we also have to put back the
		     item that got cancelled. */

		  else if (*previous == OP_NOT || *previous == OP_TYPE ||
			   *previous == OP_TYPENOT)
		    code++;
		  /* If the maximum is unlimited, insert an OP_MAXSTAR. */

		  if (repeat_max < 0)
		    {
		      *code++ = c;
		      *code++ = OP_MAXSTAR + repeat_type;
		    }

		  /* Else insert an UPTO if the max is greater than the min. */

		  else if (repeat_max != repeat_min)
		    {
		      *code++ = c;
		      repeat_max -= repeat_min;
		      *code++ = OP_MAXUPTO + repeat_type;
		      *code++ = repeat_max >> 8;
		      *code++ = (repeat_max & 255);
		    }
		}

	      /* The character or character type itself comes last in all cases. */

	      *code++ = c;
	    }

	  /* If previous was a character class or a back reference, we put the repeat
	     stuff after it, but just skip the item if the repeat was {0,0}. */

	  else if (*previous == OP_CLASS || *previous == OP_REF)
	    {
	      if (repeat_max == 0)
		{
		  code = previous;
		  goto END_REPEAT;
		}

	      if (*previous == OP_CLASS)
		cd->max_match_size += repeat_max - 1;
	      else
		repeat_type += OP_REF - OP_CLASS;

	      if (repeat_min == 0 && repeat_max == -1)
		*previous = OP_CL_MAXSTAR + repeat_type;
	      else if (repeat_min == 1 && repeat_max == -1)
		*previous = OP_CL_MAXPLUS + repeat_type;
	      else if (repeat_min == 0 && repeat_max == 1)
		*previous = OP_CL_MAXQUERY + repeat_type;
	      else
		{
		  *previous = OP_CL_MAXRANGE + repeat_type;
		  *code++ = repeat_min >> 8;
		  *code++ = repeat_min & 255;
		  if (repeat_max == -1)
		    repeat_max = 0;	/* 2-byte encoding for max */
		  *code++ = repeat_max >> 8;
		  *code++ = repeat_max & 255;
		}
	    }

	  /* If previous was a bracket group, we may have to replicate it in certain
	     cases. */

	  else if ((int) *previous >= OP_BRA)
	    {
	      int i;
	      int ketoffset = 0;
	      int len = code - previous;

	      /* If the maximum repeat count is unlimited, find the end of the bracket
	         by scanning through from the start, and compute the offset back to it
	         from the current code pointer. There may be an OP_OPT setting following
	         the final KET, so we can't find the end just by going back from the code
	         pointer. */

	      if (repeat_max == -1)
		{
		  uschar *ket = previous;
		  do
		    ket += (ket[1] << 8) + ket[2];
		  while (*ket != OP_KET);
		  ketoffset = code - ket;
		}
	      else
		cd->max_match_size += subcountlits * (repeat_max - 1);

	      /* The case of a zero minimum is special because of the need to stick
	         OP_BRAZERO in front of it, and because the group appears once in the
	         data, whereas in other cases it appears the minimum number of times. For
	         this reason, it is simplest to treat this case separately, as otherwise
	         the code gets far too messy. There are several special subcases when the
	         minimum is zero. */

	      if (repeat_min == 0)
		{
		  /* If we set up a required char from the bracket, we must back off
		     to the previous value and reset the countlits value too. */

		  if (subcountlits > 0)
		    {
		      *reqchar = prevreqchar;
		      *countlits -= subcountlits;
		    }

		  /* If the maximum is also zero, we just omit the group from the output
		     altogether. */

		  if (repeat_max == 0)
		    {
		      code = previous;
		      goto END_REPEAT;
		    }

		  /* If the maximum is 1 or unlimited, we just have to stick in the
		     BRAZERO and do no more at this point. */

		  if (repeat_max <= 1)
		    {
		      memmove (previous + 1, previous, len);
		      code++;
		      *previous++ = OP_BRAZERO + repeat_type;
		    }

		  /* If the maximum is greater than 1 and limited, we have to replicate
		     in a nested fashion, sticking OP_BRAZERO before each set of brackets.
		     The first one has to be handled carefully because it's the original
		     copy, which has to be moved up. The remainder can be handled by code
		     that is common with the non-zero minimum case below. We just have to
		     adjust the value or repeat_max, since one less copy is required. */

		  else
		    {
		      int offset;
		      memmove (previous + 1, previous, len);
		      code += 1;
		      *previous++ = OP_BRAZERO + repeat_type;
		    }

		  repeat_max--;
		}

	      /* If the minimum is greater than zero, replicate the group as many
	         times as necessary, and adjust the maximum to the number of subsequent
	         copies that we need. */

	      else
		{
		  for (i = 1; i < repeat_min; i++)
		    {
		      memcpy (code, previous, len);
		      code += len;
		    }
		  if (repeat_max > 0)
		    repeat_max -= repeat_min;
		}

	      /* This code is common to both the zero and non-zero minimum cases. If
	         the maximum is limited, it replicates the group in a nested fashion,
	         remembering the bracket starts on a stack. In the case of a zero minimum,
	         the first one was set up above. In all cases the repeat_max now specifies
	         the number of additional copies needed. */

	      if (repeat_max >= 0)
		{
		  for (i = repeat_max - 1; i >= 0; i--)
		    {
		      *code++ = OP_BRAZERO + repeat_type;

		      memcpy (code, previous, len);
		      code += len;
		    }
		}

	      /* If the maximum is unlimited, set a repeater in the final copy. We
	         can't just offset backwards from the current code point, because we
	         don't know if there's been an options resetting after the ket. The
	         correct offset was computed above. */

	      else
		code[-ketoffset] = OP_KET_MAXSTAR + repeat_type;
	    }

	  /* Else there's some kind of shambles */

	  else
	    {
	      *errorptr = pcre_estrings[11];
	      goto FAILED;
	    }

	  
	  if (repeat_max == -1)
	    /* An unlimited repeat forces us to disable this optimization */
	    cd->max_match_size = INT_MIN;

	  /* In all case we no longer have a previous item. */

	END_REPEAT:
	  previous = NULL;
	  repeat_min = 1;
	  repeat_max = 1;
	}

      if (ptr > cd->end)
	break;

      c = *ptr;

      switch (c)
	{
	  /* The branch terminates at end of string, |, or ). */

	case '|':
	case ')':
	  goto SUCCEEDED;

	  /* Handle single-character metacharacters */

	case '^':
	  if (!first)
	    goto NORMAL_CHAR;
	  previous = code;
	  *code++ = OP_CIRC;
	  break;

	case '$':
	  if (ptr < cd->end && ptr[1] != '|' && ptr[1] != ')')
	    goto NORMAL_CHAR;
	  previous = NULL;
	  /* Perl has different semantics than POSIX for dollars, when
	     multiline mode is not in effect */
	  *code++ = (options & PCRE_MULTILINE) ? OP_DOLL : OP_EOD;
	  break;

	case '.':
	  previous = code;
	  cd->max_match_size++;
	  *code++ = OP_ANY;
	  break;

	  /* Character classes. These always build a 32-byte bitmap of the permitted
	     characters, except in the special case where there is only one character.
	     For negated classes, we build the map as usual, then invert it at the end.
	   */

	case '[':
	  previous = code;
	  cd->max_match_size++;
	  *code++ = OP_CLASS;

	  /* If the first character is '^', set the negation flag and skip it. */

	  if ((c = *(++ptr)) == '^')
	    {
	      negate_class = TRUE;
	      c = *(++ptr);
	    }
	  else
	    negate_class = FALSE;

	  /* Keep a count of chars so that we can optimize the case of just a single
	     character. */

	  class_charcount = 0;
	  class_lastchar = -1;

	  /* Initialize the 32-char bit map to all zeros. We have to build the
	     map in a temporary bit of store, in case the class contains only 1
	     character, because in that case the compiled code doesn't use the
	     bit map. */

	  memset (class, 0, 32 * sizeof (uschar));

	  /* Process characters until ] is reached. By writing this as a "do" it
	     means that an initial ] is taken as a data character.  We also have
	     to take care of this in the `if' below: it is an error if we reach the
	     last character and we have not added a data character yet, because a
	     closing bracket will not be considered as a terminator. */

	  do
	    {
	      if (ptr > cd->end
		  || (ptr == cd->end && *ptr != ']')
		  || (ptr == cd->end && class_charcount == 0))
		{
		  *errorptr = pcre_estrings[6];
		  goto FAILED;
		}

	      /* Recognize the POSIX constructions [.ch.] and [=ch=] for a
	         single character. */
	      if (c == '[' && (cd->end + 1) - ptr >= 5
		  && (ptr[1] == '.' || ptr[1] == '=') && ptr[3] == ptr [1]
		  && ptr[4] == ']')
		{
		  c = ptr[2];
		  /* A further ++ptr is done below.  */
		  ptr += 4;
		}

	      /* Handle POSIX class names. A square bracket that doesn't match the
	         syntax is treated as a literal. We also recognize the POSIX
	         constructions [.ch.] and [=ch=] ("collating elements") and
	         fault them, as Perl 5.6 does. */

	      else if (c == '[' &&
		  (ptr[1] == ':' || ptr[1] == '.' || ptr[1] == '=') &&
		  check_posix_syntax (ptr, &tempptr, cd))
		{
		  int posix_class, i;
		  const uschar *cbits = cd->cbits;

		  if (ptr[1] != ':')
		    {
		      *errorptr = pcre_estrings[31];
		      goto FAILED;
		    }

		  ptr += 2;

		  posix_class = check_posix_name (ptr, tempptr - ptr);
		  if (posix_class < 0)
		    {
		      *errorptr = pcre_estrings[30];
		      goto FAILED;
		    }

		  /* If matching is caseless, upper and lower are converted to
		     alpha. This relies on the fact that the class table starts with
		     alpha, lower, upper as the first 3 entries. */

		  if ((options & PCRE_CASELESS) != 0 && posix_class <= 2)
		    posix_class = 0;

		  /* Or into the map we are building up to 3 of the static class
		     tables, or their negations. */

		  posix_class *= 3;
		  for (i = 0; i < 3; i++)
		    {
		      int taboffset = posix_class_maps[posix_class + i];
		      if (taboffset < 0)
			break;
		      for (c = 0; c < 32; c++)
			class[c] |= cbits[c + taboffset];
		    }

		  ptr = tempptr + 1;
		  class_charcount = 10;	/* Set > 1; assumes more than 1 per class */
		  continue;
		}

	      /* Escapes inside character classes are not POSIX */
	      else if (c == '\\')
		c =
		  check_escape (&ptr, errorptr, *brackets, options, TRUE, cd);

	      /* A single character may be followed by '-' to form a range. However,
	         Perl and POSIX do not permit ']' to be the end of the range. An hyphen
	         here is treated as a literal. */

	      if (ptr[1] == '-' && ptr[2] != ']')
		{
		  int d;
		  ptr += 2;

		  if (ptr > cd->end)
		    {
		      *errorptr = pcre_estrings[6];
		      goto FAILED;
		    }

		  d = *ptr;
		  if (d == '\\')
		    d =
		      check_escape (&ptr, errorptr, *brackets, options, TRUE,
				    cd);

		  if (d < c)
		    {
		      *errorptr = pcre_estrings[8];
		      goto FAILED;
		    }

		  for (; c <= d; c++)
		    {
		      class[c / 8] |= (1 << (c & 7));
		      if ((options & PCRE_CASELESS) != 0)
			{
			  int uc = cd->fcc[c];	/* flip case */
			  class[uc / 8] |= (1 << (uc & 7));
			}
		      class_charcount++;	/* in case a one-char range */
		      class_lastchar = c;
		    }
		  continue;	/* Go get the next char in the class */
		}

	      /* Handle a lone single character */

	      class[c / 8] |= (1 << (c & 7));
	      if ((options & PCRE_CASELESS) != 0)
		{
		  c = cd->fcc[c];	/* flip case */
		  class[c / 8] |= (1 << (c & 7));
		}
	      class_charcount++;
	      class_lastchar = c;
	    }

	  /* Loop until ']' reached; the check for end of string happens inside the
	     loop. This "while" is the end of the "do" above. */

	  while ((c = *(++ptr)) != ']');

	  /* If class_charcount is 1 and class_lastchar is not negative, we saw
	     precisely one character. This doesn't need the whole 32-byte bit map.
	     We turn it into a 1-character OP_CHAR if it's positive, or OP_NOT if
	     it's negative. */

	  if (class_charcount == 1 && class_lastchar >= 0)
	    {
	      if (negate_class)
		{
		  code[-1] = OP_NOT;
		}
	      else
		{
		  code[-1] = OP_CHARS;
		  *code++ = 1;
		}
	      *code++ = class_lastchar;
	    }

	  /* Otherwise, negate the 32-byte map if necessary, and copy it into
	     the code vector. */

	  else
	    {
	      if (negate_class)
		for (c = 0; c < 32; c++)
		  code[c] = ~class[c];
	      else
		memcpy (code, class, 32);
	      code += 32;
	    }
	  break;

	  /* Various kinds of repeat */

	case '{':
	  if (previous == NULL)
	    goto NORMAL_CHAR;

	  if (!is_counted_repeat (ptr + 1, cd))
	    *errorptr = pcre_estrings[14];
	  else
	    {
	      ptr = read_repeat_counts (ptr + 1, &repeat_min, &repeat_max,
					errorptr, cd);
	      if (*errorptr != NULL)
		goto FAILED;
	    }
	  break;

	case '*':
	  if (previous == NULL)
	    goto NORMAL_CHAR;

	  repeat_min = 0;
	  repeat_max = -1;
	  break;

	case '+':
	  if (previous == NULL)
	    goto NORMAL_CHAR;

	  repeat_max = -1;
	  break;

	case '?':
	  if (previous == NULL)
	    goto NORMAL_CHAR;

	  repeat_min = 0;
	  break;

	  /* Start of nested bracket sub-expression. */

	case '(':
	  newoptions = options;
	  skipbytes = 0;
	  if (ptr == cd->end)
	    {
	      *errorptr = pcre_estrings[22];
	      goto FAILED;
	    }

	  /* In POSIX we always have a referencing group; adjust the opcode. If the bracket
	     number is greater than EXTRACT_BASIC_MAX, we set the opcode one higher, and
	     arrange for the true number to follow later, in an OP_BRANUMBER item. */

	  ptr++;
	  if (++(*brackets) > EXTRACT_BASIC_MAX)
	    {
	      bravalue = OP_BRA + EXTRACT_BASIC_MAX + 1;
	      code[3] = OP_BRANUMBER;
	      code[4] = *brackets >> 8;
	      code[5] = *brackets & 255;
	      skipbytes = 3;
	    }
	  else
	    bravalue = OP_BRA + *brackets;

	  /* Process nested bracketed re. Assertions may not be repeated, but other kinds
	     can be. Pass in a new setting for the ims options if they have changed. */

	  previous = code;
	  *code = bravalue;

	  if (!compile_regex (options | PCRE_INGROUP,	/* Set for all nested groups */
			      ((options & PCRE_IMS) !=
			       (newoptions & PCRE_IMS)) ? newoptions &
			      PCRE_IMS : -1,	/* Pass ims options if changed */
			      brackets,	/* Capturing bracket count */
			      &code,	/* Where to put code (updated) */
			      &ptr,	/* Input pointer (updated) */
			      errorptr,	/* Where to put an error message */
			      FALSE,	/* Whether this is a lookbehind assertion */
			      skipbytes,		/* Skip over OP_BRANUMBER */
			      &subreqchar,	/* For possible last char */
			      &subcountlits,	/* For literal count */
			      cd,	/* Tables block */
			      compile_branch_posix))
	    goto FAILED;

	  /* At the end of compiling, code is still pointing to the start of the
	     group, while code has been updated to point past the end of the group
	     and any option resetting that may follow it. The pattern pointer (ptr)
	     is on the bracket. */

	  /* Handle updating of the required character. If the subpattern didn't
	     set one, leave it as it was. Otherwise, update it for normal brackets of
	     all kinds, forward assertions, and conditions with two branches. Don't
	     update the literal count for forward assertions, however. If the bracket
	     is followed by a quantifier with zero repeat, we have to back off. Hence
	     the definition of prevreqchar and subcountlits outside the main loop so
	     that they can be accessed for the back off. */

	  if (subreqchar > 0 && bravalue >= OP_BRA)
	    {
	      prevreqchar = *reqchar;
	      *reqchar = subreqchar;
	      *countlits += subcountlits;
	    }

	  /* Error if hit end of pattern */

	  if (ptr > cd->end || *ptr != ')')
	    {
	      *errorptr = pcre_estrings[14];
	      goto FAILED;
	    }
	  break;

	  /* Check \ for being a real metacharacter; if not, fall through and handle
	     it as a data character at the start of a string. Escape items are checked
	     for validity in the pre-compiling pass. */

	case '\\':
	  tempptr = ptr;
	  c = check_escape (&ptr, errorptr, *brackets, options, FALSE, cd);

	  /* Handle metacharacters introduced by \. For ones like \d, the ESC_ values
	     are arranged to be the negation of the corresponding OP_values. For the
	     back references, the values are ESC_REF plus the reference number. Only
	     back references and those types that consume a character may be repeated. */

	  if (c < 0)
	    {
	      c = -c;
	      if (c >= ESC_REF)
		{
		  int number = c - ESC_REF;
		  previous = code;
		  *code++ = OP_REF;
		  *code++ = number >> 8;
		  *code++ = number & 255;
		}
	      else if (c >= ESC_FIRST_CONSUME && c <= ESC_LAST_CONSUME)
		{
		  c -= ESC_FIRST_CONSUME;
		  previous = code;
		  *code++ = (c & 1) ? OP_TYPE : OP_TYPENOT;
		  *code++ = (c >> 1) + 1;
		}
	      else
		{
		  previous = NULL;
		  *code++ = c;
		}
	      continue;
	    }

	  /* Data character: reset and fall through */

	  ptr = tempptr;
	  c = '\\';

	  /* Handle a run of data characters until a metacharacter is encountered.
	     The first character is guaranteed not to be whitespace or # when the
	     extended flag is set. */

	NORMAL_CHAR:
	default:
	  previous = code;
	  *code = OP_CHARS;
	  code += 2;
	  length = 0;

	  do
	    {
	      /* Backslash may introduce a data char or a metacharacter. Escaped items
	         are checked for validity in the pre-compiling pass. Stop the string
	         before a metaitem. */

	      if (c == '\\')
		{
		  tempptr = ptr;
		  c =
		    check_escape (&ptr, errorptr, *brackets, options,
				  FALSE, cd);
		  if (c < 0)
		    {
		      ptr = tempptr - 1;
		      c = '\\';
		      break;
		    }
		}

	      /* Ordinary character or single-char escape */

	      *code++ = c;
	      cd->max_match_size++;
	      length++;
	      c = 0;
	    }

	  /* This "while" is the end of the "do" above. */

	  while (length < MAXLIT && ptr < cd->end &&
		 (cd->ctypes[c = *(++ptr)] & ctype_meta) == 0);

	  /* Update the last character and the count of literals */

	  prevreqchar = (length > 1) ? code[-2] : *reqchar;
	  *reqchar = code[-1];
	  *countlits += length;

	  /* Compute the length and set it in the data vector, and advance to
	     the next state. */

	  previous[1] = length;
	  if (cd->ctypes[c] & ctype_meta)
	    ptr--;
	  break;
	}
    }				/* end of big loop */

SUCCEEDED:
  *codeptr = code;
  *ptrptr = ptr;
  return TRUE;

  /* Control never reaches here by falling through, only by a goto for all the
     error states. Pass back the position in the pattern so that it can be displayed
     to the user for diagnosing the error. */

FAILED:
  *ptrptr = ptr;
  return FALSE;
}



/************************************************
*      Handle Basic Regular Expressions          *
*************************************************/



char *
basic_to_extended_regexp (s, patlen)
     const char *s;
     int *patlen;
{
  char *d, *base;
  const char *start, *end;
  int literal;

  start = s;
  base = d = (pcre_malloc) (*patlen * 2 + 1);
  end = s + *patlen;

  while (s < end)
    switch (*s)
      {
      case '\\':
	s++;
	if (s == end)
	  {
	    *d++ = '\\';
	    break;
	  }

	/* Remove the slash -- e.g. \+ --> + except before: ... */
	switch (*s)
	  {
	  case '<':		/* characters that form backslashed GNU */
	  case '>':		/* extensions */
	  case '\'':
	  case '`':
	  case 'b':
	  case 'B':
	  case 'G':
	  case 's':
	  case 'S':
	  case 'w':
	  case 'W':

	  case '1':		/* digits */
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
	    *d++ = '\\';
	    *d++ = *s++;
	    literal = 0;
	    break;

	  case '+':
	  case '{':
	    if (!literal && s != start)
	      {
		if (s[-1] == '^' || s[-1] == '(')
		  /* Brace and plus are literals in ^\{, ^\+, (\{, (\+ */
		  literal = 1;
	      }
	    else
	      literal = 0;
	    
	    *d++ = *s++;
	    break;

	  case '.':		/* characters that are special in basic regexps too */
	  case '*':
	  case '[':
	  case '^':
	  case '$':

	  case 'a':		/* characters that must be escaped in basic */
	  case 'f':		/* regexps too based on GNU sed 3.02.80 */
	  case 'e':
	  case 'n':
	  case 'r':
	  case 't':
	  case 'v':
	  case 'd':
	  case 'o':
	  case 'x':
	  case '\n':
	  case '\\':
	    *d++ = '\\';

	  default:
	    *d++ = *s++;
	    literal = 1;
	    break;
	  }
	break;

	/* Skip character classes */
      case '[':
	literal = 1;
	*d++ = *s++;
	if (s < end && *s == '^')
	  *d++ = *s++;
	if (s < end && *s == ']')
	  *d++ = *s++;
	while (s < end && *s != ']')
	  *d++ = *s++;
	if (s < end && *s == ']')
	  *d++ = *s++;
	break;

      case '^':
	literal = 0;
	*d++ = *s++;
	break;

      case '*':
	if (!literal && s != start)
	  {
	    if (s[-1] == '^' || s[-1] == '(')
	      /* ^* and (* are a literal * */
	      literal = 1;
	  }
	else
	  literal = 0;

	*d++ = *s++;
	break;

	/* Add a slash if the character is special in extended regexps */
      case '+':
      case '?':
      case '|':
      case '(':
      case ')':
      case '{':
      case '}':
	*d++ = '\\';

	/* Fall through */

      default:
	literal = 1;
	*d++ = *s++;
      }

  *patlen = d - base;
  return base;
}

void
basic_regexp_erroroffset (s, offset)
     const char *s;
     int *offset;
{
  (pcre_free) (basic_to_extended_regexp (s, offset));
}


/*************************************************
*        Compile a Regular Expression            *
*************************************************/

/* This function takes a string and returns a pointer to a block of store
   holding a compiled version of the expression.

   Arguments:
   pattern      the regular expression
   options      various option bits
   errorptr     pointer to pointer to error text
   erroroffset  ptr offset in pattern where error was detected
   tables       pointer to character tables or NULL

   Returns:       pointer to compiled data block, or NULL on error,
   with errorptr and erroroffset set
 */

pcre *
pcre_posix_compile (pattern, options, errorptr, erroroffset, tables)
     const char *pattern;
     int options;
     const char **errorptr;
     int *erroroffset;
     const unsigned char *tables;
{
  return pcre_posix_compile_nuls (pattern, (int) strlen (pattern), options,
				  errorptr, erroroffset, tables);
}

pcre *
pcre_posix_compile_nuls (pattern, patlen, options,
			 errorptr, erroroffset, tables)
     const char *pattern;
     int patlen;
     int options;
     const char **errorptr;
     int *erroroffset;
     const unsigned char *tables;
{
  pcre *re;
  int length = 3;		/* For initial BRA plus length */
  int runlength;
  int c, reqchar, countlits, prev;
  int bracount = 0;
  int top_backref = 0;
  int branch_extra = 0;
  int branch_newextra;
  unsigned int brastackptr = 0;
  size_t size;
  uschar *code;
  const uschar *ptr;
  compile_data compile_block;
  int brastack[BRASTACK_SIZE];
  uschar bralenstack[BRASTACK_SIZE];
  static const unsigned char *def_tables;

#ifdef DEBUG
  uschar *code_base, *code_end;
#endif

  /* We can't pass back an error message if errorptr is NULL; I guess the best we
     can do is just return NULL. */
  if (errorptr == NULL)
    return NULL;
  *errorptr = NULL;

  /* However, we can give a message for this error */
  if (erroroffset == NULL)
    {
      *errorptr = pcre_estrings[16];
      return NULL;
    }
  *erroroffset = 0;

  if ((options & ~PUBLIC_OPTIONS) != 0)
    {
      *errorptr = pcre_estrings[17];
      return NULL;
    }

  /* Call ourselves recursively with an extended regular expression if we were
     given a basic one.  Extended regular expressions are similar to Perl's
     than basic regular expression, so we share most of the code with regperl.c
     if we work this way. */
  if (!(options & PCRE_EXTENDED))
    {
      uschar *ere = basic_to_extended_regexp (pattern, &patlen);
      re = pcre_posix_compile_nuls (ere, patlen, options | PCRE_EXTENDED,
				    errorptr, erroroffset, tables);

      if (*errorptr != NULL)
	basic_regexp_erroroffset (ere, erroroffset);
      (pcre_free) (ere);
      return re;
    }

  /* Set up pointers to the individual character tables */
  if (!tables)
    {
      if (!def_tables)
	def_tables = pcre_maketables ();
      tables = def_tables;
    }
  compile_block.lcc = tables + lcc_offset;
  compile_block.fcc = tables + fcc_offset;
  compile_block.cbits = tables + cbits_offset;
  compile_block.ctypes = tables + ctypes_offset;
  compile_block.end = pattern + patlen - 1;
  compile_block.max_match_size = 0;

  /* Reflect pattern for debugging output */
  DPRINTF (
	   ("------------------------------------------------------------------\n"));
  DFWRITE ((pattern, 1, patlen, stdout));
  DPRINTF (("\n"));

  /* The first thing to do is to make a pass over the pattern to compute the
     amount of store required to hold the compiled code. This does not have to be
     perfect as long as errors are overestimates. At the same time we can detect any
     internal flag settings. */
  for (prev = -1, ptr = (const uschar *) pattern;
       ptr <= compile_block.end;
       prev = c, ptr++)
    {
      int min = 1, max = 1;
      int class_charcount;
      int bracket_length;

      c = *ptr;
      switch (c)
	{
	  /* A backslashed item may be an escaped "normal" character or a
	     character type. For a "normal" character, put the pointers and
	     character back so that tests for whitespace etc. in the input
	     are done correctly. */

	case '\\':
	  {
	    const uschar *save_ptr = ptr;
	    if (ptr == compile_block.end)
	      *errorptr = pcre_estrings[1];
	    else
	      c = check_escape (&ptr, errorptr, bracount, options,
				FALSE, &compile_block);

	    if (*errorptr != NULL)
	      goto PCRE_ERROR_RETURN;
	    if (c >= 0)
	      {
		ptr = save_ptr;
		c = '\\';
		goto NORMAL_CHAR;
	      }
	  }

	  /* A back reference needs an additional 2 bytes, plus either zero or 4
	     bytes for a repeat. We also need to keep the value of the highest
	     back reference. */

	  if (c <= -ESC_REF)
	    {
	      int refnum = -c - ESC_REF;
	      if (refnum > top_backref)
		top_backref = refnum;
	      length += 3;	/* For single back reference */
	      if (ptr < compile_block.end && ptr[1] == '{'
		  && is_counted_repeat (ptr + 2, &compile_block))
		{
		  ptr =
		    read_repeat_counts (ptr + 2, &min, &max, errorptr,
					&compile_block);
		  if (*errorptr != NULL)
		    goto PCRE_ERROR_RETURN;
		  if ((min == 0 && (max == 1 || max == -1)) ||
		      (min == 1 && max == -1))
		    ;
		  else
		    length += 4;
		}
	    }
	  else if (c <= 0)
	    length += 2;
	  else
	    length++;
	  continue;

	case '*':		/* These repeats won't be after brackets; */
	case '+':		/* those are handled separately */
	case '?':
	  if (prev == -1)
	    goto NORMAL_CHAR;

	case '.':
          length += (ptr != compile_block.end && *(ptr + 1) == '{') ? 2 : 1;
	  continue;

	  /* This covers the cases of repeats after a single char, metachar, class,
	     or back reference. */

	case '{':
	  if (!is_counted_repeat (ptr + 1, &compile_block))
	    goto NORMAL_CHAR;
	  ptr =
	    read_repeat_counts (ptr + 1, &min, &max, errorptr,
				&compile_block);
	  if (*errorptr != NULL)
	    goto PCRE_ERROR_RETURN;
	  if ((min == 0 && (max == 1 || max == -1)) ||
	      (min == 1 && max == -1))
	    length++;
	  else
	    {
	      length--;		/* Uncount the original char or metachar */
	      if (min == 1)
		length++;
	      else if (min > 0)
		length += 4;
	      if (max > 0)
		length += 4;
	      else
		length += 2;
	    }
	  continue;

	  /* An alternation contains an offset to the next branch or ket. If any ims
	     options changed in the previous branch(es), extra space will be
	     needed at the start of the branch. This is handled by branch_extra. */

	case '|':
	  length += 3 + branch_extra;
	  continue;

	  /* A character class uses 33 characters. Don't worry about character types
	     that aren't allowed in classes - they'll get picked up during the compile.
	     A character class that contains only one character uses 2 or 3 bytes,
	     depending on whether it is negated or not. Notice this where we can. */

	case '[':
	  class_charcount = 0;
	  if (ptr < compile_block.end && *(++ptr) == '^')
	    ptr++;

	  if (ptr <= compile_block.end)
	    do
	      {
		if (*ptr == '\\')
		  {
		    if (ptr == compile_block.end)
		      {
			/* Lone backslash, signal an error */
			*errorptr = pcre_estrings[1];
			goto PCRE_ERROR_RETURN;
		      }
		    else
		      {
			int i;

			/* Inside a class, literal escapes are interpreted as a
			   separate backslash followed by another character. */
			c = ptr[1];
			if (c >= ' ' && c <= 'z' &&
			    (i == escapes[c - ' '] > 0) && i != c)
			  {
			    class_charcount++;
			    ptr++;
			  }
		      }
		  }

		class_charcount++;
		ptr++;
	      }
	    while (ptr <= compile_block.end && *ptr != ']');

	  /* Repeats for negated single chars are handled by the general code */

	  if (class_charcount == 1)
	    length += 3;
	  else
	    {
	      length += 33;

	      /* A repeat needs either 0 or 4 bytes. */

	      if (ptr < compile_block.end && ptr[1] == '{'
		  && is_counted_repeat (ptr + 2, &compile_block))
		{
		  ptr =
		    read_repeat_counts (ptr + 2, &min, &max, errorptr,
					&compile_block);
		  if (*errorptr != NULL)
		    goto PCRE_ERROR_RETURN;
		  if ((min == 0 && (max == 1 || max == -1)) ||
		      (min == 1 && max == -1))
		    ;
		  else
		    length += 4;
		}
	    }
	  continue;

	  /* Brackets may be genuine groups or special things */

	case '(':
	  branch_newextra = 0;
	  bracket_length = 3;

	  /* Capturing brackets must be counted so we can process escapes in a
	     Perlish way. */

	  bracount++;
	  if (bracount > EXTRACT_BASIC_MAX) 
	    bracket_length += 3;

	  /* Save length for computing whole length at end if there's a repeat that
	     requires duplication of the group. Also save the current value of
	     branch_extra, and start the new group with the new value. If non-zero, this
	     will either be 2 for a (?imsx: group, or 3 for a lookbehind assertion. */

	  if (brastackptr >= sizeof (brastack) / sizeof (int))
	    {
	      *errorptr = pcre_estrings[19];
	      goto PCRE_ERROR_RETURN;
	    }

	  bralenstack[brastackptr] = branch_extra;
	  branch_extra = branch_newextra;

	  brastack[brastackptr++] = length;
	  length += bracket_length;

	  /* Treat a *+? after a ( as a literal char */
	  c = -1;
	  continue;

	  /* Handle ket. Look for subsequent max/min; for certain sets of values we
	     have to replicate this bracket up to that many times. If brastackptr is
	     0 this is an unmatched bracket which will generate an error, but take care
	     not to try to access brastack[-1] when computing the length and restoring
	     the branch_extra value. */

	case ')':
	  length += 3;
	  {
	    int minval = 1;
	    int maxval = 1;
	    int duplength;

	    if (brastackptr > 0)
	      {
		duplength = length - brastack[--brastackptr];
		branch_extra = bralenstack[brastackptr];
	      }
	    else
	      duplength = 0;

	    /* Leave ptr at the final char; for read_repeat_counts this happens
	       automatically; for the others we need an increment. */

	    if (ptr == compile_block.end)
	      {
	      }
	    else if ((c = ptr[1]) == '{' &&
		     is_counted_repeat (ptr + 2, &compile_block))
	      {
		ptr =
		  read_repeat_counts (ptr + 2, &minval, &maxval, errorptr,
				      &compile_block);
		if (*errorptr != NULL)
		  goto PCRE_ERROR_RETURN;
	      }
	    else if (c == '*')
	      {
		minval = 0;
		maxval = -1;
		ptr++;
	      }
	    else if (c == '+')
	      {
		maxval = -1;
		ptr++;
	      }
	    else if (c == '?')
	      {
		minval = 0;
		ptr++;
	      }

	    /* If the minimum is zero, we have to allow for an OP_BRAZERO before the
	       group, and if the maximum is greater than zero, we have to replicate
	       maxval-1 times; each replication acquires an OP_BRAZERO plus a nesting
	       bracket set - hence the 7. */

	    if (minval == 0)
	      {
		length++;
		if (maxval > 0)
		  length += (maxval - 1) * (duplength + 7);
	      }

	    /* When the minimum is greater than zero, 1 we have to replicate up to
	       minval-1 times, with no additions required in the copies. Then, if
	       there is a limited maximum we have to replicate up to maxval-1 times
	       allowing for a BRAZERO item before each optional copy and nesting
	       brackets for all but one of the optional copies. */

	    else
	      {
		length += (minval - 1) * duplength;
		if (maxval > minval)	/* Need this test as maxval=-1 means no limit */
		  length += (maxval - minval) * (duplength + 7) - 6;
	      }
	  }
	  continue;

	  /* Non-special character. For a run of such characters the length required
	     is the number of characters + 2, except that the maximum run length is 255.
	     We won't get a skipped space or a non-data escape or the start of a #
	     comment as the first character, so the length can't be zero.
	     We put carets and dollars here because they can be either metacharacters
	     or literal characters; determining which they are is indeed possible
	     but an extra chore -- so we always count them as literal characters
	     (which take more space than metacharacters). */

	case '^':
	case '$':
	NORMAL_CHAR:
	default:
	  length += 2;
	  runlength = 0;
	  do
	    {
	      /* Backslash may introduce a data char or a metacharacter; stop the
	         string before the latter. */

	      if (c == '\\')
		{
		  const uschar *saveptr = ptr;
		  c =
		    check_escape (&ptr, errorptr, bracount, options, FALSE,
				  &compile_block);
		  if (*errorptr != NULL)
		    goto PCRE_ERROR_RETURN;
		  if (c < 0)
		    {
		      ptr = saveptr - 1;
		      c = '\\';
		      break;
		    }
		}

	      /* Ordinary character or single-char escape */

	      runlength++;
	      c = 0;
	    }
	  /* This "while" is the end of the "do" above. */

	  while (runlength < MAXLIT && ptr <= compile_block.end &&
		 (compile_block.ctypes[c = *(++ptr)] & ctype_meta) == 0);

	  if (compile_block.ctypes[c] & ctype_meta)
	    ptr--;
	  length += runlength;
	  continue;
	}
    }

  length += 4;			/* For final KET and END */

  if (length > 65539)
    {
      *errorptr = pcre_estrings[20];
      return NULL;
    }

  /* Compute the size of data block needed and get it, either from malloc or
     externally provided function. We specify "code[0]" in the offsetof() expression
     rather than just "code", because it has been reported that one broken compiler
     fails on "code" because it is also an independent variable. It should make no
     difference to the value of the offsetof(). */
  size = length + offsetof (pcre, code[0]);
  re = (pcre *) (pcre_malloc) (size);

  if (re == NULL)
    {
      *errorptr = pcre_estrings[21];
      return NULL;
    }

  /* Put in the magic number, and save the size, options, and table pointer */
  re->magic_number = MAGIC_NUMBER;
  re->size = size;
  re->options = options;
  re->tables = tables;

  /* Set up a starting, non-capturing bracket, then compile the expression. On
     error, *errorptr will be set non-NULL, so we don't need to look at the result
     of the function here. */
  ptr = (const uschar *) pattern;
  code = re->code;
  *code = OP_BRA;
  bracount = 0;
  (void) compile_regex (options, -1, &bracount, &code, &ptr, errorptr, FALSE,
			0, &reqchar, &countlits, &compile_block, compile_branch_posix);
  re->top_bracket = bracount;
  re->top_backref = top_backref;

  /* If not reached end of pattern on success, there's an excess bracket. */
  if (*errorptr == NULL && ptr <= compile_block.end)
    *errorptr = pcre_estrings[22];

  /* Fill in the terminating state and check for disastrous overflow, but
     if debugging, leave the test till after things are printed out. */
  *code++ = OP_END;

#ifndef DEBUG
  if (code - re->code > length)
    *errorptr = pcre_estrings[23];
#endif

  /* Give an error if there's back reference to a non-existent capturing
     subpattern. */
  if (top_backref > re->top_bracket)
    *errorptr = pcre_estrings[15];

  /* Failed to compile */
  if (*errorptr != NULL)
    {
      (pcre_free) (re);
    PCRE_ERROR_RETURN:
      *erroroffset = ptr - (const uschar *) pattern;
      return NULL;
    }

  /* If the anchored option was not passed, set flag if we can determine that the
     pattern is anchored by virtue of ^ characters or \A or anything else (such as
     starting with .* when DOTALL is set).

     Otherwise, see if we can determine what the first character has to be, because
     that speeds up unanchored matches no end. If not, see if we can set the
     PCRE_STARTLINE flag. This is helpful for multiline matches when all branches
     start with ^. and also when all branches start with .* for non-DOTALL matches.  */
  if ((options & PCRE_ANCHORED) == 0)
    {
      int temp_options = options;
      if (is_anchored (re->code, &temp_options))
	re->options |= PCRE_ANCHORED;
      else
	{
	  int ch = find_firstchar (re->code, &temp_options);
	  if (ch >= 0)
	    {
	      re->first_char = ch;
	      re->options |= PCRE_FIRSTSET;
	    }
	  else if (is_startline (re->code))
	    re->options |= PCRE_STARTLINE;
	}
    }

  /* If we cannot determine the maximum match size or it is useless,
     set the field to -1 */
  if (compile_block.max_match_size < 0 || !is_endline(re->code))
    re->max_match_size = -1;
  else
    re->max_match_size = compile_block.max_match_size;

  /* Save the last required character if there are at least two literal
     characters on all paths, or if there is no first character setting. */
  if (reqchar >= 0 && (countlits > 1 || (re->options & PCRE_FIRSTSET) == 0))
    {
      re->req_char = reqchar;
      re->options |= PCRE_REQCHSET;
    }

  /* Print out the compiled data for debugging */
#ifdef DEBUG
  pcre_debug (re);

  if (code - re->code > length)
    {
      *errorptr = pcre_estrings[23];
      (pcre_free) (re);
      *erroroffset = ptr - (uschar *) pattern;
      return NULL;
    }
#endif

  if (*errorptr && (options & PCRE_ENGLISH_ERRORS) == 0)
    *errorptr = gettext (*errorptr);

  return (pcre *) re;
}
