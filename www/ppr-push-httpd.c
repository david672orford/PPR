/*
** mouse:~ppr/src/www/ppr-push-httpd.c
** Copyright 1995--2002, Trinity College Computing Center.
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
** Last modified 19 November 2002.
*/

#include "before_system.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "gu.h"
#include "global_defines.h"
#include "commentary.h"

#define MAX_PRINTERS_TRACKED 100
struct SETTINGS
	{
	int categories;								/* which category bits? */
	int severity;								/* how server must event be to be played? (1 -- 10) */
	int printers_count;							/* number of printers to monitor */
	char *printers[MAX_PRINTERS_TRACKED];		/* printers to monitor */
	int silly_sounds;							/* play silly sounds? */
	char *voice;								/* which sound file set? */
	int method;									/* Browser sound playing method */
	char *player;								/* URL of speach player */
	char *images;								/* URL of images directory */
	};

static void send_header(const char content_type[])
	{
	printf("HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"Cache-Control: no-cache\r\n"
		"Pragma: no-cache\r\n"
		"Content-Type: %s\r\n"
		"\r\n", content_type);
	}

/*===========================================================================
** URL content generation routines follow.
===========================================================================*/

static gu_boolean do_audio1_callback(char *line, void *extra)
	{
	char *printer, *cooked, *raw1, *raw2;
	int category, severity;
	gu_boolean retval = FALSE;
	int len, x;
	struct SETTINGS *settings = (struct SETTINGS *)extra;

	if(!line)
		{
		printf("<script>tick();</script>\n");
		fflush(stdout);
		return TRUE;
		}

	/* This is not really a while() loop!  It is an if() with the
	   oportunity to use break to drop out.	 Look at the end. */
	printer = cooked = raw1 = raw2 = NULL;
	while(gu_sscanf(line, "COMMENTARY %S %d %Q %Q %Q %s", &printer, &category, &cooked, &raw1, &raw2, &severity) == 6)
		{
		/* If the user doesn't want to hear about events of this class,
		   skip it. */
		if((category & settings->categories) != category)
			break;

		/* If this event does meet the sererity thresold which the user set,
		   skip it. */
		if(severity < settings->severity)
			break;

		/* If this event didn't occur on a printer the user wants to hear
		   about, skip it.	Notice that if the user selects no printers,
		   then events are shown for all of them! */
		if(settings->printers_count > 0)
			{
			for(x=0; x < settings->printers_count; x++)
				{
				if(strcmp(settings->printers[x], printer) == 0)
					break;
				}
			if(x >= settings->printers_count)
				break;
			}

		/* If we got this far, announce the event. */
		if(category & COM_PRINTER_ERROR)
			printf("The printer %s reports the error \"%s\".\n", printer, cooked);
		if(category & COM_PRINTER_STATUS)
			printf("The printer %s reports the status \"%s\".\n", printer, cooked);
		if(category & COM_STALL)
			printf("The printer %s %s.\n", printer, cooked);
		if(category & COM_EXIT)
			printf("Printing on %s ended with result \"%s\".\n", printer, cooked);
		printf("<br>\n");

		/* URL encode the messages.	 This is not correct. */
		len = strlen(cooked);
		for(x=0; x < len; x++) if(cooked[x] == ' ') cooked[x] = '+';
		len = strlen(raw1);
		for(x=0; x < len; x++) if(raw1[x] == ' ') raw1[x] = '+';
		len = strlen(raw2);
		for(x=0; x < len; x++) if(raw2[x] == ' ') raw2[x] = '+';

		/* Netscape <embed> tag */
		if(settings->method == 1)
			{
			printf("<embed type=\"audio/basic\" src=\"%s?printer=%s;category=%d;cooked=%s;raw1=%s;raw2=%s;severity=%d;silly_sounds=%d\"><br>\n",
				settings->player, printer, category, cooked, raw1, raw2, severity, settings->silly_sounds);
			}

		/* IE ActiveX instance of Windows Media Player */
		else if(settings->method == 2)
			{
			printf("<script>document.MediaPlayer1.FileName = '%s?printer=%s;category=%d;cooked=%s;raw1=%s;raw2=%s;severity=%d;silly_sounds=%d';</script>",
				settings->player, printer, category, cooked, raw1, raw2, severity, settings->silly_sounds);
			}

		/* Open the audio file in a tiny window and hope something can play it. */
		else
			{
			printf("<script>\n");
			/* printf("window.open('%s?printer=%s;category=%d;cooked=%s;raw1=%s;raw2=%s;severity=%d;silly_sounds=%d', 'ppr_player', 'width=100,height=100');\n", */
			printf("parent.ppr_player.location='%s?printer=%s;category=%d;cooked=%s;raw1=%s;raw2=%s;severity=%d;silly_sounds=%d';\n",
				settings->player, printer, category, cooked, raw1, raw2, severity, settings->silly_sounds);
			printf("</script>\n");
			}

		fflush(stdout);
		retval = TRUE;

		/* I told you that this is not a real while() loop! */
		break;
		}

	if(printer) gu_free(printer);
	if(cooked) gu_free(cooked);
	if(raw1) gu_free(raw1);
	if(raw2) gu_free(raw2);

	return retval;
	} /* end of do_audio1_callback() */

static void do_audio1(const char request_method[], char *content[], int content_count)
	{
	struct SETTINGS settings;
	int x;
	int tempint;

	settings.categories = 0;
	settings.silly_sounds = 0;
	settings.voice = "male1";
	settings.severity = 1;
	settings.printers_count = 0;
	settings.method = 0;
	settings.player = "/cgi-bin/commentary_speach.cgi";
	settings.images = "/images/";

	/*
	** Copy the CGI variables into the structure which we will be using to
	** communicate with do_audio1_callback().
	*/
	for(x=0; x < content_count; x++)
		{
		if(strncmp(content[x], "printer=", 8) == 0)
			{
			if(settings.printers_count < MAX_PRINTERS_TRACKED)
				{
				settings.printers[settings.printers_count++] = &content[x][8];
				}
			continue;
			}
		if(gu_sscanf(content[x], "categories=%d", &tempint) == 1)
			{
			settings.categories |= tempint;
			continue;
			}
		if(gu_sscanf(content[x], "silly_sounds=%d", &settings.silly_sounds) == 1)
			continue;
		if(strncmp(content[x], "voice=", 6) == 0)
			{
			settings.voice = &content[x][6];
			continue;
			}
		if(gu_sscanf(content[x], "severity=%d", &settings.severity) == 1)
			continue;
		if(gu_sscanf(content[x], "method=%d", &settings.method) == 1)
			continue;
		}

	send_header("text/html");

	printf("<html>\n"
		"<head>\n"
		"<title>Printer Commentary</title>\n"
		"<script>\n"
		"var tick_count = 0;\n"
		"function tick() { tick_count++;\n"
				"document.images[0].src = '%sprogress-' + (tick_count %% 4) + '.png';\n"
				"}\n"
		"</script\n"
		"</head>\n"
		"<body bgcolor=\"white\" fgcolor=\"black\">\n"
		"<img height=168 width=168 src=\"%sprogress-0.png\" align=\"left\">\n",
		settings.images, settings.images);

	/*
	** This might be considered debugging code.
	*/
	if(1)
		{
		printf("<table border=1>\n"
			"<tr><th rowspan=2>Events</th><th>Errors</th><th>Status</th><th>Stalled</th><th>Attempts</th></tr>\n"
			"<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><tr>\n"
			"<tr><th rowspan=2>Presentation</th><th>Threshold</th><th>Voice</th><th>Silly</th></tr>\n"
			"<tr><td>%d</td><td>%s</td><td>%s</td><tr>\n"
			"<tr><th>Printers</th><td colspan=4>",
										settings.categories & COM_PRINTER_ERROR ? "yes" : "no",
										settings.categories & COM_PRINTER_STATUS ? "yes" : "no",
										settings.categories & COM_STALL ? "yes" : "no",
										settings.categories & COM_EXIT ? "yes" : "no",
										settings.severity,
										settings.voice,
										settings.silly_sounds ? "yes" : "no");

		/* Print a comma separated list of the printers for which we will
		   make announcements.	If the list is empty, we will make announcements
		   for all printers, since to make announcements for no printers
		   would be pointless.	Also, on servers with a lot of printeres
		   it is a drag to check all those boxes. */
		if(settings.printers_count > 0)
			{
			for(x=0; x < settings.printers_count; x++)
				{
				if(x > 0)
					printf(", ");
				printf("%s", settings.printers[x]);
				}
			}
		else
			{
			printf("all");
			}

		printf("</td></tr>\n"
			"</table>\n");
		}

	printf("<br clear=\"left\">\n");

	/* Browser sound playing method 2 requires an Active X plugin. */
	if(settings.method == 2)
		{
		printf("<OBJECT ID=\"MediaPlayer1\" width=300 height=70
						classid=\"CLSID:22D6F312-B0F6-11D0-94AB-0080C74C7E95\"
						CODEBASE=\"http://activex.microsoft.com/activex/controls/mplayer/en/nsmp2inf.cab#Version=6,4,5,715\"
						standby=\"Loading Microsoft® Windows® Media Player components...\"
						type=\"application/x-oleobject\">
				<PARAM NAME=\"AutoStart\" VALUE=\"true\">
				<PARAM NAME=\"animationatStart\" VALUE=\"False\">
				<PARAM NAME=\"ShowControls\" VALUE=\"True\">
				<PARAM NAME=\"ShowStatusBar\" VALUE=\"True\">
				Player object missing!
				</OBJECT>
				");
		}

	printf("<hr>\n");

	fflush(stdout);

	/* This is not expected to return.	It reads lines from the status
	   files and calls our callback function. */
	tail_status(FALSE, TRUE, do_audio1_callback, 5, (void*)&settings);
	} /* end of do_audio1() */

static void do_debug1(const char request_method[], char *content[], int content_count)
	{
	const char page_title[] = "PPR Commentary Server Debug Page";
	int x;

	send_header("text/html");

	printf("<html>\n"
		"<head>\n"
		"<title>%s</title>\n"
		"</head>\n"
		"<body>\n"
		"<h1>%s</h1>\n"
		"<pre>\n", page_title, page_title);

	for(x=0; x < content_count; x++)
		{
		printf("%s\n", content[x]);
		}

	printf("</pre>\n"
		"</body>\n"
		"</html>\n");
	} /* end of do_debug1() */

static gu_boolean do_tail_status_callback(char *line, void *extra)
	{
	if(!line)
		{
		printf("No activity for 10 minutes.\n");
		}
	else
		{
		printf("%s\n", line);
		}
	fflush(stdout);
	return TRUE;
	} /* end of do_tail_status_callback() */

static void do_tail_status(const char request_method[], char *content[], int content_count)
	{
	send_header("text/plain");
	fflush(stdout);
	tail_status(TRUE, TRUE, do_tail_status_callback, 600, NULL);
	}

/*==========================================================================
** Server support routines and main() follow.
==========================================================================*/

static void http_error(int code, const char message[], const char explain[], ...)
	{
	va_list va;

	printf("HTTP/1.1 %d %s\r\n"
		"Connection: close\r\n"
		"Cache-Control: no-cache\r\n"
		"Pragma: no-cache\r\n"
		"Content-Type: text/html\r\n"
		"\r\n", code, message);

	printf("<html>\n"
		"<head>\n"
		"<title>%d %s</title>\n"
		"</head>\n"
		"<body>\n"
		"<h1>%d %s</h1>\n"
		"<p>",
		code, message, code, message);


	va_start(va, explain);
	vfprintf(stdout, explain, va);
	va_end(va);

	printf("</p>\n</body>\n</html>\n");
	}

/* If the character pointed to is a hexadecimal digit, return its value
   otherwise, return -1. */
static int hexdigit(const char *p)
	{
	if(*p >= '0' && *p <= '9')
		return *p - '0';
	if(*p >= 'a' && *p <= 'f')
		return *p - 'a' + 10;
	if(*p >= 'A' && *p <= 'F')
		return *p - 'A' + 10;
	return -1;
	}

/* Read up to 2 hexadecimal digits pointed to by si and store the value of
   the number at di.  Return the number of digits at si consumed. */
static int hexpair(char *di, const char *si)
	{
	int count = 0;
	int dval;
	int i = '?';
	if(*si && (dval = hexdigit(si++)) != -1)
		{
		i = dval;
		count++;
		}
	if(*si && (dval = hexdigit(si++)) != -1)
		{
		i = (i << 4) | dval;
		count++;
		}
	*di = i;
	return count;
	}

int main(int argc, char *argv[])
	{
	char *line = NULL;
	int line_space = 80;

	char *request_method = NULL;
	char *url = NULL;
	char *host = NULL;
	int content_length = -1;
	char *entity_body = NULL;
	#define MAX_PAIRS 100
	char *content[MAX_PAIRS];
	int content_count = 0;

	/* Set an alarm so that if a client connects to this server and then never
	   finishes the transaction, we will not hang around forever. */
	alarm(60);

	/* Read the request line. */
	{
	char *request_line;
	if((request_line = line = gu_getline(line, &line_space, stdin)))
		{
		/*
		** The first line might look like this:
		** POST /audio1 HTTP/1.1
		*/
		gu_sscanf(request_line, "%S %S", &request_method, &url);

		/* See RFC 2068 5.1.2 for why we must do this: */
		if(gu_strncasecmp(url, "http://", 7) == 0)
			{
			int hostlen;
			url += 7;
			hostlen = strcspn(url, "/");
			host = gu_strndup(url, hostlen);
			url += hostlen;
			}

		/* Now we must decode the URL. */
		{
		char *si, *di;
		for(di=si=url; *si; )
			{
			if(*si == '%')
				{
				si++;
				si += hexpair(di++, si);
				}
			else
				{
				*(di++) = *(si++);
				}
			}
		*di = '\0';
		}
		}
	}

	/* Read the HTTP header. */
	while((line = gu_getline(line, &line_space, stdin)))
		{
		if(gu_strncasecmp(line, "Content-Length:", 15) == 0)
			{
			content_length = atoi(line + 15);
			continue;
			}

		else if(gu_strncasecmp(line, "Host:", 5) == 0 && !host)
			{
			gu_sscanf(line, " %S", &host);
			}

		/* A blank line signals the end of the header. */
		if(strspn(line, " \t") == strlen(line))
			break;
		}

	/* We can cancel the alarm now.	 From now on we will rely
	   on SIGPIPE to tell us when to die. */
	alarm(0);

	/* Free the memory left from reading the header lines. */
	if(line)
		gu_free(line);

	/* Look for invalid things in the request. */
	if(!request_method || !url)
		{
		http_error(400, "Bad Request", "The request line is in the wrong format.");
		return 0;
		}
	if(!host)
		{
		http_error(400, "Bad Request", "There is no \"Host:\" line in the request header.");
		return 0;
		}
	if(strcmp(request_method, "POST"))
		{
		http_error(501, "Not Implemented", "The only request method this server supports is POST.  (You sent a %s request.)", request_method);
		return 0;
		}
	if(content_length < 0)
		{
		http_error(411, "Length Required", "There is no \"Content-Length:\" line in the request header.");
		return 0;
		}
	if(content_length > 100000)
		{
		http_error(413, "Request Entity Too Large", "The content length (%d) is unreasonably large.", content_length);
		return 0;
		}

	/* Read the POST data. */
	entity_body = (char*)gu_alloc(content_length + 1, 1);
	if(fread(entity_body, 1, content_length, stdin) != content_length)
		{
		http_error(400, "Bad Request", "Read error.");
		return 0;
		}
	entity_body[content_length] = '\0';

	/* Reverse the CGI encoding and make an array of pointer to the
	   name=value pairs. */
	{
	char *p = entity_body;
	char *si, *di;
	while((si = gu_strsep(&p, "&;")))
		{
		if(content_count >= MAX_PAIRS)
			{
			http_error(400, "Bad Request", "Too many form variables posted.");
			return 0;
			}

		/* Save a pointer to this string in the list of name=value pairs. */
		content[content_count++] = si;

		/* Undo the encoding of disallowed characters. */
		for(di=si; *si; )
			{
			if(*si == '+')
				{
				*(di++) = ' ';
				si++;
				}
			else if(*si == '%')
				{
				si++;
				si += hexpair(di++, si);
				}
			else
				{
				*(di++) = *(si++);
				}
			}
		*di = '\0';
		}
	}

	/*
	** All of the pre-processing is done.  It is time to generate the content.
	*/
	if(strcmp(url, "/push/audio1") == 0)
		{
		do_audio1(request_method, content, content_count);
		}
	else if(strcmp(url, "/push/debug1") == 0)
		{
		do_debug1(request_method, content, content_count);
		}
	else if(strcmp(url, "/push/tail_status") == 0)
		{
		do_tail_status(request_method, content, content_count);
		}
	else
		{
		http_error(404, "Not Found", "The file \"%s\" does not exist.", url);
		}

	return 0;
	} /* end of main() */

/* end of file */

