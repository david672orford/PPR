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
** Last modified 2 April 2004.
*/

#include "before_system.h"
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
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

int main(int argc, char *argv[])
	{
	in_addr_t first_ip, last_ip, current_ip;
	struct gu_snmp *s;
	struct gu_snmp_items items[4];
	int items_count;
	char *str[4];
	char query_packet[1024];
	int query_packet_len;
	char *response_buffer;
	int response_buffer_len;
	int fd;
	int selret;
	gu_boolean done_sending;

	/* Initialize international messages library. */
	#ifdef INTERNATIONAL
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	#endif

	current_ip = first_ip = ntohl(inet_addr(argv[1]));
	last_ip = ntohl(inet_addr(argv[2]));
	
	/* Create an unconnected SNMP query object. */
	s = gu_snmp_open(INADDR_NONE, "public");

	/* Retrieve the FD of the UDP socket belonging to
	 * the SNMP query object. */
	fd = gu_snmp_fd(s);

	/* We will use the response buffer inside the SNMP object. */
	response_buffer = gu_snmp_recv_buf(s, &response_buffer_len);

	/* Construct a list of items which we will ask for from the
	 * SNMP nodes. */
	items[0].oid = "1.3.6.1.2.1.1.5.0";			/* sysName */
	items[0].type = GU_SNMP_STR;
	items[0].ptr = &str[0];
	items[1].oid = "1.3.6.1.2.1.1.1.0";			/* sysDescr */
	items[1].type = GU_SNMP_STR;
	items[1].ptr = &str[1];
	items[2].oid = "1.3.6.1.2.1.1.6.0";			/* sysLocation */
	items[2].type = GU_SNMP_STR;
	items[2].ptr = &str[2];
	items[3].oid = "1.3.6.1.2.1.25.3.2.1.3.1";	/* hrDeviceDescr */
	items[3].type = GU_SNMP_STR;
	items[3].ptr = &str[3];
	items_count = 3;

	/* Construct an SNMP query packet which we will use many times. */
	query_packet_len = gu_snmp_create_packet(s, query_packet, items, items_count);

	for(selret = 0, done_sending = FALSE; !done_sending || selret > 1; )
		{
		fd_set rfds;
		struct timeval timeout;

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		if(current_ip <= last_ip)
			{
			struct sockaddr_in target_node;

			target_node.sin_family = AF_INET;
			target_node.sin_addr.s_addr = htonl(current_ip++);
			target_node.sin_port = htons(161);

			if(sendto(fd, query_packet, query_packet_len, 0, &target_node, sizeof(target_node)) == -1)
				gu_Throw("sendto() failed, errno=%d (%s)", errno, gu_strerror(errno));

			timeout.tv_sec = 0;
			timeout.tv_usec = 10000;	/* 100000 == 1/10 second */
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
			int responding_node_len;
			int len;

			if((len = recvfrom(fd, response_buffer, response_buffer_len, 0, &responding_node, &responding_node_len)) == -1)
				gu_Throw("recvfrom() failed, errno=%d (%s)", errno, gu_strerror(errno));
			
			printf("ip: %s\n", inet_ntoa(responding_node.sin_addr));
			printf("response size: %d\n", len);

			gu_snmp_set_result_len(s, len);
			if(gu_snmp_parse_response(s, items, items_count) == -1)
				{
				printf("Invalid response\n");
				}
			else
				{
				int i;

				printf("sysName: %s\n", str[0]);
				printf("sysDescription: %s\n", str[1]);
				printf("sysLocation: %s\n", str[2]);
				/*printf("hrDeviceDesc: %s\n", str[3]);*/

				for(i=0; i < items_count; i++)
					{
					gu_free(str[i]);
					}
				}

			printf("\n");
			}
		}

	gu_snmp_close(s);

	return 0;
	} /* end of main() */

/* end of file */
