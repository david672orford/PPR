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

#ifndef PSTRING_H
#define PSTRING_H

#include <stdio.h>
#include <stdarg.h>

#include <pcre.h>

#include <pool.h>
#include <vector.h>

/* Function: pstrdup - duplicate a string or area of memory
 * Function: pstrndup
 * Function: pmemdup
 *
 * @code{pstrdup} duplicates string @code{s}, allocating new memory for the
 * string in pool @code{pool}.
 *
 * @code{pstrndup} duplicates just the first @code{n} characters of the
 * string.
 *
 * @code{pmemdup} duplicates an arbitrary area of memory of size
 * @code{size} bytes starting at address @code{data}.
 */
extern char *pstrdup (pool, const char *s);
extern char *pstrndup (pool, const char *s, int n);
extern void *pmemdup (pool, const void *data, size_t size);

/* Function: pstrsplit - split a string on a character, string or regexp.
 * Function: pstrcsplit
 * Function: pstrresplit
 * Function: pstrsplit2
 * Function: pstrcsplit2
 * Function: pstrresplit2
 * 
 * These functions split string @code{str} on either a string
 * @code{sep}, a character @code{c} or a regular expression @code{re}.
 *
 * The result is a vector of newly created substrings.
 *
 * The @code{*2} variants split the string in the same way
 * on the regular expression, but keeps the matching splitting text as
 * separate elements in the vector. To illustrate this, imagine that
 * @code{pstrresplit} and @code{pstrresplit2} are called on the string
 * "This text is <b>bold</b>" with the regular expression @code{[<>]}.
 *
 * @code{pstrresplit} will return a vector containing:
 *
 * @code{ ( "This text is ", "b", "bold", "/b" ) }
 *
 * whereas @code{pstrcsplit2} will return:
 *
 * @code{ ( "This text is ", "<", "b", ">", "bold", "<", "/b", ">" ) }
 *
 * Note that the first element of the vector might be splitting
 * text, or might be ordinary text as in the example above. Also
 * the elements may not be interleaved like this (think about
 * what would happen if the original string contained @code{"<b></b>"}).
 * The only way to decide would be to call @code{prematch} on each element.
 *
 * This turns out to be very useful for certain sorts of simple
 * parsing, or if you need to reconstruct the original string (just
 * concatenate all of the elements together using @code{pconcat}).
 *
 * In common with Perl's @code{split} function, all of these functions
 * return a zero length vector if @code{str} is the empty string.
 *
 * See also: @ref{prematch(3)}, @ref{pconcat(3)}.
 */
extern vector pstrsplit (pool, const char *str, const char *sep);
extern vector pstrcsplit (pool, const char *str, char c);
extern vector pstrresplit (pool, const char *str, const pcre *re);
extern vector pstrsplit2 (pool, const char *str, const char *sep);
extern vector pstrcsplit2 (pool, const char *str, char c);
extern vector pstrresplit2 (pool, const char *str, const pcre *re);

/* Function: pconcat - concatenate a vector of strings
 * Function: pjoin
 *
 * @code{pconcat} concatenates a vector of strings to form a string.
 *
 * @code{pjoin} is similar except that @code{sep} is inserted between
 * each concatenated string in the output.
 *
 * @code{pjoin} is kind of the opposite of @ref{pstrsplit(3)}.
 */
extern char *pconcat (pool, vector);
extern char *pjoin (pool, vector, const char *sep);

/* Function: pchrs - generate a string of n repeated characters or strings
 * Function: pstrs
 *
 * @code{pchrs (pool, 'c', n)} is similar to the Perl expression
 * @code{'c' x n}. It generates a pool-allocated string of @code{n} copies
 * of character @code{'c'}.
 *
 * @code{pstrs (pool, str, n)} is similar to the Perl expression
 * @code{str x n}. It generates a pool-allocated string of @code{n} copies
 * of the string @code{str}.
 */
extern char *pchrs (pool, char c, int n);
extern char *pstrs (pool, const char *str, int n);

/* Function: pvector - generate a vector from a list or array of strings
 * Function: pvectora
 *
 * @code{pvector} takes a NULL-terminated list of strings as arguments
 * and returns a vector of strings. @code{pvectora} takes a pointer to
 * an array of strings and the number of strings and returns a vector
 * of strings.
 *
 * A typical use of this is to quickly concatenate strings:
 *
 * @code{s = pconcat (pool, pvector (pool, s1, s2, s3, NULL));}
 *
 * which is roughly equivalent to:
 *
 * @code{s = psprintf (pool, "%s%s%s", s1, s2, s3);}
 *
 * See also: @ref{pconcat(3)}, @ref{psprintf(3)}.
 */
extern vector pvector (pool, ...);
extern vector pvectora (pool, const char *array[], int n);

/* Function: psort - sort a vector of strings
 *
 * Sort a vector of strings, using @code{compare_fn} to compare
 * strings. The vector is sorted in-place.
 *
 * It is a common mistake to try to use @code{strcmp} directly
 * as your comparison function. This will not work. See the
 * C FAQ, section 12, question 12.2
 * (@code{http://www.lysator.liu.se/c/c-faq/c-12.html}).
 */
extern void psort (vector, int (*compare_fn) (const char **, const char **));

/* Function: pchomp - remove line endings from a string
 *
 * Remove line endings (either CR, CRLF or LF) from the string argument.
 * The string is modified in-place and a pointer to the string
 * is also returned.
 */
extern char *pchomp (char *line);

/* Function: ptrim - remove whitespace from the ends of a string
 * Function: ptrimfront
 * Function: ptrimback
 *
 * @code{ptrim} modifies a string of text in place, removing any
 * whitespace characters from the beginning and end of the line.
 *
 * @code{ptrimfront} is the same as @code{ptrim} but only removes
 * whitespace from the beginning of a string.
 *
 * @code{ptrimback} is the same as @code{ptrim} but only removes
 * whitespace from the end of a string.
 */
extern char *ptrim (char *str);
extern char *ptrimfront (char *str);
extern char *ptrimback (char *str);

/* Function: psprintf - sprintf which allocates the result in a pool
 * Function: pvsprintf
 *
 * The @code{psprintf} function is equivalent to @code{sprintf}
 * but it allocates the result string in @code{pool}.
 *
 * @code{pvsprintf} works similarly to @code{vsprintf}.
 */
extern char *psprintf (pool, const char *format, ...) __attribute__ ((format (printf, 2, 3)));
extern char *pvsprintf (pool, const char *format, va_list ap);

/* Function: pitoa - convert number types to strings
 * Function: pdtoa
 * Function: pxtoa
 *
 * These functions convert a decimal @code{int}, @code{double} or
 * hexadecimal @code{unsigned} into a string, which is allocated
 * in @code{pool}.
 *
 * @code{pitoa} is equivalent to @code{psprintf (pool, "%d", i)},
 * and the other functions have similar equivalents.
 */
extern char *pitoa (pool, int);
extern char *pdtoa (pool, double);
extern char *pxtoa (pool, unsigned);

/* Function: pvitostr - convert vectors of numbers to vectors of strings
 * Function: pvdtostr
 * Function: pvxtostr
 *
 * Promote vector of numbers to vector of strings.
 *
 * @code{pvitostr} expects a vector of @code{int}.
 *
 * @code{pvdtostr} expects a vector of @code{double}.
 *
 * @code{pvxtostr} expects a vector of hexadecimal @code{unsigned}.
 *
 * All functions return a vector of @code{char *}.
 */
extern vector pvitostr (pool, vector);
extern vector pvdtostr (pool, vector);
extern vector pvxtostr (pool, vector);

/* Function: pstrcat - extend a string
 * Function: pstrncat
 *
 * @code{str} is a string allocated in @code{pool}.
 * Append @code{ending} to @code{str}, reallocating
 * @code{str} if necessary.
 *
 * Because @code{str} may be reallocated (ie. moved) you
 * must invoke this function as follows:
 *
 * @code{str = pstrcat (pool, str, ending);}
 *
 * @code{pstrncat} is similar to @code{pstrcat} except that
 * only the first @code{n} characters of @code{ending}
 * are appended to @code{str}.
 */
extern char *pstrcat (pool, char *str, const char *ending);
extern char *pstrncat (pool, char *str, const char *ending, size_t n);

/* Function: psubstr - return a substring of a string
 *
 * Return the substring starting at @code{offset} and of length
 * @code{len} of @code{str}, allocated
 * as a new string. If @code{len} is negative,
 * everything up to the end of @code{str}
 * is returned.
 */
extern char *psubstr (pool, const char *str, int offset, int len);

/* Function: pstrupr - convert a string to upper- or lowercase
 * Function: pstrlwr
 *
 * Convert a string, in-place, to upper or lowercase by applying
 * @code{toupper} or @code{tolower} to each character in turn.
 */
extern char *pstrupr (char *str);
extern char *pstrlwr (char *str);

/* Function: pgetline - read a line from a file, optionally removing comments
 * Function: pgetlinex
 * Function: pgetlinec
 *
 * @code{pgetline} reads a single line from a file and returns it. It
 * allocates enough space to read lines of arbitrary length. Line ending
 * characters ('\r' and '\n') are automatically removed from the end
 * of the line.
 *
 * The @code{pool} argument is a pool for allocating the line. The
 * @code{fp} argument is the C @code{FILE} pointer. The @code{line}
 * argument is a pointer to a string allocated in pool which will
 * be reallocated and filled with the contents of the line. You may
 * pass @code{line} as @code{NULL} to get a newly allocated buffer.
 *
 * Use @code{pgetline} in one of the following two ways:
 *
 * @code{line = pgetline (pool, fp, line);}
 *
 * or
 *
 * @code{line = pgetline (pool, fp, NULL);}
 *
 * @code{pgetlinex} is a more advanced function which reads a line
 * from a file, optionally removing comments, concatenating together
 * lines which have been split with a backslash, and ignoring blank
 * lines. @code{pgetlinex} (and the related macro @code{pgetlinec}) are
 * very useful for reading lines of input from a configuration file.
 *
 * The @code{pool} argument is a pool for allocating the line. The
 * @code{fp} argument is the C @code{FILE} pointer. The @code{line}
 * argument is a buffer allocated in pool which will be reallocated
 * and filled with the result. @code{comment_set} is the set of
 * possible comment characters -- eg. @code{"#!"} to allow either
 * @code{#} or @code{!} to be used to introduce comments.
 * @code{flags} is zero or more of the following flags OR-ed
 * together:
 *
 * @code{PGETL_NO_CONCAT}: Don't concatenate lines which have been
 * split with trailing backslash characters.
 *
 * @code{PGETL_INLINE_COMMENTS}: Treat everything following a comment
 * character as a comment. The default is to only allow comments which
 * appear on a line on their own.
 *
 * @code{pgetlinec} is a helper macro which calls @code{pgetlinex}
 * with @code{comment_set == "#"} and @code{flags == 0}.
 */
extern char *pgetline (pool, FILE *fp, char *line);
extern char *pgetlinex (pool, FILE *fp, char *line, const char *comment_set, int flags);
#define pgetlinec(p,fp,line) pgetlinex ((p), (fp), (line), "#", 0)

#define PGETL_NO_CONCAT 1
#define PGETL_INLINE_COMMENTS 2

/* Function: pmap - map, search vectors of strings
 * Function: pgrep
 *
 * @code{pmap} takes a @code{vector} of strings (@code{char *}) and
 * transforms it into another @code{vector} of strings by applying
 * the function @code{char *map_fn (pool, const char *)} to each
 * string.
 *
 * @code{pgrep} applies the function @code{int grep_fn (pool, const char *)}
 * to each element in a @code{vector} of strings, and returns a
 * new vector of strings containing only those strings where
 * @code{grep_fn} returns true.
 *
 * See also: @ref{vector_map_pool(3)}, @ref{vector_grep_pool(3)}.
 */
vector pmap (pool, const vector v, char *(*map_fn) (pool, const char *));
vector pgrep (pool, const vector v, int (*grep_fn) (pool, const char *));

#endif /* PSTRING_H */
