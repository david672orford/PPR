/*
** mouse:~ppr/src/ipp/ipp_utils.h
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 19 April 2006.
*/

/*! \file

This file defines an IPP server API.  It is loosly based on the CUPS IPP 
client API.

When one creates an IPP object by calling ipp_new() one specifies the file
descriptors from which to read the request and to which to send the reply
as well as the base path for building IPP object URL's, and the 
PATH_INFO.

Once the object has bee created, it is necessary to call
ipp_parse_request_header() and ipp_parse_request_body() in order to read in the
full request.  Once this is done, One can call methods of the IPP object to
enumerate the request attributes.  

Other methods of the IPP object can be used to build a reply which, when
ipp_send_reply() is called, is formatted and sent to the reply file descriptor
which ch was specified as a parameter to ipp_new().

Here is a partial example of using the IPP object from a CGI "script":

	char *p;
	void *ipp = ipp_new("http://myserver/myscript", getenv("PATH_INFO"), atoi(getenv(CONTENT_LENGTH), 0, 1);
	if((p = getenv("REMOTE_USER")) && *p)
		ipp_set_remote_user(ipp, p);
	if((p = getenv("REMOTE_ADDR")))
		ipp_set_remote_addr(ipp, p);
	ipp_parse_request(ipp);

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
	const char *template;
	int num_values;
	ipp_value_t values[1];
	} ipp_attribute_t;

/* The IPP object. */
struct IPP
	{
	int magic;
	void *pool;

	int debug_level;

	const char *root;
	const char *path_info;
	int bytes_left;
	int in_fd;
	int out_fd;

	const char *remote_user;
	const char *remote_addr;
	
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

	gu_boolean suppress_unsupported;
	};

/* IPP object methods */
struct IPP *ipp_new(const char root[], const char path_info[], int content_length, int in_fd, int out_fd);
void ipp_set_debug_level(struct IPP *ipp, int level);
void ipp_delete(struct IPP *ipp);
int ipp_get_block(struct IPP *ipp, char **pptr);
void ipp_set_remote_user(struct IPP *ipp, const char remote_user[]);
void ipp_set_remote_addr(struct IPP *ipp, const char remote_addr[]);
void ipp_parse_request(struct IPP *ipp);
void ipp_send_reply(struct IPP *ipp, gu_boolean header);
void ipp_copy_attribute(struct IPP *ipp, int group, ipp_attribute_t *attr);
void ipp_add_end(struct IPP *ipp, int group);
void ipp_add_integer(struct IPP *ipp, int group, int tag, const char name[], int value);
void ipp_add_integers(struct IPP *ipp, int group, int tag, const char name[], int num_values, int values[]);
void ipp_add_string(struct IPP *ipp, int group, int tag, const char name[], const char value[]);
void ipp_add_strings(struct IPP *ipp, int group, int tag, const char name[], int num_values, const char *values[]);
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
ipp_attribute_t *ipp_claim_attribute(struct IPP *ipp, int group, int tag, const char name[]);
struct URI *ipp_claim_uri(struct IPP *ipp, const char name[]);
int ipp_claim_positive_integer(struct IPP *ipp, const char name[]);
const char *ipp_claim_name(struct IPP *ipp, const char name[]);
const char *ipp_claim_keyword(struct IPP *ipp, const char name[], ...);

/*==================== ipp_req_attrs.c ========================*/

/* Operation attributes handling class */
struct REQUEST_ATTRS {
	void *requested_attributes;
	gu_boolean requested_attributes_all;
	char *printer_uri;
	struct URI *printer_uri_obj;
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
enum REQ_SUPPORTS {
   	REQ_SUPPORTS_PRINTER = 1,		/* printer-uri */
   	REQ_SUPPORTS_PRINTERS = 2,		/* printer-info, printer-location, printer-type, printer-type-mask, limit */
	REQ_SUPPORTS_JOB = 4,			/* job-uri, printer-uri, job-id */
	REQ_SUPPORTS_JOBS = 8,			/* printer-uri, which-jobs, limit */
	REQ_SUPPORTS_DEVICES = 16,		/* device-class, limit */
	REQ_SUPPORTS_PPDS = 32,			/* ppd-make, limit */
	REQ_SUPPORTS_PCREATE = 64		/* device-uri, ppd-name, etc. */
	};

struct REQUEST_ATTRS *request_attrs_new(struct IPP *ipp, int supported);
void request_attrs_free(struct REQUEST_ATTRS *this);
gu_boolean request_attrs_attr_requested(struct REQUEST_ATTRS *this, char name[]);
char *request_attrs_destname(struct REQUEST_ATTRS *this);
int request_attrs_jobid(struct REQUEST_ATTRS *this);

/* end of file */
