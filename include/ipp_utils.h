/*
** mouse:~ppr/src/ipp/ipp_utils.h
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
** Last modified 30 March 2006.
*/

/*! \file

This file defines an IPP server API.  It is loosly based on the CUPS IPP 
client API.

When one creates an IPP object by calling ipp_new() one specifies the file
descriptors from which to read the request and to which to send the reply
as well as the base path for building IPP object URL's, and the 
PATH_INFO.

Once the object has bee created, it is necessary to call ipp_parse_request_header()
and ipp_parse_request_body() in order to read in the full request.  Once this is 
done, One can call methods of the IPP object to
enumerate the request attributes.  

Other methods of the IPP object can be used
to build a reply which, when ipp_send_reply() is called, is formatted and sent
to the reply file descriptor which ch was specified as a parameter to ipp_new().

Here is a partial example of using the IPP object from a CGI "script":

	char *p;
	void *ipp = ipp_new("http://myserver/myscript", getenv("PATH_INFO"), atoi(getenv(CONTENT_LENGTH), 0, 1);
	if((p = getenv("REMOTE_USER")) && *p)
		ipp_set_remote_user(ipp, p);
	if((p = getenv("REMOTE_ADDR")))
		ipp_set_remote_addr(ipp, p);
	ipp_parse_request_header(ipp);
	ipp_parse_request_body(ipp);

	... read request and build reply ...
	
	ipp_send_reply(ipp, TRUE);

Fuller examples can be found in ../ipp/ipp.c and ../pprd/pprd_ipp.c.

*/

void debug(const char message[], ...)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 1, 2) ))
	#endif
	;

/*=========================== ipp_obj.c ===============================*/

/* This union holds any kind of IPP value. */
typedef union
	{
	int integer;
	gu_boolean boolean;
	struct
		{
		char *text;
		} string;
	struct
		{
		int length;
		void *data;
		} unknown;
	} ipp_value_t;

/* This structure holds any kind of IPP attribute. */
typedef struct ipp_attribute_s
	{
	struct ipp_attribute_s *next;
	int group_tag;
	int value_tag;
	char *name;
	gu_boolean free_name;
	gu_boolean free_values;
	const char *template;
	int num_values;
	ipp_value_t values[1];
	} ipp_attribute_t;

/* The IPP object. */
struct IPP
	{
	int magic;

	const char *root;
	const char *path_info;
	int bytes_left;
	int in_fd;
	int out_fd;

	const char *remote_user;
	const char *remote_addr;
	
	int subst_reply_fd;
	char readbuf[512];
	char readbuf_guard;
	int readbuf_i;
	int readbuf_remaining;
	char writebuf[512];
	char writebuf_guard;
	int writebuf_i;
	int writebuf_remaining;
	
	int version_minor;
	int version_major;
	int operation_id;
	int response_code;
	int request_id;
	
	ipp_attribute_t *request_attrs;
	ipp_attribute_t *response_attrs_operation;	
	ipp_attribute_t *response_attrs_printer;	
	ipp_attribute_t *response_attrs_job;	
	ipp_attribute_t *response_attrs_unsupported;	
	};

/* IPP object methods */
struct IPP *ipp_new(const char root[], const char path_info[], int content_length, int in_fd, int out_fd);
void ipp_delete(struct IPP *p);
int ipp_get_block(struct IPP *p, char **pptr);
void ipp_set_remote_user(struct IPP *p, const char remote_user[]);
void ipp_set_remote_addr(struct IPP *p, const char remote_addr[]);
void ipp_request_to_fd(struct IPP *p, int fd);
void ipp_reply_from_fd(struct IPP *p, int fd);
char ipp_get_byte(struct IPP *p);
void ipp_put_byte(struct IPP *p, char val);
int ipp_get_sb(struct IPP *p);
int ipp_get_ss(struct IPP *p);
int ipp_get_si(struct IPP *p);
void ipp_put_sb(struct IPP *p, int val);
void ipp_put_ss(struct IPP *p, int val);
void ipp_put_si(struct IPP *p, int val);
char *ipp_get_bytes(struct IPP *p, int len);
void ipp_put_bytes(struct IPP *ipp, const char *data, int len);
void ipp_put_string(struct IPP *ipp, const char string[]);
void ipp_put_attr(struct IPP *ipp, ipp_attribute_t *attr);
void ipp_parse_request_header(struct IPP *ipp);
void ipp_parse_request_body(struct IPP *ipp);
gu_boolean ipp_validate_request(struct IPP *ipp);
void ipp_send_reply(struct IPP *ipp, gu_boolean header);
void ipp_copy_attribute(struct IPP *ipp, int group, ipp_attribute_t *attr);
void ipp_add_end(struct IPP *ipp, int group);
void ipp_add_integer(struct IPP *ipp, int group, int tag, const char name[], int value);
void ipp_add_integers(struct IPP *ipp, int group, int tag, const char name[], int num_values, int values[]);
void ipp_add_string(struct IPP *ipp, int group, int tag, const char name[], const char value[], gu_boolean free_value);
void ipp_add_strings(struct IPP *ipp, int group, int tag, const char name[], int num_values, const char *values[], gu_boolean free_values);
void ipp_add_printf(struct IPP *ipp, int group, int tag, const char name[], const char value[], ...)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 5, 6) ))
	#endif
	;
void ipp_add_template(struct IPP *ipp, int group, int tag, const char name[], const char template[], ...)
	#ifdef __GNUC__
	__attribute__ (( format (printf, 5, 6) ))
	#endif
	;
void ipp_add_boolean(struct IPP *ipp, int group, int tag, const char name[], gu_boolean value);
ipp_attribute_t *ipp_find_attribute(struct IPP *ipp, int group, int tag, const char name[]);

/*==================== ipp_req_attrs.c ========================*/

/* Operation attributes handling class */
struct REQUEST_ATTRS {
	void *requested_attributes;
	gu_boolean requested_attributes_all;
	char *printer_uri;
	struct URI *printer_uri_obj;
	char *printer_name;
	char *job_uri;
	struct URI *job_uri_obj;
	int job_id;
	char *device_class;
	char *device_uri;
	char *ppd_make;
	char *ppd_name;
	int limit;
	};

/* Use enum to define bit constants */
enum REQUEST_ATTR_SUPPORTS {
   	REQUEST_ATTRS_SUPPORTS_PRINTER = 1,			/* printer-name, printer-uri */
	REQUEST_ATTRS_SUPPORTS_JOB = 2,				/* job-uri, printer-name, job_id */
	REQUEST_ATTRS_SUPPORTS_LIMIT = 4,
	REQUEST_ATTRS_SUPPORTS_DEVICE_CLASS = 8,	/* device-class */
	REQUEST_ATTRS_SUPPORTS_PPD_MAKE = 16,		/* ppd-make */
	REQUEST_ATTRS_SUPPORTS_PCREATE = 32			/* device-uri, ppd-name, etc. */
	};

struct REQUEST_ATTRS *request_attrs_new(struct IPP *ipp, int supported);
void request_attrs_free(struct REQUEST_ATTRS *this);
gu_boolean request_attrs_attr_requested(struct REQUEST_ATTRS *this, char name[]);
char *request_attrs_destname(struct REQUEST_ATTRS *this);
int request_attrs_jobid(struct REQUEST_ATTRS *this);

/* end of file */
