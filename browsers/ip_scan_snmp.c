/*
** mouse:~ppr/src/browsers/ip_scan_snmp.c
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 15 April 2004.
*/

#include "before_system.h"
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifndef INADDR_NONE
#define INADDR_NONE -1
#endif
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "util_exits.h"
#include "version.h"

gu_boolean debug = 0;
#define SNMP_ATTEMPTS 3
#define MAX_ITEMS 4
#define NSTEPS 2
#define PACKET_DELAY 5000	/* 100000 == 1/10 second */

struct NODE_SCAN_STATE {
	unsigned char attempts_remaining;
	unsigned char step;
	char *sysName;
	char *sysDescr;
	char *sysLocation;
	char *hrDeviceDescr;
	};

struct QUERY_STEP {
	char query_packet[512];
	int query_packet_len;
	struct gu_snmp_items items[4];
	int items_count;
	int request_id;
	};

static void do_scan(in_addr_t first_ip, in_addr_t last_ip)
	{
	int ip_count, ip_index;
	struct NODE_SCAN_STATE *states;
	struct QUERY_STEP steps[NSTEPS];
	struct gu_snmp *s;
	char *str[MAX_ITEMS];
	char *response_buffer;
	int response_buffer_len;
	int fd;
	int selret;
	gu_boolean done_sending;

	ip_count = last_ip - first_ip + 1;
	states = gu_alloc(ip_count, sizeof(struct NODE_SCAN_STATE));

	/* Create an unconnected SNMP query object. */
	s = gu_snmp_open(INADDR_NONE, "public");

	/* Retrieve the FD of the UDP socket belonging to
	 * the SNMP query object. */
	fd = gu_snmp_fd(s);

	/* We will use the response buffer inside the SNMP object. */
	response_buffer = gu_snmp_recv_buf(s, &response_buffer_len);

	/* In the first step we ask for information that any SNMP node should have. */
	steps[0].items[0].oid = "1.3.6.1.2.1.1.5.0";			/* sysName */
	steps[0].items[0].type = GU_SNMP_STR;
	steps[0].items[0].ptr = &str[0];
	steps[0].items[1].oid = "1.3.6.1.2.1.1.1.0";			/* sysDescr */
	steps[0].items[1].type = GU_SNMP_STR;
	steps[0].items[1].ptr = &str[1];
	steps[0].items[2].oid = "1.3.6.1.2.1.1.6.0";			/* sysLocation */
	steps[0].items[2].type = GU_SNMP_STR;
	steps[0].items[2].ptr = &str[2];
	steps[0].items_count = 3;

	/* In the second step we ask for Printer MIB information. */
	steps[1].items[0].oid = "1.3.6.1.2.1.25.3.2.1.3.1";	/* hrDeviceDescr */
	steps[1].items[0].type = GU_SNMP_STR;
	steps[1].items[0].ptr = &str[0];
	steps[1].items_count = 1;

	/* Here we build a query packet for each of the steps. */
	{
	int i;
	for(i = 0; i < NSTEPS; i++)
		{
		steps[i].query_packet_len = 
			gu_snmp_create_packet(s, steps[i].query_packet, &steps[i].request_id, steps[i].items, steps[i].items_count);
		}
	}

	for(ip_index=0; ip_index < ip_count; ip_index++)
		{
		states[ip_index].step = 0;
		states[ip_index].attempts_remaining = SNMP_ATTEMPTS;
		states[ip_index].sysName = NULL;
		states[ip_index].sysDescr = NULL;
		states[ip_index].sysLocation = NULL;
		states[ip_index].hrDeviceDescr = NULL;
		}

	for(selret=0, done_sending=FALSE, ip_index=-1; !done_sending || selret > 1; )
		{
		fd_set rfds;
		struct timeval timeout;

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		if(!done_sending)
			{
			int step_count = 0;
			do
				{
				if(++ip_index == ip_count)
					ip_index = 0;
				if(++step_count > ip_count)
					{
					done_sending = TRUE;
					break;
					}
				} while(states[ip_index].attempts_remaining < 1);
			}

		if(!done_sending)
			{
			struct sockaddr_in target_node;
			struct QUERY_STEP *step;

			target_node.sin_family = AF_INET;
			target_node.sin_addr.s_addr = htonl(first_ip + ip_index);
			target_node.sin_port = htons(161);
			
			states[ip_index].attempts_remaining--;
			if(debug)
				fprintf(stderr, "to ip: %s (step %d, attempts remaining: %d)\n", inet_ntoa(target_node.sin_addr), states[ip_index].step, states[ip_index].attempts_remaining);

			step = &steps[states[ip_index].step];
			if(sendto(fd, step->query_packet, step->query_packet_len, 0, &target_node, sizeof(target_node)) == -1)
				gu_Throw("sendto() failed, errno=%d (%s)", errno, gu_strerror(errno));

			timeout.tv_sec = 0;
			timeout.tv_usec = PACKET_DELAY;	/* 100000 == 1/10 second */
			}
		else
			{
			timeout.tv_sec = 5;
			timeout.tv_usec = 0;
			done_sending = TRUE;
			}

		if((selret = select(fd + 1, &rfds, NULL, NULL, &timeout)) == -1)
			gu_Throw("select() failed, errno=%d (%s)", errno, gu_strerror(errno));

		if(selret > 0 && FD_ISSET(fd, &rfds))
			{
			struct sockaddr_in responding_node;
			unsigned int responding_node_index;
			int responding_node_len = sizeof(responding_node);
			int len;
			struct QUERY_STEP *step;

			if((len = recvfrom(fd, response_buffer, response_buffer_len, 0, &responding_node, &responding_node_len)) == -1)
				gu_Throw("recvfrom() failed, errno=%d (%s)", errno, gu_strerror(errno));
			
			if(debug)
				{
				fprintf(stderr, "ip: %s\n", inet_ntoa(responding_node.sin_addr));
				fprintf(stderr, "response size: %d\n", len);
				}

			responding_node_index = ntohl(responding_node.sin_addr.s_addr);
			responding_node_index -= first_ip;
			if(responding_node_index < 0 || responding_node_index >= ip_count)
				{
				if(debug)
					fprintf(stderr, "response is rogue!\n\n");
				continue;
				}

			step = &steps[states[responding_node_index].step];
			gu_snmp_set_result_len(s, len);

			gu_Try {
				if(gu_snmp_parse_response(s, step->request_id, step->items, step->items_count) == -1)
					{
					if(debug)
						fprintf(stderr, "Invalid response\n\n");
					continue;
					}
				}
			gu_Catch
				{
				if(debug)
					fprintf(stderr, "step %d (of 0 thru %d) failed for %s, %s\n", states[responding_node_index].step, NSTEPS, inet_ntoa(responding_node.sin_addr), gu_exception);
				continue;
				}
			
			switch(states[responding_node_index].step)
				{
				case 0:
					if(debug)
						{
						printf("sysName: %s\n", str[0]);
						printf("sysDescr: %s\n", str[1]);
						printf("sysLocation: %s\n", str[2]);
						}

					states[responding_node_index].sysName = str[0];
					states[responding_node_index].sysDescr = str[1];
					states[responding_node_index].sysLocation = str[2];
					break;
				case 1:
					if(debug)
						printf("hrDeviceDesc: %s\n", str[0]);
					states[responding_node_index].hrDeviceDescr = str[0];
					break;
				}

			/* We have a response, there is no need for more attempts. */
			states[responding_node_index].attempts_remaining = 0;

			/* If there are more steps left, move on the the next
			 * and allocate the usual number of attempts to it. */
			if(++states[responding_node_index].step < NSTEPS)
				states[responding_node_index].attempts_remaining = SNMP_ATTEMPTS;

			if(debug)
				fprintf(stderr, "\n");
			}
		}

	gu_snmp_close(s);

	/*
	 * Look thru the table of IP scanned IP addresses and emit records
	 * for all printers that responded.
	 */
	for(ip_index=0; ip_index < ip_count; ip_index++)
		{
		if(states[ip_index].sysName)
			{
			printf("[%s]\n", states[ip_index].sysName);
			#if 0
			if(states[ip_index].sysLocation)
				printf("location=%s\n", states[ip_index].sysLocation);
			if(states[ip_index].sysDescr)
				printf("firmware=%s\n", states[ip_index].sysDescr);
			#endif
			if(states[ip_index].hrDeviceDescr)
				printf("model=%s\n", states[ip_index].hrDeviceDescr);

			/* HP Jetdirect */
			if(states[ip_index].sysDescr && strstr(states[ip_index].sysDescr, "JETDIRECT"))
				{
				if(states[ip_index].sysName)
					{
					printf("interface=jetdirect,%s\n", states[ip_index].sysName);
					}
				else
					{
					struct sockaddr_in target_node;
					target_node.sin_addr.s_addr = htonl(first_ip + ip_index);
					printf("interface=jetdirect,%s\n", inet_ntoa(target_node.sin_addr));
					}
				}
			printf("\n");
			}
		}
	} /* end of do_scan() */

int main(int argc, char *argv[])
	{
	struct GU_INI_ENTRY *ini_section;
	const struct GU_INI_ENTRY *entry;
	const char *first_str = NULL, *last_str = NULL;
	in_addr_t first_ip, last_ip;
	int i;

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	{
	FILE *ppr_conf;
	if(!(ppr_conf = fopen(PPR_CONF, "r")))
		{
		printf(";Can't open \"%s\", errno=%d (%s)\n", PPR_CONF, errno, gu_strerror(errno));
		return 0;
		}
	ini_section = gu_ini_section_load(ppr_conf, "ip_snmp_scan");
	fclose(ppr_conf);
	if(!ini_section)
		{
		printf(";No zones defined in \"%s\".\n", PPR_CONF);
		return 0;
		}
	}

	if(argc < 2)
		{
		i = 0;
		while((entry = gu_ini_section_get_value_by_index(ini_section, i++)))
			{
			printf("%s\n", gu_ini_value_index(entry, 0, "?"));
			}
		return 0;
		}

	i = 0;
	entry = NULL;
	while((entry = gu_ini_section_get_value_by_index(ini_section, i++)))
		{
		const char *name = gu_ini_value_index(entry, 0, "?");
		if(strcmp(name, argv[1]) == 0)
			{
			break;
			}
		}

	if(!entry)
		{
		printf(";No such zone: \"%s\"\n", argv[1]);
		return 0;
		}

	first_str = gu_ini_value_index(entry, 1, NULL);
	last_str = gu_ini_value_index(entry, 2, NULL);

	if(!first_str || !last_str)
		{
		printf("; Bad range for zone \"%s\".\n", argv[1]);
		return 0;
		}

	first_ip = ntohl(inet_addr(first_str));
	last_ip = ntohl(inet_addr(last_str));

	if(!(first_ip <= last_ip))
		{
		printf("; Bad range for zone \"%s\".\n", argv[1]);
		return 0;
		}

	do_scan(first_ip, last_ip);

	return 0;
	} /* end of main() */

/* end of file */
