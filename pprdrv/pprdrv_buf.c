/*
** mouse:~ppr/src/pprdrv/pprdrv_buf.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 7 February 2001.
*/

/*
** These routines buffer output going out over
** the pipe to the interface.
*/

#include "before_system.h"
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"
#include "interface.h"

/* We should be the only module outside of pprdrv.c that needs this: */
extern int intstdin;

/* The write buffer: */
#define BUFSIZE 8192			/* size of write buffer, was 4096 */
static char wbuf[BUFSIZE];		/* write buffer */
static char *bptr;			/* buffer pointer */
static int wbuf_space;			/* buffer space left */

/* The number of CTRL-D characters sent minus the number received: */
int control_d_count;

/* These values are used by the macros
   printer_putc(), printer_pugs(), and printer_write().
   */
void (*ptr_printer_putc)(int c);
void (*ptr_printer_puts)(const char *str);
void (*ptr_printer_write)(const char *buf, size_t size);

/*
** Initialize the buffer structures.
*/
void printer_bufinit(void)
    {
    bptr = wbuf;
    wbuf_space = BUFSIZE;

    control_d_count = 0;

    printer_TBCP_off();
    } /* end of printer_bufinit() */

/*
** Write all of the buffered data to the pipe now.
**
** We can of course call this in order to flush out query code,
** but it is more often called by the other routines when the
** buffer becomes full.
*/
int printer_flush(void)
    {
    const char *function = "printer_flush";
    int total, remain;
    char *wptr;		/* pointer to current position in buffer */
    int rval;
    fd_set wfds, rfds;	/* file descriptor sets for select() */
    struct timeval sleep_time;
    int readyfds;
    int setsize;	/* first parameter for select() */

    DODEBUG_INTERFACE_GRITTY(("%s()", function));

    /* Do control-D counting stuff? */
    if(printer.Jobbreak == JOBBREAK_CONTROL_D && printer.Feedback)
    	{
	char *p;

	remain = BUFSIZE - wbuf_space;
	wptr = wbuf;

	while(remain && (p = (char*)memchr(wptr, 0x04, remain)))
	    {
	    control_d_count++;
	    remain -= (p - wptr + 1);
	    wptr = p + 1;
	    }
	}

    total = remain = (BUFSIZE - wbuf_space);	/* compute bytes now in buffer */
    wptr = wbuf;				/* start of buffer */

    /* Compute the size of the select() file
       descriptor set. */
    setsize = intstdin > intstdout ? intstdin : intstdout;
    setsize++;

    /* We will continue to call write() until our buffer is empty. */
    while(remain > 0)
	{
	writemon_start("WRITE");
	while(writemon_sleep_time(&sleep_time, 0))
	    {
            FD_ZERO(&rfds);
            FD_ZERO(&wfds);
            FD_SET(intstdout, &rfds);
            FD_SET(intstdin, &wfds);

            if((readyfds = select(setsize, &rfds, &wfds, NULL, &sleep_time)) < 0)
                {
                if(errno == EINTR)
                    {
		    interface_fault_check();
                    continue;
                    }
                fatal(EXIT_PRNERR, "%s(): select() failed, errno=%d (%s)", function, errno, gu_strerror(errno));
                }

	    if(readyfds > 0)
	    	{
		/* If there is data to read from the printer, */
		if(FD_ISSET(intstdout, &rfds))
		    {
		    interface_fault_check();
		    feedback_reader();
		    }

                /* If space to write to pipe, drop out to write() code. */
                else if(FD_ISSET(intstdin, &wfds))
                    {
                    break;
                    }

		/* If it is anything else, something is very wrong. */
                else
                    {
                    fatal(EXIT_PRNERR_NORETRY, "%s(): assertion failed", function);
                    }
	    	}
	    }

	/* Call write(), restarting it if it is interupted.
	   by a signal such as SIGALRM. */
	while((rval = write(intstdin, wptr, remain)) < 0)
	    {
	    /* Handle interuption by signals. */
	    if(errno == EINTR)
	    	{
	    	DODEBUG_INTERFACE_GRITTY(("%s(): write() interupted", function));
		interface_fault_check();
	    	DODEBUG_INTERFACE_GRITTY(("%s(): restarting write()", function));
	    	continue;
	    	}

	    /* For a broken pipe error, we prefer that the interface call alert()
	       and then exit since that will presumably give the user more
	       information about what went wrong.  If it does so before sleep()
	       expires, then reapchild() will call exit(). */
	    if(errno == EPIPE)
	    	{
		DODEBUG_INTERFACE(("Pipe write error, Waiting 10 seconds for SIGCHLD"));
		sleep(10);
		interface_fault_check();
		DODEBUG_INTERFACE(("10 seconds are over"));
	    	}

	    fatal(EXIT_PRNERR, "%s(): write failed, errno=%d (%s)", function, errno, gu_strerror(errno));
	    }

	DODEBUG_INTERFACE_GRITTY(("%s(): wrote %d bytes: \"%.*s\"", function, rval, rval, wptr));

	remain -= rval;	/* reduce length left to write */
	wptr += rval;	/* move pointer forward */

	/* If this isn't a banner page, update the "Progress:"
	   line in the queue file. */
	if(doing_primary_job)
	    progress_bytes_sent(rval);

	/* If coming out of a stall, say so. */
	writemon_unstalled("WRITE");

	/* If we wanted throttle the bandwidth consumption, we could pause here. */
	#if 0
	usleep(25000);
	#endif
	}

    bptr = wbuf;
    wbuf_space = BUFSIZE;

    DODEBUG_INTERFACE_GRITTY(("%s(): done, %d bytes total", function, total));

    return total;
    } /* end of printer_flush() */

/*
** Add a single character to the output buffer.
**
** non-TBCP version
*/
static void raw_printer_putc(int c)
    {
    if(wbuf_space == 0)		/* if the buffer is full, */
	printer_flush();	/* write it out */

    *(bptr++) = c;		/* otherwise, store it in the buffer */
    wbuf_space--;		/* and reduce buffer empty space count */
    } /* end of raw_printer_putc() */

/*
** Write the indicated number of bytes to the interface.
** This is used to write lines when the lines may
** contain NULLs.
**
** non-TBCP version
*/
static void raw_printer_write(const char *buf, size_t len)
    {
    while(len--)
    	raw_printer_putc(*(buf++));
    } /* end of raw_printer_write() */

/*
** Print a string to the interface.
**
** non-TBCP version
*/
static void raw_printer_puts(const char *string)
    {
    while(*string)
	raw_printer_putc(*(string++));
    } /* end of raw_printer_puts() */

/*
** Add a single character to the output buffer.
**
** TBCP version
*/
static void tbcp_printer_putc(int c)
    {
    switch(c)
    	{
	case 0x01:	/* control-A */
	case 0x03:	/* control-C */
	case 0x04:	/* control-D */
	case 0x05:	/* control-E */
	case 0x11:	/* control-Q */
	case 0x13:	/* control-S */
	case 0x14:	/* control-T */
	case 0x1b:	/* ESC */
	case 0x1c:	/* FS */
	    raw_printer_putc(1);
	    raw_printer_putc(c ^ 0x40);
	    break;

	default:
	    raw_printer_putc(c);
	    break;
    	}
    } /* end of tbcp_printer_putc() */

/*
** Write the indicated number of bytes to the interface.
** This is used to write lines when the lines may
** contain NULLs.
**
** TBCP version
*/
static void tbcp_printer_write(const char *buf, size_t len)
    {
    while(len--)
    	tbcp_printer_putc(*(buf++));
    } /* end of tbcp_printer_write() */

/*
** Print a string to the interface.
**
** TBCP version
*/
static void tbcp_printer_puts(const char *string)
    {
    while(*string)
	tbcp_printer_putc(*(string++));
    } /* end of tbcp_printer_puts() */

/*
** Turn TBCP (Tagged Binary Communications Protocol)
** on or off.  Initially it will be off.
*/
void printer_TBCP_on(void)
    {
    ptr_printer_putc = tbcp_printer_putc;
    ptr_printer_puts = tbcp_printer_puts;
    ptr_printer_write = tbcp_printer_write;
    } /* end of printer_TBCP_on() */

void printer_TBCP_off(void)
    {
    ptr_printer_putc = raw_printer_putc;
    ptr_printer_puts = raw_printer_puts;
    ptr_printer_write = raw_printer_write;
    } /* end of printer_TBCP_off() */

/*
** Send a string to the interface and add a newline.
*/
void printer_putline(const char *string)
    {
    printer_puts(string);
    printer_putc('\n');
    } /* end of printer_putline() */

/*
** Print a formated string to the interface.
*/
void printer_printf(const char *string, ... )
    {
    va_list va;
    const char *sptr;
    int n;
    double dn;
    char nstr[25];

    va_start(va, string);

    while(*string)
	{
	if(*string == '%')
	    {
	    string++;			/* discard the "%%" */
	    switch(*(string++))
		{
		case '%':		/* literal '%' */
		    printer_putc('%');
		    break;
		case 's':		/* a string */
		    sptr = va_arg(va, char *);
		    printer_puts(sptr);
		    break;
		case 'd':		/* a decimal value */
		    n = va_arg(va, int);
		    sprintf(nstr, "%d", n);
		    printer_puts(nstr);
		    break;
		case 'o':		/* an octal value */
		    n = va_arg(va, int);
		    sprintf(nstr, "%.3o", n);	/* We assume three digits */
		    printer_puts(nstr);		/* because PostScript needs 3. */
		    break;
		case 'f':		/* a double */
		case 'g':
		    dn = va_arg(va, double);
		    sptr = gu_dtostr(dn);
		    printer_puts(sptr);
		    break;
		default:
		    fatal(EXIT_PRNERR_NORETRY, "printer_printf(): illegal format spec: %s", string-2);
		    break;
		}
	    }
	else
	    {
	    printer_putc(*(string++));
	    }
	}

    va_end(va);
    } /* end of printer_printf() */

/*
** Write a QuotedValue to the interface pipe.
** This is used to send strings from the PPD file.
**
** For a definition of a QuotedValue, see "PostScript Printer
** Description File Format Specification" version 4.0, page 20.
*/
void printer_puts_QuotedValue(const char *s)
    {
    int x;                  /* partial value */
    int val
    #ifdef GNUC_HAPPY
    = 0
    #endif
    ;
    int place;              /* set to 1 after 1st character of hex byte */

    while(*s)               /* loop until end of string */
	{
	if( *s != '<' )     /* if doesn't introduce a hex substring, */
	    {               /* just */
	    printer_putc(*(s++));  /* send it */
	    }
	else                /* hex substring: */
	    {
	    s++;            /* we don't need "<" anymore */
	    place=0;
	    while(*s && (*s != '>'))    /* loop until end of substring */
		{
		if( *s >= '0' && *s <= '9' )    /* convert hex to int */
		    x = (*(s++) - '0');
		else if( *s >= 'a' && *s <= 'z' )
		    x = (*(s++) - 'a' + 10);
		else if( *s >= 'A' && *s <= 'Z' )
		    x = (*(s++) - 'A' + 10);
		else                            /* ignore other chars */
		    {                           /* (such as spaces, */
		    s++;                        /* tabs and newlines) */
		    continue;
		    }

		if(place)                       /* if 2nd character */
		    {
		    val += x;			/* add to last */
		    printer_putc(val);		/* and send to output */
		    place = 0;			/* and set back to no chars */
		    }
		else				/* if 1st character */
		    {
		    val = x << 4;		/* store it */
		    place = 1;			/* and note fact */
		    }

		} /* end of inner while */
	    if(*s)          /* skip closeing ">" */
		s++;
	    } /* end of else */
	} /* end of outer while */
    } /* end of printer_puts_QuotedValue() */

/*
** Print a character or string with PostScript quoted string
** encoding applied.  That is, a backslash (\) is inserted
** before "(", ")", and "\"; and non-ASCII characters are
** represented in octal.
**
** These routines are used to generate PostScript strings
** to set the jobname and draft notice and print banner pages
** and stuff like that.
*/
void printer_putc_escaped(int c)
    {
    switch(c)
        {
        case '(':
        case ')':
        case 0x5C:              /* backslash */
            printer_putc(0x5C);
            printer_putc(c);
            break;
        default:
            if(c >= 32 && c < 127)          /* If ASCII printable, */
                printer_putc(c);
            else                            /* if not, */
                printer_printf("\%o", c);
            break;
        }
    }

void printer_puts_escaped(const char *str)
    {
    int c;

    while((c = *(str++)))
	{
	printer_putc_escaped(c);
	}
    } /* end of printer_puts_escaped() */

/* end of file */
