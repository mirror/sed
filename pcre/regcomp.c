/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/*
   This is a library of functions to support regular expressions whose syntax
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



/* Tables of names of POSIX character classes and their lengths. The list is
   terminated by a zero length entry. The first three must be alpha, upper, lower,
   as this is assumed for handling case independence. */

static const char *posix_names[] =
{
  "alpha", "lower", "upper",
  "alnum", "ascii", "cntrl", "digit", "graph",
  "print", "punct", "space", "word", "xdigit",
  "blank"
};

static const uschar posix_name_lengths[] =
{
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 6, 5, 0
};

static const int posix_class_maps[] =
{
  cbit_lower, cbit_upper, -1,   /* alpha */
  cbit_lower, -1, -1,           /* lower */
  cbit_upper, -1, -1,           /* upper */
  cbit_digit, cbit_lower, cbit_upper,   /* alnum */
  cbit_print, cbit_cntrl, -1,   /* ascii */
  cbit_cntrl, -1, -1,           /* cntrl */
  cbit_digit, -1, -1,           /* digit */
  cbit_graph, -1, -1,           /* graph */
  cbit_print, -1, -1,           /* print */
  cbit_punct, -1, -1,           /* punct */
  cbit_space, -1, -1,           /* space */
  cbit_word, -1, -1,            /* word */
  cbit_xdigit, -1, -1,          /* xdigit */
  cbit_blank, -1, -1            /* blank */
};



/*************************************************
*            Check for counted repeat            *
*************************************************/

/* This function is called when a '{' is encountered in a place where it might
   start a quantifier. It looks ahead to see if it really is a quantifier or not.
   It is only a quantifier if it is one of the forms {ddd} {ddd,} or {ddd,ddd}
   where the ddds are digits.

   Arguments:
   p         pointer to the first char after '{'
   cd        pointer to char tables block

   Returns:    TRUE or FALSE
 */

static BOOL
is_counted_repeat (p, cd)
     const uschar *p;
     compile_data *cd;
{
  if (p > cd->end)
    return FALSE;
  if ((cd->ctypes[*p++] & ctype_digit) == 0)
    return FALSE;
  while (p <= cd->end && (cd->ctypes[*p] & ctype_digit) != 0)
    p++;

  /* Check end of regexp */
  if (p > cd->end)
    return FALSE;
  if (p == cd->end)
    return (*p == '}');

  /* Not at end of regexp */
  if (*p == '}')
    return TRUE;
  if (*p++ != ',')
    return FALSE;

  /* Check again for end of regexp */
  if (p == cd->end)
    return (*p == '}');
  if (*p == '}')
    return TRUE;

  if (p > cd->end)
    return FALSE;
  if ((cd->ctypes[*p++] & ctype_digit) == 0)
    return FALSE;
  while (p <= cd->end && (cd->ctypes[*p] & ctype_digit) != 0)
    p++;

  return (p <= cd->end && *p == '}');
}



/*************************************************
*         Read repeat counts                     *
*************************************************/

/* Read an item of the form {n,m} and return the values. This is called only
   after is_counted_repeat() has confirmed that a repeat-count quantifier exists,
   so the syntax is guaranteed to be correct, but we need to check the values.

   Arguments:
   p          pointer to first char after '{'
   minp       pointer to int for min
   maxp       pointer to int for max
   returned as -1 if no max
   errorptr   points to pointer to error message
   cd         pointer to character tables clock

   Returns:     pointer to '}' on success;
   current ptr on error, with errorptr set
 */

static const uschar *
read_repeat_counts (p, minp, maxp, errorptr, cd)
     const uschar *p;
     int *minp;
     int *maxp;
     const char **errorptr;
     compile_data *cd;
{
  int min = 0;
  int max = -1;

  while ((cd->ctypes[*p] & ctype_digit) != 0)
    min = min * 10 + *p++ - '0';

  if (*p == '}')
    max = min;
  else
    {
      if (*(++p) != '}')
	{
	  max = 0;
	  while ((cd->ctypes[*p] & ctype_digit) != 0)
	    max = max * 10 + *p++ - '0';
	  if (max < min)
	    {
	      *errorptr = pcre_estrings[4];
	      return p;
	    }
	}
    }

  min *= *minp;
  if (max > 0)
    max *= *maxp;

  /* Do paranoid checks, then fill in the required variables, and pass back the
     pointer to the terminating '}'. */

  if (min > 65535 || max > 65535)
    *errorptr = pcre_estrings[5];
  else
    {
      *minp = min;
      *maxp = max;
    }

  return p;
}



/*************************************************
*        Find the fixed length of a pattern      *
*************************************************/

/* Scan a pattern and compute the fixed length of subject that will match it,
   if the length is fixed. This is needed for dealing with backward assertions.

   Arguments:
   code     points to the start of the pattern (the bracket)
   options  the compiling options

   Returns:   the fixed length, or -1 if there is no fixed length
 */

static int
find_fixedlength (code, options)
     uschar *code;
     int options;
{
  int length = -1;

  int branchlength = 0;
  uschar *cc = code + 3;

/* Scan along the opcodes for this branch. If we get to the end of the
   branch, check the length against that of the other branches. */

  for (;;)
    {
      int d;
      int op = *cc;
      if (op >= OP_BRA)
	op = OP_BRA;

      switch (op)
	{
	case OP_BRA:
	case OP_ONCE:
	case OP_COND:
	  d = find_fixedlength (cc, options);
	  if (d < 0)
	    return -1;
	  branchlength += d;
	  do
	    cc += (cc[1] << 8) + cc[2];
	  while (*cc == OP_ALT);
	  cc += 3;
	  break;

	  /* Reached end of a branch; if it's a ket it is the end of a nested
	     call. If it's ALT it is an alternation in a nested call. If it is
	     END it's the end of the outer call. All can be handled by the same code. */

	case OP_ALT:
	case OP_KET:
	case OP_KET_MAXSTAR:
	case OP_KET_MINSTAR:
	case OP_END:
	  if (length < 0)
	    length = branchlength;
	  else if (length != branchlength)
	    return -1;
	  if (*cc != OP_ALT)
	    return length;
	  cc += 3;
	  branchlength = 0;
	  break;

	  /* Skip over assertive subpatterns */

	case OP_ASSERT:
	case OP_ASSERT_NOT:
	case OP_ASSERTBACK:
	case OP_ASSERTBACK_NOT:
	  do
	    cc += (cc[1] << 8) + cc[2];
	  while (*cc == OP_ALT);
	  cc += 3;
	  break;

	  /* Skip over things that don't match chars */

	case OP_REVERSE:
	case OP_BRANUMBER:
	case OP_CREF:
	  cc++;
	  /* Fall through */

	case OP_OPT:
	  cc++;
	  /* Fall through */

	case OP_SOD:
	case OP_EOD:
	case OP_EODN:
	case OP_CIRC:
	case OP_DOLL:
	case OP_NOT_WORD_BOUNDARY:
	case OP_WORD_BOUNDARY:
	case OP_ANCHOR_MATCH:
	  cc++;
	  break;

	  /* Handle char strings. */

	case OP_CHARS:
	  branchlength += *(++cc);
	  cc += *cc + 1;
	  break;

	  /* Handle exact repetitions */

	case OP_EXACT:
	case OP_TYPEEXACT:
	  branchlength += (cc[1] << 8) + cc[2];
	  cc += 4;
	  break;

	  /* Handle single-char matchers */

	case OP_TYPE:
	case OP_TYPENOT:
	  branchlength++;
	  cc += 2;
	  break;

	case OP_ANY:
	  branchlength++;
	  cc++;
	  break;

	  /* Check a class for variable quantification */

	case OP_CLASS:
	  branchlength++;
	  cc += 33;
	  break;

	case OP_CL_MAXSTAR:
	case OP_CL_MINSTAR:
	case OP_CL_MAXQUERY:
	case OP_CL_MINQUERY:
	  return -1;

	case OP_CL_MAXRANGE:
	case OP_CL_MINRANGE:
	  cc += 33;
	  if ((cc[1] << 8) + cc[2] != (cc[3] << 8) + cc[4])
	    return -1;

	  branchlength += (cc[1] << 8) + cc[2];
	  cc += 4;
	  break;

	  /* Anything else is variable length */

	default:
	  return -1;
	}
    }
/* Control never gets here */
}




/*************************************************
*           Check for POSIX class syntax         *
*************************************************/

/* This function is called when the sequence "[:" or "[." or "[=" is
   encountered in a character class. It checks whether this is followed by an
   optional ^ and then a sequence of letters, terminated by a matching ":]" or
   ".]" or "=]".

   Argument:
   ptr      pointer to the initial [
   endptr   where to return the end pointer
   cd       pointer to compile data

   Returns:   TRUE or FALSE
 */

static BOOL
check_posix_syntax (ptr, endptr, cd)
     const uschar *ptr;
     const uschar **endptr;
     compile_data *cd;
{
  int terminator;		/* Don't combine these lines; the Solaris cc */
  terminator = *(++ptr);	/* compiler warns about "non-constant" initializer. */
  if (*(++ptr) == '^')
    ptr++;

  /* Use < because we have to look one character ahead */
  while (ptr < cd->end && (cd->ctypes[*ptr] & ctype_letter) != 0)
    ptr++;
  if (ptr < cd->end && *ptr == terminator && ptr[1] == ']')
    {
      *endptr = ptr;
      return TRUE;
    }
  return FALSE;
}




/*************************************************
*          Check POSIX class name                *
*************************************************/

/* This function is called to check the name given in a POSIX-style class entry
   such as [:alnum:].

   Arguments:
   ptr        points to the first letter
   len        the length of the name

   Returns:     a value representing the name, or -1 if unknown
 */

static int
check_posix_name (ptr, len)
     const uschar *ptr;
     int len;
{
  int yield = 0;
  while (posix_name_lengths[yield] != 0)
    {
      if (len == posix_name_lengths[yield] &&
	  strncmp ((const char *) ptr, posix_names[yield], len) == 0)
	return yield;
      yield++;
    }
  return -1;
}




/*************************************************
*     Compile sequence of alternatives           *
*************************************************/

/* On entry, ptr is pointing past the bracket character, but on return
   it points to the closing bracket, or vertical bar, or end of string.
   The code variable is pointing at the byte into which the BRA operator has been
   stored. If the ims options are changed at the start (for a (?ims: group) or
   during any branch, we need to insert an OP_OPT item at the start of every
   following branch to ensure they get set correctly at run time, and also pass
   the new options into every subsequent branch compile.

   Argument:
   options     the option bits
   optchanged  new ims options to set as if (?ims) were at the start, or -1
   for no change
   brackets    -> int containing the number of capturing brackets used
   codeptr     -> the address of the current code pointer
   ptrptr      -> the address of the current pattern pointer
   errorptr    -> pointer to error message
   lookbehind  TRUE if this is a lookbehind assertion
   skipbytes   skip this many bytes at start (for OP_COND, OP_BRANUMBER)
   reqchar     -> place to put the last required character, or a negative number
   countlits   -> place to put the shortest literal count of any branch
   cd          points to the data block with tables pointers

   Returns:      TRUE on success
 */

static BOOL
compile_regex (options, optchanged, brackets, codeptr, ptrptr, errorptr, lookbehind, skipbytes, reqchar, countlits, cd,
	       compile_branch_func)
     int options;
     int optchanged;
     int *brackets;
     uschar **codeptr;
     const uschar **ptrptr;
     const char **errorptr;
     BOOL lookbehind;
     int skipbytes;
     int *reqchar;
     int *countlits;
     compile_data *cd;
     BOOL (*compile_branch_func) (int, int *, uschar **, const uschar **, const char **, int *, int *, int *, compile_data *);
{
  const uschar *ptr = *ptrptr;
  uschar *code = *codeptr;
  uschar *last_branch = code;
  uschar *start_bracket = code;
  uschar *reverse_count = NULL;
  int oldoptions = options & PCRE_IMS;
  int branchreqchar, branchcountlits;

  *reqchar = -1;
  *countlits = INT_MAX;
  code += 3 + skipbytes;

/* Loop for each alternative branch */

  for (;;)
    {
      int length;

      /* Handle change of options */

      if (optchanged >= 0)
	{
	  *code++ = OP_OPT;
	  *code++ = optchanged;
	  options = (options & ~PCRE_IMS) | optchanged;
	}

      /* Set up dummy OP_REVERSE if lookbehind assertion */

      if (lookbehind)
	{
	  *code++ = OP_REVERSE;
	  reverse_count = code;
	  *code++ = 0;
	  *code++ = 0;
	}

      /* Now compile the branch */

      if (!compile_branch_func (options, brackets, &code, &ptr, errorptr, &optchanged,
			        &branchreqchar, &branchcountlits, cd))
	{
	  *ptrptr = ptr;
	  return FALSE;
	}

      /* Fill in the length of the last branch */

      length = code - last_branch;
      last_branch[1] = length >> 8;
      last_branch[2] = length & 255;

      /* Save the last required character if all branches have the same; a current
         value of -1 means unset, while -2 means "previous branch had no last required
         char".  */

      if (*reqchar != -2)
	{
	  if (branchreqchar >= 0)
	    {
	      if (*reqchar == -1)
		*reqchar = branchreqchar;
	      else if (*reqchar != branchreqchar)
		*reqchar = -2;
	    }
	  else
	    *reqchar = -2;
	}

      /* Keep the shortest literal count */

      if (branchcountlits < *countlits)
	*countlits = branchcountlits;
      DPRINTF (("literal count = %d min=%d\n", branchcountlits, *countlits));

      /* If lookbehind, check that this branch matches a fixed-length string,
         and put the length into the OP_REVERSE item. Temporarily mark the end of
         the branch with OP_END. */

      if (lookbehind)
	{
	  *code = OP_END;
	  length = find_fixedlength (last_branch, options);
	  DPRINTF (("fixed length = %d\n", length));
	  if (length < 0)
	    {
	      *errorptr = pcre_estrings[25];
	      *ptrptr = ptr;
	      return FALSE;
	    }
	  reverse_count[0] = (length >> 8);
	  reverse_count[1] = length & 255;
	}

      /* Reached end of expression, either ')' or end of pattern. Insert a
         terminating ket and the length of the whole bracketed item, and return,
         leaving the pointer at the terminating char. If any of the ims options
         were changed inside the group, compile a resetting op-code following. */

      if (ptr > cd->end || *ptr != '|')
	{
	  length = code - start_bracket;
	  *code++ = OP_KET;
	  *code++ = length >> 8;
	  *code++ = length & 255;
	  if (optchanged >= 0)
	    {
	      *code++ = OP_OPT;
	      *code++ = oldoptions;
	    }
	  *codeptr = code;
	  *ptrptr = ptr;
	  return TRUE;
	}

      /* Another branch follows; insert an "or" node and advance the pointer. */

      *code = OP_ALT;
      last_branch = code;
      code += 3;
      ptr++;
    }
/* Control never reaches here */
}




/*************************************************
*      Find first significant op code            *
*************************************************/

/* This is called by several functions that scan a compiled expression looking
   for a fixed first character, or an anchoring op code etc. It skips over things
   that do not influence this. For one application, a change of caseless option is
   important.

   Arguments:
   code       pointer to the start of the group
   options    pointer to external options
   optbit     the option bit whose changing is significant, or
   zero if none are
   optstop    TRUE to return on option change, otherwise change the options
   value and continue

   Returns:     pointer to the first significant opcode
 */

static const uschar *
first_significant_code (code, options, optbit, optstop)
     const uschar *code;
     int *options;
     int optbit;
     BOOL optstop;
{
  for (;;)
    {
      switch ((int) *code)
	{
	case OP_OPT:
	  if (optbit > 0 && ((int) code[1] & optbit) != (*options & optbit))
	    {
	      if (optstop)
		return code;
	      *options = (int) code[1];
	    }
	  code += 2;
	  break;

	case OP_CREF:
	case OP_BRANUMBER:
	  code += 3;
	  break;

	case OP_WORD_BOUNDARY:
	case OP_NOT_WORD_BOUNDARY:
	  code++;
	  break;

	case OP_ASSERT_NOT:
	case OP_ASSERTBACK:
	case OP_ASSERTBACK_NOT:
	  do
	    code += (code[1] << 8) + code[2];
	  while (*code == OP_ALT);
	  code += 3;
	  break;

	default:
	  return code;
	}
    }
/* Control never reaches here */
}




/*************************************************
*          Check for anchored expression         *
*************************************************/

/* Try to find out if this is an anchored regular expression. Consider each
   alternative branch. If they all start with OP_SOD or OP_CIRC, or with a bracket
   all of whose alternatives start with OP_SOD or OP_CIRC (recurse ad lib), then
   it's anchored. However, if this is a multiline pattern, then only OP_SOD
   counts, since OP_CIRC can match in the middle.

   A branch is also implicitly anchored if it starts with .* and DOTALL is set,
   because that will try the rest of the pattern at all possible matching points,
   so there is no point trying them again.

   Arguments:
   code       points to start of expression (the bracket)
   options    points to the options setting

   Returns:     TRUE or FALSE
 */

static BOOL
is_anchored (code, options)
     const uschar *code;
     int *options;
{
  do
    {
      const uschar *scode = first_significant_code (code + 3, options,
						    PCRE_MULTILINE, FALSE);
      int op = *scode;
      if (op >= OP_BRA || op == OP_ASSERT || op == OP_ONCE || op == OP_COND)
	{
	  if (!is_anchored (scode, options))
	    return FALSE;
	}
      else if ((op == OP_TYPE_MAXSTAR || op == OP_TYPE_MINSTAR) &&
	       (*options & PCRE_DOTALL) != 0)
	{
	  if (scode[1] != OP_ANY)
	    return FALSE;
	}
      else if (op != OP_SOD && op != OP_ANCHOR_MATCH
               && ((*options & PCRE_MULTILINE) != 0 || op != OP_CIRC))
        return FALSE;
      code += (code[1] << 8) + code[2];
    }
  while (*code == OP_ALT);
  return TRUE;
}



/*************************************************
*         Check for starting with ^ or .*        *
*************************************************/

/* This is called to find out if every branch starts with ^ or .* so that
   "first char" processing can be done to speed things up in multiline
   matching and for non-DOTALL patterns that start with .* (which must start at
   the beginning or after \n).

   Argument:  points to start of expression (the bracket)
   Returns:   TRUE or FALSE
 */

static BOOL
is_startline (code)
     const uschar *code;
{
  do
    {
      const uschar *scode = first_significant_code (code + 3, NULL, 0, FALSE);
      int op = *scode;
      if (op >= OP_BRA || op == OP_ASSERT || op == OP_ONCE || op == OP_COND)
	{
	  if (!is_startline (scode))
	    return FALSE;
	}
      else if (op == OP_TYPE_MAXSTAR || op == OP_TYPE_MINSTAR)
	{
	  if (scode[1] != OP_ANY)
	    return FALSE;
	}
      else if (op != OP_CIRC)
	return FALSE;
      code += (code[1] << 8) + code[2];
    }
  while (*code == OP_ALT);
  return TRUE;
}



/*************************************************
*             Check for ending with $            *
*************************************************/

/* This is called to find out if every branch ends with $ so that if the
   match can only have a limited length we can start the match directly
   at the end of the line.

   Argument:  points to start of expression (the bracket)
   Returns:   TRUE or FALSE
 */

static BOOL
is_endline (code)
     const uschar *code;
{
  BOOL was_dollar = FALSE, is_dollar = FALSE, found_dollar = FALSE;
  for (;;)
    {
      int op;

      was_dollar = is_dollar;
      is_dollar = FALSE;
      op = *code;
      if (op >= OP_BRA)
	op = OP_BRA;

      switch (op)
	{

	case OP_EODN:
	case OP_EOD:
	  found_dollar = is_dollar = TRUE;
	  /* Fall through */

	case OP_END:
	  return found_dollar;

	case OP_OPT:
	  code++;
	  break;

	case OP_COND:
	  code += 2;
	  break;

	case OP_CREF:
	  code++;
	  break;
	  
	case OP_CHARS:
	  code += code[1] + 2;
	  break;
	  
	case OP_KET_MAXSTAR:
	case OP_KET_MINSTAR:
	case OP_ALT:
	case OP_KET:
	  if (!was_dollar)
	    return FALSE;
	  
	case OP_ASSERT:
	case OP_ASSERT_NOT:
	case OP_ASSERTBACK:
	case OP_ASSERTBACK_NOT:
	case OP_ONCE:
	  code += 2;
	  break;
	  
	case OP_REVERSE:
	  code += 2;
	  break;
	  
	case OP_MAXSTAR:
	case OP_MINSTAR:
	case OP_MAXPLUS:
	case OP_MINPLUS:
	case OP_MAXQUERY:
	case OP_MINQUERY:
	case OP_TYPE_MAXSTAR:
	case OP_TYPE_MINSTAR:
	case OP_TYPE_MAXPLUS:
	case OP_TYPE_MINPLUS:
	case OP_TYPE_MAXQUERY:
	case OP_TYPE_MINQUERY:
	  code++;
	  break;
	  
	case OP_EXACT:
	case OP_MAXUPTO:
	case OP_MINUPTO:
	  code += 3;
	  break;
	  
	case OP_TYPEEXACT:
	case OP_TYPE_MAXUPTO:
	case OP_TYPE_MINUPTO:
	  code += 3;
	  break;
	  
	case OP_NOT:
	  code++;
	  break;
	  
	case OP_NOT_MAXSTAR:
	case OP_NOT_MINSTAR:
	case OP_NOT_MAXPLUS:
	case OP_NOT_MINPLUS:
	case OP_NOT_MAXQUERY:
	case OP_NOT_MINQUERY:
	  code++;
	  break;
	  
	case OP_NOTEXACT:
	case OP_NOT_MAXUPTO:
	case OP_NOT_MINUPTO:
	  code += 3;
	  break;
	  
	case OP_CL_MAXSTAR:
	case OP_CL_MINSTAR:
	case OP_CL_MAXPLUS:
	case OP_CL_MINPLUS:
	case OP_CL_MAXQUERY:
	case OP_CL_MINQUERY:
	case OP_CLASS:
	  code += 32;
	  break;

	case OP_CL_MAXRANGE:
	case OP_CL_MINRANGE:
	  code += 36;
	  break;

	case OP_REF:
	case OP_REF_MAXSTAR:
	case OP_REF_MINSTAR:
	case OP_REF_MAXPLUS:
	case OP_REF_MINPLUS:
	case OP_REF_MAXQUERY:
	case OP_REF_MINQUERY:
	  code++;
	  break;

	case OP_REF_MAXRANGE:
	case OP_REF_MINRANGE:
	  code += 5;
	  break;

	case OP_BRA:
	  code += 3;
	  break;

	  /* Anything else is just a one-node item */
	  
	default:
	  code++;
	  break;
	}
    }
}



/*************************************************
*          Check for fixed first char            *
*************************************************/

/* Try to find out if there is a fixed first character. This is called for
   unanchored expressions, as it speeds up their processing quite considerably.
   Consider each alternative branch. If they all start with the same char, or with
   a bracket all of whose alternatives start with the same char (recurse ad lib),
   then we return that char, otherwise -1.

   Arguments:
   code       points to start of expression (the bracket)
   options    pointer to the options (used to check casing changes)

   Returns:     -1 or the fixed first char
 */

static int
find_firstchar (code, options)
     const uschar *code;
     int *options;
{
  int c = -1;
  do
    {
      int d;
      const uschar *scode = first_significant_code (code + 3, options,
						    PCRE_CASELESS, TRUE);
      int op = *scode;

      if (op >= OP_BRA)
	op = OP_BRA;

      switch (op)
	{
	default:
	  return -1;

	case OP_BRA:
	case OP_ASSERT:
	case OP_ONCE:
	case OP_COND:
	  if ((d = find_firstchar (scode, options)) < 0)
	    return -1;
	  if (c < 0)
	    c = d;
	  else if (c != d)
	    return -1;
	  break;

	case OP_EXACT:		/* Fall through */
	  scode++;

	case OP_CHARS:		/* Fall through */
	  scode++;

	case OP_MAXPLUS:
	case OP_MINPLUS:
	  if (c < 0)
	    c = scode[1];
	  else if (c != scode[1])
	    return -1;
	  break;
	}

      code += (code[1] << 8) + code[2];
    }
  while (*code == OP_ALT);
  return c;
}
