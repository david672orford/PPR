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
#include <stdlib.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <pcre.h>

#include <pool.h>
#include <vector.h>
#include <pstring.h>
#include <pre.h>

/* These private functions are used to capture memory allocations made
 * by the PCRE library.
 */
static void *malloc_in_pool (size_t);
static void do_nothing (void *);
static pool malloc_pool = 0;

pcre *
precomp (pool pool, const char *pattern, int options)
{
  const char *errptr;
  int erroffset;
  pcre *result;
  void *(*old_malloc)(size_t);
  void (*old_free) (void *);

  /* Allocations to the pool. */
  old_malloc = pcre_malloc;
  old_free = pcre_free;
  pcre_malloc = malloc_in_pool;
  malloc_pool = pool;
  pcre_free = do_nothing;

  /* Compile the pattern. */
  result = pcre_compile (pattern, options, &errptr, &erroffset, 0);
  if (result == 0)
	{
	  fprintf (stderr,
			   "pcre: internal error compiling regular expression:\n"
			   "pcre: %s\n"
			   "pcre: %s\n"
			   "pcre: %s^\n",
			   errptr,
			   pattern,
			   pchrs (pool, ' ', erroffset-1));
	  exit (1);
	}

  /* Restore memory allocation. */
  pcre_malloc = old_malloc;
  pcre_free = old_free;

  return result;
}

vector
prematch (pool pool, const char *str, const pcre *pattern, int options)
{
  int err, n, i, ovecsize;
  int *ovector;
  vector result;
  void *(*old_malloc)(size_t);
  void (*old_free) (void *);

  /* Allocations to the pool. */
  old_malloc = pcre_malloc;
  old_free = pcre_free;
  pcre_malloc = malloc_in_pool;
  malloc_pool = pool;
  pcre_free = do_nothing;

  /* Get the number of capturing substrings in the pattern (n). */
  if ((err = pcre_fullinfo (pattern, 0, PCRE_INFO_CAPTURECOUNT, &n)) != 0)
	abort ();

  /* Allocate a vector large enough to contain the resulting substrings. */
  ovecsize = (n+1) * 3;
  ovector = alloca (ovecsize * sizeof (int));

  /* Do the match. n is the number of strings found. */
  n = pcre_exec (pattern, 0, str, strlen (str), 0, options, ovector, ovecsize);

  /* Restore memory allocation. */
  pcre_malloc = old_malloc;
  pcre_free = old_free;

  if (n == PCRE_ERROR_NOMATCH) /* No match, return NULL. */
	return 0;
  else if (n <= 0)				/* Some other error. */
	abort ();

  /* Some matches. Construct the vector. */
  result = new_vector (pool, char *);
  for (i = 0; i < n; ++i)
	{
	  char *s = 0;
	  int start = ovector[i*2];
	  int end = ovector[i*2+1];

	  if (start >= 0)
		s = pstrndup (pool, str + start, end - start);
	  vector_push_back (result, s);
	}

  return result;
}

static int do_match_and_sub (pool pool, const char *str, char **newstrp,
							 const pcre *pattern, const char *sub,
							 int startoffset, int options, int cc,
							 int *ovector, int ovecsize, int placeholders);

const char *
presubst (pool pool, const char *str,
		  const pcre *pattern, const char *sub,
		  int options)
{
  char *newstr = pstrdup (pool, "");
  int cc, err, n, ovecsize;
  int *ovector;
  void *(*old_malloc)(size_t);
  void (*old_free) (void *);
  int placeholders = (options & PRESUBST_NO_PLACEHOLDERS) ? 0 : 1;
  int global = (options & PRESUBST_GLOBAL) ? 1 : 0;

  options &= ~(PRESUBST_NO_PLACEHOLDERS | PRESUBST_GLOBAL);

  /* Allocations to the pool. */
  old_malloc = pcre_malloc;
  old_free = pcre_free;
  pcre_malloc = malloc_in_pool;
  malloc_pool = pool;
  pcre_free = do_nothing;

  /* Get the number of capturing substrings in the pattern. */
  if ((err = pcre_fullinfo (pattern, 0, PCRE_INFO_CAPTURECOUNT, &cc)) != 0)
	abort ();

  /* Allocate a vector large enough to contain the resulting substrings. */
  ovecsize = (cc+1) * 3;
  ovector = alloca (ovecsize * sizeof (int));

  /* Find a match and substitute. */
  n = do_match_and_sub (pool, str, &newstr, pattern, sub, 0, options, cc,
						ovector, ovecsize, placeholders);

  if (global)
	{
	  /* Do the remaining matches. */
	  while (n > 0)
		{
		  n = do_match_and_sub (pool, str, &newstr, pattern, sub, n,
								options, cc,
								ovector, ovecsize, placeholders);
		}
	}
  else if (n > 0)
	{
	  /* Concatenate the remainder of the string. */
	  newstr = pstrcat (pool, newstr, str + n);
	}

  /* Restore memory allocation. */
  pcre_malloc = old_malloc;
  pcre_free = old_free;

  return newstr;
}

static int
do_match_and_sub (pool pool, const char *str, char **newstrp,
				  const pcre *pattern, const char *sub,
				  int startoffset, int options, int cc,
				  int *ovector, int ovecsize, int placeholders)
{
  int so, eo, err;
  char *newstr = *newstrp;

  /* Find the next match. */
  err = pcre_exec (pattern, 0, str, strlen (str), startoffset,
				   options, ovector, ovecsize);
  if (err == PCRE_ERROR_NOMATCH) /* No match. */
	{
	  if (startoffset == 0)
		/* Special case: we can just return the original string. */
		*newstrp = (char *) str;
	  else
		{
		  /* Concatenate the end of the string. */
		  newstr = pstrcat (pool, newstr, str + startoffset);
		  *newstrp = newstr;
		}
	  return -1;
	}
  else if (err != cc+1)			/* Some other error. */
	abort ();

  /* Get position of the match. */
  so = ovector[0];
  eo = ovector[1];

  /* Substitute for the match. */
  newstr = pstrncat (pool, newstr, str + startoffset, so - startoffset);
  if (placeholders)
	{
	  int i;

	  /* Substitute $1, $2, ... placeholders with captured substrings. */
	  for (i = 0; i < strlen (sub); ++i)
		{
		  if (sub[i] == '$' && (sub[i+1] >= '0' && sub[i+1] <= '9'))
			{
			  int n = sub[i+1] - '0';

			  if (n > cc)
				newstr = pstrncat (pool, newstr, &sub[i], 2);
			  else
				{
				  int nso = ovector[n*2];
				  int neo = ovector[n*2+1];

				  newstr = pstrncat (pool, newstr, str+nso, neo-nso);
				}

			  i++;
			}
		  else
			newstr = pstrncat (pool, newstr, &sub[i], 1);
		}
	}
  else
	newstr = pstrcat (pool, newstr, sub);

  *newstrp = newstr;
  return eo;
}

static void *
malloc_in_pool (size_t n)
{
  return pmalloc (malloc_pool, n);
}

static void
do_nothing (void *p)
{
  /* Yes, really, do nothing. */
}
