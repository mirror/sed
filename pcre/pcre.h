/*************************************************
*       Perl-Compatible Regular Expressions      *
*************************************************/

/* Copyright (c) 1997-2000 University of Cambridge */

#ifndef _PCRE_H
#define _PCRE_H

/* Have to include stdlib.h in order to ensure that size_t is defined;
   it is needed here for malloc. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef BOOTSTRAP
#include <malloc.h>
#endif

/* Allow for C++ users */

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __STDC__
#define __PCRE_PROTO(a) a
#else
#define __PCRE_PROTO(a) ()
#endif

/* Options */

#define PCRE_CASELESS        0x0001
#define PCRE_MULTILINE       0x0002
#define PCRE_DOTALL          0x0004
#define PCRE_EXTENDED        0x0008
#define PCRE_ANCHORED        0x0010
#define PCRE_DOLLAR_ENDONLY  0x0020
#define PCRE_EXTRA           0x0040
#define PCRE_NOTBOL          0x0080
#define PCRE_NOTEOL          0x0100
#define PCRE_UNGREEDY        0x0200
#define PCRE_NOTEMPTY        0x0400
#define PCRE_ENGLISH_ERRORS  0x0800
#define PCRE_STUDY_NO_PRUNE  0x04
#define PCRE_STUDY_NO_START  0x08

/* Exec-time and get-time error codes */

#define PCRE_ERROR_NOMATCH        (-1)
#define PCRE_ERROR_NULL           (-2)
#define PCRE_ERROR_BADOPTION      (-3)
#define PCRE_ERROR_BADMAGIC       (-4)
#define PCRE_ERROR_UNKNOWN_NODE   (-5)
#define PCRE_ERROR_NOMEMORY       (-6)
#define PCRE_ERROR_NOSUBSTRING    (-7)

/* Request types for pcre_fullinfo() */

#define PCRE_INFO_OPTIONS         0
#define PCRE_INFO_SIZE            1
#define PCRE_INFO_CAPTURECOUNT    2
#define PCRE_INFO_BACKREFMAX      3
#define PCRE_INFO_FIRSTCHAR       4
#define PCRE_INFO_FIRSTTABLE      5
#define PCRE_INFO_LASTLITERAL     6
#define PCRE_INFO_BMTABLE         7

/* Types */

struct pcre;
struct pcre_extra;
typedef struct pcre pcre;
typedef struct pcre_extra pcre_extra;

/* Store get and free functions. These can be set to alternative malloc/free
   functions if required. */

extern void *(*pcre_malloc) (size_t);
extern void (*pcre_free) (void *);

/* Functions */

extern pcre *pcre_posix_compile __PCRE_PROTO ((const char *, int, const char **,
					       int *, const unsigned char *));
extern pcre *pcre_compile __PCRE_PROTO ((const char *, int, const char **,
					 int *, const unsigned char *));
extern pcre *pcre_posix_compile_nuls __PCRE_PROTO ((const char *, int, int,
						    const char **, int *,
						    const unsigned char *));
extern pcre *pcre_compile_nuls __PCRE_PROTO ((const char *, int, int,
					      const char **, int *,
					      const unsigned char *));
extern int pcre_copy_substring __PCRE_PROTO ((const char *, int *, int, int,
					      char *, int));
extern int pcre_exec __PCRE_PROTO ((const pcre *, const pcre_extra *,
				    const char *, int, int, int, int *, int));
extern void pcre_free_substring __PCRE_PROTO ((const char *));
extern void pcre_free_substring_list __PCRE_PROTO ((const char **));
extern int pcre_get_substring __PCRE_PROTO ((const char *, int *, int, int,
					     const char **));
extern int pcre_get_substring_list __PCRE_PROTO ((const char *, int *, int,
						  const char ***));
extern int pcre_info __PCRE_PROTO ((const pcre *, const pcre_extra *, int,
				    void *));
extern const unsigned char *pcre_maketables __PCRE_PROTO ((void));
extern pcre_extra *pcre_study __PCRE_PROTO ((const pcre *, int, const char **));

#ifdef __cplusplus
}				/* extern "C" */
#endif
#undef __PCRE_PROTO

#endif
