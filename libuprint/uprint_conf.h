/*
** mouse:~ppr/src/include/uprint_conf.h
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 26 April 2002.
*/

struct PATH_SET
    {
    const char *lpr;
    const char *lpq;
    const char *lprm;
    const char *lp;
    const char *lpstat;
    const char *cancel;
    };

struct UPRINT_CONF
    {
    struct PATH_SET well_known;
    struct PATH_SET sidelined;

    struct
    	{
	gu_boolean installed;
    	gu_boolean sidelined;
	const char *printers;
	const char *classes;
	const char *flavor;
	float flavor_version;
    	} lp;
    struct
    	{
	gu_boolean installed;
    	gu_boolean sidelined;
	const char *flavor;
	float flavor_version;
    	} lpr;

    struct
        {
        const char *lp;
        const char *lpr;
        } default_destinations;

    } ;

void uprint_read_conf(void);
extern struct UPRINT_CONF conf;

/* end of file */

