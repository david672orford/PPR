/*
** mouse:~ppr/src/z_install_end/id.c
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
** Last modified 5 March 2003.
*/

/*
** We have this partial implementation of id because some implementations
** don't have the features which we need for these scripts.
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

extern char *optarg;
extern int optind;

int main(int argc, char *argv[])
    {
    int opt_n=0, opt_u=0, opt_g=0, opt_r=0;
    int c;
    uid_t uid;
    gid_t gid;
    while((c = getopt(argc, argv, "nugGr")) != -1)
	{
	switch(c)
	    {
	    case 'u':
		opt_u = 1;
		break;
	    case 'g':
		opt_g = 1;
		break;
	    case 'n':
	    	opt_n = 1;
	    	break;
	    case 'G':
	    	fprintf(stderr, "%s: not implemented\n", argv[0]);
	    	return 1;
	    case 'r':
	    	opt_r = 1;
	    	break;
	    default:
	    	return 1;
	    }
	}

    if(opt_u && opt_g)
	{
	fprintf(stderr, "%s: can't print both\n", argv[0]);
	return 1;
	}

    if(opt_r)
	{
	uid = getuid();
	gid = getgid();
	}
    else
	{
	uid = geteuid();
	gid = getegid();
	}

    if(opt_n)
	{
	if(opt_u)
	    {
	    struct passwd *pw;
	    if((pw = getpwuid(uid)))
	    	{
		printf("%s\n", pw->pw_name);
		return 0;
	    	}
	    }
	else if(opt_g)
	    {
	    struct group *gr;
	    if((gr = getgrgid(gid)))
	    	{
	    	printf("%s\n", gr->gr_name);
	    	return 0;
	    	}
	    }
	}

    if(opt_u)
	{
	printf("%ld\n", (long)uid);
	return 0;
	}
    if(opt_g)
	{
	printf("%ld\n", (long)gid);
	return 0;
	}

    {
    struct passwd *pw = getpwuid(uid);
    struct group  *gr = getgrgid(gid);
    printf("uid=%ld(%s) gid=%ld(%s)\n", (long)uid, pw?pw->pw_name:"?", (long)gid, gr?gr->gr_name:"?");
    return 0;
    }
    }



/* end of file */
