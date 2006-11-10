/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/*
   This is a library of functions to support regular expressions whose syntax
   and semantics are as close as possible to those of the Perl 5 language. See
   the file Tech.Notes for some information on the internals.

   This module is a wrapper that provides a POSIX API to the underlying PCRE
   functions.

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
#include "regexp.h"
#include "stdlib.h"


/* Corresponding tables of PCRE error messages and POSIX error codes. */

const char *pcre_estrings[] =
{
  "",
  N_("\\ at end of pattern"),
  N_("\\c at end of pattern"),
  N_("unrecognized character follows \\"),
  N_("numbers out of order in {} quantifier"),
  N_("number too big in {} quantifier"),
  N_("missing terminating ] for character class"),
  N_("invalid escape sequence in character class"),
  N_("range out of order in character class"),
  N_("nothing to repeat"),
  N_("operand of unlimited repeat could match the empty string"),
  N_("internal error: unexpected repeat"),
  N_("unrecognized character after (?"),
  N_("unused error"),
  N_("unmatched braces"),
  N_("back reference to non-existent subpattern"),
  N_("erroffset passed as NULL"),
  N_("unknown option bit(s) set"),
  N_("missing ) after comment"),
  N_("parentheses nested too deeply"),
  N_("regular expression too large"),
  N_("failed to get memory"),
  N_("unmatched parentheses"),
  N_("internal error: code overflow"),
  N_("unrecognized character after (?<"),
  N_("lookbehind assertion is not fixed length"),
  N_("malformed number after (?("),
  N_("conditional group contains more than two branches"),
  N_("assertion expected after (?("),
  N_("(?p must be followed by )"),
  N_("unknown POSIX class name"),
  N_("POSIX collating elements are not supported"),
  N_("bad condition (?(0)")
};

static int eint[] =
{
  0,
  REG_EESCAPE,	/* \\ at end of pattern */
  REG_EESCAPE,	/* \\c at end of pattern */
  REG_EESCAPE,	/* unrecognized character follows \\ */
  REG_BADBR,	/* numbers out of order in {} quantifier */
  REG_BADBR,	/* number too big in {} quantifier */
  REG_EBRACK,	/* missing terminating ] for character class */
  REG_ECTYPE,	/* invalid escape sequence in character class */
  REG_ERANGE,	/* range out of order in character class */
  REG_BADRPT,	/* nothing to repeat */
  REG_BADRPT,	/* operand of unlimited repeat could match the empty string */
  REG_ASSERT,	/* internal error: unexpected repeat */
  REG_BADPAT,	/* unrecognized character after (? */
  REG_ASSERT,	/* unused error */
  REG_EPAREN,	/* missing ) */
  REG_ESUBREG,	/* back reference to non-existent subpattern */
  REG_INVARG,	/* erroffset passed as NULL */
  REG_INVARG,	/* unknown option bit(s) set */
  REG_EPAREN,	/* missing ) after comment */
  REG_ESIZE,	/* parentheses nested too deeply */
  REG_ESIZE,	/* regular expression too large */
  REG_ESPACE,	/* failed to get memory */
  REG_EPAREN,	/* unmatched brackets */
  REG_ASSERT,	/* internal error: code overflow */
  REG_BADPAT,	/* unrecognized character after (?< */
  REG_BADPAT,	/* lookbehind assertion is not fixed length */
  REG_BADPAT,	/* malformed number after (?( */
  REG_BADPAT,	/* conditional group containe more than two branches */
  REG_BADPAT,	/* assertion expected after (?( */
  REG_BADPAT,	/* (?p must be followed by ) */
  REG_ECTYPE,	/* unknown POSIX class name */
  REG_BADPAT,	/* POSIX collating elements are not supported */
  REG_BADPAT,	/* character value in \x{...} sequence is too large */
  REG_BADPAT	/* invalid condition (?(0) */
};

/* Table of texts corresponding to POSIX error codes */

static const char *pstring[] =
{
  "",
  N_("internal error"),
  N_("invalid repeat counts in {}"),
  N_("pattern error"),
  N_("nothing to repeat"),
  N_("unmatched braces"),
  N_("missing terminating ] for character class"),
  N_("bad collating element"),
  N_("unknown POSIX class name"),
  N_("bad escape sequence"),
  N_("empty expression"),
  N_("unmatched parentheses"),
  N_("range out of order in character class"),
  N_("regular expression too large"),
  N_("failed to get memory"),
  N_("back reference to non-existent subpattern"),
  N_("bad argument"),
  N_("match failed")
};




/*************************************************
*          Translate PCRE text code to int       *
*************************************************/

/* PCRE compile-time errors are given as strings defined as macros. We can just
   look them up in a table to turn them into POSIX-style error codes. */

static int
pcre_posix_error_code (s)
     const char *s;
{
  size_t i;
  for (i = 1; i < sizeof (pcre_estrings) / sizeof (char *); i++)
    if (strcmp (s, pcre_estrings[i]) == 0)
      return eint[i];
  return REG_ASSERT;
}



/*************************************************
*          Translate error code to string        *
*************************************************/

size_t
regerror (errcode, preg, errbuf, errbuf_size)
     int errcode;
     const regex_t *preg;
     char *errbuf;
     size_t errbuf_size;
{
  const char *message, *addmessage;
  size_t size;

  message = (errcode >= (int) (sizeof (pstring) / sizeof (char *))) ?
    _("unknown error code") : gettext(pstring[errcode]);

  addmessage = _("%s at offset %-6d");

  if (errbuf_size > 0)
    {
      if (preg != NULL && (int) preg->re_erroffset != -1)
	size = snprintf (errbuf, errbuf_size - 1, addmessage,
		  message, (int) preg->re_erroffset);
      else
	  strncpy (errbuf, message, errbuf_size - 1);
    }

  errbuf[errbuf_size - 1] = 0;
  return strlen(errbuf);
}




/*************************************************
*           Free store held by a regex           *
*************************************************/

void
regfree (preg)
     regex_t *preg;
{
  (pcre_free) (preg->re_pcre);
}




/*************************************************
*            Compile a regular expression        *
*************************************************/

/*
   Arguments:
   preg        points to a structure for recording the compiled expression
   pattern     the pattern to compile
   cflags      compilation flags

   Returns:      0 on success
   various non-zero codes on failure
 */

int
regcomp (preg, pattern, cflags)
     regex_t *preg;
     const char *pattern;
     int cflags;
{
  return regncomp (preg, pattern, (int) strlen (pattern), cflags);
}

int
regncomp (preg, pattern, length, cflags)
     regex_t *preg;
     const char *pattern;
     int length, cflags;
{
  const char *errptr;
  int errofs;
  int options = PCRE_ENGLISH_ERRORS;
  int count;

  options |= (cflags & REG_ICASE) ? PCRE_CASELESS : 0;
  options |= (cflags & REG_EXTENDED) ? PCRE_EXTENDED : 0;

  if ((cflags & REG_PERL) != 0)
    {
      options |= (cflags & REG_NEWLINE) ? PCRE_MULTILINE : 0;
      options |= (cflags & REG_DOTALL) ? PCRE_DOTALL : 0;
      preg->re_pcre = pcre_compile_nuls (pattern, length, options,
					      &errptr, &errofs, NULL);
    }
  else
    {
      options |= (cflags & REG_NEWLINE) ? PCRE_MULTILINE : PCRE_DOTALL;
      preg->re_pcre = pcre_posix_compile_nuls (pattern, length, options,
					       &errptr, &errofs, NULL);
    }

  preg->re_erroffset = errofs;

  if (preg->re_pcre == NULL)
    return pcre_posix_error_code (errptr);

  preg->re_study = pcre_study (preg->re_pcre, 0, &errptr);

  pcre_info (preg->re_pcre, preg->re_study,
	     PCRE_INFO_CAPTURECOUNT, &count);
  preg->re_nsub = count;
  return 0;
}




/*************************************************
*              Match a regular expression        *
*************************************************/

int
regexec (preg, string, nmatch, pmatch, eflags)
     regex_t *preg;
     const char *string;
     size_t nmatch;
     regmatch_t pmatch[];
     int eflags;
{

  if (eflags & REG_STARTEND)
    return regnexec (preg, string, 0, nmatch, pmatch, eflags);
  else
    return regnexec (preg, string, (int) strlen (string),
		     nmatch, pmatch, eflags);
}

int
regnexec (preg, string, length, nmatch, pmatch, eflags)
     regex_t *preg;
     const char *string;
     int length;
     size_t nmatch;
     regmatch_t pmatch[];
     int eflags;
{
  int start;
  int rc;
  int options;
  int *ovector = NULL;

  if (eflags & REG_STARTEND)
    {
      start = pmatch[0].rm_so;
      length = pmatch[0].rm_eo;
    }
  else
    start = 0;

  options =
    ((eflags & REG_NOTBOL) ? PCRE_NOTBOL : 0) |
    ((eflags & REG_NOTEOL) ? PCRE_NOTEOL : 0);

  preg->re_erroffset = (size_t) (-1);	/* Only has meaning after compile */

  /* Unfortunately, PCRE requires 3 ints of working space for each captured
     substring, so we have to get and release working store instead of just using
     the POSIX structures as was done in earlier releases when PCRE needed only 2
     ints. */

  ovector = (int *) (pcre_malloc) (sizeof (int) * (preg->re_nsub * 3 + 3));
  if (ovector == NULL)
    return REG_ESPACE;

  rc = pcre_exec (preg->re_pcre, preg->re_study, string, length,
		  start, options, ovector, preg->re_nsub * 3 + 3);

  if (rc > 0)
    {
      size_t i, max;
      max = rc < nmatch ? rc : nmatch;

      for (i = 0; i < max; i++)
	{
	  pmatch[i].rm_so = ovector[i * 2];
	  pmatch[i].rm_eo = ovector[i * 2 + 1];
	}
      for (; i < nmatch; i++)
	pmatch[i].rm_so = pmatch[i].rm_eo = -1;

      (pcre_free) (ovector);
      return 0;
    }

  else
    {
      (pcre_free) (ovector);
      switch (rc)
	{
	case PCRE_ERROR_NOMATCH:
	  return REG_NOMATCH;
	case PCRE_ERROR_NULL:
	  return REG_INVARG;
	case PCRE_ERROR_BADOPTION:
	  return REG_INVARG;
	case PCRE_ERROR_BADMAGIC:
	  return REG_INVARG;
	case PCRE_ERROR_UNKNOWN_NODE:
	  return REG_ASSERT;
	case PCRE_ERROR_NOMEMORY:
	  return REG_ESPACE;
	default:
	  return REG_ASSERT;
	}
    }
}


