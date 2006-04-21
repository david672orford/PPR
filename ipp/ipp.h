#if 1
#define DEBUG(a) debug a
#else
#define DEBUG(a)
#endif

/* ipp.c */
void debug(const char message[], ...);
const char *printer_uri_validate(struct URI *printer_uri, enum QUEUEINFO_TYPE *qtype);
const char *destname_to_uri_template(const char destname[]);

/* ipp_print.c */
void ipp_print_job(struct IPP *ipp);
	
/* ipp_jobs.c */
void ipp_get_jobs(struct IPP *ipp);
void ipp_cancel_job(struct IPP *ipp);
void ipp_hold_job(struct IPP *ipp);
void ipp_release_job(struct IPP *ipp);

/* ipp_cups_admin.c */
void cups_get_devices(struct IPP *ipp);
void cups_get_ppds(struct IPP *ipp);
void cups_add_printer(struct IPP *ipp);

/* ipp_destinations.c */
void ipp_get_printer_attributes(struct IPP *ipp);
void cups_get_printers(struct IPP *ipp);
void cups_get_classes(struct IPP *ipp);
void cups_get_default(struct IPP *ipp);

/* ipp_run.c */
FILE *gu_popen(char *argv[], const char type[]);
int gu_pclose(FILE *f);
int run(char command[], ...);
