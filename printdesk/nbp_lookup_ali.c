/*
** mouse:~ppr/src/printdesk/nbp_lookup_ali.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 22 November 2000.
*/

/*
** An interface to nbp_lookup() which is used by atchooser
** and this CGI scripts in "../www".
**
** This program must be linked with Netatalk and NATALI.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <at/appletalk.h>
#include <at/nbp.h>
#include "gu.h"

int basic(char *argv[])
    {
    char *name = argv[0];
    int max = 100;
    at_retry_t retries;
    at_entity_t parsed_name;
    at_nbptuple_t *buffer;
    u_char more;
    int ret, x;

    /*
    ** Parse the options.
    */
    max = 100;
    retries.retries = 8;
    retries.interval = 1;

    if(argv[1])
    	{
	if((max = atoi(argv[1])) < 1)
	    {
	    fprintf(stderr, "Max value of \"%s\" is invalid.\n", argv[1]);
	    return 1;
	    }

	if(argv[2])
	    {
	    if( (retries.interval = atoi(argv[2])) < 1 )
		{
		fprintf(stderr, "Retry interval of \"%s\" is invalid.\n", argv[2]);
		return 1;
		}

	    if(argv[3])
	    	{
		if((retries.retries = atoi(argv[3])) < 1)
		    {
		    fprintf(stderr, "Retry count of \"%s\" is invalid.\n", argv[3]);
		    return 1;
		    }
		}
	    }
	}

    /*
    ** Now, parse the name.
    */
    if(nbp_parse_entity(&parsed_name, name) == -1)
    	{
    	fprintf(stderr, "Syntax error in name \"%s\".\n", name);
	return 1;
	}

    /*
    ** Allocate a buffer to hold the result.
    */
    if((buffer = (at_nbptuple_t*)calloc(max, sizeof(at_nbptuple_t))) == (at_nbptuple_t*)NULL)
    	{
    	fprintf(stderr, "calloc() failed, errno=%d (%s)\n", errno, gu_strerror(errno));
	return 1;
    	}

    /*
    ** Do the lookup.
    */
    if((ret = nbp_lookup(&parsed_name, buffer, max, &retries, &more)) == -1)
    	{
    	fprintf(stderr, "Lookup failed!\n");
	return 1;
    	}

    /*
    ** Print what we found.
    */
    for(x=0; x < ret; x++)
    	{
	at_nbptuple_t *t = &buffer[x];
	printf("%d:%03d:%03d %d %.*s:%.*s@%.*s\n",
		(int)t->enu_addr.net, (int)t->enu_addr.node, (int)t->enu_addr.socket,
		(int)t->enu_enum,
		(int)t->enu_entity.object.len, t->enu_entity.object.str,
		(int)t->enu_entity.type.len, t->enu_entity.type.str,
		(int)t->enu_entity.zone.len, t->enu_entity.zone.str);
    	}

    /*
    ** Did the buffer overflow?
    */
    if(more)
    	{
    	printf("MORE\n");
    	return 1;
    	}

    return 0;
    }

struct NAME { int iteration; char *str; };

int gui_backend(char *argv[])
    {
    char *name = argv[0];
    at_retry_t retries;
    at_entity_t parsed_name;
    struct NAME *names = (struct NAME *)NULL;
    int names_count = 0;
    int names_arraysize = 0;
    at_nbptuple_t *buffer = (at_nbptuple_t*)NULL;
    int max = 0;
    u_char more;
    int ret, x, y;
    int iteration;

    /*
    ** Now, parse the name.
    */
    if(nbp_parse_entity(&parsed_name, name) == -1)
    	{
    	fprintf(stderr, "Syntax error in name \"%s\".\n", name);
	return 1;
	}

    for(iteration=1, more=1; 1; iteration++)
	{
	/* printf("iteration %d\n", iteration); */

	if(iteration == 1)
	    {
	    retries.retries = 2;
	    retries.interval = 1;
	    }
	else if(iteration == 2)
	    {
	    retries.retries = 4;
	    retries.interval = 1;
	    }
	else if(iteration == 3)
	    {
	    retries.retries = 8;
	    retries.interval = 1;
	    }
	else
	    {
	    retries.retries = 4;
	    retries.interval = 15;
	    }

	/*
	** If more is set because this is the first iteration or if it is set because
	** nbp_lookup() set it on the previous iteration, allocate 100 more tuple buffer
	** slots than we had last time.
	*/
	if(more)
	    {
	    max += 100;
	    if((buffer = (at_nbptuple_t*)realloc(buffer, sizeof(at_nbptuple_t) * max)) == (at_nbptuple_t*)NULL)
	    	{
		fprintf(stderr, "realloc() failed, errno=%d (%s)\n", errno, gu_strerror(errno));
		return 1;
	    	}
	    }

	if((ret = nbp_lookup(&parsed_name, buffer, max, &retries, &more)) == -1)
	    {
	    fprintf(stderr, "Lookup failed!\n");
	    return 1;
    	    }

	for(x=0; x < ret; x++)
	    {
	    char temp[100];
	    at_nbptuple_t *t = &buffer[x];
	    int cmp;

	    sprintf(temp, "%.*s:%.*s@%.*s",
		(int)t->enu_entity.object.len, t->enu_entity.object.str,
		(int)t->enu_entity.type.len, t->enu_entity.type.str,
		(int)t->enu_entity.zone.len, t->enu_entity.zone.str);

	    /* printf("x = %d, temp = \"%s\"\n", x, temp); */

	    for(y=0, cmp=1; y < names_count; y++)
		{
		cmp = gu_strcasecmp(temp, names[y].str); /*  || strcmp(temp, names[y].str); */

		/* printf("y = %d, names[y] = {%d, \"%s\"}, cmp = %d\n", y, names[y].iteration, names[y].str, cmp); */

		if(cmp <= 0)
		    break;
		}

	    if(cmp != 0)
	    	{
		if(names_count == names_arraysize)
		    {
		    names_arraysize += 100;
		    if( (names = (struct NAME *)realloc(names, names_arraysize * sizeof(struct NAME))) == (struct NAME *)NULL )
		    	{
		    	fprintf(stderr, "realloc() failed, errno=%d (%s)\n", errno, gu_strerror(errno));
		    	return 1;
		    	}
		    }

		if(y < names_count)
		    memmove(&names[y+1], &names[y], ((names_count - y) * sizeof(struct NAME)));

		if((names[y].str = strdup(temp)) == (char*)NULL)
		    {
		    fprintf(stderr, "strdup() failed, errno=%d (%s)\n", errno, gu_strerror(errno));
		    return 1;
		    }

	    	names_count++;

		printf("%d %s\n", y, temp);
	    	}

	    names[y].iteration = iteration;
	    }

	/* Remove names which have disappeared. */
	for(x=0; x < names_count; x++)
	    {
	    if( (iteration - names[x].iteration) > 2 )
	    	{
		/* printf("Dropping %s\n", names[x].str); */
		printf("%d\n", x);
	    	if((x+1) < names_count)
		    {
		    free(names[x].str);
	    	    memmove(&names[x], &names[x+1], names_count - x - 1);
		    x--;
	    	    }
		names_count--;
	    	}
	    }

	fputc('\n', stdout);	/* divider line */
	fflush(stdout);
	} /* end of never ending loop */
    } /* end of gui_backend() */

int main(int argc, char *argv[])
    {
    if(argc > 2 && strcmp(argv[1], "--gui-backend") == 0)
    	{
    	return gui_backend(&argv[2]);
    	}
    else if(argc > 1)
    	{
    	return basic(&argv[1]);
	}
    else
    	{
    	fputs("Usage: nbp_lookup <name> [<max>] [<delay>] [<retries>]\n"
    	      "       nbp_lookup --gui-backend <name>\n", stderr);
	return 1;
    	}
    }

/* end of file */
