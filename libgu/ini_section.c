/*
** mouse:~ppr/src/libgu/gu_ini_section.c
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
** Last modified 21 November 2000.
*/

#include "before_system.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"

/*
** This function reads a specified section out of a configuration file.
** The file is in a format derived from the format of Microsoft
** Windows .ini files.  The section is parsed and stored in a memory
** block and a pointer to that block is returned.  Other functions
** may be used to extract name=value pairs from the memory block.
**
** If the configuration file cannot be opened or the requested section
** cannot be found, then a NULL pointer is returned.  The caller may
** test the pointer for a NULL value, but is not required to since all
** of the functions which take a pointer to such a memory block as an
** argument have a reasonable default action to take when they are passed
** a NULL pointer.
*/
struct GU_INI_ENTRY *gu_ini_section_load(FILE *file, const char section_name[])
    {
    char *line = NULL;			/* the line we are working on */
    int line_available = 100;		/* initial buffer size */
    char *si, *di;			/* general use copying pointers */

    struct GU_INI_ENTRY *list = NULL;	/* the list of name=value pairs */
    int list_used = 0;
    int list_avail = 0;

    if(! file) return NULL;		/* fopen() could have failed */

    fseek(file, 0, SEEK_SET);		/* rewind the file */

    /* Go until we se the desired section or run out of file. */
    {
    while((line = gu_getline(line, &line_available, file)))
	{
	if(line[0] == '[' && strchr(line, ']'))
	    {
	    int c;
	    for(di=si=line+1; (c=*(si++)) != ']'; )
	    	{
		if(!isspace(c))
		    *(di++) = tolower(c);
	    	}
	    *di = '\0';

	    if(strcmp(line + 1, section_name) == 0)
	    	{
	    	break;
	    	}
	    }
	}

    if(!line)		/* if section wasn't found, */
    	return NULL;
    } /* end of find section */

    /*
    ** Parse the entire section, storing the name=value
    ** pairs.
    */
    {
    int list_count;			/* number of elements in list */
    int quote_mode;			/* are we within quote marks now? */
    int c, lastc;			/* current and previous characters */
    int this_member_count;		/* number of characters so far in current list member */

    while((line = gu_getline(line, &line_available, file)))
	{
	di = si = line;
	si += strspn(si, " \t");

	/* skip blank links */
	if(!*si) continue;
	/* skip comments */
	if(*si == '#' || *si == ';') continue;
	/* stop before start of next section */
	if(*si == '[') break;

	if(strchr(si, '='))		/* If the name part is present, */
	    {
	    /* Convert name to lower case and remove spaces */
	    int c;
	    while((c=*(si++)) != '=')
	    	{
		if(!isspace(c))
		    *(di++) = tolower(c);
	    	}
	    }
	*(di++) = '\0';

	list_count = 0;
	quote_mode = FALSE;
	this_member_count = 0;
	lastc = '\0';
	do  {
	    c = *(si++);
	    if(c == '"' && lastc != 0x5c)	/* unescaped quote mark */
	    	{
		quote_mode = !quote_mode;	/* flip quote mode */
		continue;
		}
	    else if(c == 0x5c && lastc != 0x5c)	/* backslash not literalized */
		{				/* is only noted in lastc */
		continue;
		}
	    else if(quote_mode && c != '\0')	/* in quote mode, only EOL ends the member */
	    	{
            	*(di++) = c;
            	this_member_count++;
	    	}
	    else if(isspace(c) && this_member_count == 0)
	    	{				/* spaces at the begining of */
	    	continue;			/* unquoted strings are ignored */
	    	}
	    else if(isspace(c) || c == ',' || c == '\0')
		{
		*(di++) = '\0';
		list_count++;
		this_member_count = 0;
		}
            else
            	{
            	*(di++) = c;
            	this_member_count++;
            	}
	    } while(c);

	/* Save the line we have just parsed */
	{
	char *name, *first_value;
        int len = ((di - line) + 1);
	name = (char*)gu_alloc(len, sizeof(char));
	memcpy(name, line, len);
	first_value = name + strlen(name) + 1;

	if(list_used == list_avail)
	    {
	    list_avail += 15;
	    list = (struct GU_INI_ENTRY *)ppr_realloc(list, list_avail, sizeof(struct GU_INI_ENTRY));
	    }
	list[list_used].name = name;
	list[list_used].values = first_value;
	list[list_used].nvalues = list_count;
	list_used++;
	} /* end of save line */

	} /* end of section line loop */
    } /* end of parse section */

    /* Free the line buffer if gu_getline() didn't do it on end of file. */
    if(line) gu_free(line);

    /* Adjust the list to the proper size and terminate it. */
    list_used++;
    if(list_used != list_avail)
    	list = (struct GU_INI_ENTRY *)ppr_realloc(list, list_used, sizeof(struct GU_INI_ENTRY));
    list_used--;
    list[list_used].name = NULL;

    return list;
    } /* end of gu_ini_section_load() */

/*
** This function de-allocates the memory used by the return value
** of gu_ini_section_load().  Notice that if the pointer is NULL,
** this function does nothing.
*/
void gu_ini_section_free(struct GU_INI_ENTRY *section)
    {
    if(section)
	{
	int x;
	for(x=0; section[x].name; x++)	/* because it is in the same block as the name, */
	    gu_free(section[x].name);	/* we don't have to free the value */
	gu_free(section);
	}
    } /* end of gu_ini_section_free() */

/*
** This function extracts a value from a section loaded by the
** gu_ini_section_load() function.
*/
const struct GU_INI_ENTRY *gu_ini_section_get_value(const struct GU_INI_ENTRY *section, const char key_name[])
    {
    int x;
    const struct GU_INI_ENTRY *match = NULL;

    if(!section) return NULL;

    for(x=0; section[x].name; x++)
	{
	if(strcmp(section[x].name, key_name) == 0)
	    {
	    match = &section[x];
	    break;
	    }
	}

    return match;
    } /* end of gu_ini_section_get_value() */

/*
** The values returned by gu_ini_section_get_value() are lists (arrays).
** This function returns a value at a specified possition in the
** list.  The first item is at possition 0.
*/
const char *gu_ini_value_index(const struct GU_INI_ENTRY *array, int array_index, const char *default_value)
    {
    const char *values;

    if(!array || array_index >= array->nvalues)
    	return default_value;

    values = array->values;
    while(array_index > 0)
	{
	if(*(values++) == '\0')
	    array_index--;
	}

    return values;
    } /* end of gu_ini_index() */

/*
** This assigns
**
** This function leaks memory when it fails!!!
*/
int gu_ini_vassign(const struct GU_INI_ENTRY *array, va_list args)
    {
    enum GU_INI_TYPES this_type;
    int i;

    for(i=0; (this_type = va_arg(args, enum GU_INI_TYPES)) != GU_INI_TYPE_END; i++)
    	{
	switch(this_type)
	    {
	    case GU_INI_TYPE_SKIP:
	    	break;

	    case GU_INI_TYPE_NONEMPTY_STRING:
		{
		const char *t = gu_ini_value_index(array, i, "");
		const char **p;
		if(!*t) return -1;
		p = va_arg(args, const char**);
		*p = gu_strdup(t);
		}
	    	break;

	    case GU_INI_TYPE_STRING:
		{
		const char *t = gu_ini_value_index(array, i, NULL);
		const char **p;
		if(!t) return -1;
		p = va_arg(args, const char**);
		*p = gu_strdup(t);
		}
	    	break;

	    case GU_INI_TYPE_POSITIVE_DOUBLE:
	        {
		double t = strtod(gu_ini_value_index(array, i, "0.0"), NULL);
		double *p;
		if(t <= 0.0) return -1;
		p = va_arg(args, double*);
		*p = t;
	        }
	        break;

	    case GU_INI_TYPE_NONNEG_DOUBLE:
	        {
		double t = strtod(gu_ini_value_index(array, i, "-1.0"), NULL);
		double *p;
		if(t < 0.0) return -1;
		p = va_arg(args, double*);
		*p = t;
	        }
	        break;

	    case GU_INI_TYPE_NONNEG_INT:
	    	{
		int t = atoi(gu_ini_value_index(array, i, "-1"));
		int *p;
		if(t < 0) return -1;
		p = va_arg(args, int*);
		*p = t;
	    	}
		break;

	    #ifdef GNUC_HAPPY
	    case GU_INI_TYPE_END:
	        break;
	    #endif
	    }
    	}

    return 0;
    } /* end of gu_ini_vassign() */

int gu_ini_assign(const struct GU_INI_ENTRY *array, ...)
    {
    va_list va;
    int retval;
    va_start(va, array);
    retval = gu_ini_vassign(array, va);
    va_end(va);
    return retval;
    } /* end of gu_ini_assign() */

/*
** This is used to quickly open a file, get a section, get values off of a
** line, close it again, and clean up.
**
** The arguments are as follows:
**
**	file_name	The name of the config file
**	section_name	The section, without the []'s
**	key_name	The name on the left hand side of the equals sign
**	...		alternating enum GU_INI_TYPES and pointers
*/
const char *gu_ini_scan_list(const char file_name[], const char section_name[], const char key_name[], ...)
    {
    FILE *cf;
    struct GU_INI_ENTRY *section;		/* not const because we free it */
    const struct GU_INI_ENTRY *value;	/* const because not freed directly */
    va_list va;
    const char *retval = NULL;

    do  {	/* try */
        if(!(cf = fopen(file_name, "r")))
            {
            retval = N_("can't open config file");
            break;
            }

	do  {	/* try */
            if(!(section = gu_ini_section_load(cf, section_name)))
                {
                retval = N_("can't find section");
                break;
                }

            do  {   /* try */
                if(!(value = gu_ini_section_get_value(section, key_name)))
                    {
                    retval = N_("can't find key in section");
                    break;
                    }

                /* block for local variables */
                    {
                    int iretval;
                    va_start(va, key_name);
                    iretval = gu_ini_vassign(value, va);
                    va_end(va);
                    if(iretval == -1)
			{
                    	retval = N_("value is not in the correct format");
                    	break;
                    	}
                    }
                } while(0);		/* try gu_ini_section_get_value() */
	    gu_ini_section_free(section);
	    } while(0);			/* try gu_ini_section_load() */
        fclose(cf);
	} while(0);			/* try fopen() */

    return retval;
    } /* end of gu_ini_scan_list() */

/*
** This returns a string value from the INI file.  If there is none,
** the default value is returned.
*/
char *gu_ini_query(const char file_name[], const char section_name[], const char key_name[], int index, const char default_value[])
    {
    FILE *cf = fopen(file_name, "r");
    struct GU_INI_ENTRY *section = gu_ini_section_load(cf, section_name);
    char *value = gu_strdup(gu_ini_value_index(gu_ini_section_get_value(section, key_name), index, default_value));
    gu_ini_section_free(section);
    if(cf) fclose(cf);
    return value;
    } /* end of gu_ini_query() */

/* end of file */

