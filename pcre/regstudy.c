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

uschar pruned[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  0,
  OP_ONCESTAR,    OP_ONCESTAR,    OP_ONCESTAR,
  OP_ONCEPLUS,    OP_ONCEPLUS,    OP_ONCEPLUS,
  OP_ONCEQUERY,   OP_ONCEQUERY,   OP_ONCEQUERY,
  OP_ONCEUPTO,    OP_ONCEUPTO,    OP_ONCEUPTO,
  0,
  
  0,
  OP_NOT_ONCESTAR,    OP_NOT_ONCESTAR,    OP_NOT_ONCESTAR,
  OP_NOT_ONCEPLUS,    OP_NOT_ONCEPLUS,    OP_NOT_ONCEPLUS,
  OP_NOT_ONCEQUERY,   OP_NOT_ONCEQUERY,   OP_NOT_ONCEQUERY,
  OP_NOT_ONCEUPTO,    OP_NOT_ONCEUPTO,    OP_NOT_ONCEUPTO,
  0,
  
  0,
  OP_TYPE_ONCESTAR,    OP_TYPE_ONCESTAR,    OP_TYPE_ONCESTAR,
  OP_TYPE_ONCEPLUS,    OP_TYPE_ONCEPLUS,    OP_TYPE_ONCEPLUS,
  OP_TYPE_ONCEQUERY,   OP_TYPE_ONCEQUERY,   OP_TYPE_ONCEQUERY,
  OP_TYPE_ONCEUPTO,    OP_TYPE_ONCEUPTO,    OP_TYPE_ONCEUPTO,
  0,
  
  0,
  OP_TYPENOT_ONCESTAR,    OP_TYPENOT_ONCESTAR,    OP_TYPENOT_ONCESTAR,
  OP_TYPENOT_ONCEPLUS,    OP_TYPENOT_ONCEPLUS,    OP_TYPENOT_ONCEPLUS,
  OP_TYPENOT_ONCEQUERY,   OP_TYPENOT_ONCEQUERY,   OP_TYPENOT_ONCEQUERY,
  OP_TYPENOT_ONCEUPTO,    OP_TYPENOT_ONCEUPTO,    OP_TYPENOT_ONCEUPTO,
  0,
  
  0,
  OP_CL_ONCESTAR,    OP_CL_ONCESTAR,    OP_CL_ONCESTAR,
  OP_CL_ONCEPLUS,    OP_CL_ONCEPLUS,    OP_CL_ONCEPLUS,
  OP_CL_ONCEQUERY,   OP_CL_ONCEQUERY,   OP_CL_ONCEQUERY,
  OP_CL_ONCERANGE,   OP_CL_ONCERANGE,   OP_CL_ONCERANGE,
  
  0,
  OP_REF_ONCESTAR,    OP_REF_ONCESTAR,    OP_REF_ONCESTAR,
  OP_REF_ONCEPLUS,    OP_REF_ONCEPLUS,    OP_REF_ONCEPLUS,
  OP_REF_ONCEQUERY,   OP_REF_ONCEQUERY,   OP_REF_ONCEQUERY,
  OP_REF_ONCERANGE,   OP_REF_ONCERANGE,   OP_REF_ONCERANGE,
  
  0, 0, 0,
  
  OP_KET_ONCESTAR, OP_KET_ONCESTAR, OP_KET_ONCESTAR
};


/*************************************************
*   Set an entry and maybe its alternate case    *
*************************************************/

/* Given a character, set its bit in the table, and also the bit for the other
   version of a letter if we are caseless.

   Arguments:
   bmtable       points to the Boyer-Moore table
   c             is the character
   length        the value of the entry.
   caseless      the caseless flag
   cd            the block with char table pointers

   Returns:        nothing
 */

static void
set_bmtable (bmtable, c, length, caseless, cd)
     uschar *bmtable;
     int c;
     int length;
     BOOL caseless;
     compile_data *cd;
{
  bmtable[c] = length;
  if (caseless && (cd->ctypes[c] & ctype_letter) != 0)
    {
      bmtable[cd->fcc[c]] = length;
    }
}




/*************************************************
*      Set a bit and maybe its alternate case    *
*************************************************/

/* Given a character, set its bit in the table, and also the bit for the other
   version of a letter if we are caseless.

   Arguments:
   bmtable       points to the bit map
   c             is the character
   caseless      the caseless flag
   cd            the block with char table pointers

   Returns:        nothing
 */

static void
set_bit (start_bits, c, caseless, cd)
     uschar *start_bits;
     int c;
     BOOL caseless;
     compile_data *cd;
{
  start_bits[c / 8] |= (1 << (c & 7));
  if (caseless && (cd->ctypes[c] & ctype_letter) != 0)
    start_bits[cd->fcc[c] / 8] |= (1 << (cd->fcc[c] & 7));
}



/*************************************************
*           Create map of starting string        *
*************************************************/

/* This function scans a compiled unanchored expression and attempts to build a
   map of the characters occurring in an initial fixed-length string. It
   returns the length of the string it finds. 

   Arguments:
   code         points to an expression
   bmtable      points to a 256-byte table, initialized to 0
   length       base value to be given to the table
   caseless     the current state of the caseless flag
   cd           the block with char table pointers

   Returns:       TRUE if table built, FALSE otherwise
 */

static int
set_boyer_moore (code, bmtable, length, caseless, cd)
     const uschar *code;
     uschar *bmtable;
     int length;
     BOOL caseless;
     compile_data *cd;
{
  const uschar *tcode = code + 3;
  BOOL try_next = TRUE;

  /* If the branch has more than one alternative, it's too
     complicated, so exit. */
  if (code[(code[1] << 8) + code[2]] == OP_ALT)
    return length;

  do
    {
      int i;

      switch (*tcode)
	{
	default:
	  /* If a branch starts with a bracket, recurse to set the string
	     from within them. That's all for this branch. */
	  if ((int) *tcode >= OP_BRA)
	    length = set_boyer_moore (tcode, bmtable, length, caseless, cd);
	  return length;

	  /* Skip over extended extraction bracket number */
		
	case OP_BRANUMBER:
	  tcode += 3;
	  break;

	  /* Skip over lookbehind assertions */

	case OP_ASSERTBACK:
	case OP_ASSERTBACK_NOT:
	  do
	    tcode += (tcode[1] << 8) + tcode[2];
	  while (*tcode == OP_ALT);
	  tcode += 3;
	  break;

	  /* Skip over an option setting, changing the caseless flag */

	case OP_OPT:
	  caseless = (tcode[1] & PCRE_CASELESS) != 0;
	  tcode += 2;
	  break;

	  /* BRAZERO does the bracket, but carries on. */

	case OP_BRAZERO:
	case OP_BRAMINZERO:
	  try_next = FALSE;
	  break;

	  /* Single-char * ? upto stops the search */

	case OP_MAXPLUS:
	case OP_MINPLUS:
	case OP_ONCEPLUS:
	  set_bmtable (bmtable, tcode[1], ++length, caseless, cd);

	case OP_MAXSTAR:
	case OP_MINSTAR:
	case OP_ONCESTAR:
	case OP_MAXQUERY:
	case OP_MINQUERY:
	case OP_ONCEQUERY:
	case OP_MAXUPTO:
	case OP_MINUPTO:
	case OP_ONCEUPTO:
	  try_next = FALSE;
	  break;

	  /* At least one single char sets the bit and stops */

	case OP_EXACT:
	  length += (tcode[1] << 8) + tcode[2];
	  if (length > 255)
	    length = 255;
	  set_bmtable (bmtable, tcode[3], length, caseless, cd);
	  tcode += 4;
	  break;

	case OP_CHARS:
	  {
	    int nchars = tcode[1];
	    if (nchars > 255 - length)
	      nchars = 255 - length;

	    tcode += 2;
	    while (nchars--)
	      set_bmtable (bmtable, *tcode++, ++length, caseless, cd);
	  }
	  break;

	  /* Single character type sets the bits and stops */

	case OP_TYPENOT:
	  tcode++;
	REPEATTYPENOT:
	  length++;
	  for (i = 0; i < 255; i++)
	    if (!*tcode || !(cd->ctypes[i] & (1 << *tcode)))
	      set_bmtable (bmtable, i, length, caseless, cd);

	  break;

	case OP_TYPE:
	  tcode++;
	REPEATTYPE:
	  length++;
	  for (i = 0; i < 255; i++)
	    if (cd->ctypes[i] & (*tcode ? 1 << *tcode : 0))
	      set_bmtable (bmtable, i, length, caseless, cd);

	  break;

	  /* One or more character type fudges the pointer and restarts, knowing
	     it will hit a single character. */

	case OP_TYPE_MAXPLUS:
	case OP_TYPE_MINPLUS:
	case OP_TYPE_ONCEPLUS:
	  tcode++;
	  try_next = FALSE;
	  goto REPEATTYPE;

	case OP_TYPEEXACT:
	  tcode += 3;
	  length += (tcode[1] << 8) + tcode[2] - 1;
	  if (length > 255)
	    length = 255;
	  goto REPEATTYPE;

	case OP_TYPENOT_MAXPLUS:
	case OP_TYPENOT_MINPLUS:
	case OP_TYPENOT_ONCEPLUS:
	  tcode++;
	  try_next = FALSE;
	  goto REPEATTYPENOT;

	case OP_TYPENOTEXACT:
	  tcode += 3;
	  length += (tcode[1] << 8) + tcode[2] - 1;
	  if (length > 255)
	    length = 255;
	  goto REPEATTYPENOT;

	  /* Zero or more repeats of character types stop here. */

	case OP_TYPE_MAXUPTO:
	case OP_TYPE_MINUPTO:
	case OP_TYPE_ONCEUPTO:
	case OP_TYPE_MAXSTAR:
	case OP_TYPE_MINSTAR:
	case OP_TYPE_ONCESTAR:
	case OP_TYPE_MAXQUERY:
	case OP_TYPE_MINQUERY:
	case OP_TYPE_ONCEQUERY:
	case OP_TYPENOT_MAXUPTO:
	case OP_TYPENOT_MINUPTO:
	case OP_TYPENOT_ONCEUPTO:
	case OP_TYPENOT_MAXSTAR:
	case OP_TYPENOT_MINSTAR:
	case OP_TYPENOT_ONCESTAR:
	case OP_TYPENOT_MAXQUERY:
	case OP_TYPENOT_MINQUERY:
	case OP_TYPENOT_ONCEQUERY:
	  try_next = FALSE;
	  break;

	  /* Character class: set the bits and either carry on or not,
	     according to the repeat count. */

	case OP_CLASS:
	case OP_CL_MAXRANGE:
	case OP_CL_MINRANGE:
	case OP_CL_ONCERANGE:
	case OP_CL_MAXSTAR:
	case OP_CL_MINSTAR:
	case OP_CL_ONCESTAR:
	case OP_CL_MAXPLUS:
	case OP_CL_MINPLUS:
	case OP_CL_ONCEPLUS:
	case OP_CL_MAXQUERY:
	case OP_CL_MINQUERY:
	case OP_CL_ONCEQUERY:
	  {
	    const uschar *classptr = tcode + 1;
	    tcode++;
	    switch (tcode[-1])
	      {
	      case OP_CL_MAXSTAR:
	      case OP_CL_MINSTAR:
	      case OP_CL_ONCESTAR:
	      case OP_CL_MAXQUERY:
	      case OP_CL_MINQUERY:
	      case OP_CL_ONCEQUERY:
		try_next = FALSE;
		break;
		      
	      case OP_CL_MAXRANGE:
	      case OP_CL_MINRANGE:
	      case OP_CL_ONCERANGE:
		try_next = FALSE;
		if (((tcode[0] << 8) + tcode[1]) == 0)
		  break;

		/* There is a minimum length, fall through */
		length += (tcode[0] << 8) + tcode[1] - 1;
		if (length > 255)
		  length = 255;

		tcode += 4;

	      case OP_CL_MAXPLUS:
	      case OP_CL_MINPLUS:
	      case OP_CL_ONCEPLUS:
		try_next = FALSE;
		      
	      default:
		length++;
		for (i = 0; i < 255; i++)
		  if (classptr[i >> 3] & (1 << (i & 7)))
		    set_bmtable (bmtable, i, length, caseless, cd);
		break;
	      }

	    tcode += 32;
	    break;		/* End of class handling */
	  }
	}			/* End of switch */
    }			/* End of try_next loop */
  while (try_next && length < 255);

  return length;
}


/*************************************************
*           Create map of starting chars         *
*************************************************/

/* This function scans a compiled unanchored expression and attempts to build a
   map of the characters occurring at the start of the match. If it can't, it
   returns FALSE. As time goes by, we may be able to get more clever at doing
   this.

   Arguments:
   code         points to an expression
   start_bits   points to a 32-byte table, initialized to 0
   caseless     the current state of the caseless flag
   cd           the block with char table pointers

   Returns:       TRUE if table built, FALSE otherwise
 */


static BOOL
set_start_bits (code, start_bits, caseless, cd)
     const uschar *code;
     uschar *start_bits;
     BOOL caseless;
     compile_data *cd;
{
  int i;

  /* This next statement and the later reference to dummy are here in order to
     trick the optimizer of the IBM C compiler for OS/2 into generating correct
     code. Apparently IBM isn't going to fix the problem, and we would rather not
     disable optimization (in this module it actually makes a big difference, and
     the pcre module can use all the optimization it can get). */

  volatile int dummy;

  do
    {
      const uschar *tcode = code + 3;
      BOOL try_next = TRUE;

      do
	{
	  /* If a branch starts with a bracket or a positive lookahead assertion,
	     recurse to set bits from within them. That's all for this branch. */

	  if ((int) *tcode >= OP_BRA || *tcode == OP_ASSERT)
	    {
	      if (!set_start_bits (tcode, start_bits, caseless, cd))
		return FALSE;
	      break;
	    }

	  switch (*tcode)
	    {
	    default:
	      if ((int) *tcode >= OP_BRA || *tcode == OP_ASSERT)
		abort ();
	      else
		return FALSE;

	      /* Skip over extended extraction bracket number */
		
	    case OP_BRANUMBER:
	      tcode += 3;
	      break;

	      /* Skip over lookbehind and negative lookahead assertions */
	      
	    case OP_ASSERT_NOT:
	    case OP_ASSERTBACK:
	    case OP_ASSERTBACK_NOT:
	      do
		tcode += (tcode[1] << 8) + tcode[2];
	      while (*tcode == OP_ALT);
	      tcode += 3;
	      break;
	      
		/* Skip over an option setting, changing the caseless flag */
	      
	    case OP_OPT:
	      caseless = (tcode[1] & PCRE_CASELESS) != 0;
	      tcode += 2;
	      break;
	      
	      /* BRAZERO does the bracket, but carries on. */
	      
	    case OP_BRAZERO:
	    case OP_BRAMINZERO:
	      if (!set_start_bits (++tcode, start_bits, caseless, cd))
		return FALSE;
	      dummy = 1;
	      do
		tcode += (tcode[1] << 8) + tcode[2];
	      while (*tcode == OP_ALT);
	      tcode += 3;
	      break;
	      
	      /* Single-char * or ? sets the bit and tries the next item */
	      
	    case OP_MAXSTAR:
	    case OP_MINSTAR:
	    case OP_ONCESTAR:
	    case OP_MAXQUERY:
	    case OP_MINQUERY:
	    case OP_ONCEQUERY:
	      set_bit (start_bits, tcode[1], caseless, cd);
	      tcode += 2;
	      break;

	      /* Single-char upto sets the bit and tries the next */
	      
	    case OP_MAXUPTO:
	    case OP_MINUPTO:
	    case OP_ONCEUPTO:
	      set_bit (start_bits, tcode[3], caseless, cd);
	      tcode += 4;
	      break;
	      
	      /* At least one single char sets the bit and stops */
	      
	    case OP_EXACT:	/* Fall through */
	      tcode++;
	      
	    case OP_CHARS:	/* Fall through */
	      tcode++;
	      
	    case OP_MAXPLUS:
	    case OP_MINPLUS:
	    case OP_ONCEPLUS:
	      set_bit (start_bits, tcode[1], caseless, cd);
	      try_next = FALSE;
	      break;
	      
	      /* Single character type sets the bits and stops */
	      
	    case OP_TYPENOT:
	      tcode++;
	      try_next = FALSE;
	    REPEATTYPENOT:
	      if (!*tcode)
		return FALSE;
	      
	      for (i = 0; i < 32; i++)
		start_bits[i] |= ~cd->cbits[i + *tcode * 32];
	      break;
	      
	    case OP_TYPE:
	      tcode++;
	      try_next = FALSE;
	    REPEATTYPE:
	      if (!*tcode)
		  return FALSE;
	      
	      for (i = 0; i < 32; i++)
		start_bits[i] |= cd->cbits[i + *tcode * 32];
	      break;
	      
	      /* One or more character type fudges the pointer and restarts. */
	      
	    case OP_TYPEEXACT:
		tcode += 2;
		
	    case OP_TYPE_MAXPLUS:
	    case OP_TYPE_MINPLUS:
	    case OP_TYPE_ONCEPLUS:
	      tcode++;
	      try_next = FALSE;
	      goto REPEATTYPE;
	      
	      /* Zero or more repeats of character types set the bits and then
		 try again. */
	      
	    case OP_TYPE_MAXUPTO:
	    case OP_TYPE_MINUPTO:
	    case OP_TYPE_ONCEUPTO:
	      tcode += 2;	/* Fall through */
	      
	    case OP_TYPE_MAXSTAR:
	    case OP_TYPE_MINSTAR:
	    case OP_TYPE_ONCESTAR:
	    case OP_TYPE_MAXQUERY:
	    case OP_TYPE_MINQUERY:
	    case OP_TYPE_ONCEQUERY:
	      tcode++;
	      goto REPEATTYPE;
	      break;
	      
	      /* One or more character type fudges the pointer and restarts. */
	      
	    case OP_TYPENOTEXACT:
	      tcode += 2;	/* Fall through */
	      
	    case OP_TYPENOT_MAXPLUS:
	    case OP_TYPENOT_MINPLUS:
	    case OP_TYPENOT_ONCEPLUS:
	      try_next = FALSE;
	      goto REPEATTYPENOT;
	      
	    case OP_TYPENOT_MAXUPTO:
	    case OP_TYPENOT_MINUPTO:
	    case OP_TYPENOT_ONCEUPTO:
	      tcode += 2;	/* Fall through */
	      
	    case OP_TYPENOT_MAXSTAR:
	    case OP_TYPENOT_MINSTAR:
	    case OP_TYPENOT_ONCESTAR:
	    case OP_TYPENOT_MAXQUERY:
	    case OP_TYPENOT_MINQUERY:
	    case OP_TYPENOT_ONCEQUERY:
	      tcode++;
	      goto REPEATTYPENOT;
	      
	      /* Character class: set the bits and either carry on or not,
		 according to the repeat count. */
	      
	    case OP_CLASS:
	    case OP_CL_MAXSTAR:
	    case OP_CL_MINSTAR:
	    case OP_CL_ONCESTAR:
	    case OP_CL_MAXPLUS:
	    case OP_CL_MINPLUS:
	    case OP_CL_ONCEPLUS:
	    case OP_CL_MAXQUERY:
	    case OP_CL_MINQUERY:
	    case OP_CL_ONCEQUERY:
	    case OP_CL_MAXRANGE:
	    case OP_CL_MINRANGE:
	    case OP_CL_ONCERANGE:
	      {
		tcode++;
		for (i = 0; i < 32; i++)
		  start_bits[i] |= tcode[i];
		tcode += 32;
		switch (tcode[-33])
		  {
		  case OP_CL_MAXSTAR:
		  case OP_CL_MINSTAR:
		  case OP_CL_ONCESTAR:
		  case OP_CL_MAXQUERY:
		  case OP_CL_MINQUERY:
		  case OP_CL_ONCEQUERY:
		    break;
		    
		  case OP_CL_MAXRANGE:
		  case OP_CL_MINRANGE:
		  case OP_CL_ONCERANGE:
		    if (((tcode[1] << 8) + tcode[2]) == 0)
		      tcode += 4;
		    else
		      try_next = FALSE;
		    break;
		  }
	      }
	      break;		/* End of class handling */

	    }			/* End of switch */
	}			/* End of try_next loop */
      while (try_next);

      code += (code[1] << 8) + code[2];		/* Advance to next branch */
    }
  while (*code == OP_ALT);
  return TRUE;
}



/*************************************************
*           Prune paths from a bracket           *
*************************************************/

/* This function is handed a pointer to the opcodes for a bracket in
   which it must prune impossible backtracking paths.  This is done
   by replacing _MAX* and _MIN* opcodes by the corresponding _ONCE*
   opcodes.

   Arguments:
   codeptr       points to the first opcode we're interested in,
                 on output it is rewritten with the end of the bracket
   bracket_start points to an array of bitsets that contain
                 the possible starting characters of the capturing
		 subpatterns
   bracket_end   points to an array of bitsets that contain
                 the possible ending characters of the capturing
		 subpatterns
   caseless      the caseless flag
   cd            the block with char table pointers
   p_start       points to where the function will store the character
		 class starting the bracket
   p_end         points to where the function will store the character
                 class for the end, and will get the character class
		 for the character immediately preceding the bracket
 */

BOOL
prune_bracket (codeptr, bracket_start, 
	       bracket_end, caseless, cd, p_start, p_end)
     uschar       **codeptr;
     BOOL         caseless;
     bitset       *bracket_start, *bracket_end, *p_start, *p_end;
     compile_data *cd;
{
  bitset all_alternatives_start, all_alternatives_end;
  bitset prev_class, curr_class, start, end;
  uschar *previous, *code = *codeptr;
  BOOL is_final_opcode, found_start, end_same_as_start;
  BOOL can_be_empty, bracket_can_be_empty = FALSE;

  /* Skip over the assertion at the beginning of a conditional */
  if (*code == OP_COND && code[3] == OP_ASSERT)
    code += (code[4] << 8) | code[5];

  code += 3;
  memset (all_alternatives_start, 0, 32);
  memset (all_alternatives_end, 0, 32);

 restart:
  if (p_end)
    memcpy (curr_class, p_end, 32);
  else
    memset (curr_class, 0, 32);

  memset (start, 0, 32);
  memset (end, ~0, 32);
  previous = NULL;
  found_start = FALSE;

  for (;;)
    {
      int i, backref;
      uschar *current = code;

      memcpy (prev_class, curr_class, 32);
      memset (curr_class, 0, 32);
      can_be_empty = TRUE;
      end_same_as_start = TRUE;
      is_final_opcode = FALSE;

      switch (*code)
	{
	  /* Skip over informational opcodes */

	case OP_REVERSE:
	case OP_CREF:
	case OP_BRANUMBER:
	  code += 3;
	  break;

	case OP_END:
	case OP_KET:
	case OP_KET_MINSTAR:
	case OP_KET_MAXSTAR:
	case OP_KET_ONCESTAR:
	  is_final_opcode = TRUE;

	case OP_ALT:
	  code += 3;
	  for (i = 0; i < 32; i++)
	    {
	      all_alternatives_start[i] |= start[i];
	      all_alternatives_end[i] |= end[i];
	    }

	  if (!found_start)
	    {
	      bracket_can_be_empty = TRUE;
	      memset (all_alternatives_start, ~0, 32);
	      if (p_end)
	        for (i = 0; i < 32; i++)
		  all_alternatives_end[i] |= (*p_end)[i];
	      else
	        memset (all_alternatives_end, ~0, 32);
	    }

	  if (is_final_opcode)
	    {
	      if (p_start)
	        memcpy (p_start, all_alternatives_start, 32);
	      if (p_end)
	        memcpy (p_end, all_alternatives_end, 32);

	      *codeptr = code;
	      return bracket_can_be_empty;
	    }
	  else
	    goto restart;

	  /* Skip over things that don't match chars */

	case OP_DOLL:
	case OP_EODN:
	  end_same_as_start = FALSE;
	  memcpy (end, prev_class, 32);
	  memset (curr_class, ~0, 32);
	  curr_class[13 / 8] ^= 1 << (13 % 8);

	case OP_ANCHOR_MATCH:
	case OP_SOD:
	case OP_CIRC:
	case OP_EOD:
	  code++;
	  break;

	case OP_NOT_WORD_BOUNDARY:
	case OP_WORD_BOUNDARY:
	  end_same_as_start = FALSE;
	  memcpy (end, prev_class, 32);
	  memset (curr_class, ~0, 32);
	  code++;
	  break;

	case OP_BEG_WORD:
	  for (i = 0; i < 32; i++)
	    curr_class[i] = cd->cbits[i + cbit_word];
	  code++;
	  break;

	case OP_END_WORD:
	  for (i = 0; i < 32; i++)
	    curr_class[i] = ~cd->cbits[i + cbit_word];
	  code++;
	  break;

	  /* Skip over lookbehind and negative lookahead assertions */

	case OP_ASSERTBACK:
	case OP_ASSERTBACK_NOT:
	  do
	    code += (code[1] << 8) + code[2];
	  while (*code == OP_ALT);
	  memset (curr_class, ~0, 32);
	  code += 3;
	  break;

	case OP_ASSERT:
	  prune_bracket (&code, bracket_start, bracket_end,
			 caseless, cd, &curr_class, &end);
	  memcpy (end, prev_class, 32);
	  end_same_as_start = FALSE;
	  break;

	case OP_ASSERT_NOT:
	  do
	    code += (code[1] << 8) + code[2];
	  while (*code == OP_ALT);
	  code += 3;
	  memset (curr_class, ~0, 32);
	  break;

	  /* Skip over an option setting, changing the caseless flag */

	case OP_OPT:
	  caseless = (code[1] & PCRE_CASELESS) != 0;
	  /* FIXME m and s too */
	  code += 2;
	  break;

	  /* BRAZERO does the bracket, but can be empty the classes. */

	case OP_BRAZERO:
	case OP_BRAMINZERO:
	  code++;
	  prune_bracket (&code, bracket_start, bracket_end,
			 caseless, cd, &curr_class, &end);
	  end_same_as_start = FALSE;
	  memset (curr_class, ~0, 32);
	  for (i = 0; i < 32; i++)
	    end[i] |= prev_class[i];
	  break;

	  /* Single character type sets the bits */

	case OP_TYPENOT:
	  code++;
	  can_be_empty = FALSE;
	REPEATTYPENOT:
	  if (!*code)
	    memset (curr_class, ~0, 32);
	  else
	    for (i = 0; i < 32; i++)
	      curr_class[i] = ~cd->cbits[i + *code * 32];

	  code++;
	  break;

	case OP_TYPE:
	  code++;
	  can_be_empty = FALSE;
	REPEATTYPE:
	  if (!*code)
	    memset (curr_class, ~0, 32);
	  else
	    for (i = 0; i < 32; i++)
	      curr_class[i] = cd->cbits[i + *code * 32];

	  code++;
	  break;

	case OP_CHARS:
	  code++;
	  can_be_empty = FALSE;
	  end_same_as_start = FALSE;
	  set_bit (curr_class, code[1], caseless, cd);
	  code += 1 + (int) *code;
	  memset (end, 0, 32);
	  set_bit (end, code[-1], caseless, cd);
	  break;

	case OP_ANY:
	  code++;
	  can_be_empty = FALSE;
	  memset (curr_class, ~0, 32);
	  break;

	case OP_NOT:
	  code++;
	  can_be_empty = FALSE;
	REPEATNOT:
	  set_bit (curr_class, *code++, caseless, cd);
	  for (i = 0; i < 32; i++)
	    curr_class[i] = ~curr_class[i];

	  break;

	  /* One or more character type fudges the pointer and restarts. */

	case OP_TYPEEXACT:
	  code += 2;

	case OP_TYPE_MAXPLUS:
	case OP_TYPE_MINPLUS:
	case OP_TYPE_ONCEPLUS:
	  code++;
	  can_be_empty = FALSE;
	  goto REPEATTYPE;

	  /* Zero or more repeats of character types set the bits and then
	     try again. */

	case OP_TYPE_MAXUPTO:
	case OP_TYPE_MINUPTO:
	case OP_TYPE_ONCEUPTO:
	  code += 2;	/* Fall through */

	case OP_TYPE_MAXSTAR:
	case OP_TYPE_MINSTAR:
	case OP_TYPE_ONCESTAR:
	case OP_TYPE_MAXQUERY:
	case OP_TYPE_MINQUERY:
	case OP_TYPE_ONCEQUERY:
	  code++;
	  goto REPEATTYPE;
	  break;

	  /* One or more character type fudges the pointer and restarts. */

	case OP_TYPENOTEXACT:
	  code += 2;	/* Fall through */

	case OP_TYPENOT_MAXPLUS:
	case OP_TYPENOT_MINPLUS:
	case OP_TYPENOT_ONCEPLUS:
	  code++;
	  can_be_empty = FALSE;
	  goto REPEATTYPENOT;

	case OP_TYPENOT_MAXUPTO:
	case OP_TYPENOT_MINUPTO:
	case OP_TYPENOT_ONCEUPTO:
	  code += 2;	/* Fall through */

	case OP_TYPENOT_MAXSTAR:
	case OP_TYPENOT_MINSTAR:
	case OP_TYPENOT_ONCESTAR:
	case OP_TYPENOT_MAXQUERY:
	case OP_TYPENOT_MINQUERY:
	case OP_TYPENOT_ONCEQUERY:
	  code++;
	  goto REPEATTYPENOT;

	  /* One or more character type fudges the pointer and restarts. */

	case OP_EXACT:
	  code += 2;

	case OP_MAXPLUS:
	case OP_MINPLUS:
	case OP_ONCEPLUS:
	  code++;
	  can_be_empty = FALSE;
	  goto REPEATCHARS;

	  /* Zero or more repeats of character types set the bits and then
	     try again. */

	case OP_MAXUPTO:
	case OP_MINUPTO:
	case OP_ONCEUPTO:
	  code += 2;	/* Fall through */

	case OP_MAXSTAR:
	case OP_MINSTAR:
	case OP_ONCESTAR:
	case OP_MAXQUERY:
	case OP_MINQUERY:
	case OP_ONCEQUERY:
	  code++;

	REPEATCHARS:
	  set_bit (curr_class, *code++, caseless, cd);
	  break;

	  /* One or more character type fudges the pointer and restarts. */

	case OP_NOTEXACT:
	  code += 2;	/* Fall through */

	case OP_NOT_MAXPLUS:
	case OP_NOT_MINPLUS:
	case OP_NOT_ONCEPLUS:
	  can_be_empty = FALSE;
	  code++;
	  goto REPEATNOT;

	case OP_NOT_MAXUPTO:
	case OP_NOT_MINUPTO:
	case OP_NOT_ONCEUPTO:
	  code += 2;	/* Fall through */

	case OP_NOT_MAXSTAR:
	case OP_NOT_MINSTAR:
	case OP_NOT_ONCESTAR:
	case OP_NOT_MAXQUERY:
	case OP_NOT_MINQUERY:
	case OP_NOT_ONCEQUERY:
	  code++;
	  goto REPEATNOT;

	  /* Character class: set the bits and either carry on or not,
	     according to the repeat count. */

	case OP_CLASS:
	case OP_CL_MAXSTAR:
	case OP_CL_MINSTAR:
	case OP_CL_ONCESTAR:
	case OP_CL_MAXPLUS:
	case OP_CL_MINPLUS:
	case OP_CL_ONCEPLUS:
	case OP_CL_MAXQUERY:
	case OP_CL_MINQUERY:
	case OP_CL_ONCEQUERY:
	case OP_CL_MAXRANGE:
	case OP_CL_MINRANGE:
	case OP_CL_ONCERANGE:
	  {
	    code++;
	    for (i = 0; i < 32; i++)
	      curr_class[i] |= code[i];
	    code += 32;
	    switch (code[-33])
	      {
	      case OP_CL_MAXSTAR:
	      case OP_CL_MINSTAR:
	      case OP_CL_ONCESTAR:
	      case OP_CL_MAXQUERY:
	      case OP_CL_MINQUERY:
	      case OP_CL_ONCEQUERY:
		break;

	      case OP_CL_MAXRANGE:
	      case OP_CL_MINRANGE:
	      case OP_CL_ONCERANGE:
		if (((code[1] << 8) + code[2]) == 0)
		  code += 4;
		else
		  can_be_empty = FALSE;
		break;

	      default:
		can_be_empty = FALSE;
		break;
	      }
	  }
	  break;		/* End of class handling */

	  /* Character class: set the bits and either carry on or not,
	     according to the repeat count. */

	case OP_REF:
	case OP_REF_MAXSTAR:
	case OP_REF_MINSTAR:
	case OP_REF_ONCESTAR:
	case OP_REF_MAXPLUS:
	case OP_REF_MINPLUS:
	case OP_REF_ONCEPLUS:
	case OP_REF_MAXQUERY:
	case OP_REF_MINQUERY:
	case OP_REF_ONCEQUERY:
	case OP_REF_MAXRANGE:
	case OP_REF_MINRANGE:
	case OP_REF_ONCERANGE:
	  {
	    int number = (code[1] << 8) | code[2];
	    code += 3;
	    memcpy (curr_class, bracket_start[number], 32);
	    end_same_as_start = FALSE;
	    switch (code[-3])
	      {
	      case OP_REF_MAXSTAR:
	      case OP_REF_MINSTAR:
	      case OP_REF_ONCESTAR:
	      case OP_REF_MAXQUERY:
	      case OP_REF_MINQUERY:
	      case OP_REF_ONCEQUERY:
		for (i = 0; i < 32; i++)
		  end[i] = bracket_end[number][i];
		break;
		
	      case OP_REF_MAXRANGE:
	      case OP_REF_MINRANGE:
	      case OP_REF_ONCERANGE:
		if (((code[1] << 8) + code[2]) == 0)
		  code += 4;

	      case OP_REF_MAXPLUS:
	      case OP_REF_MINPLUS:
	      case OP_REF_ONCEPLUS:
		memcpy (end, bracket_end[number], 32);
	      }
	  }
	  break;		/* End of class handling */

	default:
	  if ((int) *code >= OP_BRA)
	    {
	      backref = *code - OP_BRA;
	      if (backref > EXTRACT_BASIC_MAX)
	        backref = (code[4] << 8) | code[5];
	    }
	  else if (*code == OP_ONCE || *code == OP_COND)
	    backref = 0;
	  else
	    {
	      printf ("What's this `%s'?\n", pcre_OP_names[*code]);
	      abort();
	    }

	  can_be_empty = prune_bracket(&code, bracket_start, bracket_end,
				       caseless, cd, &curr_class, &end);

	  memcpy (bracket_start[backref], curr_class, 32);
	  memcpy (bracket_end[backref], end, 32);
	  end_same_as_start = FALSE;
	  break;
	}			/* End of switch */

      /* Check if we can prune the last path.  The check
	 on can_be_empty avoids pruning b* in b*c*b+. */
      if (previous && !can_be_empty)
	{
	  for (i = 0; i < 32; i++)
	    if (curr_class[i] & prev_class[i])
	      break;
	      
	  if (i == 32)
	    *previous = pruned[*previous];
	}

      if (end_same_as_start)
	{
	  if (can_be_empty)
	    {
	      int i;
	      for (i = 0; i < 32; i++)
		end[i] |= curr_class[i];
	    }
	  else
	    memcpy (end, curr_class, 32);
	}

      /* We found the starting class for this bracket (or part of it if
	 there are alternative branches). */
      if (!found_start)
	{
	  found_start = !can_be_empty;
	  for (i = 0; i < 32; i++)
	    start[i] |= curr_class[i];
	}

      previous = (*current < sizeof (pruned) && pruned[*current]) ? current : NULL;
    }
}



/*************************************************
*     Prune paths from a compiled expression     *
*************************************************/

/* This function is handed a compiled expression of which it must prune
   impossible backtracking paths.  This is done by replacing _MAX* and
   _MIN* opcodes by the corresponding _ONCE* opcodes.

   Arguments:
   re            points to the compiled expression
   caseless      the caseless flag
   cd            the block with char table pointers
 */

void
prune_backtracking_paths (re, caseless, cd)
     const pcre    *re;
     BOOL          caseless;
     compile_data  *cd;
{
  uschar *code = re->code;

  /* The first entry is used as scratch in both bitsets.  */
  bitset *bracket_start = (bitset *)
    alloca(sizeof(bitset) * (1 + re->top_bracket));

  bitset *bracket_end = (bitset *)
    alloca(sizeof(bitset) * (1 + re->top_bracket));

  prune_bracket (&code, bracket_start, bracket_end,
		 caseless, cd, NULL, NULL);
}




/*************************************************
*          Study a compiled expression           *
*************************************************/

/* This function is handed a compiled expression that it must study to produce
   information that will speed up the matching. It returns a pcre_extra block
   which then gets handed back to pcre_exec().

   Arguments:
   re        points to the compiled expression
   options   contains option bits
   errorptr  points to where to place error messages;
   set NULL unless error

   Returns:    pointer to a pcre_extra block,
   NULL on error or if no optimization possible
 */

pcre_extra *
pcre_study (re, options, errorptr)
     const pcre *re;
     int options;
     const char **errorptr;
{
  pcre_extra *extra, myextra;
  compile_data compile_block;
  int length;
  int caseless;

  *errorptr = NULL;

  if (re == NULL || re->magic_number != MAGIC_NUMBER)
    {
      *errorptr = "argument is not a compiled regular expression";
      return NULL;
    }

  if ((options & ~PUBLIC_STUDY_OPTIONS) != 0)
    {
      *errorptr = "unknown or incorrect option bit(s) set";
      return NULL;
    }


  /* Set the character tables in the block which is passed around */

  compile_block.lcc = re->tables + lcc_offset;
  compile_block.fcc = re->tables + fcc_offset;
  compile_block.cbits = re->tables + cbits_offset;
  compile_block.ctypes = re->tables + ctypes_offset;
  caseless = (re->options & PCRE_CASELESS) != 0;

  if (!(options & PCRE_STUDY_NO_PRUNE))
    prune_backtracking_paths(re, caseless, &compile_block);

  /* For an anchored pattern, or a multiline pattern that matches only at
     "line starts", no further processing at present. */

  if ((re->options & (PCRE_ANCHORED | PCRE_STARTLINE)) != 0
      || (options & PCRE_STUDY_NO_START))
    return NULL;

  /* See if we can find a fixed length initial string for the pattern. */

  memset ((char *) &myextra, 0, sizeof (myextra));

  if ((length = set_boyer_moore (re->code, myextra.data.bmtable, 0,
				 caseless, &compile_block)) > 1)
    {
      int i;
      myextra.options = PCRE_STUDY_BM;
      for (i = 0; i < 257; i++)
	myextra.data.bmtable[i] = length - myextra.data.bmtable[i];
    }
  else
    {
      if ((re->options & PCRE_FIRSTSET) != 0)
	return NULL;

      /* See if we can find a fixed set of initial characters for the pattern. */

      memset ((char *) &myextra, 0, sizeof (myextra));
      if (set_start_bits (re->code, myextra.data.start_bits, 
			  caseless, &compile_block))
	myextra.options = PCRE_STUDY_MAPPED;
    } 

  if (!myextra.options)
    return NULL;

  /* Get an "extra" block and put the information therein. */

  extra = (pcre_extra *) (pcre_malloc) (sizeof (pcre_extra));
  if (extra == NULL)
    {
      *errorptr = "failed to get memory";
      return NULL;
    }

  memcpy ((char *) extra, (char *) &myextra, sizeof (myextra));
  return (pcre_extra *) extra;
}
