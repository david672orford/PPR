/*
** mouse:~ppr/src/libscript/ppr_conf_query.c
** Copyright 1995--2005, Trinity College Computing Center.
** Written by David Chappell.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
** 
** * Redistributions of source code must retain the above copyright notice,
** this list of conditions and the following disclaimer.
** 
** * Redistributions in binary form must reproduce the above copyright
** notice, this list of conditions and the following disclaimer in the
** documentation and/or other materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE 
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
** POSSIBILITY OF SUCH DAMAGE.
**
** Last modified 20 January 2005.
*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"

const char myname[] = "ppr_conf_query";

/*
** This program is used to query ppr.conf.  It can either return a specific list
** member from a specific key in a specific section or it can dump an entire section.
**
** When reading this code, it is important to know that the gu_ini_*() functions
** in libppr are vary tolerant of receiving NULL pointers.  When they do they
** just report failure.
**
** In the form which retrieves a specific value rather than an entire section,
** the absence of the config file, the section, or the key are not fatal errors.
** They simply result in the returning of the default value.
*/
int main(int argc, char *argv[])
	{
	struct GU_INI_ENTRY *section = NULL;
	char *section_name;
	char *key_name = NULL;
	int value_index = 0;
	char *default_value = "";
	FILE *cf;

	if(argc < 2)
		{
		fprintf(stderr, "%s: bad usage\n", myname);
		return 1;
		}

	section_name = argv[1];
	if(argc >= 3) key_name = argv[2];
	if(argc >= 4) value_index = atoi(argv[3]);
	if(argc >= 5) default_value = argv[4];

	do	{
		if(!(cf = fopen(PPR_CONF, "r")))
			{
			fprintf(stderr, "%s: can't open \"%s\", errno=%d (%s)\n", myname, PPR_CONF, errno, gu_strerror(errno));
			return 1;
			}

		if(!(section = gu_ini_section_load(cf, section_name)))
			{
			fprintf(stderr, "%s: warning: there is no section named [%s]\n", myname, section_name);
			}

		fclose(cf);
		} while(0);

	/* If a specific key was requested */
	if(key_name)
		{
		const struct GU_INI_ENTRY *value = NULL;
		if(section && !(value = gu_ini_section_get_value(section, key_name)))
			fprintf(stderr, "%s: warning: there is no item named \"%s\" in section [%s]\n", myname, key_name, section_name);
		puts(gu_ini_value_index(value, value_index, default_value));
		}

	/* If whole section dump requested, but section not found, */
	else if(!section)
		{
		return 1;
		}

	/* If whole section dump requested and section found, */
	else
		{
		int x, y, z, c;
		for(x=0; section[x].name; x++)					/* iterate over the keys */
			{
			const char *vp = section[x].values;
			for(y=0; y < section[x].nvalues; y++)		/* iterate over the value list */
				{
				fputs("CONF_", stdout);
				for(z=0; (c=section[x].name[z]); z++)	/* iterate over the name characters */
					{
					fputc(toupper(c), stdout);
					}
				if(y > 0) printf("_%d", y);
				fputs("=\"", stdout);
				for( ; (c = *(vp++)); )					/* iterate over this list item's characters */
					{
					if(c == '\"' || c == 0x5c || c == '$')
						fputc(0x5c, stdout);
					fputc(c, stdout);
					}
				fputs("\"\n", stdout);
				}
			}
		}

	gu_ini_section_free(section);

	return 0;
	} /* end of main() */

/* end of file */

