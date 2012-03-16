/*  Functions from hack's utils library.
    Copyright (C) 1989, 1990, 1991, 1998, 1999, 2003
    Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. */

#include <stdio.h>

#include "basicdefs.h"

void panic (const char *str, ...);

FILE *ck_fopen (const char *name, const char *mode, int fail);
FILE *ck_fdopen (int fd, const char *name, const char *mode, int fail);
void ck_fwrite (const void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t ck_fread (void *ptr, size_t size, size_t nmemb, FILE *stream);
void ck_fflush (FILE *stream);
void ck_fclose (FILE *stream);
const char *follow_symlink (const char *path);
size_t ck_getdelim (char **text, size_t *buflen, char buffer_delimiter, FILE *stream);
FILE * ck_mkstemp (char **p_filename, const char *tmpdir, const char *base,
		   const char *mode);
void ck_rename (const char *from, const char *to, const char *unlink_if_fail);

void *ck_malloc (size_t size);
void *xmalloc (size_t size);
void *ck_realloc (void *ptr, size_t size);
char *ck_strdup (const char *str);
void *ck_memdup (const void *buf, size_t len);

struct buffer *init_buffer (void);
char *get_buffer (struct buffer *b);
size_t size_buffer (struct buffer *b);
char *add_buffer (struct buffer *b, const char *p, size_t n);
char *add1_buffer (struct buffer *b, int ch);
void free_buffer (struct buffer *b);

extern const char *myname;
