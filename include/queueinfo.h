/*
** mouse:~ppr/src/include/queueinfo.h
** Copyright 1995--2004, Trinity College Computing Center.
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
** Last modified 10 February 2004.
*/

enum QUEUEINFO_TYPE { QUEUEINFO_SEARCH, QUEUEINFO_ALIAS, QUEUEINFO_GROUP, QUEUEINFO_PRINTER };
void *queueinfo_new(enum QUEUEINFO_TYPE qit, const char name[]);
void queueinfo_delete(void *p);
void *queueinfo_new_load_config(enum QUEUEINFO_TYPE qit, const char name[]);
void queueinfo_add_printer(void *p, const char name[]);
void queueinfo_add_hypothetical_printer(void *p, const char name[], const char ppdfile[], const char installed_memory[]);

void queueinfo_set_warnings_file(void *p, FILE *errors);
void queueinfo_set_debug_level(void *p, int debug_level);

const char *queueinfo_name(void *p);
const char *queueinfo_comment(void *p);
gu_boolean  queueinfo_transparentMode(void *);
gu_boolean queueinfo_psPassThru(void *p);
gu_boolean  queueinfo_binaryOK(void *);
const char *queueinfo_ppdFile(void *p);
const char *queueinfo_product(void *p);
int         queueinfo_psLanguageLevel(void *p);
const char *queueinfo_shortNickName(void *p);
const char *queueinfo_psVersionStr(void *p);
double      queueinfo_psVersion(void *p);
int         queueinfo_psRevision(void *p);
int         queueinfo_psFreeVM(void *p);
const char *queueinfo_resolution(void *p);				/* "300dpi", "600x300dpi" */
gu_boolean  queueinfo_colorDevice(void *p);
const char *queueinfo_faxSupport(void *p);				/* "Base" */
const char *queueinfo_ttRasterizer(void *p);			/* "None", "Type42", "Accept68K" */
int         queueinfo_fontCount(void *p);
const char *queueinfo_font(void *p, int index);
gu_boolean  queueinfo_fontExists(void *p, const char name[]);
const char *queueinfo_optionValue(void *p, const char name[]);
const char *queueinfo_computedMetaFontMode(void *p);
const char *queueinfo_computedDefaultFilterOptions(void *p);

/* end of file */
