/* Regular expression functions which allocate in the pool.
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

#ifndef PRE_H
#define PRE_H

#include <pcre.h>

#include <pool.h>
#include <vector.h>

/* Function: precomp - Compile, match, substitute regular expressions.
 * Function: prematch
 * Function: presubst
 *
 * These functions are wrappers around the Perl Compatible
 * Regular Expressions (PCRE) library (see
 * @code{http://www.pcre.org/}).
 *
 * @code{precomp} compiles the regular expression @code{pattern}
 * returning a pointer to the opaque @code{pcre} structure. The
 * structure is allocated in @code{pool}. The @code{options} argument
 * is a list of PCRE options, passed directly to the
 * @code{pcre_compile} function (see @ref{pcre(3)}). You
 * should normally set @code{options} to 0.
 *
 * @code{prematch} matches the string @code{str} with the
 * compiled regular expression @code{pattern}.
 *
 * If there is no match, this returns @code{NULL}. If the string
 * matches, then this function returns a @code{vector} of @code{char *},
 * allocated in @code{pool}.  The first element of this vector is
 * the portion of the original string which matched the whole
 * pattern. The second and subsequent elements of this vector are
 * captured substrings. It is possible in rare circumstances for some
 * of these captured substrings to be @code{NULL} (see the
 * @ref{pcre(3)} manual page for an example).
 *
 * The @code{options} argument is passed directly to
 * @code{pcre_exec}. You should normally set @code{options} to 0.
 *
 * @code{presubst} substitutes @code{sub} for @code{pattern}
 * wherever @code{pattern} occurs in @code{str}. It is equivalent
 * to the @code{str =~ s/pat/sub/} function in Perl.
 *
 * Placeholders @code{$1}, @code{$2}, etc. in @code{sub} are
 * substituted for the matching substrings of @code{pattern}.
 * Placeholder substitution can be disabled completely by
 * including the @code{PRESUBST_NO_PLACEHOLDERS} flag in @code{options}.
 *
 * If the @code{PRESUBST_GLOBAL} flag is given, then all
 * matches are substituted. Otherwise only the first match
 * is substituted.
 *
 * The @code{options} argument is passed to @code{pcre_exec}
 * (after removing the @code{PRESUBST_*} flags).
 *
 * The return value from @code{presubst} is the string with
 * replacements.
 *
 * See also: @ref{pcre(3)}.
 */
pcre *precomp (pool pool, const char *pattern, int options);
vector prematch (pool pool, const char *str, const pcre *pattern, int options);
const char *presubst (pool pool, const char *str, const pcre *pattern, const char *sub, int options);

#define PRESUBST_NO_PLACEHOLDERS 0x10000000
#define PRESUBST_GLOBAL          0x20000000

#endif /* PRE_H */
