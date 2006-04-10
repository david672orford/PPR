/*
** mouse:~ppr/src/ipp/ppr-xml-ipp-client.c
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
** Last modified 3 April 2006.
*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <cups/cups.h>
#include <cups/language.h>
#include "gu.h"
#include "ipp_constants.h"

/* This function steps through the node set attributes and adds what it finds to request.
 * request -- IPP request we are constructing
 * group_entity -- name of entity from which we extracted attributes (for error messages)
 * group_tag -- group tag to set when adding attributes to request
 * doc -- XML document object which contains attributes
 * attributes -- node list containing attributes nodes to be added to request
 */
static void do_attributes(ipp_t *request, const char group_entity[], int group_tag, xmlDocPtr doc, xmlNodePtr attributes, void *params)
	{
	const char **values = NULL;	/* growable array for temporary storage of attribute values */
	int values_space = 0;		/* number of members currently allocated */
	gu_Try
		{
		/* Step through the IPP attribute entitites.  They will be such things
		 * as <integer>, <boolean> and <stringWithoutLanguage>.
		 */
		xmlNodePtr cur, cur2;
		for(cur=attributes; cur; cur = cur->next)
			{
			int tag, tag_class;
			char *name = NULL;
			char *lang = NULL;
			int values_count = 0;
			
			if(xmlStrcmp(cur->name, (const xmlChar*)"text") == 0)
				continue;
			if(xmlStrcmp(cur->name, (const xmlChar*)"comment") == 0)
				continue;

			/* If the item has an ifdef= attribute, skip it if its value
			 * is the name of an undefined parameter.
			 */
			{
			const char *ifdef;
			if((ifdef = (const char *)xmlGetProp(cur, (const xmlChar*)"ifdef")))
				{
				if(!gu_pch_get(params, ifdef))
					continue;
				}
			}
			
		    if((tag = ipp_str_to_tag((char*)cur->name)) == IPP_TAG_UNSUPPORTED_VALUE)
				gu_Throw("Unsupported tag type: %s", cur->name);

			tag_class = ipp_tag_simplify(tag);
			
			/* Step through the contents of the current IPP attribute entity.
			 * An IPP attribute entity might look like this:
			 * <integer><name>limit</name><value>20</value></integer>
			 * At this point we simply collect these things in variables.
			 * We will convert the values to proper form and add the attributes
			 * to the request in a moment.
			 */
			for(cur2 = cur->xmlChildrenNode; cur2; cur2 = cur2->next)
				{
				if(xmlStrcmp(cur2->name, (const xmlChar*)"name") == 0)
					{
					name = (char*)xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
					continue;
					}
				if(xmlStrcmp(cur2->name, (const xmlChar*)"lang") == 0)
					{
					lang = (char*)xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
					continue;
					}
				if(xmlStrcmp(cur2->name, (const xmlChar*)"value") == 0)
					{
					if(values_count == values_space)	/* growable array */
						{
						values_space += 16;
						values = gu_realloc(values, values_space, sizeof(int));
						}
					values[values_count++] = (char*)xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
					continue;
					}
				if(xmlStrcmp(cur2->name, (const xmlChar*)"param-value") == 0)
					{
					char *name;
					if(values_count == values_space)	/* growable array */
						{
						values_space += 16;
						values = gu_realloc(values, values_space, sizeof(int));
						}
					name = (char*)xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
					if(!(values[values_count++] = gu_pch_get(params, name)))
						gu_Throw("Parameter not defined: %s", name);
					continue;
					}
				if(xmlStrcmp(cur2->name, (const xmlChar*)"text") == 0)
					continue;
				if(xmlStrcmp(cur2->name, (const xmlChar*)"comment") == 0)
					continue;
				gu_Throw("Unknown tag in <%s><%s>: <%s>", group_entity, cur->name, cur2->name);
				}
	
			/*printf("tag_class=0x%02x, tag=0x%02x, name=%s, values_count=%d\n", tag_class, tag, name, values_count);*/

			/* Ok, we have scanned it, let's add the IPP attribute to request. */
			switch(tag_class)
				{
				case IPP_TAG_INTEGER:
					{
					int iii;
					int *values_int = gu_alloc(values_count, sizeof(int));
					for(iii=0; iii<values_count; iii++)
						values_int[iii] = atoi(values[iii]);
					ippAddIntegers(request, group_tag, tag, name, values_count, values_int);
					gu_free(values_int);
					}
					break;
				case IPP_TAG_BOOLEAN:
					{
					int iii;
					char *values_char;
					gu_Try
						{
						values_char = gu_alloc(values_count, sizeof(char));
						for(iii=0; iii<values_count; iii++)
							{
							if(strcmp(values[iii], "true") == 0)
								values_char[iii] = 1;
							else if(strcmp(values[iii], "false") == 0)
								values_char[iii] = 0;
							else
								gu_Throw("Not boolean: %s", values[iii]);
							}
						ippAddBooleans(request, group_tag, name, values_count, values_char);
						}
					gu_Final
						{
						gu_free(values_char);
						}
					gu_Catch
						{
						gu_ReThrow();
						}
					}
					break;
				case IPP_TAG_STRING:
					ippAddStrings(request, group_tag, tag, name, values_count, lang, values);
					break;
				default:
					gu_Throw("Missing case: 0x%02x", tag_class);
				}	
			}
		}
	gu_Final
		{
		if(values)
			gu_free(values);
		}
	gu_Catch
		{
		gu_ReThrow();
		}
	}

int main(int argc, char *argv[])
	{
	/* command line parameters */
	char *opt_request_xml_file = NULL;
	char *opt_url = NULL;
	char *opt_filename = NULL;
	void *params = gu_pch_new(10);

	struct URI *uri = NULL;		/* URI to which to send request */
	xmlDocPtr doc = NULL;		/* request encoded as XML */
	ipp_t *request = NULL;		/* IPP request to send */
	http_t *http = NULL;		/* HTTP connexion to IPP server */
	ipp_t *response = NULL;		/* Received IPP response */

	if(argc >= 2)
		opt_request_xml_file = argv[1];
	if(argc >= 3)
		{
		opt_url = argv[2];
		gu_pch_set(params, "uri", opt_url);
		}

	/* Treat the remaining command-line arguments as name-value pairs and 
	 * load them into the Perl Compatible Hash params{}.
	 */
	{
	int iii;
	for(iii = 3; iii < argc; iii++)
		{
		char *p = argv[iii];
		char *name = gu_strsep(&p, "=");
		char *value = gu_strsep(&p, "");
		gu_pch_set(params, name, value);
		if(strcmp(name, "filename") == 0)	/* filename gets special treatment */
			opt_filename = value;
		}
	}
	
	gu_Try
		{
		xmlNodePtr cur;
		unsigned char ipp_version[2] = {1, 0};
		int operation_id = -1;
		int request_id = 1;
		xmlNodePtr operation_attributes = NULL;
		xmlNodePtr job_attributes = NULL;

		if(!(uri = gu_uri_new(opt_url)))
			gu_Throw("can't parse URL: %s", opt_url);
		
		if(!(doc = xmlParseFile(opt_request_xml_file)))
			gu_Throw("failed to parse %s", opt_request_xml_file);

		if(!(cur = xmlDocGetRootElement(doc)))
			gu_Throw("request XML document is empty");

		if(xmlStrcmp(cur->name, (const xmlChar*)"ipp"))
			gu_Throw("root element of IPP request XML document is not \"ipp\"");

		for(cur = cur->xmlChildrenNode; cur; cur = cur->next)
			{
			if(xmlStrcmp(cur->name, (const xmlChar*)"request") == 0)
				break;
			}
		if(!cur)
			gu_Throw("No <request> element");
		
		/* Step through the top level of the <ipp> entity. */
		for(cur = cur->xmlChildrenNode; cur; cur = cur->next)
			{
			if(xmlStrcmp(cur->name, (const xmlChar*)"version-number") == 0)
				{
				xmlChar *temp = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
				gu_sscanf((char*)temp, "%d.%d", &ipp_version[0], &ipp_version[1]);
				continue;
				}
			if(xmlStrcmp(cur->name, (const xmlChar*)"operation-id") == 0)
				{
				xmlChar *temp = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
				if((operation_id = ipp_str_to_operation_id((char*)temp)) == -1)
					gu_Throw("Unknown operation-id: %s", temp);
				continue;
				}
			if(xmlStrcmp(cur->name, (const xmlChar*)"request-id") == 0)
				{
				request_id = atoi((char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
				continue;
				}
			if(xmlStrcmp(cur->name, (const xmlChar*)"operation-attributes") == 0)
				{
				operation_attributes = cur->xmlChildrenNode;
				continue;
				}
			if(xmlStrcmp(cur->name, (const xmlChar*)"job-attributes") == 0)
				{
				job_attributes = cur->xmlChildrenNode;
				continue;
				}
			if(xmlStrcmp(cur->name, (const xmlChar*)"text") == 0)
				continue;
			if(xmlStrcmp(cur->name, (const xmlChar*)"comment") == 0)
				continue;
			gu_Throw("Unknown tag in <ipp>: <%s>", cur->name);
			}
	
		/* Construct an IPP request from the data gathered above. */	
		request = ippNew();
		request->request.op.version[0] = ipp_version[0];
		request->request.op.version[1] = ipp_version[1];
		request->request.op.operation_id = operation_id;
		request->request.op.request_id = request_id;

		if(operation_attributes)
			do_attributes(request, "operation-attributes", IPP_TAG_OPERATION, doc, operation_attributes, params);

		if(job_attributes)
			do_attributes(request, "job-attributes", IPP_TAG_JOB, doc, job_attributes, params);

		/* Connect to the IPP server using HTTP. */
		if(!(http = httpConnect(uri->node, uri->port)))
			gu_Throw("can not connect to %s port %d: %s", uri->node, uri->port, strerror(errno));

		/* Send the IPP request which we assembled above. */
		response = cupsDoFileRequest(http, request, uri->path ? uri->path : "/", opt_filename);
		request = NULL;		/* cupsDoFileRequest() freed request.  Ouch! */
		if(!response)
			gu_Throw("request failed: 0x%04x", cupsLastError());

		/* Print the response as XML */
		{
		ipp_attribute_t *attr;
		int current_group_tag = IPP_TAG_ZERO;
		const char *current_group_tag_string = NULL;

		printf("<?xml version=\"1.0\"?>\n");
		printf("<ipp>\n");
		printf("<response>\n");
		printf("<version-number>%d.%d</version-number>\n", response->request.status.version[0], response->request.status.version[1]);
		printf(" <status-code>0x%04x</status-code>\n", response->request.status.status_code);
		printf(" <request-id>%d</request_id>\n", response->request.status.request_id);
	
		/* Step thru the IPP attributes in the response.  We will keep track 
		 * of the current group tag in order to generate one XML entity for
		 * each group. */	
		for(attr=response->attrs; attr; attr=attr->next)
			{
			int tag_class = ipp_tag_simplify(attr->value_tag);
			const char *tag = ipp_tag_to_str(attr->value_tag);
			int iii;

			/*printf("group_tag=0x%02x, value_tag=0x%02x\n", attr->group_tag, attr->value_tag);*/
			
			/* Is a new tag group starting? */
			if(attr->group_tag != current_group_tag)
				{
				/* Close any preceding group. */
				if(current_group_tag_string)
					printf(" </%s>\n", current_group_tag_string);

				/* make the new tag the current one */
				current_group_tag = attr->group_tag;
				current_group_tag_string = NULL;

				/* CUPS library uses a pseudo-attribute with group_tag and 
				 * value_tag both set to IPP_TAG_ZERO to mark the divisions 
				 * between consecutive instances of the same tag.  The closing
				 * XML will have been generated above and when the next record
				 * starts it will trigger a start tag, so we don't have to do
				 * any more processing of the pseudo-attribute. */
				if(current_group_tag == IPP_TAG_ZERO)
					continue;

				current_group_tag_string = ipp_tag_to_str(attr->group_tag);
				printf(" <%s>\n", current_group_tag_string);
				}
		
			/* Start an entity cooresponding to the tag type. */	
			printf("  <%s>", tag);
			if(attr->name)		/* if name isn't missing, */
				printf("<name>%s</name>\n", attr->name);
			for(iii=0; iii < attr->num_values; iii++)
				{
				switch(tag_class)
					{
					case IPP_TAG_INTEGER:
						printf("    <value>%d</value>\n", attr->values[iii].integer);
						break;
					case IPP_TAG_BOOLEAN:
						printf("    <value>%s</value>\n", attr->values[iii].boolean ? "true" : "false");
						break;
					case IPP_TAG_TEXTLANG:
					case IPP_TAG_NAMELANG:
						printf("    <lang>%s</lang>\n", attr->values[iii].string.charset);
						/* fall thru */
					case IPP_TAG_STRING:
						printf("    <value>%s</value>\n", attr->values[iii].string.text);
						break;
					default:
						gu_Throw("no handler for values of type 0x%02X", tag_class);
					}
				}
			printf("    </%s>\n", tag);		/* close entity which represents data item */
			}

		/* Close any unclosed tag group */
		if(current_group_tag_string)
			printf(" </%s>\n", current_group_tag_string);

		printf("</response>\n");
		printf("</ipp>\n");
		}

		}
	gu_Final
		{
		if(uri)
			gu_uri_free(uri);
		if(doc)
			xmlFreeDoc(doc);
		if(request)
			ippDelete(request);
		if(http)
			httpClose(http);
		if(response)
			ippDelete(response);
		gu_pch_free(params);
		}
	gu_Catch
		{
		gu_ReThrow();
		}	

	return 0;
	}

/* end of file */
