/*
** mouse:~ppr/src/include/queueinfo.h
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
** Last modified 17 April 2006.
*/

/* An opaque type declaration */
typedef struct QUEUE_INFO *QUEUE_INFO;

enum QUEUEINFO_TYPE { QUEUEINFO_SEARCH, QUEUEINFO_ALIAS, QUEUEINFO_GROUP, QUEUEINFO_PRINTER };
QUEUE_INFO queueinfo_new(enum QUEUEINFO_TYPE qit, const char name[]);
void queueinfo_free(QUEUE_INFO qip);
QUEUE_INFO queueinfo_new_load_config(enum QUEUEINFO_TYPE qit, const char name[]);
void queueinfo_add_printer(QUEUE_INFO qip, const char name[]);
void queueinfo_add_hypothetical_printer(QUEUE_INFO qip, const char name[], const char ppdfile[], const char installed_memory[]);

void queueinfo_set_warnings_file(QUEUE_INFO qip, FILE *errors);
void queueinfo_set_debug_level(QUEUE_INFO qip, int debug_level);

void *queueinfo_hoist_value(QUEUE_INFO qip, const void *value);

/* basic */
const char *queueinfo_name(QUEUE_INFO qip);
gu_boolean queueinfo_is_group(QUEUE_INFO qip);
const char *queueinfo_comment(QUEUE_INFO qip);
const char *queueinfo_location(QUEUE_INFO qip, int printer_index);

/* IPP */
const char *queueinfo_device_uri(QUEUE_INFO qip, int printer_index);
int queueinfo_queued_job_count(QUEUE_INFO qip);
const char *queueinfo_modelName(QUEUE_INFO qip);
int queueinfo_accepting(QUEUE_INFO qip);
int queueinfo_status(QUEUE_INFO qip);
const char *queueinfo_membername(QUEUE_INFO qip, int index);

/* papd */
gu_boolean  queueinfo_transparentMode(QUEUE_INFO qip);
gu_boolean queueinfo_psPassThru(QUEUE_INFO qip);
gu_boolean  queueinfo_binaryOK(QUEUE_INFO qip);
const char *queueinfo_ppdFile(QUEUE_INFO qip);
const char *queueinfo_product(QUEUE_INFO qip);
int         queueinfo_psLanguageLevel(QUEUE_INFO qip);
const char *queueinfo_shortNickName(QUEUE_INFO qip);
const char *queueinfo_psVersionStr(QUEUE_INFO qip);
double      queueinfo_psVersion(QUEUE_INFO qip);
int         queueinfo_psRevision(QUEUE_INFO qip);
int         queueinfo_psFreeVM(QUEUE_INFO qip);
const char *queueinfo_resolution(QUEUE_INFO qip);			/* "300dpi", "600x300dpi" */
gu_boolean  queueinfo_colorDevice(QUEUE_INFO qip);
const char *queueinfo_faxSupport(QUEUE_INFO qip);			/* "Base" */
const char *queueinfo_ttRasterizer(QUEUE_INFO qip);			/* "None", "Type42", "Accept68K" */
gu_boolean  queueinfo_chargeExists(QUEUE_INFO qip);
int         queueinfo_fontCount(QUEUE_INFO qip);
const char *queueinfo_font(QUEUE_INFO qip, int index);
gu_boolean  queueinfo_fontExists(QUEUE_INFO qip, const char name[]);
const char *queueinfo_optionValue(QUEUE_INFO qip, const char name[]);

/* ppad */
const char *queueinfo_computedMetaFontMode(QUEUE_INFO qip);
const char *queueinfo_computedDefaultFilterOptions(QUEUE_INFO qip);

/* end of file */
