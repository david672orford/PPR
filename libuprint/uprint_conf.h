/*
** mouse:~ppr/src/include/uprint_conf.h
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
** Last modified 3 August 2003.
*/

struct PATH_SET
	{
	const char *lpr;
	const char *lpq;
	const char *lprm;
	const char *lpc;
	const char *lp;
	const char *lpstat;
	const char *cancel;
	};

struct UPRINT_CONF
	{
	struct PATH_SET well_known;
	struct PATH_SET sidelined;

	struct
		{
		gu_boolean installed;
		gu_boolean sidelined;
		const char *printers;
		const char *classes;
		const char *flavor;
		float flavor_version;
		} lp;
	struct
		{
		gu_boolean installed;
		gu_boolean sidelined;
		const char *flavor;
		float flavor_version;
		} lpr;

	struct
		{
		const char *lp;
		const char *lpr;
		} default_destinations;

	} ;

void uprint_read_conf(void);
extern struct UPRINT_CONF conf;

/* end of file */

