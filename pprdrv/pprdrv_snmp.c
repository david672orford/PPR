/*
** mouse:~ppr/src/pprdrv/pprdrv_snmp.c
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 4 May 2001.
*/

#include "before_system.h"
#include <string.h>
#include <stdlib.h>
#include "gu.h"
#include "global_defines.h"
#include "pprdrv.h"

/*
** Convert a printer MIB PrinterStatus name to the cooresponding
** code number.
*/
int snmp_DeviceStatus(const char name[])
	{
	if(strcmp(name, "unknown") == 0)
		return 1;
	else if(strcmp(name, "running") == 0)
		return 2;
	else if(strcmp(name, "warning") == 0)
		return 3;
	else if(strcmp(name, "testing") == 0)
		return 4;
	else if(strcmp(name, "down") == 0)
		return 5;
	else if(strspn(name, "0123456789") == strlen(name))
		return atoi(name);
	else
		return -1;
	}

/*
** Convert a printer MIB PrinterStatus name to the cooresponding
** code number.
*/
int snmp_PrinterStatus(const char name[])
	{
	if(strcmp(name, "other") == 0)
		return 1;
	else if(strcmp(name, "unknown") == 0)
		return 2;
	else if(strcmp(name, "idle") == 0)
		return 3;
	else if(strcmp(name, "printing") == 0)
		return 4;
	else if(strcmp(name, "warmup") == 0)
		return 5;
	else if(strspn(name, "0123456789") == strlen(name))
		return atoi(name);
	else
		return -1;
	}

/*
** Convert a printer MIB PrinterDetectedErrorState condition name to the
** cooresponding code number.
*/
int snmp_PrinterDetectedErrorState(const char name[])
	{
	if(strcmp(name, "lowPaper") == 0)
		return 0;
	else if(strcmp(name, "noPaper") == 0)
		return 1;
	else if(strcmp(name, "lowToner") == 0)
		return 2;
	else if(strcmp(name, "noToner") == 0)
		return 3;
	else if(strcmp(name, "doorOpen") == 0)
		return 4;
	else if(strcmp(name, "jammed") == 0)
		return 5;
	else if(strcmp(name, "offline") == 0)
		return 6;
	else if(strcmp(name, "serviceRequested") == 0)
		return 7;
	else if(strcmp(name, "inputTrayMissing") == 0)
		return 8;
	else if(strcmp(name, "outputTrayMissing") == 0)
		return 9;
	else if(strcmp(name, "markerSupplyMissing") == 0)
		return 10;
	else if(strcmp(name, "outputNearFull") == 0)
		return 11;
	else if(strcmp(name, "outputFull") == 0)
		return 12;
	else if(strcmp(name, "inputTrayEmpty") == 0)
		return 13;
	else if(strcmp(name, "overduePreventMaint") == 0)
		return 14;
	else if(strspn(name, "0123456789") == strlen(name))
		return atoi(name);
	else
		return -1;
	}

/* end of file */
