/*
** This is a shell glob matching routine which was placed
** in the public domain by its author.
**
** Changes for PPR:
**
**	* changed arguments to const
**	* added ANSI_CONTROLS_EXTENSION and the code it enables
**	* renamed it to gu_wildmat() to avoid conflicts
**
** Last PPR change: 28 February 2005
*/
#include "gu.h"
#undef TRUE
#undef FALSE
#define ANSI_CONTROLS_EXTENSION

/*  $Revision$
**
**  Do shell-style pattern matching for ?, \, [], and * characters.
**  Might not be robust in face of malformed patterns; e.g., "foo[a-"
**  could cause a segmentation violation.  It is 8bit clean.
**
**  Written by Rich $alz, mirror!rs, Wed Nov 26 19:03:17 EST 1986.
**  Rich $alz is now <rsalz@osf.org>.
**  April, 1991:  Replaced mutually-recursive calls with in-line code
**  for the star character.
**
**  Special thanks to Lars Mathiesen <thorinn@diku.dk> for the ABORT code.
**  This can greatly speed up failing wildcard patterns.  For example:
**	pattern: -*-*-*-*-*-*-12-*-*-*-m-*-*-*
**	text 1:	 -adobe-courier-bold-o-normal--12-120-75-75-m-70-iso8859-1
**	text 2:	 -adobe-courier-bold-o-normal--12-120-75-75-X-70-iso8859-1
**  Text 1 matches with 51 calls, while text 2 fails with 54 calls.  Without
**  the ABORT code, it takes 22310 calls to fail.  Ugh.  The following
**  explanation is from Lars:
**  The precondition that must be fulfilled is that DoMatch will consume
**  at least one character in text.  This is true if *p is neither '*' nor
**  '\0'.)  The last return has ABORT instead of FALSE to avoid quadratic
**  behaviour in cases like pattern "*a*b*c*d" with text "abcxxxxx".  With
**  FALSE, each star-loop has to run to the end of the text; with ABORT
**  only the last one does.
**
**  Once the control of one instance of DoMatch enters the star-loop, that
**  instance will return either TRUE or ABORT, and any calling instance
**  will therefore return immediately after (without calling recursively
**  again).  In effect, only one star-loop is ever active.  It would be
**  possible to modify the code to maintain this context explicitly,
**  eliminating all recursive calls at the cost of some complication and
**  loss of clarity (and the ABORT stuff seems to be unclear enough by
**  itself).  I think it would be unwise to try to get this into a
**  released version unless you have a good test data base to try it out
**  on.
*/

/* a small change was done by Masaaki Kumagai to compile with c++ */

#define TRUE			1
#define FALSE			0
#define ABORT			-1


    /* What character marks an inverted character class? */
#define NEGATE_CLASS		'^'
    /* Is "*" a common pattern? */
#define OPTIMIZE_JUST_STAR
    /* Do tar(1) matching rules, which ignore a trailing slash? */
#undef MATCH_TAR_PATTERN


/*
**  Match text and p, return TRUE, FALSE, or ABORT.
*/
static int DoMatch(const char *text, const char *p)
{
    register int	last;
    register int	matched;
    register int	reverse;

    for ( ; *p; text++, p++) {
	if (*text == '\0' && *p != '*')
	    return ABORT;
	switch (*p) {
	case '\\':
	    /* Literal match with following character. */
	    p++;
#ifdef ANSI_CONTROLS_EXTENSION
	    if(*p >= '0' && *p <= '7')		/* octal */
		{
		int c = 0;
		int digits = 3;
		do  {
		    c *= 8;
		    c += *(p++) - '0';
		    } while(--digits && *p >= '0' && *p <= '7');
		p--;
		if(*text == c)
		    continue;
		return FALSE;
		}
	    if(*p == 'x')			/* hex */
	    	{
		int c = 0, c2;
		int digits = 2;
	    	p++;

		while(digits-- && (c2 = *p)
				&& ( ((c2 -= '0') >= 0 && c2 <= 9)
		    		|| ((c2 -= ('A' - '0' - 10)) >= 10 && c2 <= 15)
		    		|| ((c2 -= ('a' - 'A')) >= 10 && c2 <= 15)
		    		)
		    	)
		    	{
			p++;
			c = ((c << 4) | c2);
		    	}
		p--;
		if(*text == c)
		    continue;
		return FALSE;
	    	}
	    switch(*p)				/* controls */
	    	{
		case 'a': if(*text == '\a') continue; else return FALSE;
		case 'b': if(*text == '\b') continue; else return FALSE;
		case 'f': if(*text == '\f') continue; else return FALSE;
		case 'n': if(*text == '\n') continue; else return FALSE;
		case 'r': if(*text == '\r') continue; else return FALSE;
		case 't': if(*text == '\t') continue; else return FALSE;
		case 'v': if(*text == '\v') continue; else return FALSE;
	    	}
#endif
	    /* FALLTHROUGH */
	default:
	    if (*text != *p)
		return FALSE;
	    continue;
	case '?':
	    /* Match anything. */
	    continue;
	case '*':
	    while (*++p == '*')
		/* Consecutive stars act just like one. */
		continue;
	    if (*p == '\0')
		/* Trailing star matches everything. */
		return TRUE;
	    while (*text)
		if ((matched = DoMatch(text++, p)) != FALSE)
		    return matched;
	    return ABORT;
	case '[':
	    reverse = p[1] == NEGATE_CLASS ? TRUE : FALSE;
	    if (reverse)
		/* Inverted character class. */
		p++;
	    matched = FALSE;
	    if (p[1] == ']' || p[1] == '-')
		if (*++p == *text)
		    matched = TRUE;
	    for (last = *p; *++p && *p != ']'; last = *p)
		/* This next line requires a good C compiler. */
		if (*p == '-' && p[1] != ']'
		    ? *text <= *++p && *text >= last : *text == *p)
		    matched = TRUE;
	    if (matched == reverse)
		return FALSE;
	    continue;
	}
    }

#ifdef	MATCH_TAR_PATTERN
    if (*text == '/')
	return TRUE;
#endif	/* MATCH_TAR_ATTERN */
    return *text == '\0';
}


/*
**  User-level routine.  Returns TRUE or FALSE.
*/
int
#ifdef ANSI_CONTROLS_EXTENSION
gu_wildmat
#else
wildmat
#endif
(const char *text, const char *p)

{
#ifdef	OPTIMIZE_JUST_STAR
    if (p[0] == '*' && p[1] == '\0')
	return TRUE;
#endif	/* OPTIMIZE_JUST_STAR */
    return DoMatch(text, p) == TRUE;
}



#if	defined(TEST)
#include <stdio.h>

/* Yes, we use gets not fgets.  Sue me. */
extern char	*gets();


int
main()
{
    char	 p[80];
    char	 text[80];

    printf("Wildmat tester.  Enter pattern, then strings to test.\n");
    printf("A blank line gets prompts for a new pattern; a blank pattern\n");
    printf("exits the program.\n");

    for ( ; ; ) {
	printf("\nEnter pattern:  ");
	(void)fflush(stdout);
	if (gets(p) == NULL || p[0] == '\0')
	    break;
	for ( ; ; ) {
	    printf("Enter text:  ");
	    (void)fflush(stdout);
	    if (gets(text) == NULL)
		exit(0);
	    if (text[0] == '\0')
		/* Blank line; go back and get a new pattern. */
		break;
	    printf("      %s\n", wildmat(text, p) ? "YES" : "NO");
	}
    }

    exit(0);
    /* NOTREACHED */
}
#endif	/* defined(TEST) */

