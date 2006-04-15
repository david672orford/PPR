/*
** mouse:~ppr/src/libppr/pprd_call.c
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
** Last modified 8 February 2006.
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
int pprd_call(const char command[], ...)
	{
	const char function[] = "pprd_call";
	char *temp = NULL;
	int fd = -1;
	int result;

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

		result = atoi(buffer);
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
