/*
** mouse:~ppr/src/lprsrv/lprsrv_client_info.c
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
** Last modified 11 September 2000.
*/

/*
** This modules contains functions which call the low level TCP/IP
** and DNS functions to determine the name of the host at the other
** end of the connexion or to determine the fully qualified name
** of the host lprsrv is running on.
*/

#include "before_system.h"
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

#include "lprsrv.h"
#include "uprint.h"

#ifdef DEBUG_SECURITY
static void dump_hostent(struct hostent *hostent, const char message[], ...)
    {
    va_list va;
    FILE *logfile;
    if((logfile = fopen(LPRSRV_LOGFILE, "a")) != NULL)
	{
	int x;
	fprintf(logfile, "DEBUG: (%ld, %s) ", (long)getpid(), datestamp());
	va_start(va, message);
	vfprintf(logfile, message, va);
	fprintf(logfile, "\n");
	va_end(va);
	fprintf(logfile, "struct hostent {\n");
	fprintf(logfile, "\tchar *h_name = \"%s\"\n", hostent->h_name);
	fprintf(logfile, "\tchar **h_aliases = {");
	for(x=0; hostent->h_aliases[x]; x++)
	    fprintf(logfile, "%s\n\t\t\"%s\"", x ? "," : "", hostent->h_aliases[x]);
	fprintf(logfile, "\n\t\t}\n");
	fprintf(logfile, "\tint h_addrtype = %d\n", hostent->h_addrtype);
	fprintf(logfile, "\tint h_length = %d\n", hostent->h_length);
	fprintf(logfile, "\tchar **h_addr_list = {");
	for(x=0; hostent->h_addr_list[x]; x++)
	    {
	    struct sockaddr_in temp;
	    memcpy(&temp.sin_addr, hostent->h_addr_list[x], hostent->h_length);
	    fprintf(logfile, "%s\n\t\t\"%s\"", x ? "," : "", inet_ntoa(temp.sin_addr));
	    }
	fprintf(logfile, "\n\t\t}\n");
	fprintf(logfile, "\t}\n");
	fclose(logfile);
	}
    }
#endif

/*
** Determine the name and port of the remote end of stdin.
*/
void get_client_info(char *client_dns_name, char *client_ip, int *client_port)
    {
    struct sockaddr_in client_address;
    struct hostent *result;
    char *name;
    const char *function = "get_client_info";

    /* Learn the IP address of the one we are talking to. */
    {
    unsigned int client_address_len = sizeof(client_address);	/* !!! things are changing !!! */
    if(getpeername(0, (struct sockaddr *)&client_address, &client_address_len) == -1)
	{
	if(errno == ENOTSOCK)
	    fatal(1, _("stdin is not a TCP socket (Run from Inetd or use -s option.)"));
	fatal(1, "%s(): getpeername() failed, errno=%d (%s)", function, errno, gu_strerror(errno) );
	}
    }

    /* Make sure it is an internet address. */
    if(client_address.sin_family != AF_INET)
	fatal(1, "%s(): stdin doesn't have an address of type AF_INET!", function);

    /* Convert the IP address to a string and store it for use in logs. */
    strcpy(client_ip, inet_ntoa(client_address.sin_addr));

    /* Make a note of the port the request is coming from. */
    *client_port = ntohs(client_address.sin_port);

    /* Copy the IP address into the name field in case the
       DNS loopup fails. */
    strcpy(client_dns_name, client_ip);

    /* Get the name of the one we are talking to. */
    if((result = gethostbyaddr((char*)&client_address.sin_addr, sizeof(client_address.sin_addr), client_address.sin_family)) == (struct hostent*)NULL)
	{
	#ifdef HAVE_H_ERRNO
	debug("%s(): gethostbyaddr() failed for %s, h_errno=%d", function, client_ip, h_errno);
	#else
	debug("%s(): gethostbyaddr() failed for %s", function, client_ip);
	#endif
	return;
	}

    #ifdef DEBUG_SECURITY
    dump_hostent(result, "gethostbyaddr() for \"%s\":", client_ip);
    #endif

    /* Save the name since result points to a static buffer. */
    name = gu_strdup(result->h_name);

    /* Now take the name the resverse lookup returned
       and look it up to see if we get the same IP address. */
    if((result = gethostbyname(name)) == (struct hostent *)NULL)
	{
	#ifdef HAVE_H_ERRNO
	debug("%s(): gethostbyname(\"%s\") failed, h_errno=%d", function, name, h_errno);
	#else
	debug("%s(): gethostbyname(\"%s\") failed", function, name);
	#endif
	gu_free(name);
	return;
	}

    #ifdef DEBUG_SECURITY
    dump_hostent(result, "gethostbyname(\"%s\"):", name);
    #endif

    /*
    ** Make sure that query returned a list of addresses
    ** of the proper type and size.
    */
    if(result->h_addrtype != AF_INET)
	{
	debug("%s(): gethostbyname() didn't return an AF_INET address");
	gu_free(name);
	return;
	}
    if(result->h_length != sizeof(client_address.sin_addr))
    	{
    	debug("%s() gethostbyname() returned wrong size (returned %d, correct %d)", result->h_length, sizeof(client_address));
	gu_free(name);
	return;
	}

    /*
    ** Search for a matching IP address in the returned list.
    */
    {
    char *p;
    int x = 0;
    do	{
	if(!(p = result->h_addr_list[x]))
	    {
	    debug("%s(): none of %d IP addresses for \"%s\" match %s", name, client_ip);
	    gu_free(name);
	    return;
	    }
	x++;
	} while(memcmp(p, &client_address.sin_addr, sizeof(client_address.sin_addr)));
    DODEBUG_SECURITY(("match on address %d", x - 1));
    }

    /* Copy the name saved above into the storage space provided
       by the caller and convert it to lower case. */
    {
    char *ptr;
    clipcopy(client_dns_name, name, MAX_HOSTNAME);
    for(ptr=client_dns_name; *ptr; ptr++) *ptr = tolower(*ptr);
    }

    gu_free(name);
    } /* end of get_client_info() */

/* end of file */

