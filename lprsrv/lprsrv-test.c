/*
** mouse:~ppr/src/lprsrv/lprsrv-test.c
** Copyright 1995--2003, Trinity College Computing Center.
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
** Last modified 19 February 2003.
*/

#include "before_system.h"
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pwd.h>
#ifdef INTERNATIONAL
#include <locale.h>
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "lprsrv.h"
#include "util_exits.h"

void fatal(int exitcode, const char message[], ... )
    {
    va_list va;

    va_start(va, message);
    fputs(_("FATAL: "), stderr);
    vfprintf(stderr, message, va);
    fputs("\n", stderr);
    va_end(va);

    exit(exitcode);
    } /* end of fatal() */

void warning(const char message[], ... )
    {
    va_list va;

    va_start(va, message);
    fputs(_("WARNING: "), stderr);
    vfprintf(stderr, message, va);
    fputs("\n", stderr);
    va_end(va);
    } /* end of warning() */

void debug(const char message[], ... )
    {
    va_list va;

    va_start(va, message);
    fputs("DEBUG: ", stderr);
    vfprintf(stderr, message, va);
    fputs("\n", stderr);
    va_end(va);
    } /* end of debug() */

/*
** Try to determine the fully qualified host name of
** a specified computer.
**
** The returned string is in malloc() allocated memory.
*/
static const char *fully_qualify_hostname(const char *hostname)
    {
    struct hostent *hostinfo;
    struct in_addr my_addr;

    if( (hostinfo=gethostbyname(hostname)) == (struct hostent *)NULL )
	{
	fprintf(stderr, _("Network node \"%s\" does not exist.\n"), hostname);
	return (const char *)NULL;
	}

    if(hostinfo->h_addrtype != AF_INET)
	{
	fprintf(stderr, "Address of \"%s\" is not an internet address.\n", hostname);
	return (const char *)NULL;
	}

    memcpy(&my_addr, hostinfo->h_addr_list[0], sizeof(my_addr));

    if( (hostinfo=gethostbyaddr((char*)&my_addr, sizeof(my_addr), AF_INET)) == (struct hostent *)NULL )
	{
	fprintf(stderr, _("Reverse lookup failed for \"%s\" address \"%s\".\n"), hostname, inet_ntoa(my_addr));
	return (const char *)NULL;
	}

    return gu_strdup(hostinfo->h_name);
    } /* end of fully_qualify_hostname() */

int main(int argc, char *argv[])
    {
    const char *hostname;
    struct ACCESS_INFO access_info;

    /* Initialize internation messages library. */
    #ifdef INTERNATIONAL
    setlocale(LC_MESSAGES, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    #endif

    if(argc < 2)
    	{
	fputs(_("Usage:\n\tlprsrv-test hostname [user [user]]\n\n"), stdout);
	fputs(_("This program is used to test an lprsrv.conf file.\n"), stdout);
	return EXIT_OK;
	}

    /* Look up the hostname using DNS.  If this fails
       it will print a message so we need only exit. */
    if(! (hostname = fully_qualify_hostname(argv[1])))
	return EXIT_NOTFOUND;

    printf(_(";\n"
    	"; All lprsrv.conf sections which apply to \"%s\"\n"
    	"; taken together have the same effect as the following\n"
    	"; simplified section.\n"
    	";\n"), hostname);

    printf("[%s]\n", hostname);

    get_access_settings(&access_info, hostname);

    printf("allow = %s\n", access_info.allow ? "yes" : "no");
    printf("insecure ports = %s\n", access_info.insecure_ports ? "yes" : "no");
    printf("ppr become user = %s\n", access_info.ppr_become_user ? "yes" : "no");
    printf("other become user = %s\n", access_info.other_become_user ? "yes" : "no");
    printf("ppr root as = %s\n", access_info.ppr_root_as);
    printf("other root as = %s\n", access_info.other_root_as);
    printf("ppr proxy user = %s\n", access_info.ppr_proxy_user);
    printf("other proxy user = %s\n", access_info.other_proxy_user);
    printf("ppr proxy class = %s\n", access_info.ppr_proxy_class);
    printf("ppr user format = %s\n", access_info.ppr_from_format);
    fputs("\n", stdout);

    if(!access_info.allow)
    	{
	printf(_("No access allowed from node \"%s\".\n"), hostname);
    	}

    #define FORMAT "%-10.10s %-14.14s %-14.14s %-30.30s\n"
    else if(argc > 2)
	{
	int x;
	struct passwd *pw;
	uid_t uid;
	gid_t gid;
	const char *proxy_class;
	const char *username;
	char proxy_for[82];

	printf(_("Based on the lprsrv.conf section shown above, the following table shows the local\n"
		"user IDs and proxy for strings that will be used when accepting remote jobs from\n"
		"the node \"%s\" for the remote user or users which you have\n"
		"specified.\n"), hostname);
	printf("\n");

	printf(FORMAT, _("Spooler"), _("Remote User"), _("Local User"), _("As Proxy For"));
	printf("----------------------------------------------------------------\n");

	for(x = 2; x < argc; x++)
	    {
	    proxy_class = (const char *)NULL;
	    get_proxy_identity(&uid, &gid, &proxy_class, hostname, argv[x], TRUE, &access_info);
	    pw = getpwuid(uid);
	    if(pw) username = pw->pw_name; else username = "???";
	    if(proxy_class) snprintf(proxy_for, sizeof(proxy_for), "%.20s@%.60s", strcmp(argv[x], "root") == 0 ? "*" : argv[x], proxy_class);
	    printf(FORMAT, x == 2 ? "PPR" : "", argv[x], username, proxy_class ? proxy_for : "");
	    }

	for(x = 2; x < argc; x++)
	    {
	    get_proxy_identity(&uid, &gid, &proxy_class, hostname, argv[x], FALSE, &access_info);
	    pw = getpwuid(uid);
	    if(pw) username = pw->pw_name; else username = "???";
	    /* Leave "Other" untranslated because the word should
	       match the "other" in the lprsrv.conf directives. */
	    printf(FORMAT, x == 2 ? "Other" : "", argv[x], username, "");
	    }
	}

    return EXIT_OK;
    }

/* end of file */
