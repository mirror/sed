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

#include "internal.h"

/*************************************************
*        Return info about compiled pattern      *
*************************************************/

/* This is a newer "info" function which has an extensible interface so
   that additional items can be added compatibly.

   Arguments:
   external_re      points to compiled code
   external_study   points to study data, or NULL
   what             what information is required
   where            where to put the information

   Returns:           0 if data returned, negative on error
 */

int
pcre_info (re, study, what, where)
     const pcre *re;
     const pcre_extra *study;
     int what;
     void *where;
{
  if (re == NULL || where == NULL)
    return PCRE_ERROR_NULL;
  if (re->magic_number != MAGIC_NUMBER)
    return PCRE_ERROR_BADMAGIC;

  switch (what)
    {
    case PCRE_INFO_OPTIONS:
      *((unsigned long int *) where) = re->options & PUBLIC_OPTIONS;
      break;

    case PCRE_INFO_SIZE:
      *((size_t *) where) = re->size;
      break;

    case PCRE_INFO_CAPTURECOUNT:
      *((int *) where) = re->top_bracket;
      break;

    case PCRE_INFO_BACKREFMAX:
      *((int *) where) = re->top_backref;
      break;

    case PCRE_INFO_FIRSTCHAR:
      *((int *) where) =
	((re->options & PCRE_FIRSTSET) != 0) ? re->first_char :
	((re->options & PCRE_STARTLINE) != 0) ? -1 : -2;
      break;

    case PCRE_INFO_BMTABLE:
      *((const uschar **) where) =
	(study != NULL && (study->options & PCRE_STUDY_BM) != 0) ?
	study->data.bmtable : NULL;
      break;

    case PCRE_INFO_FIRSTTABLE:
      *((const uschar **) where) =
	(study != NULL && (study->options & PCRE_STUDY_MAPPED) != 0) ?
	study->data.start_bits : NULL;
      break;

    case PCRE_INFO_LASTLITERAL:
      *((int *) where) =
	((re->options & PCRE_REQCHSET) != 0) ? re->req_char : -1;
      break;

    default:
      return PCRE_ERROR_BADOPTION;
    }

  return 0;
}
