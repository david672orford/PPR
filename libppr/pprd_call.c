/*
** mouse:~ppr/src/libppr/pprd_call.c
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 27 April 2006.
*/

/*! \file
	\brief make an RPC call to a routine in pprd

*/

#include "config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#ifdef INTERNATIONAL
#include <libintl.h>
#endif
#include "gu.h"
#include "global_defines.h"

/** make an RPC call to a routine in pprd
 *
 * This routine connects to the Unix-domain socket for pprd, sends the command,
 * and waits for the return code which is returns.
*/
struct PPRD_CALL_RETVAL pprd_call(const char command[], ...)
	{
	const char function[] = "pprd_call";
	char *temp = NULL;
	int fd = -1;
	struct PPRD_CALL_RETVAL result = { 0, 0 };

	gu_Try
		{
		struct sockaddr_un server;
		va_list va;
		int writelen, writtenlen;
		char buffer[16];
		int readlen;

		if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
			gu_Throw(_("%s(): %s() failed, errno=%d (%s)"), function, "socket", errno, strerror(errno));

		server.sun_family = AF_UNIX;
		gu_strlcpy(server.sun_path, UNIX_SOCKET_NAME, sizeof(server.sun_path));
		if(connect(fd, &server, sizeof(server)) == -1)
			gu_Throw(_("%s(): %s() failed, errno=%d (%s)"), function, "connect", errno, strerror(errno));

		va_start(va, command);
		writelen = gu_vasprintf(&temp, command, va);
		va_end(va);

		if((writtenlen = write(fd, temp, writelen)) == -1)
			gu_Throw(_("%s(): %s() failed, errno=%d (%s)"), function, "write", errno, strerror(errno));
		if(writtenlen != writelen)
			gu_Throw(_("%s(): write() wrote %d of %d bytes"), function, writtenlen, writelen);

		if((readlen = read(fd, buffer, sizeof(buffer))) == -1)
			gu_Throw(_("%s(): %s() failed, errno=%d (%s)"), function, "read", errno, strerror(errno));
		if(readlen < 1 || buffer[readlen-1] != '\n')
			gu_Throw(_("%s(): invalid response from pprd"), function);
		buffer[readlen-1] = '\0';

		gu_sscanf(buffer, "%d %d", &result.status_code, &result.extra_code);
		}
	gu_Final
		{
		gu_free_if(temp);
		if(fd != -1)
			close(fd);
		}
	gu_Catch
		{
		gu_ReThrow();
		}

	return result;
	}

/* end of file */
