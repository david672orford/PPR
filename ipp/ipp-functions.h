/*
** mouse:~ppr/src/ipp/ipp.h
** Copyright 1995--2006, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 25 April 2006.
*/

/*! \file
	\brief Function prototypes for the IPP server
*/

#if 1
#define DEBUG(a) debug a
#else
#define DEBUG(a)
#endif

/* ipp.c */
void debug(const char message[], ...);
const char *printer_uri_validate(struct URI *printer_uri, enum QUEUEINFO_TYPE *qtype);
const char *extract_destname(struct IPP *ipp, enum QUEUEINFO_TYPE *qtype);
const char *extract_identity(struct IPP *ipp, gu_boolean require_authentication);
const char *destname_to_uri_template(const char destname[]);

/* ipp_print.c */
void ipp_print_job(struct IPP *ipp);
	
/* ipp_jobs.c */
void ipp_get_jobs(struct IPP *ipp);
void ipp_get_job_attributes(struct IPP *ipp);
void ipp_X_job(struct IPP *ipp);

/* ipp_cups_admin.c */
void cups_get_devices(struct IPP *ipp);
void cups_get_ppds(struct IPP *ipp);
void cups_add_printer(struct IPP *ipp);

/* ipp_destinations.c */
void ipp_get_printer_attributes(struct IPP *ipp);
void cups_get_printers(struct IPP *ipp);
void cups_get_classes(struct IPP *ipp);
void cups_get_default(struct IPP *ipp);
void ipp_X_printer(struct IPP *ipp);

/* ipp_run.c */
FILE *gu_popen(char *argv[], const char type[]);
int gu_pclose(FILE *f);
int run(char command[], ...);

/* end of file */
