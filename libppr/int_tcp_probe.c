/*
** mouse:~ppr/src/libppr/int_tcp_probe.c
** Copyright 1995--2006, Trinity College Computing Center.
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
** Last modified 31 March 2006.
*/

#include "config.h"
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "interface.h"
#include "libppr_int.h"

/*=========================================================================
** Probe mode
**
** In probe mode we print lines starting with "PROBE:" to stdout.  These
** lines may either be empty, to indicate that we are still alive, or
** may contain a name=value pair describing something which we have
** discovered about the printer.
**
** Anything intended for the user should be sent to stderr.  These will
** be intercepted by ppad and sent to stdout.
=========================================================================*/
int int_tcp_probe(const struct sockaddr_in *printer_address, const char snmp_community[])
	{
	/* These definitions are from RFC 1155 and 1156. */
	#define MIB			"1.3.6.1.2"
	#define  MGMT		MIB".1"
	#define   SYSTEM	MGMT".1"

	struct {
		const char *name;
		const char *id;
		} query_items[] =
		{
		{"sysDescr",		SYSTEM".1.0"},
		{"hrDeviceDescr",	MGMT".25.3.2.1.3.1"},
		{NULL, NULL}
		};

	/* Send an empty result line so that ppad will know we are alive and
	   extend the timeout.
	   */
	printf("PROBE:\n");

	gu_Try
		{
		struct gu_snmp *snmp_obj;

		snmp_obj = gu_snmp_open(printer_address->sin_addr.s_addr, snmp_community);

		gu_Try
			{
			int i;
			char *str;

			for(i=0; query_items[i].name; i++)
				{
				gu_Try
					{
					gu_snmp_get(snmp_obj,
						query_items[i].id, GU_SNMP_STR, &str,
						NULL
						);
					printf("PROBE: SNMP %s=%s\n", query_items[i].name, str);
					}
				gu_Catch
					{
					if(strstr(gu_exception, "(noSuchName)"))
						{
						fprintf(stderr, _("SNMP %s not found\n"), query_items[i].name);
						}
					else
						{
						gu_ReThrow();
						}
					}
				}
			}
		gu_Final
			{
			gu_snmp_close(snmp_obj);
			}
		gu_Catch
			{
			gu_ReThrow();
			}
		}
	gu_Catch
		{
		fprintf(stderr, _("Probe failed: %s\n"), gu_exception);
		return EXIT_PRNERR;
		}

	return EXIT_PRINTED;
	}

/* end of file */
