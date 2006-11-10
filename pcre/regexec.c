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

/* Use a macro for debugging printing, 'cause that limits the use of #ifdef
   inline, and there are *still* stupid compilers about that don't like indented
   pre-processor statements. I suppose it's only been 10 years... */

#ifdef DEBUG
#define DPRINTF(p) printf p
#else
#define DPRINTF(p)		/*nothing */
#endif

/* Allow compilation as C++ source code, should anybody want to do that. */

#ifdef __cplusplus
#define class pcre_class
#endif


/* Structure for building a chain of data that actually lives on the
   stack, for holding the values of the subject pointer at the start of each
   subpattern, so as to detect when an empty string has been matched by a
   subpattern - to break infinite loops. */

typedef struct eptrblock
  {
    struct eptrblock *prev;
    const uschar *saved_eptr;
    int flags;
  }
eptrblock;

/* Flag bits for the match() function */

#define match_condassert   0x01	/* Called to check a condition assertion */
#define match_isgroup      0x02	/* Set if start of bracketed group */
#define match_isbrazero    0x04	/* Set if matching optional bracketed group */


/*************************************************
*    Macros and tables for character handling    *
*************************************************/

#define GETCHARINC(c, eptr) c = *eptr++;
#define GETCHAR(c, eptr) c = *eptr;

#ifdef DEBUG
/*************************************************
*        Debugging function to print chars       *
*************************************************/

/* Print a sequence of chars in printable format, stopping at the end of the
   subject if the requested.

   Arguments:
   p           points to characters
   length      number to print
   is_subject  TRUE if printing from within md->start_subject
   md          pointer to matching data block, if is_subject is TRUE

   Returns:     nothing
 */

static void
pchars (p, length, is_subject, md)
     const uschar *p;
     int length;
     BOOL is_subject;
     match_data *md;
{
  int c;
  if (is_subject && length > md->end_subject - p)
    length = md->end_subject - p;
  while (length-- > 0)
    if (isprint (c = *(p++)))
      printf ("%c", c);
    else
      printf ("\\x%02x", c);
}
#endif

/*************************************************
*          Match a back-reference                *
*************************************************/

/* If a back reference hasn't been set, the length that is passed is greater
   than the number of characters left in the string, so the match fails.

   Arguments:
   offset      index into the offset vector
   eptr        points into the subject
   length      length to be matched
   md          points to match data block
   ims         the ims flags

   Returns:      TRUE if matched
 */

static BOOL
match_ref (offset, eptr, length, md, ims)
     int offset;
     const uschar *eptr;
     int length;
     match_data *md;
     unsigned long int ims;
{
  const uschar *p = md->start_subject + md->offset_vector[offset];

#ifdef DEBUG
  if (eptr >= md->end_subject)
    printf ("matching subject <null>");
  else
    {
      printf ("matching subject ");
      pchars (eptr, length, TRUE, md);
    }
  printf (" against backref ");
  pchars (p, length, FALSE, md);
  printf ("\n");
#endif

  /* Always fail if not enough characters left */
  if (length > md->end_subject - eptr)
    return FALSE;

  /* Separate the caselesss case for speed */
  if ((ims & PCRE_CASELESS) != 0)
    {
      while (length-- > 0)
	if (md->lcc[*p++] != md->lcc[*eptr++])
	  return FALSE;
    }
  else
    {
      while (length-- > 0)
	if (*p++ != *eptr++)
	  return FALSE;
    }

  return TRUE;
}



/*************************************************
*         Match from current position            *
*************************************************/

/* On entry ecode points to the first opcode, and eptr to the first character
   in the subject string, while eptrb holds the value of eptr at the start of the
   last bracketed group - used for breaking infinite loops matching zero-length
   strings.

   Arguments:
   eptr        pointer in subject
   ecode       position in code
   offset_top  current top pointer
   md          pointer to "static" info for the match
   ims         current /i, /m, and /s options
   eptrb       pointer to chain of blocks containing eptr at start of
   brackets - for testing for empty matches
   flags       can contain
   match_condassert - this is an assertion condition
   match_isgroup    - this is the start of a bracketed group
   match_isbrazero  - this is after a BRAZERO

   Returns:       TRUE if matched
 */

static BOOL
match (eptr, ecode, offset_top, md, ims, eptrb, flags)
     const uschar *eptr;
     const uschar *ecode;
     int offset_top;
     match_data *md;
     unsigned long int ims;
     eptrblock *eptrb;
     int flags;
{
  unsigned long int original_ims = ims;		/* Save for resetting on ')' */
  const uschar *data;
  eptrblock newptrb;

  /* At the start of a bracketed group, add the current subject pointer to the
     stack of such pointers, to be re-instated at the end of the group when we hit
     the closing ket. When match() is called in other circumstances, we don't add to
     the stack. */
  if ((flags & match_isgroup) != 0)
    {
      newptrb.prev = eptrb;
      newptrb.saved_eptr = eptr;
      newptrb.flags = flags;
      eptrb = &newptrb;
    }

  /* Now start processing the operations. */
  for (;;)
    {
      int op = (int) *ecode;
      int min, max, ctype;
      int i;
      int c;
      int kind = 0;

      /* Opening capturing bracket. If there is space in the offset vector, save
         the current subject position in the working slot at the top of the vector. We
         mustn't change the current values of the data slot, because they may be set
         from a previous iteration of this group, and be referred to by a reference
         inside the group.

         If the bracket fails to match, we need to restore this value and also the
         values of the final offsets, in case they were set by a previous iteration of
         the same bracket.

         If there isn't enough space in the offset vector, treat this as if it were a
         non-capturing bracket. Don't worry about setting the flag for the error case
         here; that is handled in the code for KET. */

      if (op > OP_BRA)
	{
	  int offset;
	  int number = op - OP_BRA;
	  
	  /* For extended extraction brackets (large number), we have to fish out the
	     number from a dummy opcode at the start. */

	  if (number > EXTRACT_BASIC_MAX) 
	    number = (ecode[4] << 8) | ecode[5];

	  offset = number << 1;

#ifdef DEBUG
	  printf ("start bracket %d subject=", number);
	  pchars (eptr, 16, TRUE, md);
	  printf ("\n");
#endif

	  if (offset < md->offset_max)
	    {
	      int save_offset1 = md->offset_vector[offset];
	      int save_offset2 = md->offset_vector[offset + 1];
	      int save_offset3 = md->offset_vector[md->offset_end - number];

	      DPRINTF (("saving %d %d %d\n", save_offset1, save_offset2, save_offset3));
	      md->offset_vector[md->offset_end - number] = eptr - md->start_subject;

	      do
		{
		  if (match (eptr, ecode + 3, offset_top, md, ims, eptrb, match_isgroup))
		    return TRUE;
		  ecode += (ecode[1] << 8) + ecode[2];
		}
	      while (*ecode == OP_ALT);

	      DPRINTF (("bracket %d failed\n", number));

	      md->offset_vector[offset] = save_offset1;
	      md->offset_vector[offset + 1] = save_offset2;
	      md->offset_vector[md->offset_end - number] = save_offset3;

	      return FALSE;
	    }

	  /* Insufficient room for saving captured contents */

	  else
	    op = OP_BRA;
	}

      /* Other types of node can be handled by a switch */

      switch (op)
	{
	case OP_BRA:		/* Non-capturing bracket: optimized */
	  DPRINTF (("start bracket 0\n"));
	  do
	    {
	      if (match (eptr, ecode + 3, offset_top, md, ims, eptrb, match_isgroup))
		return TRUE;
	      ecode += (ecode[1] << 8) + ecode[2];
	    }
	  while (*ecode == OP_ALT);
	  DPRINTF (("bracket 0 failed\n"));
	  return FALSE;

	  /* Conditional group: compilation checked that there are no more than
	     two branches. If the condition is false, skipping the first branch takes us
	     past the end if there is only one branch, but that's OK because that is
	     exactly what going to the ket would do. */

	case OP_COND:
	  if (ecode[3] == OP_CREF)	/* Condition is extraction test */
	    {
	      int offset = (ecode[4] << 9) | (ecode[5] << 1);	/* Doubled reference number */
	      return match (eptr,
			    ecode + ((offset < offset_top && md->offset_vector[offset] >= 0) ?
				     6 : 3 + (ecode[1] << 8) + ecode[2]),
			    offset_top, md, ims, eptrb, match_isgroup);
	    }

	  /* The condition is an assertion. Call match() to evaluate it - setting
	     the final argument TRUE causes it to stop at the end of an assertion. */

	  else
	    {
	      if (match (eptr, ecode + 3, offset_top, md, ims, NULL,
			 match_condassert | match_isgroup))
		{
		  ecode += 3 + (ecode[4] << 8) + ecode[5];
		  while (*ecode == OP_ALT)
		    ecode += (ecode[1] << 8) + ecode[2];
		}
	      else
		ecode += (ecode[1] << 8) + ecode[2];
	      return match (eptr, ecode + 3, offset_top, md, ims, eptrb, match_isgroup);
	    }
	  /* Control never reaches here */

	  /* Skip over conditional reference data or large extraction number data if
	     encountered */

	case OP_CREF:
	case OP_BRANUMBER:
	  ecode += 3;
	  continue;

	  /* End of the pattern. If PCRE_NOTEMPTY is set, fail if we have matched
	     an empty string - recursion will then try other alternatives, if any. */

	case OP_END:
	  if (md->notempty && eptr == md->start_match)
	    return FALSE;
	  md->end_match_ptr = eptr;	/* Record where we ended */
	  md->end_offset_top = offset_top;	/* and how many extracts were taken */
	  return TRUE;

	  /* Change option settings */

	case OP_OPT:
	  ims = ecode[1];
	  ecode += 2;
	  DPRINTF (("ims set to %02lx\n", ims));
	  continue;

	  /* Assertion brackets. Check the alternative branches in turn - the
	     matching won't pass the KET for an assertion. If any one branch matches,
	     the assertion is true. Lookbehind assertions have an OP_REVERSE item at the
	     start of each branch to move the current point backwards, so the code at
	     this level is identical to the lookahead case. */

	case OP_ASSERT:
	case OP_ASSERTBACK:
	  do
	    {
	      if (match (eptr, ecode + 3, offset_top, md, ims, NULL, match_isgroup))
		break;
	      ecode += (ecode[1] << 8) + ecode[2];
	    }
	  while (*ecode == OP_ALT);
	  if (*ecode == OP_KET)
	    return FALSE;

	  /* If checking an assertion for a condition, return TRUE. */

	  if ((flags & match_condassert) != 0)
	    return TRUE;

	  /* Continue from after the assertion, updating the offsets high water
	     mark, since extracts may have been taken during the assertion. */

	  do
	    ecode += (ecode[1] << 8) + ecode[2];
	  while (*ecode == OP_ALT);
	  ecode += 3;
	  offset_top = md->end_offset_top;
	  continue;

	  /* Negative assertion: all branches must fail to match */

	case OP_ASSERT_NOT:
	case OP_ASSERTBACK_NOT:
	  do
	    {
	      if (match (eptr, ecode + 3, offset_top, md, ims, NULL, match_isgroup))
		return FALSE;
	      ecode += (ecode[1] << 8) + ecode[2];
	    }
	  while (*ecode == OP_ALT);

	  if ((flags & match_condassert) != 0)
	    return TRUE;

	  ecode += 3;
	  continue;

	  /* Move the subject pointer back. This occurs only at the start of
	     each branch of a lookbehind assertion. If we are too close to the start to
	     move back, this match function fails. */

	case OP_REVERSE:
	  eptr -= (ecode[1] << 8) + ecode[2];

	  if (eptr < md->start_subject)
	    return FALSE;
	  ecode += 3;
	  continue;

	  /* Recursion matches the current regex, nested. If there are any capturing
	     brackets started but not finished, we have to save their starting points
	     and reinstate them after the recursion. However, we don't know how many
	     such there are (offset_top records the completed total) so we just have
	     to save all the potential data. There may be up to 99 such values, which
	     is a bit large to put on the stack, but using malloc for small numbers
	     seems expensive. As a compromise, the stack is used when there are fewer
	     than 16 values to store; otherwise malloc is used. A problem is what to do
	     if the malloc fails ... there is no way of returning to the top level with
	     an error. Save the top 15 values on the stack, and accept that the rest
	     may be wrong. */

	case OP_RECURSE:
	  {
	    BOOL rc;
	    int *save;
	    int stacksave[15];

	    c = md->offset_max;

	    if (c < 16)
	      save = stacksave;
	    else
	      {
		save = (int *) (pcre_malloc) ((c + 1) * sizeof (int));
		if (save == NULL)
		  {
		    save = stacksave;
		    c = 15;
		  }
	      }

	    for (i = 1; i <= c; i++)
	      save[i] = md->offset_vector[md->offset_end - i];
	    rc = match (eptr, md->start_pattern, offset_top, md, ims, eptrb,
			match_isgroup);
	    for (i = 1; i <= c; i++)
	      md->offset_vector[md->offset_end - i] = save[i];
	    if (save != stacksave)
	      (pcre_free) (save);
	    if (!rc)
	      return FALSE;

	    /* In case the recursion has set more capturing values, save the final
	       number, then move along the subject till after the recursive match,
	       and advance one byte in the pattern code. */

	    offset_top = md->end_offset_top;
	    eptr = md->end_match_ptr;
	    ecode++;
	  }
	  continue;

	  /* "Once" brackets are like assertion brackets except that after a match,
	     the point in the subject string is not moved back. Thus there can never be
	     a move back into the brackets. Check the alternative branches in turn - the
	     matching won't pass the KET for this kind of subpattern. If any one branch
	     matches, we carry on as at the end of a normal bracket, leaving the subject
	     pointer. */

	case OP_ONCE:
	  {
	    const uschar *prev = ecode;
	    const uschar *saved_eptr = eptr;

	    do
	      {
		if (match (eptr, ecode + 3, offset_top, md, ims, eptrb, match_isgroup))
		  break;
		ecode += (ecode[1] << 8) + ecode[2];
	      }
	    while (*ecode == OP_ALT);

	    /* If hit the end of the group (which could be repeated), fail */

	    if (*ecode != OP_ONCE && *ecode != OP_ALT)
	      return FALSE;

	    /* Continue as from after the assertion, updating the offsets high water
	       mark, since extracts may have been taken. */

	    do
	      ecode += (ecode[1] << 8) + ecode[2];
	    while (*ecode == OP_ALT);

	    offset_top = md->end_offset_top;
	    eptr = md->end_match_ptr;

	    /* For a non-repeating ket, just continue at this level. This also
	       happens for a repeating ket if no characters were matched in the group.
	       This is the forcible breaking of infinite loops as implemented in Perl
	       5.005. If there is an options reset, it will get obeyed in the normal
	       course of events. */

	    if (*ecode == OP_KET || eptr == saved_eptr)
	      {
		ecode += 3;
		continue;
	      }

	    /* The repeating kets try the rest of the pattern or restart from the
	       preceding bracket, in the appropriate order. We need to reset any options
	       that changed within the bracket before re-running it, so check the next
	       opcode. */

	    if (ecode[3] == OP_OPT)
	      {
		ims = (ims & ~PCRE_IMS) | ecode[4];
		DPRINTF (("ims set to %02lx at group repeat\n", ims));
	      }

	    if (*ecode == OP_KET_MINSTAR)
	      {
		if (match (eptr, ecode + 3, offset_top, md, ims, eptrb, 0) ||
		    match (eptr, prev, offset_top, md, ims, eptrb, match_isgroup))
		  return TRUE;
	      }
	    else
	      /* OP_KET_MAXSTAR or OP_KET_ONCESTAR (FIXME!) */
	      {
		if (match (eptr, prev, offset_top, md, ims, eptrb, match_isgroup) ||
		    match (eptr, ecode + 3, offset_top, md, ims, eptrb, 0))
		  return TRUE;
	      }
	  }
	  return FALSE;

	  /* An alternation is the end of a branch; scan along to find the end of the
	     bracketed group and go to there. */

	case OP_ALT:
	  do
	    ecode += (ecode[1] << 8) + ecode[2];
	  while (*ecode == OP_ALT);
	  continue;

	  /* BRAZERO and BRAMINZERO occur just before a bracket group, indicating
	     that it may occur zero times. It may repeat infinitely, or not at all -
	     i.e. it could be ()* or ()? in the pattern. Brackets with fixed upper
	     repeat limits are compiled as a number of copies, with the optional ones
	     preceded by BRAZERO or BRAMINZERO. */

	case OP_BRAZERO:
	  {
	    const uschar *next = ecode + 1;
	    if (match (eptr, next, offset_top, md, ims, eptrb,
		       match_isgroup | match_isbrazero))
	      return TRUE;
	    do
	      next += (next[1] << 8) + next[2];
	    while (*next == OP_ALT);
	    ecode = next + 3;
	  }
	  continue;

	case OP_BRAMINZERO:
	  {
	    const uschar *next = ecode + 1;
	    do
	      next += (next[1] << 8) + next[2];
	    while (*next == OP_ALT);
	    if (match (eptr, next + 3, offset_top, md, ims, eptrb, match_isgroup))
	      return TRUE;
	    ecode++;
	  }
	  continue;

	  /* End of a group, repeated or non-repeating. If we are at the end of
	     an assertion "group", stop matching and return TRUE, but record the
	     current high water mark for use by positive assertions. Do this also
	     for the "once" (not-backup up) groups. */

	case OP_KET:
	case OP_KET_MINSTAR:
	case OP_KET_MAXSTAR:
	case OP_KET_ONCESTAR:
	  {
	    const uschar *prev = ecode - (ecode[1] << 8) - ecode[2];
	    const uschar *saved_eptr = eptrb->saved_eptr;

	    eptrb = eptrb->prev;	/* Back up the stack of bracket start pointers */

	    if (*prev == OP_ASSERT || *prev == OP_ASSERT_NOT ||
		*prev == OP_ASSERTBACK || *prev == OP_ASSERTBACK_NOT ||
		*prev == OP_ONCE)
	      {
		md->end_match_ptr = eptr;	/* For ONCE */
		md->end_offset_top = offset_top;
		return TRUE;
	      }

	    /* In all other cases except a conditional group we have to check the
	       group number back at the start and if necessary complete handling an
	       extraction by setting the offsets and bumping the high water mark. */

	    if (*prev != OP_COND)
	      {
		int offset;
		int number = *prev - OP_BRA;

         /* For extended extraction brackets (large number), we have to fish out
	    the number from a dummy opcode at the start. */

		if (number > EXTRACT_BASIC_MAX) 
		  number = (prev[4] << 8) | prev[5];

		offset = number << 1;
#ifdef DEBUG
		printf ("end bracket %d\n", number);
#endif

		/* Void matches (eptr == saved_eptr) are reported if they are the
		   first ones (md->offset_vector[offset] is still -1) or if the
		   match is not optional (the match_isbrazero flag is not active).  */
		if (number > 0
		    && (eptr > saved_eptr
			|| offset_top <= offset
			|| md->offset_vector[offset] == -1
			|| !(eptrb->flags & match_isbrazero)))
		  {
		    if (offset >= md->offset_max)
		      md->offset_overflow = TRUE;
		      {
			if (offset_top <= offset)
			  offset_top = offset + 2;
			md->offset_vector[offset] =
			  md->offset_vector[md->offset_end - number];
			md->offset_vector[offset + 1] = eptr - md->start_subject;
		      }
		  }
	      }

	    /* Reset the value of the ims flags, in case they got changed during
	       the group. */

	    ims = original_ims;
	    DPRINTF (("ims reset to %02lx\n", ims));

	    /* For a non-repeating ket, just continue at this level. This also
	       happens for a repeating ket if no characters were matched in the group.
	       This is the forcible breaking of infinite loops as implemented in Perl
	       5.005. If there is an options reset, it will get obeyed in the normal
	       course of events. */

	    if (*ecode == OP_KET || eptr == saved_eptr)
	      {
		ecode += 3;
		continue;
	      }

	    /* The repeating kets try the rest of the pattern or restart from the
	       preceding bracket, in the appropriate order. */

	    if (*ecode == OP_KET_MINSTAR)
	      {
		if (match (eptr, ecode + 3, offset_top, md, ims, eptrb, 0) ||
		    match (eptr, prev, offset_top, md, ims, eptrb, match_isgroup))
		  return TRUE;
	      }
	    else
	      /* OP_KET_MAXSTAR or OP_KET_ONCESTAR (FIXME!) */
	      {
		if (match (eptr, prev, offset_top, md, ims, eptrb, match_isgroup) ||
		    match (eptr, ecode + 3, offset_top, md, ims, eptrb, 0))
		  return TRUE;
	      }
	  }
	  return FALSE;

	  /* Start of subject unless notbol, or after internal newline if multiline */

	case OP_CIRC:
	  if (md->notbol && eptr == md->start_subject)
	    return FALSE;
	  if ((ims & PCRE_MULTILINE) != 0)
	    {
	      if (eptr != md->start_subject && eptr[-1] != '\n')
		return FALSE;
	      ecode++;
	      continue;
	    }
	  /* ... else fall through */

	  /* Start of subject assertion */

	case OP_SOD:
	  if (eptr != md->start_subject)
	    return FALSE;
	  ecode++;
	  continue;

	  /* No implicit advancing assertion */

	case OP_ANCHOR_MATCH:
	  if (eptr != md->first_start)
	    return FALSE;
	  ecode++;
	  continue;

	  /* Assert before internal newline if multiline, or before a terminating
	     newline unless endonly is set, else end of subject unless noteol is set. */

	case OP_DOLL:
	  if ((ims & PCRE_MULTILINE) != 0)
	    {
	      if (eptr < md->end_subject)
		{
		  if (*eptr != '\n')
		    return FALSE;
		}
	      else
		{
		  if (md->noteol)
		    return FALSE;
		}
	      ecode++;
	      continue;
	    }
	  else
	    {
	      if (md->noteol)
		return FALSE;
	      if (!md->endonly)
		{
		  if (eptr < md->end_subject - 1 ||
		      (eptr == md->end_subject - 1 && *eptr != '\n'))
		    return FALSE;

		  ecode++;
		  continue;
		}
	    }
	  /* ... else fall through */

	  /* End of subject assertion (\z) */

	case OP_EOD:
	  if (eptr < md->end_subject)
	    return FALSE;
	  ecode++;
	  continue;

	  /* End of subject or ending \n assertion (\Z) */

	case OP_EODN:
	  if (eptr < md->end_subject - 1 ||
	      (eptr == md->end_subject - 1 && *eptr != '\n'))
	    return FALSE;
	  ecode++;
	  continue;

	  /* Word boundary assertions */

	case OP_NOT_WORD_BOUNDARY:
	case OP_WORD_BOUNDARY:
	  {
	    BOOL prev_is_word = (eptr != md->start_subject) &&
	    ((md->ctypes[eptr[-1]] & ctype_word) != 0);
	    BOOL cur_is_word = (eptr < md->end_subject) &&
	    ((md->ctypes[*eptr] & ctype_word) != 0);
	    if ((*ecode++ == OP_WORD_BOUNDARY) ?
		cur_is_word == prev_is_word : cur_is_word != prev_is_word)
	      return FALSE;
	  }
	  continue;

	case OP_BEG_WORD:
	case OP_END_WORD:
	  {
	    BOOL prev_is_word = (eptr != md->start_subject) &&
	    ((md->ctypes[eptr[-1]] & ctype_word) != 0);
	    BOOL cur_is_word = (eptr < md->end_subject) &&
	    ((md->ctypes[*eptr] & ctype_word) != 0);
	    if (cur_is_word == prev_is_word ||
		((*ecode++ == OP_BEG_WORD) ? prev_is_word : cur_is_word))
	      return FALSE;
	  }
	  continue;

	  /* Match a single character type; inline for speed */

	case OP_ANY:
	  if ((ims & PCRE_DOTALL) == 0 && eptr < md->end_subject && *eptr == '\n')
	    return FALSE;
	  if (eptr++ >= md->end_subject)
	    return FALSE;
	  ecode++;
	  continue;

	case OP_TYPE:
	  ctype = 1 << ecode[1];	/* Code for the character type */

	  if (eptr >= md->end_subject || (md->ctypes[*eptr++] & ctype) == 0)
	    return FALSE;
	  ecode += 2;
	  continue;

	case OP_TYPENOT:
	  ctype = ecode[1];	/* Code for the character type */
	  if (ctype != 0 || !(ims & PCRE_DOTALL))
	    ctype = 1 << ctype;

	  if (eptr >= md->end_subject || !(md->ctypes[*eptr++] & ctype) == 0)
	    return FALSE;
	  ecode += 2;
	  continue;

	  /* Match a back reference, possibly repeatedly. Look past the end of the
	     item to see if there is repeat information following. The code is similar
	     to that for character classes, but repeated for efficiency. Then obey
	     similar code to character type repeats - written out again for speed.
	     However, if the referenced string is the empty string, always treat
	     it as matched, any number of times (otherwise there could be infinite
	     loops). */

	case OP_REF:
	  {
	    int length;
	    int offset = (ecode[1] << 9) | (ecode[2] << 1);		/* Doubled reference number */
	    ecode += 3;		/* Advance past the item */

	    /* If the reference is unset, set the length to be longer than the amount
	       of subject left; this ensures that every attempt at a match fails. We
	       can't just fail here, because of the possibility of quantifiers with zero
	       minima. */

	    length = (offset >= offset_top || md->offset_vector[offset] < 0) ?
	      md->end_subject - eptr + 1 :
	      md->offset_vector[offset + 1] - md->offset_vector[offset];

	    if (!match_ref (offset, eptr, length, md, ims))
	      return FALSE;
	    eptr += length;
	    continue;	/* With the main loop */
	  }

	case OP_REF_MAXSTAR:
	case OP_REF_MINSTAR:
	case OP_REF_ONCESTAR:
	  data = ecode;
	  kind = *ecode++ - OP_REF_MAXSTAR;
	  min = 0, max = INT_MAX;
	  goto REPEATREF;

	case OP_REF_MAXPLUS:
	case OP_REF_MINPLUS:
	case OP_REF_ONCEPLUS:
	  data = ecode;
	  kind = *ecode++ - OP_REF_MAXPLUS;
	  min = 1, max = INT_MAX;
	  goto REPEATREF;

	case OP_REF_MAXQUERY:
	case OP_REF_MINQUERY:
	case OP_REF_ONCEQUERY:
	  data = ecode;
	  kind = *ecode++ - OP_REF_MAXQUERY;
	  min = 0, max = 1;
	  goto REPEATREF;

	case OP_REF_MAXRANGE:
	case OP_REF_MINRANGE:
	case OP_REF_ONCERANGE:
	  data = ecode;
	  kind = *ecode++ - OP_REF_MINRANGE;
	  min = (ecode[2] << 8) + ecode[3];
	  max = (ecode[4] << 8) + ecode[5];
	  ecode += 4;
	  
	REPEATREF:
	  {
	    int length;
	    int offset = (data[1] << 9) | (data[2] << 1);		/* Doubled reference number */
	    ecode += 2;		/* Advance past the item */
	    if (max == 0)
	      max = INT_MAX;

	    /* If the reference is unset, set the length to be longer than the amount
	       of subject left; this ensures that every attempt at a match fails. We
	       can't just fail here, because of the possibility of quantifiers with zero
	       minima. */

	    length = (offset >= offset_top || md->offset_vector[offset] < 0) ?
	      md->end_subject - eptr + 1 :
	      md->offset_vector[offset + 1] - md->offset_vector[offset];

	    /* If the length of the reference is zero, just continue with the
	       main loop. */

	    if (length == 0)
	      continue;

	    /* First, ensure the minimum number of matches are present.  */

	    for (i = 1; i <= min; i++)
	      {
		if (!match_ref (offset, eptr, length, md, ims))
		  return FALSE;
		eptr += length;
	      }

	    /* If min = max, continue at the same level without recursion.
	       They are not both allowed to be zero. */

	    if (min == max)
	      continue;

	    /* If minimizing, keep trying and advancing the pointer */

	    if (kind == KIND_MIN)
	      {
		for (i = min;; i++)
		  {
		    if (match (eptr, ecode, offset_top, md, ims, eptrb, 0))
		      return TRUE;
		    if (i >= max || !match_ref (offset, eptr, length, md, ims))
		      return FALSE;
		    eptr += length;
		  }
		/* Control never gets here */
	      }

	    /* If maximizing, find the longest string and work backwards */

	    else
	      {
		const uschar *pp = eptr;
		for (i = min; i < max; i++)
		  {
		    if (!match_ref (offset, eptr, length, md, ims))
		      break;
		    eptr += length;
		  }
		if (kind == KIND_MAX)
		  {
		    while (eptr >= pp)
		      {
			if (match (eptr, ecode, offset_top, md, ims, eptrb, 0))
			  return TRUE;
			eptr -= length;
		      }
		    return FALSE;
		  }
	      }
	  }
	  continue;


	  /* Match a character class, possibly repeatedly. Look past the end of the
	     item to see if there is repeat information following. Then obey similar
	     code to character type repeats - written out again for speed. */

	case OP_CLASS:
	  {
	    data = ecode + 1;	/* Save for matching */
	    ecode += 33;	/* Advance past the item */

	    if (eptr >= md->end_subject)
	      return FALSE;

	    GETCHARINC (c, eptr)    /* Get character; increment eptr */
	      if ((data[c / 8] & (1 << (c & 7))) == 0)
		return FALSE;
	  }
	  continue;

	case OP_CL_MAXSTAR:
	case OP_CL_MINSTAR:
	case OP_CL_ONCESTAR:
	  kind = *ecode++ - OP_CL_MAXSTAR;
	  data = ecode;
	  min = 0, max = INT_MAX;
	  goto REPEATCLASS;

	case OP_CL_MAXPLUS:
	case OP_CL_MINPLUS:
	case OP_CL_ONCEPLUS:
	  kind = *ecode++ - OP_CL_MAXPLUS;
	  data = ecode;
	  min = 1, max = INT_MAX;
	  goto REPEATCLASS;

	case OP_CL_MAXQUERY:
	case OP_CL_MINQUERY:
	case OP_CL_ONCEQUERY:
	  kind = *ecode++ - OP_CL_MAXQUERY;
	  data = ecode;
	  min = 0, max = 1;
	  goto REPEATCLASS;

	case OP_CL_MAXRANGE:
	case OP_CL_MINRANGE:
	case OP_CL_ONCERANGE:
	  kind = *ecode++ - OP_CL_MAXRANGE;
	  data = ecode;
	  min = (ecode[32] << 8) + ecode[33];
	  max = (ecode[34] << 8) + ecode[35];
	  ecode += 4;

	REPEATCLASS:
	  ecode += 32;	/* Advance past the item */
	  if (max == 0)
	    max = INT_MAX;

	  /* First, ensure the minimum number of matches are present. */

	  for (i = 1; i <= min; i++)
	    {
	      if (eptr >= md->end_subject)
		return FALSE;
	      GETCHARINC (c, eptr)	/* Get character; increment eptr */
		
		if ((data[c / 8] & (1 << (c & 7))) != 0)
		  continue;
	      return FALSE;
	    }
	  
	  /* If max == min we can continue with the main loop without the
	     need to recurse. */
	  
	  if (min == max)
	    continue;
	  
	  /* If minimizing, keep testing the rest of the expression and advancing
	     the pointer while it matches the class. */
	  
	  if (kind == KIND_MIN)
	    {
	      for (i = min;; i++)
		{
		  if (match (eptr, ecode, offset_top, md, ims, eptrb, 0))
		    return TRUE;
		  if (i >= max || eptr >= md->end_subject)
		    return FALSE;
		  GETCHARINC (c, eptr)	/* Get character; increment eptr */
		    
		    if ((data[c / 8] & (1 << (c & 7))) != 0)
		      continue;
		  return FALSE;
		}
	      /* Control never gets here */
	    }
	  
	  /* If maximizing, find the longest possible run, then work backwards. */
	  
	  else
	    {
	      const uschar *pp = eptr;
	      for (i = min; i < max; i++)
		{
		  if (eptr >= md->end_subject)
		    break;
		  GETCHAR (c, eptr)	/* Get character */
		    
		    if ((data[c / 8] & (1 << (c & 7))) == 0)
		      break;
		  eptr++;
		}
	      
	      if (kind == KIND_MAX)
		{
		  while (eptr >= pp)
		    {
		      if (match (eptr--, ecode, offset_top, md, ims, eptrb, 0))
			return TRUE;
		    }

		  return FALSE;
		}
	    }

	  continue;

	  /* Match a run of characters */

	case OP_CHARS:
	  {
	    int length = ecode[1];
	    ecode += 2;

#ifdef DEBUG
	    if (eptr >= md->end_subject)
	      printf ("matching subject <null> against pattern ");
	    else
	      {
		printf ("matching subject ");
		pchars (eptr, length, TRUE, md);
		printf (" against pattern ");
	      }
	    pchars (ecode, length, FALSE, md);
	    printf ("\n");
#endif

	    if (length > md->end_subject - eptr)
	      return FALSE;
	    if ((ims & PCRE_CASELESS) != 0)
	      {
		while (length-- > 0)
		  if (md->lcc[*ecode++] != md->lcc[*eptr++])
		    return FALSE;
	      }
	    else
	      {
		while (length-- > 0)
		  if (*ecode++ != *eptr++)
		    return FALSE;
	      }
	  }
	  continue;

	  /* Match a single character repeatedly; different opcodes share code. */

	case OP_EXACT:
	  min = max = (ecode[1] << 8) + ecode[2];
	  ecode += 3;
	  goto REPEATCHAR;

	case OP_MAXUPTO:
	case OP_MINUPTO:
	case OP_ONCEUPTO:
	  min = 0;
	  max = (ecode[1] << 8) + ecode[2];
	  kind = *ecode - OP_MAXUPTO;
	  ecode += 3;
	  goto REPEATCHAR;

	case OP_MAXSTAR:
	case OP_MINSTAR:
	case OP_ONCESTAR:
	  kind = *ecode++ - OP_MAXSTAR;
	  min = 0, max = INT_MAX;
	  goto REPEATCHAR;

	case OP_MAXPLUS:
	case OP_MINPLUS:
	case OP_ONCEPLUS:
	  kind = *ecode++ - OP_MAXPLUS;
	  min = 1, max = INT_MAX;
	  goto REPEATCHAR;

	case OP_MAXQUERY:
	case OP_MINQUERY:
	case OP_ONCEQUERY:
	  kind = *ecode++ - OP_MAXQUERY;
	  min = 0, max = 1;

	  /* Common code for all repeated single-character matches. We can give
	     up quickly if there are fewer than the minimum number of characters left in
	     the subject. */

	REPEATCHAR:
	  if (min > md->end_subject - eptr)
	    return FALSE;
	  c = *ecode++;

	  /* The code is duplicated for the caseless and caseful cases, for speed,
	     since matching characters is likely to be quite common. First, ensure the
	     minimum number of matches are present. If min = max, continue at the same
	     level without recursing. Otherwise, if minimizing, keep trying the rest of
	     the expression and advancing one matching character if failing, up to the
	     maximum. Alternatively, if maximizing, find the maximum number of
	     characters and work backwards. */

	  DPRINTF (("matching %c{%d,%d} against subject %.*s\n", c, min, max,
		    max, eptr));

	  if ((ims & PCRE_CASELESS) != 0)
	    {
	      c = md->lcc[c];
	      for (i = 1; i <= min; i++)
		if (c != md->lcc[*eptr++])
		  return FALSE;
	      if (min == max)
		continue;
	      if (kind == KIND_MIN)
		{
		  for (i = min;; i++)
		    {
		      if (match (eptr, ecode, offset_top, md, ims, eptrb, 0))
			return TRUE;
		      if (i >= max || eptr >= md->end_subject ||
			  c != md->lcc[*eptr++])
			return FALSE;
		    }
		  /* Control never gets here */
		}
	      else
		{
		  const uschar *pp = eptr;
		  for (i = min; i < max; i++)
		    {
		      if (eptr >= md->end_subject || c != md->lcc[*eptr])
			break;
		      eptr++;
		    }

		  if (kind == KIND_MAX)
		    {
		      while (eptr >= pp)
			if (match (eptr--, ecode, offset_top, md, ims, eptrb, 0))
			  return TRUE;
		      return FALSE;
		    }
		}
	    }

	  /* Caseful comparisons */

	  else
	    {
	      for (i = 1; i <= min; i++)
		if (c != *eptr++)
		  return FALSE;
	      if (min == max)
		continue;
	      if (kind == KIND_MIN)
		{
		  for (i = min;; i++)
		    {
		      if (match (eptr, ecode, offset_top, md, ims, eptrb, 0))
			return TRUE;
		      if (i >= max || eptr >= md->end_subject || c != *eptr++)
			return FALSE;
		    }
		  /* Control never gets here */
		}
	      else
		{
		  const uschar *pp = eptr;
		  for (i = min; i < max; i++)
		    {
		      if (eptr >= md->end_subject || c != *eptr)
			break;
		      eptr++;
		    }

		  if (kind == KIND_MAX)
		    {
		      while (eptr >= pp)
			if (match (eptr--, ecode, offset_top, md, ims, eptrb, 0))
			  return TRUE;
		      return FALSE;
		    }
		}
	    }

	  continue;

	  /* Match a negated single character */

	case OP_NOT:
	  if (eptr >= md->end_subject)
	    return FALSE;
	  ecode++;
	  if ((ims & PCRE_CASELESS) != 0)
	    {
	      if (md->lcc[*ecode++] == md->lcc[*eptr++])
		return FALSE;
	    }
	  else
	    {
	      if (*ecode++ == *eptr++)
		return FALSE;
	    }
	  continue;

	  /* Match a negated single character repeatedly. This is almost a repeat of
	     the code for a repeated single character, but I haven't found a nice way of
	     commoning these up that doesn't require a test of the positive/negative
	     option for each character match. Maybe that wouldn't add very much to the
	     time taken, but character matching *is* what this is all about... */

	case OP_NOTEXACT:
	  min = max = (ecode[1] << 8) + ecode[2];
	  ecode += 3;
	  goto REPEATNOTCHAR;

	case OP_NOT_MAXUPTO:
	case OP_NOT_MINUPTO:
	case OP_NOT_ONCEUPTO:
	  min = 0;
	  max = (ecode[1] << 8) + ecode[2];
	  kind = *ecode - OP_NOT_MAXUPTO;
	  ecode += 3;
	  goto REPEATNOTCHAR;

	case OP_NOT_MAXSTAR:
	case OP_NOT_MINSTAR:
	case OP_NOT_ONCESTAR:
	  kind = *ecode++ - OP_NOT_MAXSTAR;
	  min = 0, max = INT_MAX;
	  goto REPEATNOTCHAR;

	case OP_NOT_MAXPLUS:
	case OP_NOT_MINPLUS:
	case OP_NOT_ONCEPLUS:
	  kind = *ecode++ - OP_NOT_MAXPLUS;
	  min = 1, max = INT_MAX;
	  goto REPEATNOTCHAR;

	case OP_NOT_MAXQUERY:
	case OP_NOT_MINQUERY:
	case OP_NOT_ONCEQUERY:
	  kind = *ecode++ - OP_NOT_MAXQUERY;
	  min = 0, max = 1;

	  /* Common code for all repeated single-character matches. We can give
	     up quickly if there are fewer than the minimum number of characters left in
	     the subject. */

	REPEATNOTCHAR:
	  if (min > md->end_subject - eptr)
	    return FALSE;
	  c = *ecode++;

	  /* The code is duplicated for the caseless and caseful cases, for speed,
	     since matching characters is likely to be quite common. First, ensure the
	     minimum number of matches are present. If min = max, continue at the same
	     level without recursing. Otherwise, if minimizing, keep trying the rest of
	     the expression and advancing one matching character if failing, up to the
	     maximum. Alternatively, if maximizing, find the maximum number of
	     characters and work backwards. */

	  DPRINTF (("negative matching %c{%d,%d} against subject %.*s\n", c, min, max,
		    max, eptr));

	  if ((ims & PCRE_CASELESS) != 0)
	    {
	      c = md->lcc[c];
	      for (i = 1; i <= min; i++)
		if (c == md->lcc[*eptr++])
		  return FALSE;
	      if (min == max)
		continue;
	      if (kind == KIND_MIN)
		{
		  for (i = min;; i++)
		    {
		      if (match (eptr, ecode, offset_top, md, ims, eptrb, 0))
			return TRUE;
		      if (i >= max || eptr >= md->end_subject ||
			  c == md->lcc[*eptr++])
			return FALSE;
		    }
		  /* Control never gets here */
		}
	      else
		{
		  const uschar *pp = eptr;
		  for (i = min; i < max; i++)
		    {
		      if (eptr >= md->end_subject || c == md->lcc[*eptr])
			break;
		      eptr++;
		    }

		  if (kind == KIND_MAX)
		    {
		      while (eptr >= pp)
			if (match (eptr--, ecode, offset_top, md, ims, eptrb, 0))
			  return TRUE;
		      return FALSE;
		    }
		}
	    }

	  /* Caseful comparisons */

	  else
	    {
	      for (i = 1; i <= min; i++)
		if (c == *eptr++)
		  return FALSE;
	      if (min == max)
		continue;
	      if (kind == KIND_MIN)
		{
		  for (i = min;; i++)
		    {
		      if (match (eptr, ecode, offset_top, md, ims, eptrb, 0))
			return TRUE;
		      if (i >= max || eptr >= md->end_subject || c == *eptr++)
			return FALSE;
		    }
		  /* Control never gets here */
		}
	      else
		{
		  const uschar *pp = eptr;
		  for (i = min; i < max; i++)
		    {
		      if (eptr >= md->end_subject || c == *eptr)
			break;
		      eptr++;
		    }

		  if (kind == KIND_MAX)
		    {
		      while (eptr >= pp)
			if (match (eptr--, ecode, offset_top, md, ims, eptrb, 0))
			  return TRUE;
		      return FALSE;
		    }
		}
	    }
	  continue;

	  /* Match a single character type repeatedly; several different opcodes
	     share code. This is very similar to the code for single characters, but we
	     repeat it in the interests of efficiency. */

	case OP_TYPEEXACT:
	  min = max = (ecode[1] << 8) + ecode[2];
	  kind = OP_TYPE_ONCEUPTO - OP_TYPE_MAXUPTO;
	  ecode += 3;
	  goto REPEATTYPE;

	case OP_TYPE_MAXUPTO:
	case OP_TYPE_MINUPTO:
	case OP_TYPE_ONCEUPTO:
	  min = 0;
	  max = (ecode[1] << 8) + ecode[2];
	  kind = *ecode - OP_TYPE_MAXUPTO;
	  ecode += 3;
	  goto REPEATTYPE;

	case OP_TYPE_MAXSTAR:
	case OP_TYPE_MINSTAR:
	case OP_TYPE_ONCESTAR:
	  kind = *ecode++ - OP_TYPE_MAXSTAR;
	  min = 0, max = INT_MAX;
	  goto REPEATTYPE;

	case OP_TYPE_MAXPLUS:
	case OP_TYPE_MINPLUS:
	case OP_TYPE_ONCEPLUS:
	  kind = *ecode++ - OP_TYPE_MAXPLUS;
	  min = 1, max = INT_MAX;
	  goto REPEATTYPE;

	case OP_TYPE_MAXQUERY:
	case OP_TYPE_MINQUERY:
	case OP_TYPE_ONCEQUERY:
	  kind = *ecode++ - OP_TYPE_MAXQUERY;
	  min = 0, max = 1;

	  /* Common code for all repeated single character type matches */

	REPEATTYPE:
	  ctype = 1 << *ecode++;	/* Code for the character type */

	  /* First, ensure the minimum number of matches are present. Use inline
	     code for maximizing the speed, and do the type test once at the start
	     (i.e. keep it out of the loop). Also we can test that there are at least
	     the minimum number of bytes before we start. */

	  if (min > md->end_subject - eptr)
	    return FALSE;
	  if (min > 0)
	    {
	      for (i = 1; i <= min; i++)
		if ((md->ctypes[*eptr++] & ctype) == 0)
		  return FALSE;
	    }

	  /* If min = max, continue at the same level without recursing */

	  if (min == max)
	    continue;

	  /* If minimizing, we have to test the rest of the pattern before each
	     subsequent match. */

	  if (kind == KIND_MIN)
	    {
	      for (i = min;; i++)
		{
		  if (match (eptr, ecode, offset_top, md, ims, eptrb, 0))
		    return TRUE;
		  if (i >= max || eptr >= md->end_subject)
		    return FALSE;

		  if ((md->ctypes[*eptr++] & ctype) == 0)
		    return FALSE;
		}
	      /* Control never gets here */
	    }

	  /* If maximizing it is worth using inline code for speed, doing the type
	     test once at the start (i.e. keep it out of the loop). */

	  else
	    {
	      const uschar *pp = eptr;
	      for (i = min; i < max; i++)
		{
		  if (eptr >= md->end_subject || (md->ctypes[*eptr] & ctype) == 0)
		    break;
		  eptr++;
		}


	      if (kind == KIND_MAX)
		{
		  while (eptr >= pp)
		    {
		      if (match (eptr--, ecode, offset_top, md, ims, eptrb, 0))
			return TRUE;
		    }
		  return FALSE;
		}
	    }

	  continue;

	  /* Match a single character type repeatedly; several different opcodes
	     share code. This is very similar to the code for single characters, but we
	     repeat it in the interests of efficiency. */

	case OP_TYPENOTEXACT:
	  min = max = (ecode[1] << 8) + ecode[2];
	  kind = OP_TYPENOT_ONCEUPTO - OP_TYPENOT_MAXUPTO;
	  ecode += 3;
	  goto REPEATTYPENOT;

	case OP_TYPENOT_MAXUPTO:
	case OP_TYPENOT_MINUPTO:
	case OP_TYPENOT_ONCEUPTO:
	  min = 0;
	  max = (ecode[1] << 8) + ecode[2];
	  kind = *ecode - OP_TYPENOT_MAXUPTO;
	  ecode += 3;
	  goto REPEATTYPENOT;

	case OP_TYPENOT_MAXSTAR:
	case OP_TYPENOT_MINSTAR:
	case OP_TYPENOT_ONCESTAR:
	  kind = *ecode++ - OP_TYPENOT_MAXSTAR;
	  min = 0, max = INT_MAX;
	  goto REPEATTYPENOT;

	case OP_TYPENOT_MAXPLUS:
	case OP_TYPENOT_MINPLUS:
	case OP_TYPENOT_ONCEPLUS:
	  kind = *ecode++ - OP_TYPENOT_MAXPLUS;
	  min = 1, max = INT_MAX;
	  goto REPEATTYPENOT;

	case OP_TYPENOT_MAXQUERY:
	case OP_TYPENOT_MINQUERY:
	case OP_TYPENOT_ONCEQUERY:
	  kind = *ecode++ - OP_TYPENOT_MAXQUERY;
	  min = 0, max = 1;

	  /* Common code for all repeated single character type matches */

	REPEATTYPENOT:
	  ctype = *ecode++;	/* Code for the character type */
	  if (ctype != 0 || !(ims & PCRE_DOTALL))
	    ctype = 1 << ctype;

	  /* First, ensure the minimum number of matches are present. Use inline
	     code for maximizing the speed, and do the type test once at the start
	     (i.e. keep it out of the loop). Also we can test that there are at least
	     the minimum number of bytes before we start. */

	  if (min > md->end_subject - eptr)
	    return FALSE;
	  if (min > 0)
	    {
	      for (i = 1; i <= min; i++)
		if (md->ctypes[*eptr++] & ctype)
		  return FALSE;
	    }

	  /* If min = max, continue at the same level without recursing */

	  if (min == max)
	    continue;

	  /* If minimizing, we have to test the rest of the pattern before each
	     subsequent match. */

	  if (kind == KIND_MIN)
	    {
	      for (i = min;; i++)
		{
		  if (match (eptr, ecode, offset_top, md, ims, eptrb, 0) != 0)
		    return TRUE;
		  if (i >= max || eptr >= md->end_subject)
		    return FALSE;

		  if (md->ctypes[*eptr++] & ctype)
		    return FALSE;
		}
	      /* Control never gets here */
	    }

	  /* If maximizing it is worth using inline code for speed, doing the type
	     test once at the start (i.e. keep it out of the loop). */

	  else
	    {
	      const uschar *pp = eptr;
	      if (ctype == 0)
		{
		  c = max - min;
		  if (c > md->end_subject - eptr)
		    c = md->end_subject - eptr;
		  eptr += c;
		}
	      else
		for (i = min; i < max; i++)
		  {
		    if (eptr >= md->end_subject || (md->ctypes[*eptr] & ctype))
		      break;
		    eptr++;
		  }

	      if (kind == KIND_MAX)
		{
		  while (eptr >= pp)
		    {
		      if (match (eptr--, ecode, offset_top, md, ims, eptrb, 0))
			return TRUE;
		    }
		  return FALSE;
		}
	    }

	  continue;

	  /* There's been some horrible disaster. */

	default:
	  DPRINTF (("Unknown opcode %d\n", *ecode));
	  md->errorcode = PCRE_ERROR_UNKNOWN_NODE;
	  return FALSE;
	}

      /* Do not stick any code in here without much thought; it is assumed
         that "continue" in the code above comes out to here to repeat the main
         loop. */

    }				/* End of main loop */

  /* Control never reaches here */
}




/*************************************************
*         Execute a Regular Expression           *
*************************************************/

/* This function applies a compiled re to a subject string and picks out
   portions of the string if it matches. Two elements in the vector are set for
   each substring: the offsets to the start and end of the substring.

   Arguments:
   external_re     points to the compiled expression
   external_extra  points to "hints" from pcre_study() or is NULL
   subject         points to the subject string
   length          length of subject string (may contain binary zeros)
   start_offset    where to start in the subject string
   options         option bits
   offsets         points to a vector of ints to be filled in with offsets
   offsetcount     the number of elements in the vector

   Returns:          > 0 => success; value is the number of elements filled in
   = 0 => success, but offsets is not big enough
   -1 => failed to match
   < -1 => some kind of unexpected problem
 */

int
pcre_exec (re, extra, subject, length, start_offset, options, offsets, offsetcount)
     const pcre *re;
     const pcre_extra *extra;
     const char *subject;
     int length;
     int start_offset;
     int options;
     int *offsets;
     int offsetcount;
{
  int resetcount, ocount;
  int first_char = -1;
  int req_char = -1;
  int req_char2 = -1;
  unsigned long int ims = 0;
  match_data match_block;
  const uschar *start_bits = NULL;
  const uschar *bmtable = NULL;
  const uschar *start_match;
  const uschar *end_subject;
  const uschar *req_char_ptr;
  BOOL using_temporary_offsets = FALSE;
  BOOL anchored;
  BOOL startline;

  if ((options & ~PUBLIC_EXEC_OPTIONS) != 0)
    return PCRE_ERROR_BADOPTION;

  if (re == NULL || subject == NULL ||
      (offsets == NULL && offsetcount > 0))
    return PCRE_ERROR_NULL;
  if (re->magic_number != MAGIC_NUMBER)
    return PCRE_ERROR_BADMAGIC;

  anchored = ((re->options | options) & PCRE_ANCHORED) != 0;
  startline = (re->options & PCRE_STARTLINE) != 0;

  match_block.start_pattern = re->code;
  match_block.start_subject = (const uschar *) subject;
  match_block.end_subject = match_block.start_subject + length;
  match_block.first_start = match_block.start_subject + start_offset;

  match_block.endonly = (re->options & PCRE_DOLLAR_ENDONLY) != 0;

  match_block.notbol = (options & PCRE_NOTBOL) != 0;
  match_block.noteol = (options & PCRE_NOTEOL) != 0;
  match_block.notempty = (options & PCRE_NOTEMPTY) != 0;

  match_block.errorcode = PCRE_ERROR_NOMATCH;	/* Default error */

  match_block.lcc = re->tables + lcc_offset;
  match_block.ctypes = re->tables + ctypes_offset;

  start_match = match_block.first_start;
  req_char_ptr = start_match - 1;
  end_subject = match_block.end_subject;

  /* For matches anchored to the end of the pattern, we can often avoid
     analyzing most of the pattern.  length > re->max_match_size is
     implied in the second condition, because start_offset > 0. */

  if (re->max_match_size >= 0
      && length - re->max_match_size > start_offset)
    start_match = (const uschar *) subject + length - re->max_match_size;

  /* The ims options can vary during the matching as a result of the presence
     of (?ims) items in the pattern. They are kept in a local variable so that
     restoring at the exit of a group is easy. */

  ims = re->options & (PCRE_CASELESS | PCRE_MULTILINE | PCRE_DOTALL);

   /* Set up the first character to match, if available. The first_char value is
      never set for an anchored regular expression, but the anchoring may be forced
      at run time, so we have to test for anchoring. The first char may be unset for
      an unanchored pattern, of course. If there's no first char and the pattern was
      studied, there may be a bitmap of possible first characters. */

  if (!anchored)
    {
      if ((re->options & PCRE_FIRSTSET) != 0)
	{
	  first_char = re->first_char;
	  if ((ims & PCRE_CASELESS) != 0)
	    first_char = match_block.lcc[first_char];
	}
      if (!startline && extra != NULL)
	{
	  if ((extra->options & PCRE_STUDY_MAPPED) != 0)
	    start_bits = extra->data.start_bits;
	  else if ((extra->options & PCRE_STUDY_BM) != 0)
	    {
	      bmtable = extra->data.bmtable;
	      if (start_match + bmtable[256] > end_subject)
		return PCRE_ERROR_NOMATCH;
	    }
	}
    }

  /* If the expression has got more back references than the offsets supplied can
     hold, we get a temporary bit of working store to use during the matching.
     Otherwise, we can use the vector supplied, rounding down its size to a multiple
     of 3. */

  ocount = offsetcount - (offsetcount % 3);

  if (re->top_backref > 0 && re->top_backref >= ocount / 3)
    {
      ocount = re->top_backref * 3 + 3;
      match_block.offset_vector = (int *) (pcre_malloc) (ocount * sizeof (int));
      if (match_block.offset_vector == NULL)
	return PCRE_ERROR_NOMEMORY;
      using_temporary_offsets = TRUE;
      DPRINTF (("Got memory to hold back references\n"));
    }
  else
    match_block.offset_vector = offsets;

  match_block.offset_end = ocount;
  match_block.offset_max = (2 * ocount) / 3;
  match_block.offset_overflow = FALSE;

  /* Compute the minimum number of offsets that we need to reset each time. Doing
     this makes a huge difference to execution time when there aren't many brackets
     in the pattern. */
  resetcount = 2 + re->top_bracket * 2;
  if (resetcount > offsetcount)
    resetcount = ocount;

  /* Reset the working variable associated with each extraction. These should
     never be used unless previously set, but they get saved and restored, and so we
     initialize them to avoid reading uninitialized locations. */
  if (match_block.offset_vector != NULL)
    {
      int *iptr = match_block.offset_vector + ocount;
      int *iend = iptr - resetcount / 2 + 1;
      while (--iptr >= iend)
	*iptr = -1;
    }

  /* For anchored or unanchored matches, there may be a "last known required
     character" set. If the PCRE_CASELESS is set, implying that the match starts
     caselessly, or if there are any changes of this flag within the regex, set up
     both cases of the character. Otherwise set the two values the same, which will
     avoid duplicate testing (which takes significant time). This covers the vast
     majority of cases. It will be suboptimal when the case flag changes in a regex
     and the required character in fact is caseful. */
  if ((re->options & PCRE_REQCHSET) != 0)
    {
      req_char = re->req_char;
      req_char2 = ((re->options & (PCRE_CASELESS | PCRE_ICHANGED)) != 0) ?
	(re->tables + fcc_offset)[req_char] : req_char;
    }

  /* Loop for handling unanchored repeated matching attempts; for anchored regexs
     the loop runs just once. */
  do
    {
      int rc;
      int *iptr = match_block.offset_vector;
      int *iend = iptr + resetcount;
#ifdef DEBUG
      int skipped_chars = 0;
#endif

      /* Reset the maximum number of extractions we might see. */

      while (iptr < iend)
	*iptr++ = -1;

      /* Advance to a possible match for an initial string after study */

      if (bmtable != NULL)
	{
#ifdef DEBUG
	  skipped_chars += bmtable[256] - 1;
#endif
	  start_match += bmtable[256] - 1;
	  while (start_match < end_subject)
	    {
	      if (bmtable[*start_match])
#ifdef DEBUG
		skipped_chars += bmtable[*start_match],
#endif
		start_match += bmtable[*start_match];
	      else
		{
#ifdef DEBUG
		  skipped_chars -= bmtable[256] - 1;
#endif
		  start_match -= bmtable[256] - 1;
		  break;
		}
	    }
	}

      /* Or to a unique first char if possible */

      else if (first_char >= 0)
	{
	  if ((ims & PCRE_CASELESS) != 0)
	    while (start_match < end_subject &&
		   match_block.lcc[*start_match] != first_char)
#ifdef DEBUG
	      skipped_chars++,
#endif
	      start_match++;
	  else
	    while (start_match < end_subject && *start_match != first_char)
#ifdef DEBUG
	      skipped_chars++,
#endif
	      start_match++;
	}

      /* Or to just after \n for a multiline match if possible */

      else if (startline)
	{
	  if (start_match > match_block.start_subject + start_offset)
	    {
	      while (start_match < end_subject && start_match[-1] != '\n')
#ifdef DEBUG
		skipped_chars++,
#endif
		start_match++;
	    }
	}

      /* Or to a non-unique first char after study */

      else if (start_bits != NULL)
	{
	  while (start_match < end_subject)
	    {
	      int c = *start_match;
	      if ((start_bits[c / 8] & (1 << (c & 7))) == 0)
#ifdef DEBUG
		skipped_chars++,
#endif
		start_match++;
	      else
		break;
	    }
	}

#ifdef DEBUG
      if (skipped_chars)
	printf (">>>>> Skipped %d chars to reach first character\n",
		skipped_chars);
      printf (">>>> Match against: ");
      pchars (start_match, end_subject - start_match, TRUE, &match_block);
      printf ("\n");
#endif

      /* If req_char is set, we know that that character must appear in the subject
         for the match to succeed. If the first character is set, req_char must be
         later in the subject; otherwise the test starts at the match point. This
         optimization can save a huge amount of backtracking in patterns with nested
         unlimited repeats that aren't going to match. We don't know what the state of
         case matching may be when this character is hit, so test for it in both its
         cases if necessary. However, the different cased versions will not be set up
         unless PCRE_CASELESS was given or the casing state changes within the regex.
         Writing separate code makes it go faster, as does using an autoincrement and
         backing off on a match. */

      if (req_char >= 0)
	{
	  const uschar *p = start_match + ((first_char >= 0) ? 1 : 0);

	  /* We don't need to repeat the search if we haven't yet reached the
	     place we found it at last time. */

	  if (p > req_char_ptr)
	    {
	      /* Do a single test if no case difference is set up */

	      if (req_char == req_char2)
		{
		  while (p < end_subject)
		    {
		      if (*p++ == req_char)
			{
			  p--;
			  break;
			}
		    }
		}

	      /* Otherwise test for either case */

	      else
		{
		  while (p < end_subject)
		    {
		      int pp = *p++;
		      if (pp == req_char || pp == req_char2)
			{
			  p--;
			  break;
			}
		    }
		}

	      /* If we can't find the required character, break the matching loop */

	      if (p >= end_subject)
		break;

	      /* If we have found the required character, save the point where we
	         found it, so that we don't search again next time round the loop if
	         the start hasn't passed this character yet. */

	      req_char_ptr = p;
	    }
	}

      /* When a match occurs, substrings will be set for all internal extractions;
         we just need to set up the whole thing as substring 0 before returning. If
         there were too many extractions, set the return code to zero. In the case
         where we had to get some local store to hold offsets for backreferences, copy
         those back references that we can. In this case there need not be overflow
         if certain parts of the pattern were not used. */

      match_block.start_match = start_match;
      if (!match (start_match, re->code, 2, &match_block, ims, NULL, match_isgroup))
	continue;

      /* Copy the offset information from temporary store if necessary */

      if (using_temporary_offsets)
	{
	  if (offsetcount >= 4)
	    {
	      memcpy (offsets + 2, match_block.offset_vector + 2,
		      (offsetcount - 2) * sizeof (int));
	      DPRINTF (("Copied offsets from temporary memory\n"));
	    }
	  if (match_block.end_offset_top > offsetcount)
	    match_block.offset_overflow = TRUE;

	  DPRINTF (("Freeing temporary memory\n"));
	  (pcre_free) (match_block.offset_vector);
	}

      rc = match_block.offset_overflow ? 0 : match_block.end_offset_top / 2;

      if (offsetcount < 2)
	rc = 0;
      else
	{
	  offsets[0] = start_match - match_block.start_subject;
	  offsets[1] = match_block.end_match_ptr - match_block.start_subject;
	}

      DPRINTF ((">>>> returning %d\n", rc));
      return rc;
    }

  /* This "while" is the end of the "do" above */
  while (!anchored &&
	 match_block.errorcode == PCRE_ERROR_NOMATCH &&
	 start_match++ < end_subject);

  if (using_temporary_offsets)
    {
      DPRINTF (("Freeing temporary memory\n"));
      (pcre_free) (match_block.offset_vector);
    }

  DPRINTF ((">>>> returning %d\n", match_block.errorcode));

  return match_block.errorcode;
}
