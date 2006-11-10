/*************************************************
*             PCRE testing program               *
*************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Use the internal info for displaying the results of pcre_study(). */

#include "config.h"
#include "pcre.h"
#include "internal.h"

/* It is possible to compile this test program without including support for
testing the POSIX interface, though this is not available via the standard
Makefile. */

#include "regexp.h"

#ifndef CLOCKS_PER_SEC
#ifdef CLK_TCK
#define CLOCKS_PER_SEC CLK_TCK
#else
#define CLOCKS_PER_SEC 100
#endif
#endif

#define LOOPREPEAT 20000

#include "getopt.h"

static int log_store = 0;
static size_t gotten_store;
static FILE *infile, *outfile;



/* Character string printing function.  */

static void
pchars (unsigned char *p, int length)
{
  int c;
  while (length-- > 0)
    {
      if (isprint (c = *(p++)))
	fprintf (outfile, "%c", c);
      else
	fprintf (outfile, "\\x%02x", c);
    }
}



/* Alternative malloc function, to test functionality and show the size of the
compiled re. */

static void *
new_malloc (size_t size)
{
  gotten_store = size;
  if (log_store)
    fprintf (outfile, "Memory allocation (code space): %d\n",
	     (int) ((int) size - offsetof (pcre, code[0])));
  return malloc (size);
}




/* Get one piece of information from the pcre_info() function */

static void
new_info (pcre * re, pcre_extra * study, int option, void *ptr)
{
  int rc;
  if ((rc = pcre_info (re, study, option, ptr)) < 0)
    fprintf (outfile, "Error %d from pcre_fullinfo(%d)\n", rc, option);
}




/* Read lines from named file or stdin and write to named file or stdout; lines
consist of a regular expression, in delimiters and optionally followed by
options, followed by a set of test data, terminated by an empty line. */

int
main (int argc, char **argv)
{
  int options = 0;
  int study_options = 0;
  int timeit = 0;
  int showinfo = 0;
  int showstore = 0;
  int size_offsets = 45;
  int size_offsets_max;
  int *offsets;
  int posix = 0;
  int perl = 1;
  int study = 0;
  int debug = 0;
  int done = 0;
  unsigned char buffer[30000];
  unsigned char dbuffer[1024];
  char *endptr;

/* Scan options */
#define SHORTOPTS "dimo:pPSst"
  for (;;)
    {
      static struct option longopts[] = {
	{"debug", 0, NULL, 'd'},
	{"showinfo", 0, NULL, 'i'},
	{"offsets-size", 0, NULL, 'o'},
	{"regexec", 0, NULL, 'p'},
	{"posix", 0, NULL, 'P'},
	{"study", 0, NULL, 'S'},
	{"showstore", 0, NULL, 's'},
	{"time", 0, NULL, 't'},
	{NULL, 0, NULL, 0}
      };

      int c = getopt_long (argc, argv, SHORTOPTS,
			    longopts, NULL);

      /* Detect the end of the options. */
      if (c == -1)
	break;

      switch (c)
	{
	case 's':
	case 'm':
	  showstore = 1;
	  break;
	case 't':
	  timeit = 1;
	  break;
	case 'i':
	  showinfo = 1;
	  break;
	case 'd':
	  showinfo = debug = 1;
	  break;
	case 'p':
	  posix = 1;
	  break;
	case 'P':
	  perl = 0;
	  break;
	case 'S':
	  study = 1;
	  break;
	case 'o':
	  if ((size_offsets = strtoul (optarg, &endptr, 10)) && *endptr == 0)
	    break;

	default:
	  printf
	    ("Usage:   pcretest [options...] [<input> [<output>]]\n");
	  printf ("  -d   --debug          debug: show compiled code; implies -i\n");
	  printf ("  -i   --showinfo       show information about compiled pattern\n");
	  printf ("  -o N --offsets-size=N set size of offsets vector to <n>\n");
	  printf ("  -p   --regexec        use POSIX interface\n");
	  printf ("  -P   --posix          use POSIX regular expressions\n");
	  printf ("  -S   --study          study regular expressions\n");
	  printf ("  -s   --showstore      output store information\n");
	  printf ("  -t   --time           time compilation and execution\n");
	  return 1;
	}
    }

/* Sort out the input and output files */

  if (optind < argc && strcmp (argv[optind], "-") != 0)
    {
      infile = fopen (argv[optind], "r");

      if (infile == NULL)
	{
	  fprintf (stderr, "** Failed to open %s\n", argv[optind]);
	  return 1;
	}
    }
  else
    infile = stdin;

  if (++optind < argc && strcmp (argv[optind], "-") != 0)
    {
      outfile = fopen (argv[optind], "w");
      if (outfile == NULL)
	{
	  fprintf (stderr, "** Failed to open %s\n", argv[optind]);
	  return 1;
	}
    }
  else
    outfile = stdout;

/* Get the store for the offsets vector, and remember what it was */

  size_offsets_max = size_offsets;
  offsets = malloc (size_offsets_max * sizeof (int));
  if (offsets == NULL)
    {
      fprintf (stderr, "** Failed to get %d bytes of memory for offsets vector\n",
	      size_offsets_max * sizeof (int));
      return 1;
    }

/* Set alternative malloc function */

  pcre_malloc = new_malloc;

/* Main loop */

  while (!done)
    {
      pcre *re = NULL;
      pcre_extra *extra = NULL;

      regex_t preg;
      const char *error;
      unsigned char *p, *pp, *ppp;
      const unsigned char *tables = NULL;
      int do_posix = posix;
      int do_study = study;
      int do_debug = debug;
      int do_showinfo = showinfo;
      int do_G = 0;
      int do_g = 0;
      int do_showrest = 0;
      int erroroffset, len, delimiter;

#ifdef HAVE_ISATTY
      if (isatty (fileno (infile)))
	fprintf (outfile, "  re> ");
#endif
      if (fgets ((char *) buffer, sizeof (buffer), infile) == NULL)
	break;
#ifdef HAVE_ISATTY
      if (!isatty (fileno (infile)) || !isatty (fileno (outfile)))
#endif
	fprintf (outfile, "%s", (char *) buffer);

      p = buffer;
      while (isspace (*p))
	p++;
      if (*p == 0 || *p == '#')
	continue;

      /* Get the delimiter and seek the end of the pattern; if is isn't
         complete, read more. */

      delimiter = *p++;

      if (isalnum (delimiter) || delimiter == '\\')
	{
	  fprintf (stderr, "** Delimiter must not be alphameric or \\\n");
	  goto SKIP_DATA;
	}

      pp = p;

      for (;;)
	{
	  while (*pp != 0)
	    {
	      if (*pp == '\\' && pp[1] != 0)
		pp++;
	      else if (*pp == delimiter)
		break;
	      pp++;
	    }
	  if (*pp != 0)
	    break;

	  len = sizeof (buffer) - (pp - buffer);
	  if (len < 256)
	    {
	      fprintf (stderr,
		       "** Expression too long - missing delimiter?\n");
	      goto SKIP_DATA;
	    }

#ifdef HAVE_ISATTY
	  if (isatty(fileno(infile)))
	    fprintf (outfile, "    > ");
#endif
	  if (fgets ((char *) pp, len, infile) == NULL)
	    {
	      fprintf (stderr, "** Unexpected EOF\n");
	      done = 1;
	      goto CONTINUE;
	    }
#ifdef HAVE_ISATTY
	  if (!isatty (fileno (infile)) || !isatty (fileno (outfile)))
#endif
	    fprintf (outfile, "%s", (char *) pp);
	}

      /* If the first character after the delimiter is backslash, make
         the pattern end with backslash. This is purely to provide a way
         of testing for the error message when a pattern ends with backslash. */

      if (pp[1] == '\\')
	*pp++ = '\\';

      /* Terminate the pattern at the delimiter */

      *pp++ = 0;

      /* Look for options after final delimiter */

      options = 0;
      study_options = 0;
      log_store = showstore;	/* default from command line */

      while (*pp != 0)
	{
	  switch (*pp++)
	    {
	    case 'g':
	      do_g = 1;
	      break;
	    case 'i':
	      options |= PCRE_CASELESS;
	      break;
	    case 'm':
	      options |= PCRE_MULTILINE;
	      break;
	    case 's':
	      options |= PCRE_DOTALL;
	      break;
	    case 'x':
	      options |= PCRE_EXTENDED;
	      break;

	    case '+':
	      do_showrest = 1;
	      break;
	    case 'A':
	      options |= PCRE_ANCHORED;
	      break;
	    case 'D':
	      do_debug = do_showinfo = 1;
	      break;
	    case 'E':
	      options |= PCRE_DOLLAR_ENDONLY;
	      break;
	    case 'G':
	      do_G = 1;
	      break;
	    case 'I':
	      do_showinfo = 1;
	      break;
	    case 'M':
	      log_store = 1;
	      break;

	    case 'P':
	      do_posix = 1;
	      break;

	    case 'S':
	      do_study = 1;
	      break;
	    case 'U':
	      options |= PCRE_UNGREEDY;
	      break;
	    case 'X':
	      options |= PCRE_EXTRA;
	      break;

	    case 'L':
	      ppp = pp;
	      while (*ppp != '\n' && *ppp != ' ')
		ppp++;
	      *ppp = 0;
	      if (setlocale (LC_CTYPE, (const char *) pp) == NULL)
		{
		  fprintf (stderr, "** Failed to set locale \"%s\"\n", pp);
		  goto SKIP_DATA;
		}
	      tables = pcre_maketables ();
	      pp = ppp;
	      break;

	    case '\n':
	    case ' ':
	      break;
	    default:
	      fprintf (stderr, "** Unknown option '%c'\n", pp[-1]);
	      goto SKIP_DATA;
	    }
	}

      /* Handle compiling via the POSIX interface, which doesn't support the
         timing, showing, or debugging options, nor the ability to pass over
         local character tables. */

      if (do_posix)
	{
	  int rc;
	  int cflags = 0;
	  if ((options & PCRE_CASELESS) != 0)
	    cflags |= REG_ICASE;
	  if ((options & PCRE_MULTILINE) != 0)
	    cflags |= REG_NEWLINE;
	  if ((options & PCRE_DOTALL) != 0)
	    cflags |= REG_DOTALL;
	  if ((options & PCRE_EXTENDED) != 0)
	    cflags |= REG_EXTENDED;

	  rc = regcomp (&preg, (char *) p, perl ? cflags | REG_PERL : cflags);

	  /* Compilation failed; go back for another re, skipping to blank line
	     if non-interactive. */

	  if (rc != 0)
	    {
	      (void) regerror (rc, &preg, (char *) buffer, sizeof (buffer));
	      fprintf (outfile, "Failed: POSIX code %d: %s\n", rc, buffer);
	      goto SKIP_DATA;
	    }
	}

      /* Handle compiling via the native interface */

      else
	{
	  if (timeit)
	    {
	      register int i;
	      clock_t time_taken;
	      clock_t start_time = clock ();
	      for (i = 0; i < LOOPREPEAT; i++)
		{
		  re = perl
		    ? pcre_compile ((char *) p, options, &error, &erroroffset,
				    tables)
		    : pcre_posix_compile ((char *) p, options, &error,
					  &erroroffset, tables);
		  if (re != NULL)
		    free (re);
		}
	      time_taken = clock () - start_time;
	      fprintf (outfile, "Compile time %.3f milliseconds\n",
		       ((double) time_taken * 1000.0) /
		       ((double) LOOPREPEAT * (double) CLOCKS_PER_SEC));
	    }

	  re = perl
	    ? pcre_compile ((char *) p, options, &error, &erroroffset, tables)
	    : pcre_posix_compile ((char *) p, options, &error, &erroroffset,
				  tables);

	  /* Compilation failed; go back for another re, skipping to blank line
	     if non-interactive. */

	  if (re == NULL)
	    {
	      fprintf (outfile, "Failed: %s at offset %d\n", error,
		       erroroffset);
	    SKIP_DATA:
#ifdef HAVE_ISATTY
	      if (!isatty (fileno (infile)))
#endif
		{
		  for (;;)
		    {
		      if (fgets ((char *) buffer, sizeof (buffer), infile) ==
			  NULL)
			{
			  done = 1;
			  goto CONTINUE;
			}
		      len = (int) strlen ((char *) buffer);
		      while (len > 0 && isspace (buffer[len - 1]))
			len--;
		      if (len == 0)
			break;
		    }
		  fprintf (outfile, "\n");
		}
	      goto CONTINUE;
	    }

	  /* If /S was present, study the regexp to generate additional info to
	         help with the matching. */

	  if (do_study)
	    {
	      if (timeit)
		{
		  register int i;
		  clock_t time_taken;
		  clock_t start_time = clock ();
		  for (i = 0; i < LOOPREPEAT; i++)
		    extra = pcre_study (re, study_options, &error);
		  time_taken = clock () - start_time;
		  if (extra != NULL)
		    free (extra);
		  fprintf (outfile, "  Study time %.3f milliseconds\n",
			   ((double) time_taken * 1000.0) /
			   ((double) LOOPREPEAT *
			    (double) CLOCKS_PER_SEC));
		}

	      extra = pcre_study (re, study_options, &error);
	      if (error != NULL)
		fprintf (outfile, "Failed to study: %s\n", error);
	      else if (extra == NULL)
		fprintf (outfile, "Study returned NULL\n");

	      else if (do_showinfo)
		{
		  uschar *start_bits = NULL;
		  new_info (re, extra, PCRE_INFO_FIRSTTABLE, &start_bits);
		  if (start_bits == NULL)
		    fprintf (outfile, "No starting character set\n");
		  else
		    {
		      int i;
		      int c = 24;
		      fprintf (outfile, "Starting character set: ");
		      for (i = 0; i < 256; i++)
			{
			  if ((start_bits[i / 8] & (1 << (i % 8))) != 0)
			    {
			      if (c > 75)
				{
				  fprintf (outfile, "\n  ");
				  c = 2;
				}
			      if (isprint (i) && i != ' ')
				{
				  fprintf (outfile, "%c ", i);
				  c += 2;
				}
			      else
				{
				  fprintf (outfile, "\\x%02x ", i);
				  c += 5;
				}
			    }
			}
		      fprintf (outfile, "\n");
		    }
		}
	    }

	  /* Compilation succeeded; print data if required. There are now two
	     info-returning functions. The old one has a limited interface and
	     returns only limited data. Check that it agrees with the newer one. */

	  if (do_showinfo)
	    {
	      unsigned long int get_options;
	      int old_first_char, old_options, old_count;
	      int count, backrefmax, first_char, need_char;
	      size_t size;

	      if (do_debug)
		pcre_debug (re);

	      new_info (re, NULL, PCRE_INFO_OPTIONS, &get_options);
	      new_info (re, NULL, PCRE_INFO_SIZE, &size);
	      new_info (re, NULL, PCRE_INFO_CAPTURECOUNT, &count);
	      new_info (re, NULL, PCRE_INFO_BACKREFMAX, &backrefmax);
	      new_info (re, NULL, PCRE_INFO_FIRSTCHAR, &first_char);
	      new_info (re, NULL, PCRE_INFO_LASTLITERAL, &need_char);

	      if (count < 0)
		fprintf (outfile, "Error %d from pcre_info()\n", count);

	      if (size != gotten_store)
		fprintf (outfile, "Size disagreement: pcre_fullinfo=%d call to malloc for %d\n",
			 size, gotten_store);

	      fprintf (outfile, "Capturing subpattern count = %d\n", count);
	      if (backrefmax > 0)
		fprintf (outfile, "Max back reference = %d\n", backrefmax);
	      if (get_options == 0)
		fprintf (outfile, "No options\n");
	      else
		fprintf (outfile, "Options:%s%s%s%s%s%s%s%s\n",
			 ((get_options & PCRE_ANCHORED) !=
			  0) ? " anchored" : "",
			 ((get_options & PCRE_CASELESS) !=
			  0) ? " caseless" : "",
			 ((get_options & PCRE_EXTENDED) !=
			  0) ? " extended" : "",
			 ((get_options & PCRE_MULTILINE) !=
			  0) ? " multiline" : "",
			 ((get_options & PCRE_DOTALL) != 0) ? " dotall" : "",
			 ((get_options & PCRE_DOLLAR_ENDONLY) !=
			  0) ? " dollar_endonly" : "",
			 ((get_options & PCRE_EXTRA) != 0) ? " extra" : "",
			 ((get_options & PCRE_UNGREEDY) !=
			  0) ? " ungreedy" : "");

	      if (((re->options) & PCRE_ICHANGED) != 0)
		fprintf (outfile, "Case state changes\n");
	    }
	}

      /* Read data lines and test them */

      for (;;)
	{
	  unsigned char *q;
	  unsigned char *bptr = dbuffer;
	  int *use_offsets = offsets;
	  int use_size_offsets = size_offsets;
	  int count, c;
	  int copystrings = 0;
	  int getstrings = 0;
	  int getlist = 0;
	  int gmatched = 0;
	  int start_offset = 0;
	  int g_notempty = 0;

	  options = 0;

#ifdef HAVE_ISATTY
	  if (isatty (fileno (infile)))
	    fprintf (outfile, "data> ");
#endif
	  if (fgets ((char *) buffer, sizeof (buffer), infile) == NULL)
	    {
	      done = 1;
	      goto CONTINUE;
	    }
#ifdef HAVE_ISATTY
	  if (!isatty (fileno (infile)) || !isatty (fileno (outfile)))
#endif
	    fprintf (outfile, "%s", (char *) buffer);

	  len = (int) strlen ((char *) buffer);
	  while (len > 0 && isspace (buffer[len - 1]))
	    len--;
	  buffer[len] = 0;
	  if (len == 0)
	    break;

	  p = buffer;
	  while (isspace (*p))
	    p++;

	  q = dbuffer;
	  while ((c = *p++) != 0)
	    {
	      int i = 0;
	      int n = 0;
	      if (c == '\\')
		switch ((c = *p++))
		  {
		  case 'a':
		    c = 7;
		    break;
		  case 'b':
		    c = '\b';
		    break;
		  case 'e':
		    c = 27;
		    break;
		  case 'f':
		    c = '\f';
		    break;
		  case 'n':
		    c = '\n';
		    break;
		  case 'r':
		    c = '\r';
		    break;
		  case 't':
		    c = '\t';
		    break;
		  case 'v':
		    c = '\v';
		    break;

		  case '0':
		  case '1':
		  case '2':
		  case '3':
		  case '4':
		  case '5':
		  case '6':
		  case '7':
		    c -= '0';
		    while (i++ < 2 && isdigit (*p) && *p != '8'
			   && *p != '9')
		      c = c * 8 + *p++ - '0';
		    break;

		  case 'x':
		    c = 0;
		    while (i++ < 2 && isxdigit (*p))
		      {
			c =
			  c * 16 + tolower (*p) -
			  ((isdigit (*p)) ? '0' : 'W');
			p++;
		      }
		    break;

		  case 0:	/* Allows for an empty line */
		    p--;
		    continue;

		  case 'A':	/* Option setting */
		    options |= PCRE_ANCHORED;
		    continue;

		  case 'B':
		    options |= PCRE_NOTBOL;
		    continue;

		  case 'C':
		    while (isdigit (*p))
		      n = n * 10 + *p++ - '0';
		    copystrings |= 1 << n;
		    continue;

		  case 'G':
		    while (isdigit (*p))
		      n = n * 10 + *p++ - '0';
		    getstrings |= 1 << n;
		    continue;

		  case 'L':
		    getlist = 1;
		    continue;

		  case 'N':
		    options |= PCRE_NOTEMPTY;
		    continue;

		  case 'O':
		    while (isdigit (*p))
		      n = n * 10 + *p++ - '0';
		    if (n > size_offsets_max)
		      {
			size_offsets_max = n;
			free (offsets);
			use_offsets = offsets =
			  malloc (size_offsets_max * sizeof (int));
			if (offsets == NULL)
			  {
			    printf
			      ("** Failed to get %d bytes of memory for offsets vector\n",
			       size_offsets_max * sizeof (int));
			    return 1;
			  }
		      }
		    use_size_offsets = n;
		    if (n == 0)
		      use_offsets = NULL;
		    continue;

		  case 'Z':
		    options |= PCRE_NOTEOL;
		    continue;
		  }
	      *q++ = c;
	    }
	  *q = 0;
	  len = q - dbuffer;

	  /* Handle matching via the POSIX interface, which does not
	     support timing. */

	  if (do_posix)
	    {
	      int rc;
	      int eflags = 0;
	      regmatch_t *pmatch =
		malloc (sizeof (regmatch_t) * use_size_offsets);
	      if ((options & PCRE_NOTBOL) != 0)
		eflags |= REG_NOTBOL;
	      if ((options & PCRE_NOTEOL) != 0)
		eflags |= REG_NOTEOL;

	      rc =
		regnexec (&preg, (const char *) bptr, len,
			  use_size_offsets, pmatch, eflags);

	      if (rc != 0)
		{
		  (void) regerror (rc, &preg, (char *) buffer,
				   sizeof (buffer));
		  fprintf (outfile, "No match: POSIX code %d: %s\n", rc,
			   buffer);
		}
	      else
		{
		  size_t i;
		  for (i = 0; i < use_size_offsets; i++)
		    {
		      if (pmatch[i].rm_so == -1)
			continue;
		      fprintf (outfile, "%2d: ", (int) i);
		      pchars (dbuffer + pmatch[i].rm_so,
			      pmatch[i].rm_eo - pmatch[i].rm_so);
		      fprintf (outfile, "\n");
		      if (i == 0 && do_showrest)
			{
			  fprintf (outfile, " 0+ ");
			  pchars (dbuffer + pmatch[i].rm_eo,
				  len - pmatch[i].rm_eo);
			  fprintf (outfile, "\n");
			}
		    }
		}
	      free (pmatch);
	    }

	  /* Handle matching via the native interface - repeats for /g and /G */

	  else
	    for (;; gmatched++)	/* Loop for /g or /G */
	      {
		fflush(outfile);
		if (timeit)
		  {
		    register int i;
		    clock_t time_taken;
		    clock_t start_time = clock ();
		    for (i = 0; i < LOOPREPEAT; i++)
		      count = pcre_exec (re, extra, (char *) bptr, len,
					 start_offset,
					 options | g_notempty,
					 use_offsets, use_size_offsets);
		    time_taken = clock () - start_time;
		    fprintf (outfile, "Execute time %.3f milliseconds\n",
			     ((double) time_taken * 1000.0) /
			     ((double) LOOPREPEAT *
			      (double) CLOCKS_PER_SEC));
		  }

		count = pcre_exec (re, extra, (char *) bptr, len,
				   start_offset, options | g_notempty,
				   use_offsets, use_size_offsets);

		if (count == 0)
		  {
		    fprintf (outfile, "Matched, but too many substrings\n");
		    count = use_size_offsets / 3;
		  }

		/* Matched */

		if (count >= 0)
		  {
		    int i;
		    for (i = 0; i < count * 2; i += 2)
		      {
			if (use_offsets[i] < 0)
			  fprintf (outfile, "%2d: <unset>\n", i / 2);
			else
			  {
			    fprintf (outfile, "%2d: ", i / 2);
			    pchars (bptr + use_offsets[i],
				    use_offsets[i + 1] - use_offsets[i]);
			    fprintf (outfile, "\n");
			    if (i == 0)
			      {
				if (do_showrest)
				  {
				    fprintf (outfile, " 0+ ");
				    pchars (bptr + use_offsets[i + 1],
					    len - use_offsets[i + 1]);
				    fprintf (outfile, "\n");
				  }
			      }
			  }
		      }

		    for (i = 0; i < 32; i++)
		      {
			if ((copystrings & (1 << i)) != 0)
			  {
			    char copybuffer[16];
			    int rc =
			      pcre_copy_substring ((char *) bptr,
						   use_offsets,
						   count,
						   i, copybuffer,
						   sizeof (copybuffer));
			    if (rc < 0)
			      fprintf (outfile, "copy substring %d failed %d\n", i,
				      rc);
			    else
			      fprintf (outfile, "%2dC %s (%d)\n", i,
				       copybuffer, rc);
			  }
		      }

		    for (i = 0; i < 32; i++)
		      {
			if ((getstrings & (1 << i)) != 0)
			  {
			    const char *substring;
			    int rc =
			      pcre_get_substring ((char *) bptr,
						  use_offsets,
						  count,
						  i, &substring);
			    if (rc < 0)
			      fprintf (outfile, "get substring %d failed %d\n", i,
				      rc);
			    else
			      {
				fprintf (outfile, "%2dG %s (%d)\n", i,
					 substring, rc);
				/* free((void *)substring); */
				pcre_free_substring (substring);
			      }
			  }
		      }

		    if (getlist)
		      {
			const char **stringlist;
			int rc =
			  pcre_get_substring_list ((char *) bptr,
						   use_offsets,
						   count,
						   &stringlist);
			if (rc < 0)
			  fprintf (outfile, "get substring list failed %d\n", rc);
			else
			  {
			    for (i = 0; i < count; i++)
			      fprintf (outfile, "%2dL %s\n", i,
				       stringlist[i]);
			    if (stringlist[i] != NULL)
			      fprintf (outfile, "string list not terminated by NULL\n");
				/* free((void *)stringlist); */
			    pcre_free_substring_list (stringlist);
			  }
		      }
		  }

		/* Failed to match. If this is a /g or /G loop and we previously set
		   g_notempty after a null match, this is not necessarily the end.
		   We want to advance the start offset, and continue. Fudge the offset
		   values to achieve this. We won't be at the end of the string - that
		   was checked before setting g_notempty. */

		else
		  {
		    if (g_notempty != 0)
		      {
			use_offsets[0] = start_offset;
			use_offsets[1] = start_offset + 1;
		      }
		    else
		      {
			if (gmatched == 0)	/* Error if no previous matches */
			  {
			    if (count == -1)
			      fprintf (outfile, "No match\n");
			    else
			      fprintf (outfile, "Error %d\n", count);
			  }
			break;	/* Out of the /g loop */
		      }
		  }

		/* If not /g or /G we are done */

		if (!do_g && !do_G)
		  break;

		/* If we have matched an empty string, first check to see if we are at
		   the end of the subject. If so, the /g loop is over. Otherwise, mimic
		   what Perl's /g options does. This turns out to be rather cunning. First
		   we set PCRE_NOTEMPTY and PCRE_ANCHORED and try the match again at the
		   same point. If this fails (picked up above) we advance to the next
		   character. */

		g_notempty = 0;
		if (use_offsets[0] == use_offsets[1])
		  {
		    if (use_offsets[0] == len)
		      break;
		    g_notempty = PCRE_NOTEMPTY | PCRE_ANCHORED;
		  }

		/* For /g, update the start offset, leaving the rest alone */

		if (do_g)
		  start_offset = use_offsets[1];

		/* For /G, update the pointer and length */

		else
		  {
		    bptr += use_offsets[1];
		    len -= use_offsets[1];
		  }
	      }		/* End of loop for /g and /G */
	}			/* End of loop for data lines */

    CONTINUE:

      if (do_posix)
	regfree (&preg);

      if (re != NULL)
	free (re);
      if (extra != NULL)
	free (extra);
      if (tables != NULL)
	{
	  free ((void *) tables);
	  setlocale (LC_CTYPE, "C");
	}
    }

  fprintf (outfile, "\n");
  return 0;
}

/* End */
