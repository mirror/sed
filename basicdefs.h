/*  GNU SED, a batch stream editor.
    Copyright (C) 1998, 1999, 2002, 2003, 2010 Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. */

#ifndef BASICDEFS_H
#define BASICDEFS_H

#include <alloca.h>
#include <wchar.h>
#include <locale.h>
#include <wctype.h>
#include <stdbool.h>

#include <gettext.h>
#define N_(String) gettext_noop(String)
#define _(String) gettext(String)

/* type countT is used to keep track of line numbers, etc. */
typedef unsigned long countT;

/* some basic definitions to avoid undue promulgating of  ugliness */
#define MALLOC(n,t)	 ((t *)ck_malloc((n)*sizeof(t)))
#define REALLOC(x,n,t)	 ((t *)ck_realloc((void *)(x),(n)*sizeof(t)))
#define MEMDUP(x,n,t)	 ((t *)ck_memdup((void *)(x),(n)*sizeof(t)))
#define OB_MALLOC(o,n,t) ((t *)(void *)obstack_alloc(o,(n)*sizeof(t)))

#define obstack_chunk_alloc  ck_malloc
#define obstack_chunk_free   free


/* handle misdesigned <ctype.h> macros (snarfed from lib/regex.c) */
/* Jim Meyering writes:

   "... Some ctype macros are valid only for character codes that
   isascii says are ASCII (SGI's IRIX-4.0.5 is one such system --when
   using /bin/cc or gcc but without giving an ansi option).  So, all
   ctype uses should be through macros like ISPRINT...  If
   STDC_HEADERS is defined, then autoconf has verified that the ctype
   macros don't need to be guarded with references to isascii. ...
   Defining isascii to 1 should let any compiler worth its salt
   eliminate the && through constant folding."
   Solaris defines some of these symbols so we must undefine them first. */

#undef ISASCII
#if defined STDC_HEADERS || (!defined isascii && !defined HAVE_ISASCII)
# define ISASCII(c) 1
#else
# define ISASCII(c) isascii(c)
#endif

#if defined isblank || defined HAVE_ISBLANK
# define ISBLANK(c) (ISASCII (c) && isblank (c))
#else
# define ISBLANK(c) ((c) == ' ' || (c) == '\t')
#endif

#undef ISPRINT
#define ISPRINT(c) (ISASCII (c) && isprint (c))
#define ISDIGIT(c) (ISASCII (c) && isdigit ((unsigned char) (c)))
#define ISALNUM(c) (ISASCII (c) && isalnum (c))
#define ISALPHA(c) (ISASCII (c) && isalpha (c))
#define ISCNTRL(c) (ISASCII (c) && iscntrl (c))
#define ISLOWER(c) (ISASCII (c) && islower (c))
#define ISPUNCT(c) (ISASCII (c) && ispunct (c))
#define ISSPACE(c) (ISASCII (c) && isspace (c))
#define ISUPPER(c) (ISASCII (c) && isupper (c))
#define ISXDIGIT(c) (ISASCII (c) && isxdigit (c))

#ifndef initialize_main
# ifdef __EMX__
#  define initialize_main(argcp, argvp) \
  { _response(argcp, argvp); _wildcard(argcp, argvp); }
# else /* NOT __EMX__ */
#  define initialize_main(argcp, argvp)
# endif
#endif

#endif /*!BASICDEFS_H*/
