/*
** mouse:~ppr/src/ppr-papd/ppr-papd_conf.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 19 July 2001.
*/

/*
** Routines to read the PAPSRV configuration file.  The PAPSRV
** configuration file is described in the man page ppr-papd.conf(5).
*/

#include "before_system.h"
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "ppr-papd.h"

int yylex(void);
extern FILE *yyin;
struct ADV *printer;			/* for communicating with yylex() */

char *ppd_nest_fname[MAX_PPD_NEST];	/* names of all nested PPD files */
int ppd_nest_level;			/* number of PPD files now open */

/*======================================================================
** Font id routines
======================================================================*/
static char **fonts=(char **)NULL;	/* pointer to an array of character pointers */
static int fonts_count=0;		/* number of fonts in the array */
static int fonts_space=0;		/* number of array slots allocated */

/*
** Given a font name, return an id number
** which will represent it in future.
*/
static SHORT_INT make_font_id(const char fontname[])
    {
    int x;

    for(x=0; x < fonts_count; x++)
	{
	if(strcmp(fonts[x], fontname) == 0)
    	    return x;
	}

    if(fonts_count == fonts_space)	/* if more space needed */
	{
	fonts_space += 50;		/* make space for 50 more */
	fonts = (char**)gu_realloc(fonts, fonts_space, sizeof(char*) );
	}

    fonts[fonts_count++] = gu_strdup(fontname);

    return x;
    } /* end of make_font_id() */

/*
** If the font name supplied is known, return the font id
** otherwise, return -1.
*/
SHORT_INT get_font_id(const char fontname[])
    {
    int x;

    for(x=0; x < fonts_count; x++)
    	{
    	if(strcmp(fonts[x], fontname) == 0)
    	    return x;
    	}

    return -1;
    } /* end of get_font_id() */

/*
** Given a font id, return the font name.
** If the font id is not valid, return a NULL string pointer.
*/
const char *get_font_name(SHORT_INT fontid)
    {
    if(fontid >= fonts_count || fontid < 0)
    	return (const char*)NULL;

    return fonts[fontid];
    } /* end of get_font_name() */

/*====================================================================
** Routines for identifying and reading the PPD file
====================================================================*/

/*
** Given a printer name, return the PPD file name for the printer.
** The new value is in allocated memory.
**
** The variable entry_index is the adv[] array position at which
** we should store PPD option settings we happen to read.  If it
** is -1, will will ignore them.
*/
static char *get_printer_ppd(const char name[], int entry_index)
    {
    const char function[] = "get_printer_ppd";
    FILE *pf;
    char *line = NULL;
    int line_len = 128;
    char *rval = NULL;
    char *p;

    DODEBUG_STARTUP(("%s(name=\"%s\", entry_index=%d)", function, name, entry_index));

    /* Open the printer's configuration file. */
    {
    char fname[MAX_PPR_PATH];
    ppr_fnamef(fname, "%s/%s", PRCONF, name);
    if((pf = fopen(fname, "r")) == (FILE*)NULL)
	fatal(0, "%s(): fopen(\"%s\",\"r\") failed, errno=%d (%s)", function, fname, errno, gu_strerror(errno));
    }

    /* Read the printer's configuration file. */
    while((line = gu_getline(line, &line_len, pf)))
	{
	if(gu_sscanf(line, "PPDFile: %Z", &p) == 1)
	    {
	    if(rval) gu_free(rval);
	    rval = p;
	    }

	else if(entry_index != -1 && strncmp(line, "PPDOpt:", 7) == 0)
	    {
	    int len;
	    char *name, *value;

	    name = &line[7+strspn(&line[7]," \t")];	/* skip to start of first word */
	    len = strcspn(name," \t");			/* determine length of first word */
	    name[len] = '\0';				/* terminate  first word */

	    value = name + len + 1;			/* move past first word and terminator */
	    value += strspn(value, "\t ");		/* move past spaces */
	    len = strcspn(value, " \t\r\n");		/* determine length of second word */
	    value[len] = '\0';				/* terminate second word */

	    if(strcmp(name, "*InstalledMemory") == 0)
	    	{
		adv[entry_index].InstalledMemory = gu_strdup(value);
	    	}
	    else
	    	{
		struct OPTION *opt = (struct OPTION*)gu_alloc(1, sizeof(struct OPTION));
		opt->name = gu_strdup(name);
		opt->value = gu_strdup(value);
	        opt->next = adv[entry_index].options;	/* insert new option in chain */
		adv[entry_index].options = opt;
		}
	    }

	else if(entry_index != -1 && strncmp(line, "Codes:", 6) == 0)
	    {
	    int value = atoi( &line[6+strspn(&line[6], " \t")] );

	    if(value == CODES_Binary || value == CODES_TBCP)
		adv[entry_index].BinaryOK = TRUE;
	    }

	}

    fclose(pf);

    if(!rval)
	fatal(0, _("Printer \"%s\" has no \"PPDFile\" line in its config file"), name);

    return rval;			/* return the result */
    } /* end of get_printer_ppd() */

/*
** Return the PPD file name for a group.  If its printers do not all have
** the same one, return a NULL string pointer.
**
** This function returns the string in a newly allocated heap block.
*/
static const char *get_group_ppd(const char grname[])
    {
    FILE *gf;                   /* group file */
    char *line = NULL;
    int line_len = 128;
    char *ptr;
    int len;
    char *ppdname = (char*)NULL;
    struct stat statbuf;
    int grline = 0;

    DODEBUG_STARTUP(("get_group_ppd(grname=\"%s\")", grname));

    /* open the group configuration file */
    {
    char fname[MAX_PPR_PATH];
    ppr_fnamef(fname, "%s/%s", GRCONF, grname);
    if((gf = fopen(fname, "r")) == (FILE*)NULL)
	fatal(0, "get_group_ppd(): open failed");
    }

    /* read it line by line */
    while((line = gu_getline(line, &line_len, gf)))
	{
	grline++;

	if(strncmp(line, "Printer:", 8) == 0)
	    {
	    ptr = &line[8+strspn(&line[8], " \t")];
	    len = strlen(ptr);			/* remove */
	    while( len && isspace(ptr[--len]) )	/* trailing spaces */
		ptr[len] = '\0';		/* from printer name */

	    {
	    char fname[MAX_PPR_PATH];
	    ppr_fnamef(fname, "%s/%s", PRCONF, ptr); /* check if printer exists */
	    if(stat(fname, &statbuf) < 0)
		fatal(0, _("group \"%s\" member \"%s\" does not exist (%s line %d)"), grname, ptr, fname, grline);
	    }

	    {
	    char *p = get_printer_ppd(ptr, -1);
	    if(ppdname == (char*)NULL)		/* if none found yet, */
		{				/* then use this one */
		ppdname = p;
		}
	    else
		{
		if(strcmp(ppdname, p))
		    {                           /* if not same as last, */
		    gu_free(p);
		    gu_free(ppdname);
		    gu_free(line);
		    fclose(gf);                 /* then close group file */
		    return (const char*)NULL;	/* and tell caller we failed */
		    }
		}
	    }
	    }
	}

    fclose(gf);			/* close group configuration file */
    return ppdname;		/* return the name we settled on */
    } /* end of get_group_ppd() */

/*
** Read a PPD file for the benefit of a specific listen name.
** This function is passed a pointer to the ADV structure for the
** printer, the name of the PPD file, and the current line of the
** PAPSRV configuration file.
*/
static int printer_fontspace;
static void read_ppd(struct ADV *adv, const char ppdname[])
    {
    char *fname;

    DODEBUG_STARTUP(("read_ppd(adv=?, ppdname=\"%s\")", ppdname));

    /* save PPD file name */
    adv->PPDfile = ppdname;

    if(ppdname[0] == '/')
	{
	fname = gu_strdup(ppdname);	/* we free it later */
	}
    else
	{
	size_t space_needed = (sizeof(PPDDIR) + 1 + strlen(ppdname) + 1);
	fname = (char*)gu_alloc(space_needed, sizeof(char));
	snprintf(fname, space_needed, "%s/%s", PPDDIR, ppdname);
	}

    if((yyin = fopen(fname, "r")) == (FILE *)NULL)
	fatal(1, _("Can't open \"%s\", errno=%d (%s)"), fname, errno, gu_strerror(errno));

    printer = adv;		/* set global for benefit of yylex() */
    printer_fontspace = 0;

    ppd_nest_level = 0;
    ppd_nest_fname[ppd_nest_level] = fname;

    yylex();

    fclose(yyin);
    gu_free(fname);

    DODEBUG_STARTUP(("read_ppd(): done"));
    } /* end of read_ppd() */

/*
** This helper function is called by the lexer that reads the PPD file.
*/
void add_font(char *fontname)
    {
    DODEBUG_STARTUP(("add_font(fontname=\"%s\")", fontname));

    if( printer->fontcount == printer_fontspace )	/* if more space needed */
    	{
    	printer_fontspace += 50;
    	printer->fontlist = (SHORT_INT*)gu_realloc((void*)printer->fontlist, printer_fontspace, sizeof(SHORT_INT) );
    	}

    printer->fontlist[printer->fontcount++] = make_font_id(fontname);
    } /* end of add_font() */

/*===============================================================
** Read the PAPSRV configuration file.
** The format of this file is described in ppr-papd(5).
===============================================================*/
void read_conf(char *conf_fname)
    {
    FILE *conf;                 /* the configuration file */
    char line[256];             /* the current line */
    char *name, *value;
    char PPRparms[256];         /* temp storate for PPRparms: line */
    int PPRparms_linenum = 0;
    int x;                      /* used for removing trailing space */
    char *newargv[MAX_ARGV];    /* temporary parameter array */
    int line_count = 0;
    char fname[MAX_PPR_PATH];       /* use for printer/group determination */
    struct stat statbuf;
    struct ADV *thisadv;
    int eof = FALSE;		/* end of file flag */

    DODEBUG_STARTUP(("read_conf(conf_fname=\"%s\")", conf_fname));

    PPRparms[0] = '\0';
    name_count = 0;
    thisadv = &adv[name_count];

    if((conf = fopen(conf_fname, "r")) == (FILE*)NULL)
	fatal(0, _("Can't open configuration file \"%s\", errno=%d (%s)"), conf_fname, errno, gu_strerror(errno));

    for(x=0; x < PAPSRV_MAX_NAMES; x++)		/* set the whole */
	{               			/* array to default */
	adv[x].PPRname = (char*)NULL;		/* values */
	adv[x].PPDfile = (char*)NULL;
	adv[x].PAPname = (char*)NULL;
	adv[x].fontcount = 0;
	adv[x].fontlist = (SHORT_INT*)NULL;
	adv[x].LanguageLevel = 0;
	adv[x].PSVersion = (char*)NULL;
	adv[x].Resolution = (char*)NULL;
	adv[x].BinaryOK = ANSWER_UNKNOWN;
	adv[x].FreeVM = 0;
	adv[x].InstalledMemory = (char*)NULL;
	adv[x].VMOptionFreeVM = 0;
	adv[x].Product = (char*)NULL;
	adv[x].ColorDevice = ANSWER_UNKNOWN;
	adv[x].RamSize = 0;
	adv[x].FaxSupport = (char*)NULL;
	adv[x].TTRasterizer = (char*)NULL;
	adv[x].ForceAUFSSecurity = FALSE;
	adv[x].AUFSSecurityName = AUFSSECURITYNAME_USERNAME;
	adv[x].options = (struct OPTION *)NULL;
	adv[x].query_font_cache = TRUE;
	}

    /* Read until the end of the ppr-papd conf file */
    while( ! eof )
	{
	/* while ensuring a "blank line" at the end. */
	if(fgets(line, sizeof(line), conf) == (char*)NULL)
	    {
	    eof = TRUE;
	    line[0] = '\0';
	    }

	line_count++;			    /* count lines for error messages */

	if(line[0]=='#' || line[0]==';')    /* ignore comments */
	    continue;

	x = strlen(line);			/* remove trailing spaces, */
	while( (--x >= 0) && isspace(line[x]) )	/* tabs, and line feeds */
	    line[x] = '\0';

	if(name_count == PAPSRV_MAX_NAMES)
	    fatal(0, _("Too many advertised names in config file"));

	DODEBUG_STARTUP(("line = \"%s\"", line));

	if(line[0] == '[')
	    {
	    char *ptr;
	    name = "papname";
	    value = line + 1;
	    if((ptr = strrchr(value, ']')))
	    	*ptr = '\0';
	    }
	else
	    {
	    char *si = &line[strspn(line, " \t")];
	    char *di = line;
	    int c;
	    while((c = *(si++)) && c != ':' && c != '=')
	    	{
	    	if(! isspace(c))
	    	    *(di++) = tolower(c);
		}
	    *(di++) = '\0';
	    name = line;

	    value = "";
	    if(c == ':' || c == '=')
	    	{
		while(isspace(*si))
		    si++;

		value = si;
	    	}
	    }

	DODEBUG_STARTUP(("name = \"%s\", value = \"%s\"", name, value));

	/* "PAPname:" lines */
	if(strcmp(name, "papname") == 0)
	    {
	    char *ptr = value;
	    int len;
	    char name[MAX_ATALK_NAME_COMPONENT_LEN+1];
	    char type[MAX_ATALK_NAME_COMPONENT_LEN+1];
	    char zone[MAX_ATALK_NAME_COMPONENT_LEN+1];
	    char whole_name[MAX_ATALK_NAME_COMPONENT_LEN * 3 + 4];

	    if((len = strcspn(ptr, ":@")) > MAX_ATALK_NAME_COMPONENT_LEN)
	    	fatal(0, _("AppleTalk name too long (line %d)"), line_count);

	    strncpy(name, ptr, len);		/* copy name */
	    name[len] = '\0';
	    ptr += len;

	    if(*ptr == ':')		/* if type follows, */
	    	{
		ptr++;			/* skip over colon */

		if((len = strcspn(ptr,"@")) > MAX_ATALK_NAME_COMPONENT_LEN)
		    fatal(0, _("AppleTalk name \"type\" element too long (line %d)"), line_count);

		strncpy(type, ptr, len);
		type[len] = '\0';
		ptr += len;
	    	}
	    else
	    	{
	    	strcpy(type, "LaserWriter");
	    	}

	    if(*ptr == '@')		/* if zone follows, */
	    	{
		ptr++;			/* skip over at sign */

		if((len = strlen(ptr)) > MAX_ATALK_NAME_COMPONENT_LEN)
		    fatal(0, _("AppleTalk name \"zone\" element too long (line %d)"), line_count);

		strncpy(zone,ptr,len);	/* copy zone */
		zone[len] = '\0';
		ptr += len;
	    	}
	    else			/* use default zone */
	    	{
	    	strcpy(zone, default_zone);
	    	}

	    /* Assemble the whole name. */
	    snprintf(whole_name, sizeof(whole_name), "%s:%s@%s", name, type, zone);

	    /* Make a copy to keep. */
	    thisadv->PAPname = gu_strdup(whole_name);

	    continue;
	    }

	/* "PPRname:" lines */
	if(strcmp(name, "pprname") == 0)
	    {
	    if(thisadv->PPRname)
	    	fatal(0, _("Duplicate \"PPRname\" line (line %d)"), line_count);

	    thisadv->PPRname = gu_strdup(value);

	    ppr_fnamef(fname, "%s/%s", PRCONF, thisadv->PPRname);
	    if(stat(fname, &statbuf) == 0)	/* try a printer */
		{				/* if it exists, */
		char *ptr = get_printer_ppd(thisadv->PPRname, name_count);
				   /* ^ function returns pointer to its own block */
		read_ppd(thisadv, ptr);
		}

	    else                /* otherwise, try a group */
		{
		ppr_fnamef(fname, "%s/%s", GRCONF, thisadv->PPRname);
		if(stat(fname, &statbuf) == 0)
		    {
		    const char *ptr;
		    if((ptr = get_group_ppd(thisadv->PPRname)) != (char *)NULL)
			{    /* ^ function returns pointer to heap block */
			read_ppd(thisadv, ptr);
			}
		    }
		else            /* neither printer nor group */
		    {           /* this is very bad */
		    fatal(0, _("\"%s\" is not a printer or group (line %d)"),
			thisadv->PPRname, line_count);
		    }
		}
	    continue;
	    }

	/*
	** Make sure "PPRname =" has been found before we
	** allow any other lines.
	*/
	if(! thisadv->PPRname)
   	    {
	    if(name[0] == '\0')			/* Blank lines have no */
	        continue;			/* meaning yet. */
	    fatal(0, _("\"PPRname\" must be first line in record (line %d)"), line_count);
	    }

	if(strcmp(name, "ppdfile") == 0) 	/* explicitly set PPD file */
	    {
	    if(thisadv->PPDfile)
		fatal(1, _("Duplicate or unnecessary \"PPDFile\" line (line %d)"), line_count);
	    read_ppd(thisadv, gu_strdup(value));
	    continue;
	    }

	if(! thisadv->PPDfile)
	    {
	    fatal(0, _("You must put a \"PPDFile\" line here (line %d)"), line_count);
	    }

	if(strcmp(name, "pprparms") == 0)
	    {
	    strcpy(PPRparms, value);
	    PPRparms_linenum = line_count;
	    continue;
	    }

	if(strcmp(name, "binaryok") == 0)
	    {
	    if((thisadv->BinaryOK = gu_torf(value)) == ANSWER_UNKNOWN)
		fatal(0, _("\"BinaryOK\" option must be \"true\" or \"false\" (line %d)"), line_count);
	    continue;
	    }

	if(strcmp(name, "languagelevel") == 0)
	    {
	    if((thisadv->LanguageLevel = atoi(value)) < 0)
	    	fatal(0, _("\"LanguageLevel\" must be a positive integer (line %d)"), line_count);
	    continue;
	    }

	if(strcmp(name, "product") == 0)
	    {
	    thisadv->Product = gu_strdup(value);
	    continue;
	    }

	if(strcmp(name, "psversion") == 0)
	    {
	    thisadv->PSVersion = gu_strdup(value);
	    continue;
	    }

	if(strcmp(name, "faxsupport") == 0)
	    {
	    thisadv->FaxSupport = gu_strdup(value);
	    continue;
	    }

	if(strcmp(name, "resolution") == 0)
	    {
	    thisadv->Resolution = gu_strdup(value);
	    continue;
	    }

	if(strcmp(name, "ttrasterizer") == 0)
	    {
	    thisadv->TTRasterizer = gu_strdup(value);
	    continue;
	    }

	if(strcmp(name, "forceaufssecurity") == 0)
	    {
	    if((thisadv->ForceAUFSSecurity = gu_torf(value)) == ANSWER_UNKNOWN)
	    	fatal(0, _("\"ForceAUFSSecurity\" must be \"true\" or \"false\" (line %d)"), line_count);
	    if(thisadv->ForceAUFSSecurity && !aufs_security_dir)
	    	fatal(0, _("\"ForceAUFSSecurity\" option TRUE when no -X switch used"));
	    continue;
	    }

	if(strcmp(name, "aufssecurityname") == 0)
	    {
	    if(gu_strcasecmp(value, "dsc") == 0)
	    	thisadv->AUFSSecurityName = AUFSSECURITYNAME_DSC;
	    else if(gu_strcasecmp(value, "username") == 0)
	    	thisadv->AUFSSecurityName = AUFSSECURITYNAME_USERNAME;
	    else if(gu_strcasecmp(value, "realname") == 0)
	    	thisadv->AUFSSecurityName = AUFSSECURITYNAME_REALNAME;
	    else
	    	fatal(0, _("\"AUFSSecurityFor\" must be \"DSC\", \"username\", or \"realname\" (line %d)"), line_count);

	    if(thisadv->AUFSSecurityName != AUFSSECURITYNAME_DSC && ! aufs_security_dir)
	    	fatal(0, _("If the -X switch is not used then \"AUFSSecurityName\" must be \"DSC\" (line %d)"), line_count);

	    continue;
	    }

	if(strcmp(name, "ramsize") == 0)
	    {
	    if((thisadv->RamSize = atoi(value)) <= 0)
	    	fatal(0, _("\"RAMSize\" option must be a positive integer (line %d)"), line_count);
	    continue;
	    }

	if(strcmp(name, "queryfontcache") == 0)
	    {
	    int answer;
	    if((answer = gu_torf(value)) == ANSWER_UNKNOWN)
	        fatal(0, _("\"QueryFontCache\" option must be \"true\" or \"false\" (line %d)"), line_count);
	    thisadv->query_font_cache = answer ? TRUE : FALSE;
	    continue;
	    }

	/*
	** Blank line, end of record.
	*/
	if(name[0] == '\0')
	    {				/* Here we use the lines we read along */
	    int n;          		/* with the PPD file to put together */
	    char *ptr;			/* a complete picture of the printer. */
	    char tempstr[256];  	/* <-- used in excerpting */
	    int len;

	    if(! thisadv->PAPname		/* must have PAPname */
		    || ! thisadv->PPRname)	/* & PPRname */
		{
		fatal(0, "record %d is incomplete",name_count+1);
		}

	    /*
	    ** Parse the paramaters line and put the parameters into
	    ** the argv[] array for this queue.
	    */
	    ptr = PPRparms;				/* start at line start */
	    for(n=0; (n<(MAX_ARGV-1)) && *ptr; n++)	/* loop for args */
		{
		switch(*ptr)
		    {
		    case 34:				/* double quote */
			len = strcspn(ptr+1,"\"");	/* find next " */
			if(ptr[len+1] != '"')
			    fatal(0, _("Unmatched double quote in \"PPRparms\" line (line %d)"), PPRparms_linenum);
			strncpy(tempstr,ptr+1,len);	/* copy intervening */
			tempstr[len] = '\0';		/* terminate */
			ptr += (len+2);			/* eat up */
			break;
		    case 39:				/* single quote */
			len=strcspn(ptr+1,"'");
			if(ptr[len+1] != 39)
			    fatal(0, _("Unmatched single quote in \"PPRparms\" line (line %d)"), PPRparms_linenum);
			strncpy(tempstr,ptr+1,len);
			tempstr[len] = '\0';
			ptr+=(len+2);
			break;
		    default:				/* not quoted */
			len=strcspn(ptr," \t");
			strncpy(tempstr,ptr,len);
			tempstr[len] = '\0';
			ptr+=len;
		    }

		newargv[n] = gu_strdup(tempstr);		/* put in ptr in tmp array */

		ptr += strspn(ptr,"\t ");		/* eat up space */
		} /* end of for loop */

	    newargv[n++] = (char*)NULL;					/* terminate array */
	    thisadv->argv = (char**)gu_alloc(n,sizeof(char*));	/* duplicate */
	    memcpy(adv[name_count].argv, newargv, n*sizeof(char*));

	    /* Determine if the destination is protected. */
	    thisadv->isprotected = destination_protected(ppr_get_nodename(), adv[name_count].PPRname);

	    /* Advertise the name on the AppleTalk network. */
	    add_name(name_count);

	    name_count++;		/* increment count of records read */
	    thisadv = &adv[name_count];
	    PPRparms[0] = '\0';		/* delete the temp storeage */

	    continue;
	    } /* end of if blank line */

	fatal(0, "unrecognized keyword in configuration file: \"%s\"",line);
	} /* end of while() loop */

    fclose(conf);
    } /* end of read_conf() */

/* end of file */

