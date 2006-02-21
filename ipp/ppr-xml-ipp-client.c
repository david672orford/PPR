#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "gu.h"

int main(int argc, char *argv[])
	{
	xmlDocPtr doc;
	xmlNodePtr cur;

	if(!(doc = xmlParseFile("request.xml")))
		gu_Throw("failed to parse request XML");

	gu_Try
		{
		if(!(cur = xmlDocGetRootElement(doc)))
			gu_Throw("request XML document is empty");

		if(xmlStrcmp(cur->name, (const xmlChar*)"ipp"))
			gu_Throw("root element of request XML document is not \"ipp\"");
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
