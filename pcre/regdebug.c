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

const char *pcre_OP_names[] =
{
  "End", "\\<", "\\>", "\\G", "\\A", "\\B", "\\b",
  "\\Z", "\\z",
  "Opt", "^", "$", "Any",
  "chars", "*", "*?", " once *", "+", "+?", " once +", "?", "??", " once ?", "{", "{", " once {", "{",
  "not", "*", "*?", " once *", "+", "+?", " once +", "?", "??", " once ?", "{", "{", " once {", "{",
  "", "*", "*?", " once *", "+", "+?", " once +", "?", "??", " once ?", "{", "{", " once {", "{",
  "", "*", "*?", " once *", "+", "+?", " once +", "?", "??", " once ?", "{", "{", " once {", "{",
  "", "*", "*?", " once *", "+", "+?", " once +", "?", "??", " once ?", "{", "{", " once {",
  "", "*", "*?", " once *", "+", "+?", " once +", "?", "??", " once ?", "{", "{", " once {",
  "Recurse",
  "Alt", "Ket", "Ket*", "Ket*?", "Ket once *", "Assert", "Assert not",
  "AssertB", "AssertB not", "Reverse", "Once", "Cond", "Cref",
  "Brazero", "Braminzero", "Branumber", "Bra"
};

static const char *TYPE_names[] =
{
  "invalid", "\\d", "\\s", "\\w"
};

static const char *TYPENOT_names[] =
{
  "Any", "\\D", "\\S", "\\W"
};


/*************************************************
*        Debugging function to print regexp      *
*************************************************/

void
pcre_debug (re)
     pcre *re;
{
  uschar *code, *code_base;
  int c;

  printf ("Computed size = %d top_bracket = %d top_backref = %d\n",
	  re->size, re->top_bracket, re->top_backref);

  if (re->max_match_size >= 0)
    printf ("Can match up to %d characters\n",
	     re->max_match_size);


  if (re->options != 0)
    {
      printf ("%s%s%s%s%s%s%s%s\n",
	      ((re->options & PCRE_ANCHORED) != 0) ? "anchored  " : "",
	      ((re->options & PCRE_CASELESS) != 0) ? "caseless  " : "",
	     ((re->options & PCRE_ICHANGED) != 0) ? "case state changed  " : "",
	      ((re->options & PCRE_MULTILINE) != 0) ? "multiline  " : "",
	      ((re->options & PCRE_DOTALL) != 0) ? "dotall  " : "",
	      ((re->options & PCRE_DOLLAR_ENDONLY) != 0) ? "endonly  " : "",
	      ((re->options & PCRE_EXTRA) != 0) ? "extra  " : "",
	      ((re->options & PCRE_UNGREEDY) != 0) ? "ungreedy" : "");
    }

  if ((re->options & PCRE_FIRSTSET) != 0)
    {
      if (isascii (re->first_char) && isprint (re->first_char))
	printf ("First char = %c\n", re->first_char);
      else
	printf ("First char = \\x%02x\n", re->first_char);
    }

  if ((re->options & PCRE_REQCHSET) != 0)
    {
      if (isascii (re->req_char) && isprint (re->req_char))
	printf ("Req char = %c\n", re->req_char);
      else
	printf ("Req char = \\x%02x\n", re->req_char);
    }

  code_base = code = re->code;

  for (;;)
    {
      int charlength;

      printf ("%3d ", code - code_base);

      if (*code >= OP_BRA)
	{
	  if (*code - OP_BRA > EXTRACT_BASIC_MAX)
	    printf("%3d Bra extra", (code[1] << 8) + code[2]);
	  else
	    printf("%3d Bra %d", (code[1] << 8) + code[2], *code - OP_BRA);
 
	  code += 2;
	}

      else
	switch (*code)
	  {
	  case OP_OPT:
	    printf (" %.2x %s", code[1], pcre_OP_names[*code]);
	    code++;
	    break;

	  case OP_CHARS:
	    charlength = *(++code);
	    printf ("%3d ", charlength);
	    while (charlength-- > 0)
	      {
		c = *(++code);
		if (isascii (c) && isprint (c))
		  printf ("%c", c);
		else
		  printf ("\\x%02x", c);
	      }
	    break;

	  case OP_KET_MAXSTAR:
	  case OP_KET_MINSTAR:
	  case OP_KET_ONCESTAR:
	  case OP_ALT:
	  case OP_KET:
	  case OP_ASSERT:
	  case OP_ASSERT_NOT:
	  case OP_ASSERTBACK:
	  case OP_ASSERTBACK_NOT:
	  case OP_ONCE:
	  case OP_REVERSE:
	  case OP_BRANUMBER:
	  case OP_COND:
	  case OP_CREF:
	    printf ("%3d %s", (code[1] << 8) + code[2], pcre_OP_names[*code]);
	    code += 2;
	    break;

	  case OP_MAXSTAR:
	  case OP_MINSTAR:
	  case OP_ONCESTAR:
	  case OP_MAXPLUS:
	  case OP_MINPLUS:
	  case OP_ONCEPLUS:
	  case OP_MAXQUERY:
	  case OP_MINQUERY:
	  case OP_ONCEQUERY:
	    c = code[1];
	    if (isascii (c) && isprint (c))
	      printf ("    %c", c);
	    else
	      printf ("    \\x%02x", c);
	    printf ("%s", pcre_OP_names[*code++]);
	    break;

	  case OP_TYPE:
	  case OP_TYPE_MAXSTAR:
	  case OP_TYPE_MINSTAR:
	  case OP_TYPE_ONCESTAR:
	  case OP_TYPE_MAXPLUS:
	  case OP_TYPE_MINPLUS:
	  case OP_TYPE_ONCEPLUS:
	  case OP_TYPE_MAXQUERY:
	  case OP_TYPE_MINQUERY:
	  case OP_TYPE_ONCEQUERY:
	  case OP_TYPENOT:
	  case OP_TYPENOT_MAXSTAR:
	  case OP_TYPENOT_MINSTAR:
	  case OP_TYPENOT_ONCESTAR:
	  case OP_TYPENOT_MAXPLUS:
	  case OP_TYPENOT_MINPLUS:
	  case OP_TYPENOT_ONCEPLUS:
	  case OP_TYPENOT_MAXQUERY:
	  case OP_TYPENOT_MINQUERY:
	  case OP_TYPENOT_ONCEQUERY:
	    c = code[1];
	    if (*code >= OP_TYPENOT)
	      printf ("    %s", TYPENOT_names[c]);
	    else
	      printf ("    %s", TYPE_names[c]);
	    printf ("%s", pcre_OP_names[*code]);
	    code++;
	    break;

	  case OP_EXACT:
	  case OP_MAXUPTO:
	  case OP_MINUPTO:
	  case OP_ONCEUPTO:
	    c = code[3];
	    if (isascii (c) && isprint (c))
	      printf ("    %c%s", c, pcre_OP_names[*code]);
	    else
	      printf ("    \\x%02x%s", c, pcre_OP_names[*code]);
	    if (*code != OP_EXACT)
	      printf ("0,");
	    printf ("%d}", (code[1] << 8) + code[2]);
	    if (*code == OP_MINUPTO)
	      printf ("?");
	    code += 3;
	    break;

	  case OP_TYPEEXACT:
	  case OP_TYPE_MAXUPTO:
	  case OP_TYPE_MINUPTO:
	  case OP_TYPE_ONCEUPTO:
	    printf ("    %s%s", TYPE_names[code[3]], pcre_OP_names[*code]);
	    if (*code != OP_TYPEEXACT)
	      printf (",");
	    printf ("%d}", (code[1] << 8) + code[2]);
	    if (*code == OP_TYPE_MINUPTO)
	      printf ("?");
	    code += 3;
	    break;

	  case OP_TYPENOTEXACT:
	  case OP_TYPENOT_MAXUPTO:
	  case OP_TYPENOT_MINUPTO:
	  case OP_TYPENOT_ONCEUPTO:
	    printf ("    %s%s", TYPENOT_names[code[3]], pcre_OP_names[*code]);
	    if (*code != OP_TYPENOTEXACT)
	      printf (",");
	    printf ("%d}", (code[1] << 8) + code[2]);
	    if (*code == OP_TYPENOT_MINUPTO)
	      printf ("?");
	    code += 3;
	    break;

	  case OP_NOT:
	    c = *(++code);

	    if (isascii (c) && isprint (c))
	      printf ("    [^%c]", c);
	    else
	      printf ("    [^\\x%02x]", c);
	    break;

	  case OP_NOT_MAXSTAR:
	  case OP_NOT_MINSTAR:
	  case OP_NOT_ONCESTAR:
	  case OP_NOT_MAXPLUS:
	  case OP_NOT_MINPLUS:
	  case OP_NOT_ONCEPLUS:
	  case OP_NOT_MAXQUERY:
	  case OP_NOT_MINQUERY:
	  case OP_NOT_ONCEQUERY:
	    c = code[1];
	    if (isascii (c) && isprint (c))
	      printf ("    [^%c]", c);
	    else
	      printf ("    [^\\x%02x]", c);
	    printf ("%s", pcre_OP_names[*code++]);
	    break;

	  case OP_NOTEXACT:
	  case OP_NOT_MAXUPTO:
	  case OP_NOT_MINUPTO:
	  case OP_NOT_ONCEUPTO:
	    c = code[3];
	    if (isascii (c) && isprint (c))
	      printf ("    [^%c]%s", c, pcre_OP_names[*code]);
	    else
	      printf ("    [^\\x%02x]%s", c, pcre_OP_names[*code]);
	    if (*code != OP_NOTEXACT)
	      printf (",");
	    printf ("%d}", (code[1] << 8) + code[2]);
	    if (*code == OP_NOT_MINUPTO)
	      printf ("?");
	    code += 3;
	    break;

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
 	    c = *code - OP_REF + OP_CLASS;
	    printf ("    \\%d", code[1] * 256 + code[2]);
	    code += 2;
	    goto CLASS_REF_REPEAT;

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
	      int i, min, max;
	      c = *code++;
	      printf ("    [");

	      for (i = 0; i < 256; i++)
		{
		  if ((code[i / 8] & (1 << (i & 7))) != 0)
		    {
		      int j;
		      for (j = i + 1; j < 256; j++)
			if ((code[j / 8] & (1 << (j & 7))) == 0)
			  break;
		      if (i == '-' || i == ']')
			printf ("\\");
		      if (isascii (i) && isprint (i))
			printf ("%c", i);
		      else
			printf ("\\x%02x", i);
		      if (--j > i)
			{
			  printf ("-");
			  if (j == '-' || j == ']')
			    printf ("\\");
			  if (isascii (j) && isprint (j))
			    printf ("%c", j);
			  else
			    printf ("\\x%02x", j);
			}
		      i = j;
		    }
		}
	      printf ("]");
	      code += 31;

	    CLASS_REF_REPEAT:

	      printf ("%s", pcre_OP_names[c]);
	      
	      if (c == OP_CL_MAXRANGE || c == OP_CL_MINRANGE || c == OP_CL_ONCERANGE ||
		  c == OP_REF_MAXRANGE || c == OP_REF_MINRANGE || c == OP_REF_ONCERANGE)
		{
		  min = (code[1] << 8) + code[2];
		  max = (code[3] << 8) + code[4];
		  if (max == 0)
		    printf ("%d,}", min);
		  else
		    printf ("%d,%d}", min, max);
		  if (c == OP_CL_MINRANGE || c == OP_REF_MINRANGE)
		    printf ("?");
		  code += 4;
		  break;
		}
	    }
	    break;

          case OP_END:
	    printf ("    %s\n", pcre_OP_names[*code]);
	    printf ("------------------------------------------------------------------\n");
	    return;
	    
	    /* Anything else is just a one-node item */

	  default:
	    printf ("    %s", pcre_OP_names[*code]);
	    break;
	  }

      code++;
      printf ("\n");
    }
}

