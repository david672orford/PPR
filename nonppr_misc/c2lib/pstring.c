/* String functions which allocate strings on the pool.
 * By Richard W.M. Jones <rich@annexia.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

#include "../nonppr_misc/c2lib/config.h"

#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <vector.h>
#include <hash.h>
#include <pstring.h>

#define MIN(a,b) ((a)<(b)?(a):(b))

/* Duplicate a string. */
char *
pstrdup (pool pool, const char *str)
{
  int len = strlen (str);
  char *ptr = pmalloc (pool, (len+1) * sizeof (char));
  return memcpy (ptr, str, len+1);
}

/* Duplicate up to the first N characters of a string. */
char *
pstrndup (pool pool, const char *str, int n)
{
  int len = MIN (strlen (str), n);
  char *ptr = pmalloc (pool, (len+1) * sizeof (char));
  memcpy (ptr, str, len);
  ptr[len] = '\0';
  return ptr;
}

/* Duplicate a fixed-size area of memory. */
void *
pmemdup (pool pool, const void *data, size_t size)
{
  void *ptr = pmalloc (pool, size);
  return memcpy (ptr, data, size);
}

static vector generic_split (pool pool, const char *str, const void *sep, const char *(*find) (const char *str, const void *sep, const char **end_match), int keep);

static const char *
find_strstr (const char *str, const void *sep, const char **end_match)
{
  const char *csep = (const char *) sep;
  const char *t = strstr (str, csep);
  if (t) *end_match = t + strlen (csep);
  return t;
}

vector
pstrsplit (pool pool, const char *str, const char *sep)
{
  return generic_split (pool, str, sep, find_strstr, 0);
}

static const char *
find_strchr (const char *str, const void *sep, const char **end_match)
{
  char c = * (const char *) sep;
  const char *t = strchr (str, c);
  if (t) *end_match = t+1;
  return t;
}

vector
pstrcsplit (pool pool, const char *str, char c)
{
  return generic_split (pool, str, &c, find_strchr, 0);
}

static const char *
find_re (const char *str, const void *sep, const char **end_match)
{
  const pcre *re = (const pcre *) sep;
#define ovecsize 3
  int ovector[ovecsize];
  int r = pcre_exec (re, 0, str, strlen (str), 0, 0, ovector, ovecsize);

  if (r >= 0)			/* Successful match. */
    {
      int so = ovector[0];
      int eo = ovector[1];

      if (so == -1) abort ();	/* Bad pattern. */
      *end_match = str + eo;
      return str + so;
    }
  else if (r == PCRE_ERROR_NOMATCH)
    return 0;
  else
    abort ();			/* Some other error reported by PCRE. */
#undef ovecsize
}

vector
pstrresplit (pool pool, const char *str, const pcre *re)
{
  return generic_split (pool, str, re, find_re, 0);
}

vector
pstrsplit2 (pool pool, const char *str, const char *sep)
{
  return generic_split (pool, str, sep, find_strstr, 1);
}

vector
pstrcsplit2 (pool pool, const char *str, char c)
{
  return generic_split (pool, str, &c, find_strchr, 1);
}

vector
pstrresplit2 (pool pool, const char *str, const pcre *re)
{
  return generic_split (pool, str, re, find_re, 1);
}

/* Generic split function. */
static vector
generic_split (pool pool, const char *str, const void *sep,
	       const char *(*find) (const char *str, const void *sep,
				    const char **end_match),
	       int keep)
{
  const char *start_match, *end_match;
  char *s;
  vector v;

  /* If the string is zero length, always return a zero length vector. */
  if (strcmp (str, "") == 0) return new_vector (pool, char *);

  /* Find the splitting point. */
  start_match = find (str, sep, &end_match);

  if (start_match != 0)		/* Successful match. */
    {
      s = start_match > str ? pstrndup (pool, str, start_match - str) : 0;
      v = generic_split (pool, end_match, sep, find, keep);
      if (keep)			/* Keep the matching text. */
	{
	  const char *match;

	  match = pstrndup (pool, start_match, end_match - start_match);
	  vector_push_front (v, match);
	}
      if (s) vector_push_front (v, s);
    }
  else				/* Not successful match. */
    {
      s = pstrdup (pool, str);
      v = new_vector (pool, char *);
      vector_push_back (v, s);
    }

  return v;
}

/* Concatenate a vector of strings to form a string. */
char *
pconcat (pool pool, vector v)
{
  int i;
  char *s = pstrdup (pool, "");

  for (i = 0; i < vector_size (v); ++i)
    {
      char *t;

      vector_get (v, i, t);
      s = pstrcat (pool, s, t);
    }

  return s;
}

/* Join a vector of strings, separating each string by the given string. */
char *
pjoin (pool pool, vector v, const char *sep)
{
  int i;
  char *s = pstrdup (pool, "");

  for (i = 0; i < vector_size (v); ++i)
    {
      char *t;

      vector_get (v, i, t);
      s = pstrcat (pool, s, t);
      if (i < vector_size (v) - 1) s = pstrcat (pool, s, sep);
    }

  return s;
}

char *
pchrs (pool pool, char c, int n)
{
  char *s = pmalloc (pool, sizeof (char) * (n + 1));
  int i;

  for (i = 0; i < n; ++i)
    s[i] = c;
  s[n] = '\0';

  return s;
}

char *
pstrs (pool pool, const char *str, int n)
{
  int len = strlen (str);
  char *s = pmalloc (pool, sizeof (char) * (len * n + 1));
  int i, j;

  for (i = j = 0; i < n; ++i, j += len)
    memcpy (&s[j], str, len);

  s[len * n] = '\0';

  return s;
}

vector
pvector (pool pool, ...)
{
  va_list args;
  const char *s;
  vector v = new_vector (pool, const char *);

  va_start (args, pool);
  while ((s = va_arg (args, const char *)) != 0)
    vector_push_back (v, s);
  va_end (args);

  return v;
}

vector
pvectora (pool pool, const char *array[], int n)
{
  int i;
  vector v = new_vector (pool, const char *);

  for (i = 0; i < n; ++i)
    vector_push_back (v, array[i]);

  return v;
}

/* Sort a vector of strings. */
void
psort (vector v, int (*compare_fn) (const char **, const char **))
{
  vector_sort (v, (int (*) (const void *, const void *)) compare_fn);
}

/* Remove line endings (either CR, CRLF or LF) from the string. */
char *
pchomp (char *line)
{
  int len = strlen (line);

  while (line[len-1] == '\n' || line[len-1] == '\r')
    line[--len] = '\0';

  return line;
}

char *
ptrimfront (char *str)
{
  char *p;
  int len;

  for (p = str; *p && isspace ((int) *p); ++p)
    ;

  len = strlen (p);
  memmove (str, p, len + 1);

  return str;
}

char *
ptrimback (char *str)
{
  int len;
  char *p;

  len = strlen (str);
  for (p = str + len - 1; p >= str && isspace ((int) *p); --p)
    ;

  p[1] = '\0';

  return str;
}

char *
ptrim (char *str)
{
  ptrimback (str);
  ptrimfront (str);
  return str;
}

/* This is equivalent to sprintf but it allocates the result string in POOL.*/
char *
psprintf (pool pool, const char *format, ...)
{
  va_list args;
  char *s;

  va_start (args, format);
  s = pvsprintf (pool, format, args);
  va_end (args);

  return s;
}

/* Similar to vsprintf. */
char *
pvsprintf (pool pool, const char *format, va_list args)
{
#ifdef HAVE_VASPRINTF

  char *s;

  vasprintf (&s, format, args);
  if (s == 0) abort ();		/* XXX Should call bad_malloc_handler. */

  /* The pool will clean up the malloc when it goes. */
  pool_register_malloc (pool, s);

  return s;

#else /* !HAVE_VASPRINTF */

  int r, n = 256;
  char *s = alloca (n), *t;

  /* Note: according to the manual page, a return value of -1 indicates
   * that the string was truncated. We have found that this is not
   * actually true however. In fact, the library seems to return the
   * number of characters which would have been written into the string
   * excluding the '\0' (ie. r > n).
   */
  r = vsnprintf (s, n, format, args);

  if (r < n)
    {
      /* Copy the string into a pool-allocated area of the correct size
       * and return it.
       */
      n = r + 1;
      t = pmalloc (pool, n);
      memcpy (t, s, n);

      return t;
    }
  else
    {
      /* String was truncated. Allocate enough space for the string
       * in the pool and repeat the vsnprintf into this buffer.
       */
      n = r + 1;
      t = pmalloc (pool, n);

      vsnprintf (t, n, format, args);

      return t;
    }

#endif /* !HAVE_VASPRINTF */
}

/* Convert various number types to strings. */
char *
pitoa (pool pool, int n)
{
  char *s = pmalloc (pool, 16);
  snprintf (s, 16, "%d", n);
  return s;
}

char *
pdtoa (pool pool, double n)
{
  char *s = pmalloc (pool, 16);
  snprintf (s, 16, "%f", n);
  return s;
}

char *
pxtoa (pool pool, unsigned n)
{
  char *s = pmalloc (pool, 16);
  snprintf (s, 16, "%x", n);
  return s;
}

/* Promote vector of numbers to vector of strings. */
vector
pvitostr (pool pool, vector v)
{
  vector nv = new_vector (pool, char *);
  int i;

  vector_reallocate (nv, vector_size (v));

  for (i = 0; i < vector_size (v); ++i)
    {
      char *s;
      int j;

      vector_get (v, i, j);
      s = pitoa (pool, j);
      vector_push_back (nv, s);
    }

  return nv;
}

vector
pvdtostr (pool pool, vector v)
{
  vector nv = new_vector (pool, char *);
  int i;

  vector_reallocate (nv, vector_size (v));

  for (i = 0; i < vector_size (v); ++i)
    {
      char *s;
      double j;

      vector_get (v, i, j);
      s = pdtoa (pool, j);
      vector_push_back (nv, s);
    }

  return nv;
}

vector
pvxtostr (pool pool, vector v)
{
  vector nv = new_vector (pool, char *);
  int i;

  vector_reallocate (nv, vector_size (v));

  for (i = 0; i < vector_size (v); ++i)
    {
      char *s;
      unsigned j;

      vector_get (v, i, j);
      s = pxtoa (pool, j);
      vector_push_back (nv, s);
    }

  return nv;
}

/* STR is a string allocated in POOL. Append ENDING to STR, reallocating
 * STR if necessary.
 */
char *
pstrcat (pool pool, char *str, const char *ending)
{
  /* There are probably more efficient ways to implement this ... */
  int slen = strlen (str);
  int elen = strlen (ending);

  str = prealloc (pool, str, slen + elen + 1);
  strcat (str, ending);
  return str;
}

char *
pstrncat (pool pool, char *str, const char *ending, size_t n)
{
  int slen = strlen (str);
  int elen = strlen (ending);

  elen = elen > n ? n : elen;

  str = prealloc (pool, str, slen + elen + 1);
  strncat (str, ending, n);
  return str;
}

/* Return the substring starting at OFFSET and of length LEN of STR, allocated
 * as a new string. If LEN is negative, everything up to the end of STR
 * is returned.
 */
char *
psubstr (pool pool, const char *str, int offset, int len)
{
  char *new_str;

  if (len >= 0)
    {
      new_str = pmalloc (pool, len + 1);
      memcpy (new_str, str + offset, len);
      new_str[len] = '\0';
      return new_str;
    }
  else
    {
      len = strlen (str + offset);
      new_str = pmalloc (pool, len + 1);
      memcpy (new_str, str + offset, len);
      new_str[len] = '\0';
      return new_str;
    }
}

char *
pstrupr (char *str)
{
  char *s = str;
  while (*s) { *s = toupper (*s); s++; }
  return str;
}

char *
pstrlwr (char *str)
{
  char *s = str;
  while (*s) { *s = tolower (*s); s++; }
  return str;
}

/* NB. The following figures were derived by examining a large number
 * of configuration files in /etc/ on a Red Hat Linux box.
 */
#define _PGETL_INITIAL_BUFFER 96
#define _PGETL_INCR_BUFFER 32

char *
pgetline (pool pool, FILE *fp, char *line)
{
  int allocated = _PGETL_INITIAL_BUFFER;
  int len = 0;
  int c;

  /* Reallocate the buffer. */
  line = prealloc (pool, line, _PGETL_INITIAL_BUFFER);

  /* Read in the line until we reach EOF or a '\n' character. */
  while ((c = getc (fp)) != EOF && c != '\n')
    {
      if (len == allocated)
	line = prealloc (pool, line, allocated += _PGETL_INCR_BUFFER);
      line[len++] = c;
    }

  /* EOF and no content? */
  if (c == EOF && len == 0)
    return 0;

  /* Last character is '\r'? Remove it. */
  if (line[len-1] == '\r')
    len--;

  /* Append a '\0' character to the buffer. */
  if (len == allocated)
    line = prealloc (pool, line, ++allocated);
  line[len] = '\0';

  return line;
}

char *
pgetlinex (pool pool, FILE *fp, char *line, const char *comment_set,
	   int flags)
{
  int i, len;

 again:
  /* Read a single line. */
  line = pgetline (pool, fp, line);
  if (line == 0) return 0;

  len = strlen (line);

  /* Concatenate? */
  if (!(flags & PGETL_NO_CONCAT))
    {
    another_concat:
      if (line[len-1] == '\\')
	{
	  char *next;

	  line[--len] = '\0';	/* Remove backslash char from first line. */

	  next = pgetline (pool, fp, 0);
	  if (next)
	    {
	      line = pstrcat (pool, line, next);
	      len = strlen (line);
	      goto another_concat;
	    }
	}
    }

  /* Remove comments? */
  if (!(flags & PGETL_INLINE_COMMENTS))
    {
      /* No inline comments. We're searching for whitespace followed
       * by a comment character. If we find it, remove the line following
       * the comment character.
       */
      for (i = 0; i < len; ++i)
	if (!isspace ((int) line[i]))
	  {
	    if (strchr (comment_set, line[i]) != 0)
	      {
		line[i] = '\0';
		len = i;
	      }
	    break;
	  }
    }
  else
    {
      /* Inline comments. Search for the first occurance of any
       * comment character and just remove the rest of the line
       * from that point.
       */
      for (i = 0; i < len; ++i)
	if (strchr (comment_set, line[i]) != 0)
	  {
	    line[i] = '\0';
	    len = i;
	    break;
	  }
    }

  /* Trim the line. */
  ptrim (line);

  /* Ignore blank lines. */
  if (line[0] == '\0')
    goto again;

  return line;
}

vector
pmap (pool p, const vector v, char *(*map_fn) (pool, const char *))
{
  int i;
  vector nv = new_vector (p, char *);

  for (i = 0; i < vector_size (v); ++i)
    {
      const char *s;
      char *r;

      vector_get (v, i, s);
      r = map_fn (p, s);
      vector_push_back (nv, r);
    }

  return nv;
}

vector
pgrep (pool p, const vector v, int (*grep_fn) (pool, const char *))
{
  int i;
  vector nv = new_vector (p, char *);

  for (i = 0; i < vector_size (v); ++i)
    {
      const char *s;

      vector_get (v, i, s);
      if (grep_fn (p, s))
	vector_push_back (nv, s);
    }

  return nv;
}
