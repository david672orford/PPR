/*
** mouse:~ppr/pprdrv/pprdrv_progress.c
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last revised 9 May 2001.
*/

/*
** This module contains functions which write a "Progress:" line to the end of
** the queue file.  The format of this line is:
**
** Progress: xxxxxx yyyyyy zzzzzz
**
** Where xxxxxx is the number of bytes written so far, yyyyyy is the number of
** "%%Page:" comments written so far, and zzzzzz is the number of pages which
** the printer has confirmed printing.
*/

#include "before_system.h"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "global_structs.h"
#include "pprdrv.h"
#include "interface.h"

/* These are the figures which others tell us to update: */
static int total_pages_started = 0;
static int total_pages_printed = 0;
static long total_bytes = 0;

/* Don't report progress until at least this many bytes sent. */
#define SLACK_BYTES_COUNT 5120

/*
** This routine writes a line to the $VAR_SPOOL_PPR/run/state_update_pprdrv 
** file which is read by programs which present an automatically updated
** display of some aspect of PPR's state.
**
** When the file gets too full, we unlink it and start a new one with the same
** name.  We set the "other execute" bit on the unlinked file so that readers 
** who have it open will know that it is finished.
**
** Note that this is also called from pprdrv_commentary.c.
*/
void state_update_pprdrv_addline(const char line[])
    {
    const char function[] = "state_update_pprdrv_addline";
    const char filename[] = STATE_UPDATE_PPRDRV_FILE;
    static int handle = -1;
    int retry;
    struct stat statbuf;

    /*
    ** If pprdrv was invoked with the --test switch, then do nothing.
    */
    if(test_mode)
	return;

    /*
    ** We will break out of this loop as soon as we have
    ** a satisfactory progress file to write to.
    */
    for(retry=0; TRUE; retry++)
	{
	/* If not open, */
	if(handle == -1)
	    {
    	    if((handle = open(filename, O_WRONLY | O_APPEND | O_CREAT, UNIX_644)) < 0)
		fatal(EXIT_PRNERR_NORETRY, "%s(): open() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	    }

	/* Get the file size: */
	if(fstat(handle, &statbuf) < 0)
	    fatal(EXIT_PRNERR_NORETRY, "%s(): fstat() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

	/*
	** Use fstat() to examine the "other execute" to see if
	** another process has `rewound' the file.  If another process
	** has `rewound' the file, we can't use it any more.
	** If all is well, the other process will have unlinked
	** this one just after we opened it.  Of course it is possible
	** that someone left a file with the "other execute" bit set.  To
	** avoid a never ending loop we will unlink the file ourselves on
	** the second time thru.  We are reluctant to unlink it because
	** we don't want to unlink the new one which the other process
	** just created.
	*/
#ifdef UNNECESSARY_CODE
	if(statbuf.st_mode & S_IXOTH)
	    {
	    if(close(handle) < 0)
	    	fatal(EXIT_PRNERR_NORETRY, "%s(): close() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	    handle = -1;

	    if(retry)
		{
		unlink(filename);
		}

	    continue;
	    }
#endif

	/* If file is too long, we must `rewind' it, that is delete the file
	   and start another of the same name. */
	if(statbuf.st_size > STATE_UPDATE_PPRDRV_MAXBYTES)
	    {
	    if(unlink(filename) == -1 && errno != ENOENT)
	    	fatal(EXIT_PRNERR_NORETRY, "%s(): unlink() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

	    /* Set the "other execute" bit on the old file. */
	    if(fchmod(handle, UNIX_644 | S_IXOTH) < 0)
	    	fatal(EXIT_PRNERR_NORETRY, "%s(): fchmod() failed, errno=%d (%s)", function, errno, gu_strerror(errno));

	    /* We close the file and set handle to -1 so that
	       next time this function is called it will create
	       a new file. */
	    if(close(handle) < 0)
	    	fatal(EXIT_PRNERR_NORETRY, "%s(): close() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	    handle = -1;

	    /* Go back to the top and try again. */
	    continue;
	    }

	break;
	}

    /* Set the close-on-exec flag so that this file won't be available to our children. */
    gu_set_cloexec(handle);

    /* Write the line, ignoring out of space errors since
       there is not much we can do about them anyway.
       */
    if(write(handle, line, strlen(line)) < 0 && errno != ENOSPC)
	fatal(EXIT_PRNERR_NORETRY, "%s(): write() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
    } /* end of state_update_pprdrv_addline() */

/*
** This routine writes the current values to the end of the queue file.
** The first time it is called it opens the file, seeks to the end,
** records where the end is, and writes the line.  On subsequent calls
** it returns to the recorded file position and overwrites the line
** with a new one.  The file handle is never closed.
**
** Note that if the number of bytes transmitted is less than
** SLACK_BYTES_COUNT, then a "Progress:" lines of all zeros is
** written because so few bytes have probably only filled the
** buffers and may not have reached the printer at all.
*/
static void progress_write(void)
    {
    const char function[] = "progress_write";
    static char qfname[MAX_PPR_PATH];	/* The name of the queue file */
    static int handle = -1;		/* The handle of the open queue file */
    static off_t spot;			/* The offset at which we will write the line */

    char buffer[80];
    char *line;
    int len = 31;		/* Length of progress line. */
    int retval;

    /*
    ** If we are running in test mode, we have no
    ** business writing to the queue file, so we
    ** will write to stderr.
    */
    if(test_mode)
	{
	fprintf(stderr, "Progress: %010ld %04d %04d\n", total_bytes, total_pages_started, total_pages_printed);
	return;
	}

    if(handle == -1)		/* If file not open yet, */
    	{
	/* Open the file for read and write. */
    	ppr_fnamef(qfname, "%s/%s", QUEUEDIR, QueueFile);
    	if((handle = open(qfname, O_RDWR)) == -1)
    	    fatal(EXIT_PRNERR_NORETRY, "%s: progress_write(): open(\"%s\",O_WRONLY) failed, errno=%d (%s)", __FILE__, qfname, errno, gu_strerror(errno) );
	gu_set_cloexec(handle);

	/* Seek to a spot 31 bytes from the end. */
	if((spot = lseek(handle, (off_t)(0-len), SEEK_END)) < 0)
	    fatal(EXIT_PRNERR_NORETRY, "%s: progress_write(): lseek(%d,-len,SEEK_END) failed, errno=%d (%s)", __FILE__, handle, errno, gu_strerror(errno) );

	/* Read what will be "Progress: " if this was done before. */
	if((retval = read(handle, buffer, 10)) != 10)
	    fatal(EXIT_PRNERR_NORETRY, "%s: progress_write(): read() returned %d, errno=%d (%s)", __FILE__, retval, errno, gu_strerror(errno));

	/* If it isn't, seek to the end. */
	if(memcmp(buffer, "Progress: ", 10) != 0)
	    {
	    if((spot = lseek(handle, (off_t)0, SEEK_END)) < 0)
		fatal(EXIT_PRNERR_NORETRY, "%s: progress_write(): lseek(%d,0,SEEK_END) failed, errno=%d (%s)", __FILE__, handle, errno, gu_strerror(errno));
	    }
    	}

    /* Return to the proper position for the "Progress:" line. */
    if(lseek(handle, spot, SEEK_SET) < 0)
	fatal(EXIT_PRNERR_NORETRY, "%s: progress_write(): lseek(%d,%ld,SEEK_SET) failed, errno=%d (%s)", __FILE__, handle, (long)spot, errno, gu_strerror(errno));

    /* If enough bytes have been sent that we are pretty sure that some
       of them have actually reached the printer, write the "Progress:"
       line which is in the buffer.  Otherwise, just use one with
       all zeros. */
    if(total_bytes > SLACK_BYTES_COUNT)
	{
	snprintf(buffer, sizeof(buffer), "Progress: %010ld %04d %04d\n", total_bytes, total_pages_started, total_pages_printed);
	line = buffer;
	}
    else
	{
    	line = "Progress: 0000000000 0000 0000\n";
	}

    /*
    ** Write the new "Progress:" line.
    ** I can't remember why this might be
    ** interupted by a system call and not restarted.
    */
    while((retval = write(handle, line, len)) != len)
	{
	if(retval == -1 && errno == EINTR)
	    continue;

	if(retval == -1)
	    fatal(EXIT_PRNERR_NORETRY, "%s(): write() failed, errno=%d (%s)", function, errno, gu_strerror(errno) );
	else
	    fatal(EXIT_PRNERR, "%s(): write() returned %d when %d was expected", function, retval, len);
    	}

    } /* end of progress_write() */

/*
** This routine is called from the progress hook routines to possibly
** append a progress line to the "state_update_pprdrv" file.
*/
static void progress_write2(int linetype, const char text[])
    {
    char buffer[80];

    if(total_bytes <= SLACK_BYTES_COUNT)
	return;

    /*
    ** Create, in the buffer, a progress line of the
    ** requested type.
    */
    switch(linetype)
	{
	case 0:
	    snprintf(buffer, sizeof(buffer), "PGSTA %s %d\n", printer.Name, total_pages_started);
	    break;
	case 1:
	    snprintf(buffer, sizeof(buffer), "PGFIN %s %d\n", printer.Name, total_pages_printed);
	    break;
	case 2:
	    snprintf(buffer, sizeof(buffer), "BYTES %s %ld %ld\n", printer.Name, total_bytes, job.attr.postscript_bytes);
	    break;
	}

    state_update_pprdrv_addline(buffer);
    }

/*
** This routine is called just after each "%%Page:" comment is emitted.  Thus,
** it reflects the number of page descriptions and not necessarily the
** number of physical pages.
*/
void progress_page_start_comment_sent(void)
    {
    total_pages_started++;
    progress_write();
    progress_write2(0, (char*)NULL);
    }

/*
** This is called each time we get an HP PJL message from the printer
** indicating that a page has hit the output tray.  The argument
** indicates the number of page descriptions on the page.
*/
void progress_pages_truly_printed(int n)
    {
    total_pages_printed += n;
    progress_write();
    progress_write2(1, (char*)NULL);
    }

/*
** This is called as each block in flushed into the
** pipe leading to the interface.
*/
void progress_bytes_sent(int n)
    {
    total_bytes += n;
    progress_write();
    progress_write2(2, (char*)NULL);
    }

/*
** This is called every time the printer's status file is updated.
** The text[] is a status message from the printer.  The status
** file is used by the "ppop status" command to explain the specific
** reason for the printer status.
*/
void progress_new_status(const char text[])
    {
    char buffer[80];
    snprintf(buffer, sizeof(buffer), "STATUS %s %s\n", printer.Name, text);
    state_update_pprdrv_addline(buffer);
    }

/*
** This is used to subtract bytes we are using to constantly
** update the printers LCD or LED display.  This routine
** doesn't cause anything to be written to the queue file
** or the status update file.  It just changes the next
** value written.
**
** (It should be noted that the LCD/LED update feature described above
** was a failure.  It didn't work because the HP printer didn't actually
** display the changes until the job was done, at which time it displayed
** all of the changes very quicly.)
*/
void progress_bytes_sent_correction(int n)
    {
    total_bytes -= n;
    }

/*
** This is used to fetch the total of bytes set so that it can be logged.
*/
long int progress_bytes_sent_get(void)
    {
    return total_bytes;
    }

/* end of file */

