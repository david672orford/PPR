/*
** mouse:~ppr/src/browsers/usb.c
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
** Last modified 27 February 2005.
*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "version.h"

static gu_boolean debug = FALSE;

static const char *port_patterns[] =
	{
	"/dev/usb/lp%d",			/* Linux 2.4.x and 2.6.x with devfs */
	"/dev/usb/usblp%d",
	"/dev/usblp%d",
	NULL
	};

int main(int argc, char *argv[])
	{
	const char *port_pattern;
	char port_temp[64];
	unsigned char device_id[1024];
	int i;
	char *p, *item, *name, *value;
	char *mfg, *mdl, *sern;

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	if(argc >= 2 && strcmp(argv[1], "--debug") == 0)
		{
		argv++;
		argc--;
		debug = TRUE;
		}

	if(argc < 2)
		{
		printf("USB LP Devices\n");
		return 0;
		}

	if(strcmp(argv[1], "USB LP Devices") != 0)
		{
		printf("; Not a valid USB zone.\n");
		return 0;
		}
	
	for(i=0; (port_pattern = port_patterns[i]); i++)
		{
		gu_snprintf(port_temp, sizeof(port_temp), port_pattern, 0);
		if(access(port_temp, F_OK) == 0)
			break;
		}
	
	if(!port_pattern)	/* If no USB printer ports, */
		return 0;

	for(i=0; TRUE; i++)
		{
		gu_snprintf(port_temp, sizeof(port_temp), port_pattern, i);
		if(access(port_temp, F_OK) != 0)
			break;

		if(get_device_id(port_temp, device_id, sizeof(device_id)) == -1)
			{
			if(errno != ENODEV)
				printf("; Can't get device ID for port %s, errno=%d (%s)\n", port_temp, errno, gu_strerror(errno));
			continue;
			}

		printf("[USB Printer %d]\n", i);

		/*printf("\"%s\"\n", device_id);*/
		mfg = mdl = sern = NULL;
		for(p = device_id; (item = gu_strsep(&p, ";")); )
			{
			if((name = gu_strsep(&item, ":")) && (value = gu_strsep(&item, "")))
				{
				/*printf("name=%s value=%s\n", name, value);*/
				if(strcmp(name, "MFG") == 0 || strcmp(name, "MANUFACTURER") == 0)
					mfg = value;
				else if(strcmp(name, "MDL") == 0 || strcmp(name, "MODEL") == 0)
					mdl = value;
				else if(strcmp(name, "SERN") == 0)
					sern = value;
				}
			}

		if(mfg)
			printf("manufacturer=%s\n", mfg);
		if(mdl)
			printf("model=%s\n", mdl);

		/* If the serial number is available, including it will be an option
		 * but so will excluding it.  It MFG and MDL aren't available, we
		 * fall back on the name of the /dev/ node.
		 */
		if(mfg && mdl && sern)
			printf("interface=usb,\"MFG:%s;MDL:%s;SERN:%s\"\n", mfg, mdl, sern);
		if(mfg && mdl)
			printf("interface=usb,\"MFG:%s;MDL:%s\"\n", mfg, mdl);
		else
			printf("interface=usb,\"%s\"\n", port_temp);

		printf("\n");
		}

	return 0;	
	} /* end of main() */

/* end of file */
