/*
** mouse:~ppr/src/libuprint/lpr_sendfile.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell and Damian Ivereigh.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** Last modified 18 November 2000.
*/

#include "before_system.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <pwd.h>
#include "gu.h"
#include "global_defines.h"

#include "interface.h"
#include "uprint.h"
#include "uprint_private.h"

/*
** Return the nodename we should use to identify ourself in
** lpr queue files.
*/
const char *uprint_lpr_nodename()
    {
    const char function[] = "uprint_lpr_nodename";
    static struct utsname sysinfo;

    /* Get the name of this system */
    if(uname(&sysinfo) == -1)
    	{
    	uprint_error_callback("%s(): uname() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	return (char*)NULL;
    	}

    return sysinfo.nodename;
    } /* end of uprint_lpr_nodename() */

/*
** Return the next id number.  The string "file" is the full path and
** name of the file to use.  The file is locked, the last id is read,
** and incremented making the new id, and the new id is written back
** to the file, the file is closed, and the new id is returned.
*/
int uprint_lpr_nextid(void)
    {
    const char *myname = "uprint_lpr_nextid";
    int fd;		/* file descriptor of id file */
    int tid;		/* for holding id */
    const char *file = LPR_PREVID_FILE;
    char buf[11];
    int len;

    /* If we can open the ID file, */
    if((fd = open(file, O_RDWR)) != -1)
	{
	if(gu_lock_exclusive(fd, TRUE))
	    {
	    uprint_error_callback("%s(): can't lock \"%s\"", myname, file);
	    return -1;
	    }

	if((len = read(fd, buf, 10)) == -1)
	    {
	    uprint_error_callback("%s(): read() failed, errno=%d (%s)", myname, errno, gu_strerror(errno));
	    return -1;
	    }

	buf[len] = '\0';		/* make buffer ASCIIZ */
	tid = atoi(buf);		/* convert ASCII to binary */

	tid++;				/* add one to it */
	if(tid < 1 || tid > 999)	/* if id unreasonable or too large */
	    tid = 1;			/* force it to one */

	lseek(fd, (off_t)0, SEEK_SET);  /* move to start, for write */
	}
    else                            	/* does not exist */
	{
	tid = 1;                      	/* start id's at 0 */

	/* Create a new file: */
	if((fd = open(file, O_WRONLY | O_CREAT, UNIX_644)) == -1)
	    {
	    uprint_errno = UPE_INTERNAL;
	    uprint_error_callback("%s(): can't create \"%s\", errno=%d (%s)", myname, file, errno, gu_strerror(errno));
	    return -1;
	    }
	}

    snprintf(buf, sizeof(buf), "%d\n", tid);	/* write the new id */
    write(fd, buf, strlen(buf));
    close(fd);					/* and close the id file */

    return tid;
    } /* end of uprint_lpr_nextid() */

/*
** Copy stdin to the named file, return the open file
** and set the length.
*/
int uprint_file_stdin(int *length)
    {
    const char function[] = "uprint_file_stdin";
    char *copybuf;			/* buffer for copy operation */
    int copyfd;				/* file descriptor of temp file */
    int rbytes, wbytes;			/* bytes read and written */

    /* Set length initially to zero. */
    *length = 0;

    {
    char fname[MAX_PPR_PATH];
    ppr_fnamef(fname, "%s/uprint-%ld-XXXXXX", TEMPDIR, (long)getpid());

    if((copyfd = mkstemp(fname)) == -1)
    	{
    	uprint_error_callback("%s(): mkstemp() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
    	return -1;
    	}

    if(unlink(fname) < 0)
    	{
	uprint_error_callback("%s(): unlink(\"%s\") failed, errno=%d (%s)", function, fname, errno, gu_strerror(errno));
	return -1;
    	}
    }

    /* Copy stdin to a temporary file */
    if((copybuf = (char*)malloc((size_t)16384)) == (void*)NULL)
	{
	uprint_error_callback("%s(): malloc() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	close(copyfd);
	return -1;
	}

    /* copy until end of file */
    while( (rbytes=read(0, copybuf, 4096)) )
    	{
    	if(rbytes==-1)
	    {
	    uprint_error_callback("%s(): read() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	    return -1;
	    }

    	if( (wbytes=write(copyfd, copybuf, rbytes)) != rbytes )
    	    {
    	    if(wbytes == -1)
    	    	uprint_error_callback("%s(): write() failed on tempfile, errno=%d (%s)", function, errno, gu_strerror(errno));
    	    else
    	    	uprint_error_callback("%s(): disk full while writing tempfile", function);

	    return -1;
    	    }

	*length += rbytes;		/* add to total */
	}

    free(copybuf);			/* free the buffer */

    lseek(copyfd, (off_t)0, SEEK_SET);	/* return to start of file */

    return copyfd;
    } /* end of uprint_file_stdin() */

/*
** Copy the data to the printer.
*/
int uprint_lpr_send_data_file(int source, int sockfd)
    {
    const char function[] = "uprint_send_data_file";
    int just_read, bytes_left, just_written;
    char *ptr;
    char buffer[4096];		/* Buffer for data transfer */

    do	{
	if((just_read=read(source, buffer, sizeof(buffer))) == -1)
	    {
	    uprint_error_callback("%s(): error reading temp file, errno=%d (%s)", function, errno, gu_strerror(errno));
	    return -1;
	    }
	bytes_left = just_read;
	ptr = buffer;
	while(bytes_left)
	    {
	    if( (just_written=write(sockfd, ptr, bytes_left)) == -1 )
		{
	 	uprint_error_callback("%s(): error writing to socket, errno=%d (%s)", function, errno, gu_strerror(errno));
		return -1;
		}
	    ptr += just_written;
	    bytes_left -= just_written;
	    }
	} while(just_read);

    return 0;
    } /* end of uprint_lpr_send_data_file() */

/* end of line */

