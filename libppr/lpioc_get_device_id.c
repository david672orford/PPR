/*
** mouse:~ppr/src/libppr/lpioc_get_device_id.c
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
** Last modified 3 April 2006.
*/

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#ifdef PPR_LINUX
#include <sys/ioctl.h>
#include <linux/lp.h>
#endif
#include "gu.h"
#include "global_defines.h"
#include "libppr_int.h"

/** get IEEE 1284 printer device ID
 *
 * This code reads the ID string from a parallel printer or a USB printer.
 * I (David Chappell) couldn't find any documentation on how to do this.
 * I figured it out by reading the source code for the CUPS USB backend.
 * I have since learned that the definitions of the necessary macros
 * can be found in Randy Dunlap's patch which added support for the necessary
 * IOCTL to the Linux USB driver.
 */
int lpioc_get_device_id(const char port[], unsigned char *device_id, int device_id_max)
	{
	#ifdef PPR_LINUX
	int fd;
	int device_id_len;

	/* The macros as they appear in CUPS and Mr. Dunlap's patch. */
	#define IOCNR_GET_DEVICE_ID	1
	#define LPIOC_GET_DEVICE_ID(len) _IOC(_IOC_READ, 'P', IOCNR_GET_DEVICE_ID, len)

	if((fd = open(port, O_RDWR | O_EXCL)) == -1)
		return -1;

	if(ioctl(fd, LPIOC_GET_DEVICE_ID(device_id_max), device_id) == -1)
		{
		int saved_errno = errno;
		close(fd);
		errno = saved_errno;
		return -1;
		}

	close(fd);

	/* First try big endian (per IEEE 1284 standard). */
	device_id_len = ((device_id[0] << 8) + device_id[1]);

	/* If the result is unreasonable, try little endian (This hack is from CUPS.) */
	if(device_id_len > (device_id_max - 2))
		device_id_len = ((device_id[1] << 8) + device_id[0]);

	/* Interestingly, the length includes the two bytes which store the length itself! */
	if(device_id_len < 2)
		return -1;

	/* Limit to buffer size. */
	if(device_id_len > device_id_max)
		device_id_len = device_id_max;

	/* Move the string left to cover the length.  There is guaranteed to be
	 * space for the '\0'.
	 */
	memmove(device_id, device_id+2, device_id_len-2);
	device_id[device_id_len-2] = '\0';

	return 0;
	#else
	return -1;
	#endif
	}

/* end of file */
