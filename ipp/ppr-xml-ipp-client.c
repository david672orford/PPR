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
	xmlDocPtr doc;
	xmlNodePtr cur;
	http_t *http;
	ipp_t *request, *response;

	if(!(doc = xmlParseFile("request.xml")))
		gu_Throw("failed to parse request XML");

	gu_Try
		{
		if(!(cur = xmlDocGetRootElement(doc)))
			gu_Throw("request XML document is empty");

		if(xmlStrcmp(cur->name, (const xmlChar*)"ipp"))
			gu_Throw("root element of request XML document is not \"ipp\"");

		if(!(http = httpConnect(cupsServer(), ippPort())))
			gu_Throw("can not connect: %s %s", cupsServer(), strerror(errno));

		request = ippNew();


		
		}
	gu_Final
		{
		xmlFreeDoc(doc);
		}
	gu_Catch
		{
		gu_ReThrow();
		}	

	}
