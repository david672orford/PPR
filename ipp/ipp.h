#if 1
#define DEBUG(a) debug a
#else
#define DEBUG(a)
#endif

/* ipp.c */
void debug(const char message[], ...);

/* ipp_cups_admin.c */
void cups_get_devices(struct IPP *ipp);
void cups_get_ppds(struct IPP *ipp);
void cups_add_printer(struct IPP *ipp);

/* ipp_destinations.c */
void ipp_get_printer_attributes(struct IPP *ipp);
void cups_get_printers(struct IPP *ipp);
void cups_get_classes(struct IPP *ipp);
void cups_get_default(struct IPP *ipp);

/* ipp_print.c */
void ipp_print_job(struct IPP *ipp);
	
/* ipp_run.c */
FILE *gu_popen(char *argv[], const char type[]);
int gu_pclose(FILE *f);
int run(char command[], ...);
