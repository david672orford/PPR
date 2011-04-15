/*
** mouse:~ppr/src/ipp/ippd.h
** Copyright 1995--2010, Trinity College Computing Center.
** Written by David Chappell.
**
** This file is part of PPR.  You can redistribute it and modify it under the
** terms of the revised BSD licence (without the advertising clause) as
** described in the accompanying file LICENSE.txt.
**
** Last modified 9 September 2010.
*/

/*! \file
	\brief Function prototypes for the IPP server
*/

#if 0				/* debug enable */
#define DEBUG 2		/* debug level */
#define DODEBUG1(a) debug a
#else
#define DODEBUG1(a)
#endif

#if DEBUG > 1
#define DODEBUG(a) debug a
#define FUNCTION4DEBUG(a) const char function[] = a ;
#else
#define DODEBUG(a)
#define FUNCTION4DEBUG(a)
#endif

/* ippd.c */
void debug(const char message[], ...);
void error(const char message[], ...);
const char *printer_uri_validate(struct URI *printer_uri, enum QUEUEINFO_TYPE *qtype);
const char *extract_destname(struct IPP *ipp, enum QUEUEINFO_TYPE *qtype, gu_boolean required);
const char *extract_identity(struct IPP *ipp, gu_boolean require_authentication);
const char *destname_to_uri_template(const char destname[]);

/* ippd_print.c */
void ipp_print_job(struct IPP *ipp);
void ipp_send_document(struct IPP *ipp);
	
/* ippd_jobs.c */
void ipp_get_jobs(struct IPP *ipp);
void ipp_get_job_attributes(struct IPP *ipp);
void ipp_X_job(struct IPP *ipp);
void cups_move_job(struct IPP *ipp);

/* ippd_cups_admin.c */
void cups_get_devices(struct IPP *ipp);
void cups_get_ppds(struct IPP *ipp);
void cups_add_modify_printer(struct IPP *ipp);
void cups_delete_printer(struct IPP *ipp);
void cups_add_modify_class(struct IPP *ipp);
void cups_delete_class(struct IPP *ipp);

/* ippd_destinations.c */
void ipp_get_printer_attributes(struct IPP *ipp);
void cups_get_printers(struct IPP *ipp);
void cups_get_classes(struct IPP *ipp);
void cups_get_default(struct IPP *ipp);
void ipp_X_printer(struct IPP *ipp);

/* ippd_run.c */
FILE *gu_popen(const char *argv[], const char type[]);
int gu_pclose(FILE *f);
int runv(const char command[], const char *argv[]);
int runl(const char command[], ...);

/* end of file */
