/*
** mouse:~ppr/src/libscript/time_elapsed.c
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 23 May 2002.
*/

/*
** This program prints, as a well formed English phrase, the time which
** has elapsed since a time specified in Unix format.  If the elapsed time
** is shorter than a specified minimum, no output is generated.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
	{
	long start_time;
	long threshold;
	time_t now_time;
	long difference;

	if(argc != 2 && argc != 3)
		{
		fputs("Usage: time_elapsed <start_time> [<threshold>]\n", stderr);
		return 1;
		}

	start_time = atol(argv[1]);
	threshold = (argc >= 3) ? atol(argv[2]) : 0;

	time(&now_time);

	difference = now_time - start_time;

	if(difference >= threshold)
		{
		int seconds, minutes, hours, days;
		const char *pad = "";

		seconds = difference % 60;				/* remainder seconds */
		difference /= 60;						/* total minutes */
		if(seconds >= 30) difference++;
		minutes = difference % 60;				/* remainder minutes */
		difference /= 60;						/* total hours */
		hours = difference % 24;				/* remainder hours */
		days = difference / 24;					/* total days */

		if(days > 1)
			{
			if(minutes >= 30)
				hours++;
			if(hours == 24)
				{
				days++;
				hours = 0;
				}
			printf("%d days", days);
			pad = " ";
			if(hours > 0)
				printf("%s%d hour%s", pad, hours, hours > 1 ? "s" : "");
			}
		else
			{
			if(days)
				{
				printf("1 day");
				pad = " ";
				}
			if(hours > 0)
				{
				printf("%s%d hour%s", pad, hours, hours > 1 ? "s" : "");
				pad = " ";
				}
			if(minutes > 0)
				printf("%s%d minute%s", pad, minutes, minutes > 1 ? "s" : "");
			}
		}

	return 0;
	}

/* end of file */
