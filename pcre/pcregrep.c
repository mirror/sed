/*************************************************
*               pcregrep program                 *
*************************************************/

/* This is a grep program that uses the PCRE regular expression library to do
   its pattern matching. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#endif
#include <errno.h>
#include "pcre.h"

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
#else
# define gettext(s)       (s)
#endif
#define _(s)              gettext(s)

#define FALSE 0
#define TRUE 1

typedef int BOOL;



/*************************************************
*               Global variables                 *
*************************************************/

static pcre *pattern;
static pcre_extra *hints;

static BOOL count_only = FALSE;
static BOOL filenames_only = FALSE;
static BOOL invert = FALSE;
static BOOL number = FALSE;
static BOOL silent = FALSE;
static BOOL whole_lines = FALSE;




/*************************************************
*              Grep an individual file           *
*************************************************/

static int
pcregrep (in, name)
     FILE *in;
     char *name;
{
  int rc = 1;
  int linenumber = 0;
  int count = 0;
  int offsets[99];
  char buffer[BUFSIZ];

  while (fgets (buffer, sizeof (buffer), in) != NULL)
    {
      BOOL match;
      int length = (int) strlen (buffer);
      if (length > 0 && buffer[length - 1] == '\n')
	buffer[--length] = 0;
      linenumber++;

      match = pcre_exec (pattern, hints, buffer, length, 0, 0, offsets, 99) >= 0;
      if (match && whole_lines && offsets[1] != length)
	match = FALSE;

      if (match != invert)
	{
	  if (count_only)
	    count++;

	  else if (filenames_only)
	    {
	      printf("%s\n", (name == NULL) ? "<stdin>" : name);
	      return 0;
	    }

	  else if (silent)
	    return 0;

	  else
	    {
	      if (name != NULL)
		printf("%s:", name);
	      if (number)
		printf("%d:", linenumber);
	      printf("%s\n", buffer);
	    }

	  rc = 0;
	}
    }

  if (count_only)
    {
      if (name != NULL)
	printf("%s:", name);
      printf("%d\n", count);
    }

  return rc;
}




/*************************************************
*                Usage function                  *
*************************************************/

static int
usage (rc)
     int rc;
{
  fprintf (stderr, _("Usage: pcregrep [-bepchilnsvx] pattern [file] ...\n"));
  return rc;
}




/*************************************************
*                Main program                    *
*************************************************/

int
main (argc, argv)
     int argc;
     char **argv;
{
  int i;
  int rc = 1;
  int options = 0;
  int errptr;
  char kind;
  const char *error;
  BOOL filenames = TRUE;

#if HAVE_SETLOCALE
  /* Set locale according to user's wishes.  */
  setlocale (LC_ALL, "");
#endif

#if ENABLE_NLS
  /* Tell program which translations to use and where to find.  */
  /*  bindtextdomain (PACKAGE, LOCALEDIR); */
  textdomain (PACKAGE);
#endif

  /* Process the options */
  kind = 'p';
  for (i = 1; i < argc; i++)
    {
      char *s;
      if (argv[i][0] != '-')
	break;
      s = argv[i] + 1;
      while (*s != 0)
	{
	  switch (*s++)
	    {
	    case 'c':
	      count_only = TRUE;
	      break;
	    case 'b':
	      kind = 'b';
	      break;
	    case 'e':
	      kind = 'e';
	      break;
	    case 'p':
	      kind = 'p';
	      break;
	    case 'h':
	      filenames = FALSE;
	      break;
	    case 'i':
	      options |= PCRE_CASELESS;
	      break;
	    case 'l':
	      filenames_only = TRUE;
	    case 'n':
	      number = TRUE;
	      break;
	    case 's':
	      silent = TRUE;
	      break;
	    case 'v':
	      invert = TRUE;
	      break;
	    case 'x':
	      whole_lines = TRUE;
	      options |= PCRE_ANCHORED;
	      break;

	    default:
	      fprintf (stderr, _("pcregrep: unknown option %c\n"), s[-1]);
	      return usage (2);
	    }
	}
    }

  /* There must be at least a regexp argument */
  if (i >= argc)
    return usage (0);

  /* Compile the regular expression. */
  switch (kind)
    {
    case 'b':
      pattern = pcre_posix_compile (argv[i++], options, &error, &errptr, NULL);
      break;
    case 'e':
      pattern = pcre_posix_compile (argv[i++], options | PCRE_EXTENDED,
				    &error, &errptr, NULL);
      break;
    case 'p':
      pattern = pcre_compile (argv[i++], options, &error, &errptr, NULL);
      break;
    }

  if (pattern == NULL)
    {
      fprintf (stderr, _("pcregrep: error in regex at offset %d: %s\n"),
	       errptr, error);
      return 2;
    }

  /* Study the regular expression, as we will be running it may times */
  hints = pcre_study (pattern, 0, &error);
  if (error != NULL)
    {
      fprintf (stderr, _("pcregrep: error while studing regex: %s\n"), error);
      return 2;
    }

  /* If there are no further arguments, do the business on stdin and exit */
  if (i >= argc)
    return pcregrep (stdin, NULL);

  /* Otherwise, work through the remaining arguments as files. If there is only
     one, don't give its name on the output. */
  if (i == argc - 1)
    filenames = FALSE;
  if (filenames_only)
    filenames = TRUE;

  for (; i < argc; i++)
    {
      FILE *in = fopen (argv[i], "r");
      if (in == NULL)
	{
	  fprintf (stderr, _("pcregrep: failed to open %s: %s\n"),
		   argv[i], strerror (errno));
	  rc = 2;
	}
      else
	{
	  int frc = pcregrep (in, filenames ? argv[i] : NULL);
	  if (frc == 0 && rc == 1)
	    rc = 0;
	  fclose (in);
	}
    }

  return rc;
}
