/*
** mouse:~ppr/src/ipp/ipp_utils.h
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
** Last modified 30 July 2003.
*/

/* This union holds any kind of IPP value. */
typedef union
	{
	int integer;
	gu_boolean boolean;
	struct
		{
		char const *text;
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
	const char *name;
	int num_values;
	ipp_value_t values[1];
	} ipp_attribute_t;

/* The IPP object. */
struct IPP
	{
	char *path_info;

	int bytes_left;
	unsigned char readbuf[512];
	int readbuf_i;
	int readbuf_remaining;
	unsigned char writebuf[512];
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
struct IPP *ipp_new(void);
void ipp_delete(struct IPP *p);
int ipp_get_block(struct IPP *p, char **pptr);
unsigned char ipp_get_byte(struct IPP *p);
void ipp_put_byte(struct IPP *p, unsigned char val);
int ipp_get_sb(struct IPP *p);
int ipp_get_ss(struct IPP *p);
int ipp_get_si(struct IPP *p);
void ipp_put_sb(struct IPP *p, int val);
void ipp_put_ss(struct IPP *p, int val);
void ipp_put_si(struct IPP *p, int val);
unsigned char *ipp_get_bytes(struct IPP *p, int len);
void ipp_put_bytes(struct IPP *ipp, const unsigned char *data, int len);
void ipp_put_string(struct IPP *ipp, const char string[]);
void ipp_put_attr(struct IPP *ipp, ipp_attribute_t *attr);
void ipp_parse_request(struct IPP *ipp);
void ipp_send_reply(struct IPP *ipp);
void ipp_add_integer(struct IPP *ipp, int group, int tag, const char name[], int value);
void ipp_add_string(struct IPP *ipp, int group, int tag, const char name[], const char value[]);
void ipp_add_boolean(struct IPP *ipp, int group, int tag, const char name[], gu_boolean value);
ipp_attribute_t *ipp_find_attribute(struct IPP *ipp, int group, int tag, const char name[]);

/* Other functions */
void debug(const char message[], ...);

/* end of file */
