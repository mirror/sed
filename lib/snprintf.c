/* A simplified snprintf, without floating point support */

/* AIX requires this to be the first thing in the file.  */
#ifdef _AIX
#pragma alloca
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifndef alloca /* predefined by HP cc +Olibcalls */
    char *alloca ();
#  endif
# endif
#endif

#ifndef BOOTSTRAP
#include <stdio.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <ctype.h>

#include "basicdefs.h"

#define MINUS_FLAG 1
#define PLUS_FLAG 2
#define SPACE_FLAG 4
#define ALTERNATE_FLAG 8
#define ZERO_FLAG 16

static int
compute_number_length (num, base, width, prec, flags, minusp)
     unsigned long num;
     unsigned base;
     int width;
     int prec;
     int flags;
     int minusp;
{
  int len = 0;
  int i;

  /* length of the number itself */
  do
    {
      len++;
      num /= base;
    }
  while (num);

  /* ...padded to prec characters */
  if (prec > len) len = prec;

  /* ...with alternate prefix */
  if (flags & ALTERNATE_FLAG && (base == 16 || base == 8))
    len += base / 8;

  /* ...with the sign before it */
  if (minusp || (flags & SPACE_FLAG) || (flags & PLUS_FLAG))
    len++;

  /* ...and padded with spaces or zeroes (zeroes go
     before the sign, actually.  This routine does
     not consider that but vsprintf will). */

  return (width > len) ? width : len;
}

/* These can't be made into a function... */

#define PARSE_INT_FORMAT(res, args) \
if (long_flag) \
     res = va_arg(args, long); \
else \
     res = va_arg(args, int)

#define PARSE_UINT_FORMAT(res, args) \
if (long_flag) \
     res = va_arg(args, unsigned long); \
else \
     res = va_arg(args, unsigned int)

static int
compute_length (fmt, args)
     const char *fmt;
     va_list args;
{
  char c;
  const char *format;
  size_t sz;

  /* Go through the string and find the correct length. */
  sz = 0;
  format = fmt;
  while (c = *format++)
    {
      if (c == '%')
	{
	  int flags = 0;
	  int width = 0;
	  int prec = -1;
	  int long_flag = 0;

	  /* flags */
	  while ((c = *format++))
	    {
	      if (c == '-')
		flags |= MINUS_FLAG;
	      else if (c == '+')
		flags |= PLUS_FLAG;
	      else if (c == ' ')
		flags |= SPACE_FLAG;
	      else if (c == '#')
		flags |= ALTERNATE_FLAG;
	      else if (c == '0')
		flags |= ZERO_FLAG;
	      else
		break;
	    }

	  if (flags & PLUS_FLAG)
	    flags &= ~SPACE_FLAG;
	  if (flags & MINUS_FLAG)
	    flags &= ~ZERO_FLAG;

	  /* width */
	  if (isdigit (c))
	    do
	      {
		width = width * 10 + c - '0';
		c = *format++;
	      }
	    while (isdigit (c));
	  else if (c == '*')
	    {
	      width = va_arg (args, int);
	      c = *format++;
	    }

	  /* precision */
	  if (c == '.')
	    {
	      prec = 0;
	      c = *format++;
	      if (isdigit (c))
		do
		  {
		    prec = prec * 10 + c - '0';
		    c = *format++;
		  }
		while (isdigit (c));
	      else if (c == '*')
		{
		  prec = va_arg (args, int);
		  c = *format++;
		}
	    }

	  /* size */

	  if (c == 'h')
	    {
	      c = *format++;
	      if (c == 'h')
	        c = *format++;
	    }
	  else if (c == 'l')
	    {
	      long_flag = 1;
	      c = *format++;
	    }

	  switch (c)
	    {
	    case 'c':
	      sz += width + 1;
	      break;

	    case 's':
	      {
		int len;
		char *arg;

		arg = va_arg (args, char *);
		len = strlen (arg);

		if (prec != -1 && len > prec)
		  len = prec;

		sz += (width > len) ? width : len;
		break;
	      }

	    case 'd':
	    case 'i':
	      {
		long arg;
		unsigned long num;
		int minusp = 0;

		PARSE_INT_FORMAT (arg, args);

		if (arg < 0)
		  {
		    minusp = 1;
		    num = -arg;
		  }
		else
		  num = arg;

		sz += compute_number_length(num, 10, width,
						prec, flags, minusp);
		break;
	      }

	    case 'u':
	      {
		unsigned long arg;

		PARSE_UINT_FORMAT (arg, args);

		sz += compute_number_length(arg, 10, width,
						prec, flags, 0);
		break;
	      }

	    case 'o':
	      {
		unsigned long arg;

		PARSE_UINT_FORMAT (arg, args);

		sz += compute_number_length(arg, 8, width,
						prec, flags, 0);
		break;
	      }

	    case 'p':
	      flags |= ALTERNATE_FLAG;
	      /* fall through */

	    case 'x':
	    case 'X':
	      {
		unsigned long arg;

		PARSE_UINT_FORMAT (arg, args);

		sz += compute_number_length(arg, 16, width,
						prec, flags, 0);
		break;
	      }

	    case 'n':
	      break;

	    case '%':
	      sz++;
	      break;

	    default:
	      sz += 2;
	      break;
	    }
	}
      else
	sz++;
    }

  return sz;
  }

static int
my_vsnprintf (str, sz, fmt, args)
     char *str;
     size_t sz;
     const char *fmt;
     va_list args;
{
  char *buf;
  size_t realsz = compute_length(fmt, args);

  /* Here, we have to count the null character at the end! */
  if (realsz + 1 < sz)
    buf = alloca (realsz + 1);
  else
    buf = str;

  realsz = vsprintf(buf, fmt, args);
  if (buf != str)
    memcpy(str, buf, sz);

  return realsz;
}

int
#ifdef HAVE_STDARG_H
snprintf (char *str, size_t sz, const char *format,...)
#else
snprintf (va_alist)
     va_dcl
#endif
{
  va_list args;
  int ret;
#ifdef HAVE_STDARG_H
  va_start (args, format);
#else
  char *str;
  size_t sz;
  char *format;

  va_start (args);
  str = va_arg(args, char *);
  sz = va_arg(args, size_t);
  format = va_arg(args, char *);
#endif

  ret = my_vsnprintf (str, sz, format, args);
  va_end (args);
  return ret;
}
