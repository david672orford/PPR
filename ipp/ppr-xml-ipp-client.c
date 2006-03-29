/*
 * mouse:~ppr/src/ipp/ppr-xml-ipp-client.c
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

int main(int argc, char *argv[])
	{
	xmlDocPtr doc = NULL;		/* XML encoded request */
	http_t *http = NULL;
	ipp_t *request = NULL;
	ipp_t *response = NULL;

	gu_Try
		{
		xmlNodePtr cur;
		xmlChar *version_number = NULL;
		int operation_id = -1;
		xmlNodePtr operation_attributes = NULL;

		if(!(doc = xmlParseFile("request.xml")))
			gu_Throw("failed to parse request XML");

		if(!(cur = xmlDocGetRootElement(doc)))
			gu_Throw("request XML document is empty");

		if(xmlStrcmp(cur->name, (const xmlChar*)"ipp"))
			gu_Throw("root element of IPP request XML document is not \"ipp\"");

		for(cur = cur->xmlChildrenNode; cur; cur = cur->next)
			{
			if(xmlStrcmp(cur->name, (const xmlChar*)"version-number") == 0)
				{
				version_number = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
				printf("version: %s\n", version_number);
				continue;
				}
			if(xmlStrcmp(cur->name, (const xmlChar*)"operation-id") == 0)
				{
				xmlChar *temp = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
				printf("operation-id: %s\n", temp);
				operation_id = ipp_str_to_operation_id(temp);
				continue;
				}
			if(xmlStrcmp(cur->name, (const xmlChar*)"operation-attributes") == 0)
				{
				operation_attributes = cur;
				continue;
				}
			}
		
		request = ippNew();

		request->request.op.operation_id = operation_id;
		request->request.op.request_id = 42;

		for(cur=operation_attributes; cur; cur = cur->next)
			{
			
			}

		if(!(http = httpConnect(cupsServer(), ippPort())))
			gu_Throw("can not connect: %s %s", cupsServer(), strerror(errno));

		}
	gu_Final
		{
		if(doc)
			xmlFreeDoc(doc);
		if(request)
			ippDelete(request);
		if(response)
			ippDelete(response);
		if(http)
			httpClose(http);
		}
	gu_Catch
		{
		gu_ReThrow();
		}	

	}

/* end of file */
