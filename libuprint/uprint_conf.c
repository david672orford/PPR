/*
** mouse:~ppr/src/libuprint/uprint_conf.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 26 July 2000.
*/

#include "before_system.h"
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"

#include "uprint.h"
#include "uprint_conf.h"

/* Have we read the config file yet? */
static gu_boolean loaded = FALSE;

/* The information from the config file: */
struct UPRINT_CONF conf;

/* Parser states: */
enum STATE { STATE_INIT, STATE_WELL_KNOWN, STATE_SIDELINED,
	STATE_REAL_LP, STATE_REAL_LPR, STATE_PPR, STATE_DEFAULT_DESTINATIONS };

/*
** I don't remember why uprint has its own strdup() function.
** Perhaps I was planning to make uprint independent of PPR.
** Or maybe, I wanted it to call uprint_error_callback()
** in stead of fatal().
*/
const char *uprint_strdup(const char *s)
    {
    char *p;

    if((p = (char *)malloc((strlen(s) + 1) * sizeof(char))) == (char *)NULL)
    	{
    	uprint_error_callback("malloc() failed, errno=%d (%s)", errno, gu_strerror(errno));
	return "?";
	}

    strcpy(p, s);
    return p;
    }

/*
** Make sure the indicated variable is defined as a non-empty string.
*/
static void validate_variable(const char *section, const char *keyword, const char *value)
    {
    if(! value)
    	uprint_error_callback("%s: [%s] %s is undefined", UPRINTCONF, section, keyword);
    }

/*
** Make sure all of the path variables in a path set
** are defined as non empty strings.
*/
static void validate_path_set(const char *section, struct PATH_SET *p)
    {
    validate_variable(section, "lpr", p->lpr);
    validate_variable(section, "lpq", p->lpq);
    validate_variable(section, "lprm", p->lprm);
    validate_variable(section, "lp", p->lp);
    validate_variable(section, "lpstat", p->lpstat);
    validate_variable(section, "cancel", p->cancel);
    } /* end of validate_path_set() */

/*
** The member functions call this function to make sure
** that the configuration file information has been loaded.
*/
void uprint_read_conf(void)
    {
    if(loaded)    	/* do the work only once */
    	return;
    loaded = TRUE;

    {
    FILE *f;				/* opened uprint.conf */
    struct PATH_SET *path_set = (struct PATH_SET *)NULL;
    char line[256];			/* line from uprint.conf */
    char line2[256];			/* line with spaces removed */
    enum STATE state = STATE_INIT;	/* initial parser state */
    char *p1, *p2;
    int linenum = 0;

    conf.well_known.lpr = conf.well_known.lpq = conf.well_known.lprm =
    conf.well_known.lp = conf.well_known.lpstat = conf.well_known.cancel =
    conf.sidelined.lpr = conf.sidelined.lpq = conf.sidelined.lprm =
    conf.sidelined.lp = conf.sidelined.lpstat = conf.sidelined.cancel = (const char *)NULL;

    conf.lp.sidelined = conf.lpr.sidelined = FALSE;

    conf.default_destinations.lpr = NULL;
    conf.default_destinations.lp = NULL;

    if((f = fopen(UPRINTCONF, "r")) == (FILE*)NULL)
    	{
	uprint_error_callback("Can't open \"%s\", errno=%d (%s)", UPRINTCONF, errno, gu_strerror(errno));
	return;
	}

    while(fgets(line, sizeof(line), f))
	{
	linenum++;

	if(line[0] == ';' || line[0] == '#' || line[0] == '\n')
	    continue;		/* skip comments and blank lines */

	if(line[0] == '[')	/* handle section tags */
	    {
	    if(lmatch(line, "[well known]"))
		{
	    	state = STATE_WELL_KNOWN;
	    	path_set = &conf.well_known;
	    	continue;
	    	}
	    else if(lmatch(line, "[sidelined]"))
	    	{
	    	state = STATE_SIDELINED;
	    	path_set = &conf.sidelined;
	    	continue;
	    	}
	    else if(lmatch(line, "[real lpr]") || lmatch(line, "[to lpr]"))
	    	{
	    	state = STATE_REAL_LPR;
	    	continue;
	    	}
	    else if(lmatch(line, "[real lp]") || lmatch(line, "[to lp]"))
	    	{
	    	state = STATE_REAL_LP;
	    	continue;
	    	}
	    else if(lmatch(line, "[ppr]"))
		{
	    	state = STATE_PPR;
	    	continue;
	    	}
	    else if(lmatch(line, "[default destinations]"))
	    	{
		state = STATE_DEFAULT_DESTINATIONS;
		continue;
	    	}
	    else		/* unrecognized will be caught below */
	    	{
		state = STATE_INIT;
	    	}
	    }

	/* Delete spaces: */
	for(p1=line, p2=line2; *p1 && *p1 != '\n'; p1++)
	    {
	    if(! isspace(*p1))
	    	*(p2++) = *p1;
	    }
	*p2 = '\0';

	/* If that reduce the line to nothing or revealed a comment, skip it. */
	if(line2[0] == '\0' || line2[0] == '#' || line2[0] == ';')
	    continue;

	/* Act according to section. */
	switch(state)
	    {
	    case STATE_INIT:
	    	uprint_error_callback("Ignoring \"%s\" line %d: %s", UPRINTCONF, linenum, line2);
		continue;

	    case STATE_WELL_KNOWN:
	    case STATE_SIDELINED:
		if(strncmp(line2, "lpr=", 4) == 0)
		    path_set->lpr = uprint_strdup(line2 + 4);
		else if(strncmp(line2, "lpq=", 4) == 0)
		    path_set->lpq = uprint_strdup(line2 + 4);
		else if(strncmp(line2, "lprm=", 5) == 0)
		    path_set->lprm = uprint_strdup(line2 + 5);
		else if(strncmp(line2, "lp=", 3) == 0)
		    path_set->lp = uprint_strdup(line2 + 3);
		else if(strncmp(line2, "lpstat=", 7) == 0)
		    path_set->lpstat = uprint_strdup(line2 + 7);
		else if(strncmp(line2, "cancel=", 7) == 0)
		    path_set->cancel = uprint_strdup(line2 + 7);
		else
		    break;
		continue;

	    case STATE_REAL_LPR:
		if(strncmp(line2, "sidelined=", 10) == 0)
		    {
		    switch(gu_torf(line2 + 10))
		    	{
			case ANSWER_UNKNOWN:
			    uprint_error_callback("Invalid value for [lpr] sidelined");
			    break;
			case ANSWER_TRUE:
			    conf.lpr.sidelined = TRUE;
			    break;
			case ANSWER_FALSE:
			    conf.lpr.sidelined = FALSE;
			    break;
		    	}
		    }
		else
		    break;
		continue;

	    case STATE_REAL_LP:
		if(strncmp(line2, "sidelined=", 10) == 0)
		    {
		    switch(gu_torf(line2 + 10))
		    	{
			case ANSWER_UNKNOWN:
			    uprint_error_callback("Invalid value for [lp] sidelined");
			    break;
			case ANSWER_TRUE:
			    conf.lp.sidelined = TRUE;
			    break;
			case ANSWER_FALSE:
			    conf.lp.sidelined = FALSE;
			    break;
		    	}
		    }
		else
		    break;
		continue;

	    case STATE_PPR:

		continue;

	    case STATE_DEFAULT_DESTINATIONS:
		if(strncmp(line2, "uprint-lp=", 10) == 0)
		    {
		    conf.default_destinations.lp = uprint_strdup(line2 + 10);
		    }
		else if(strncmp(line2, "uprint-lpr=", 11) == 0)
		    {
		    conf.default_destinations.lpr = uprint_strdup(line2 + 11);
		    }
	    	continue;
	    }

	/* Unclaimed line: */
	uprint_error_callback("Ignoring \"%s\" line %d: %s", UPRINTCONF, linenum, line2);
	}

    fclose(f);		/* close uprint.conf */

    validate_path_set("well known", &conf.well_known);
    validate_path_set("sidelined", &conf.sidelined);
    }
    } /* end of uprint_read_conf() */

const char *uprint_path_lpr(void)
    {
    uprint_read_conf();
    return conf.lpr.sidelined ? conf.sidelined.lpr : conf.well_known.lpr;
    }

const char *uprint_path_lpq(void)
    {
    uprint_read_conf();
    return conf.lpr.sidelined ? conf.sidelined.lpq : conf.well_known.lpq;
    }

const char *uprint_path_lprm(void)
    {
    uprint_read_conf();
    return conf.lpr.sidelined ? conf.sidelined.lprm : conf.well_known.lprm;
    }

const char *uprint_path_lp(void)
    {
    uprint_read_conf();
    return conf.lp.sidelined ? conf.sidelined.lp : conf.well_known.lp;
    }

const char *uprint_path_lpstat(void)
    {
    uprint_read_conf();
    return conf.lp.sidelined ? conf.sidelined.lpstat : conf.well_known.lpstat;
    }

const char *uprint_path_cancel(void)
    {
    uprint_read_conf();
    return conf.lp.sidelined ? conf.sidelined.cancel : conf.well_known.cancel;
    }

const char *uprint_default_destinations_lpr(void)
    {
    uprint_read_conf();
    return conf.default_destinations.lpr ? conf.default_destinations.lpr : "lp";
    }

const char *uprint_default_destinations_lp(void)
    {
    uprint_read_conf();
    return conf.default_destinations.lp ? conf.default_destinations.lp : "lp";
    }

/* end of file */

