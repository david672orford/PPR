/*
** mouse:~ppr/src/papsrv/papsrv_authorize.c
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
** Last modified 29 December 2000.
*/

/*
** This module contains support for CAP AUFS authorization.  There is also
** skeletal support for the authentication described in Inside AppleTalk,
** Second Edition; which authentication is not yet implemented in MacOS.
*/

#include "before_system.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include "gu.h"
#include "global_defines.h"

#include "userdb.h"
#include "papsrv.h"

/*
** This routine is called by the child main loop just before printjob()
** is called.  This routine will not be called if a sucessful
** "%%Login:" command set preauthorized to TRUE.
**
** This routine does nothing if the printer is not protected.  If the printer
** is protected or the option "ForceAUFSSecurity" is TRUE, this routine 
** attempts to determine if the user is who he says he is.
**
** If a user is preauthorized it means that papsrv vouches for the user's 
** authenticity and ppr(1) does not need to try to determine if the user
** is authentic.  This is indicated by invoking ppr with the -A switch.  This
** routine will use *preauthorized to set a flag to true if the -A switch
** should be used.
**
** If the user isn't in the user database, or can't be authorized
** for some other reason, this routine emulates a PostScript error.
*/
void preauthorize(int sesfd, int prnid, int net, int node, char **username, int *preauthorized)
    {
    const char *dsc_username = (char*)NULL;	/* Name read from "For:" line */

    DODEBUG_AUTHORIZE(("preauthorize(sesfd=%d, prnid=%d, net=%d, node=%d, *preauthorized)", sesfd, prnid, net, node));

    /*
    ** If the destination is not protected and ForceAUFSSecurity
    ** is false, this routine has no work to do.
    */
    if( ! adv[prnid].isprotected && ! adv[prnid].ForceAUFSSecurity )
	{
	DODEBUG_AUTHORIZE(("preauthorize(): nothing to do"));
	return;
	}

    /*
    ** This code looks for a "%%For:" line in the first buffer of the
    ** print job.  If one is found, it checks to see if that name is
    ** among those authorized to use the printer.  This is a pretty
    ** lame form of authorization.  If AUFS security is being used,
    ** then this information will be put to better use.
    */
    onebuffer = TRUE;			/* don't go beyond one buffer */

    while(pap_getline(sesfd))		/* read until end of 1st buffer */
	{
	if(strncmp(line, "%%For:", 6) == 0)	/* if this is the line we are looking for */
	    {
	    dsc_username = line + 6;
	    dsc_username += strspn(dsc_username, " \t");

	    if(dsc_username[0] == '(')			/* if is a PostScript string */
		{					/* Then, */
		tokenize();				/* use tokenize() to extract it */
		dsc_username = tokens[1];
		}

	    if(strlen(dsc_username) == 0)
	    	dsc_username = "[blank Macintosh owner]";

	    DODEBUG_AUTHORIZE(("DSC user name: \"%s\"", dsc_username));

	    break;
	    } /* end of if this is the "%%For:" line */
	} /* end of line reading loop */

    reset_buffer(FALSE);		/* soft buffer reset */

    /*
    ** If no user name was found, do nothing.  We will let ppr
    ** worry about it when we fail to invoke it with the -A switch.
    */
    if(dsc_username == (char*)NULL)
	{
	DODEBUG_AUTHORIZE(("No \"%%%%For:\" found"));
	return;
	}

    /*
    ** If this is a protected destination or the configuration file
    ** option "ForceAUFSSecurity:" is TRUE, check the AUFS security file
    ** for the client network and node to make sure the user has an AUFS
    ** session with a user name which corresponds with the "For:" line.
    */
    if((adv[prnid].isprotected || adv[prnid].ForceAUFSSecurity || adv[prnid].AUFSSecurityName != AUFSSECURITYNAME_DSC) && aufs_security_dir != (char*)NULL)
	{
	char aufs_security_file[MAX_PPR_PATH];
	FILE *f;
	char aufs_username[10];
	struct passwd *pw;

	/* Construct the name of the AUFS security file. */
	ppr_fnamef(aufs_security_file, "%s/net%d.%dnode%d", aufs_security_dir, net / 256, net % 256, node);

	/*
	** Try to open the AUFS security file.  If it can't be found,
	** say the user doesn't have a volume mounted.
	*/
	if((f = fopen(aufs_security_file, "r")) == (FILE*)NULL)
	    {
	    DODEBUG_AUTHORIZE(("AUFS security file \"%s\" not found", aufs_security_file));
	    if(!adv[prnid].ForceAUFSSecurity && !adv[prnid].isprotected)
	    	return;
	    reply(sesfd,MSG_NOVOL);	/* ``bad password'' */
	    postscript_stdin_flushfile(sesfd);
	    exit(5);
	    }

	/*
	** Try to read a line from the AUFS security file.  If we can't
	** we will assume the file is empty which indicates that the user
	** has no volumes mounted.
	*/
	if(fgets(aufs_username, sizeof(aufs_username), f) == (char*)NULL)
	    {
	    DODEBUG_AUTHORIZE(("AUFS security file \"%s\" is empty", aufs_security_file));
	    if(!adv[prnid].ForceAUFSSecurity && !adv[prnid].isprotected)
	    	return;
	    reply(sesfd,MSG_NOVOL);
	    postscript_stdin_flushfile(sesfd);
	    exit(5);
	    }

	fclose(f);			/* we got what we went after */

	/* Remove any trailing line-feed */
	aufs_username[strcspn(aufs_username,"\n")] = '\0';

	DODEBUG_AUTHORIZE(("aufs_username[] = \"%s\"", aufs_username));

	/*
	** We will probably need the password entry.
	*/
	if( (pw=getpwnam(aufs_username)) == (struct passwd *)NULL )
	    {					/* if user don't exist, */
	    DODEBUG_AUTHORIZE(("AUFS security file user \"%s\" doesn't exist in /etc/passwd",aufs_username));
	    if(!adv[prnid].ForceAUFSSecurity && !adv[prnid].isprotected)
	    	return;
	    reply(sesfd, MSG_NOCHARGEACCT);	/* you aren't authorized */
	    postscript_stdin_flushfile(sesfd);
	    exit(5);
	    }

	/*
	** If they don't match immediately, try using the password database
	** to convert the aufs_username to the real name.
	*/
	if(strcmp(dsc_username, aufs_username) != 0)
	    {
	    if(strcmp(dsc_username, pw->pw_gecos) != 0)
		{				/* if no match, */
		if(!adv[prnid].ForceAUFSSecurity && !adv[prnid].isprotected)
		    return;
		reply(sesfd, MSG_NOVOL);	/* say volume not mounted  */
		postscript_stdin_flushfile(sesfd);
		exit(5);
		}
	    }

	/*
	** If we are supposed to use something other than
	** the "%%For:" name, do it now.
	*/
	switch(adv[prnid].AUFSSecurityName)
	    {
	    case AUFSSECURITYNAME_USERNAME:
		*username = gu_strdup(aufs_username);
		break;
	    case AUFSSECURITYNAME_REALNAME:
		*username = gu_strdup(pw->pw_gecos);
		break;
	    default:
	    	break;
	    }

	} /* end of if AUFS Security should be used */

    /*
    ** If this is a ``protected destination'', that is, one for which
    ** there is a per page charge (even if that charge is 0 cents per
    ** page), then make sure the user is in the PPR authorization
    ** database and is not too much overdrawn.
    **
    ** Most of this is handled by the db_auth() function in libpprdb.a.
    */
    if(adv[prnid].isprotected)
	{
	struct userdb user; 			/* for reading user database */
	const char *charge_user = *username ? *username : dsc_username;

	switch(db_auth(&user, charge_user))	/* look it up */
	    {
	    case USER_OK:			/* user is authorized */
		*preauthorized = TRUE;		/* set flag */
		return;				/* return w/out err */
	    case USER_ISNT:			/* user does not exist */
		reply(sesfd, MSG_NOCHARGEACCT);	/* in PPR database */
		break;
	    case USER_OVERDRAWN:		/* User isn't paid up */
		reply(sesfd, MSG_OVERDRAWN);
		break;
	    default:
		reply(sesfd, MSG_DBERR);
		break;
	    }
	postscript_stdin_flushfile(sesfd);	/* emulate "stdin flushfile" */
	exit(5);
	}

    } /* end of preauthorize() */

/*
** Handle a "%%Login:" command.  When this routine is called, the login
** request is in line[].  If the login request is acceptable, this routine
** should set the character pointer pointed to by username to point to a
** string containing the username, and set the integer pointed to by
** preauthorized to TRUE.  If the login fails, this routine will sleep()
** for 2 seconds and then terminate the server.  (See Inside AppleTalk,
** second edition, page 14-14.)
*/
void login_request(int sesfd, int destid, char **username, int *preauthorized)
    {
    DODEBUG_AUTHORIZE(("login_request(), line=\"%s\"", line));

    /* Break the command into words. */
    tokenize();

    /*
    ** Login method NoUserAuthent is basically meaningless,
    ** it doesn't even tell us the user name.
    */
    if(strcmp(tokens[1], "NoUserAuthent") == 0)
	{
	REPLY(sesfd, "LoginOK\n");
    	return;
    	}

    /*
    ** All remaining methods must either result in sucess or exit(),
    ** so we cane safly set this now.
    */
    *preauthorized = TRUE;

    /*
    ** Just a simple password.
    */
    if(strcmp(tokens[1], "CleartxtPasswrd") == 0)
	{

	}

    /*
    ** Netatalk Kerberos.
    */
    else if(strcmp(tokens[1], "UMICHKerberosIV") == 0)
    	{

    	}

    /*
    ** If we drop to here, the login request failed.
    */
    REPLY(sesfd,"InvalidUser\n");
    sleep(2);
    close_reply(sesfd);
    exit(5);
    } /* end of login_request() */

/* end of file */
