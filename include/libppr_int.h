/*
** mouse:~ppr/src/include/libppr_int.h
** Copyright 1995--2001, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is" without
** express or implied warranty.
**
** Last modified 1 May 2001.
*/

/*
** This is a structure into which interface arguments are stored
** for handy reference later.  The storing is done by
** interface_parse_argv().  To the right of each member is a sample
** of what it might contain.  If you want a real explaination of
** the significance of the interface options, looks at the
** appendix "Requirements for an Interface" in "PPR, a Print Spooler for
** PostScript".
*/
struct INT_CMDLINE
    {
    const char *int_name;		/* "interfaces/atalk" */
    const char *int_basename;		/* "atalk" */
    const char *printer;		/* "myprn" */
    const char *address;		/* "My Laser Printer:LaserWriter@Computing Center" */
    const char *options;		/* "idle_status_interval=60 open_retries=5" */
    int jobbreak;			/* 1 (signal) */
    gu_boolean feedback;			/* 1 (True) */
    enum CODES codes;			/* 3 (Binary) */
    const char *jobname;		/* "mouse:myprn-1001.0(mouse)" */
    const char *routing;		/* "Call David Chappell at 2114 when ready" */
    const char *forline;		/* "David Chappell" */
    const char *barbarlang;		/* "" (PostScript) */
    } ;

extern struct INT_CMDLINE int_cmdline;

/* Prototypes for functions in libppr which are used
   only by interfaces: */
#ifdef __GNUC__
void int_debug(const char format[], ...) __attribute__ ((format (printf, 1, 2)));
#endif
void int_debug(const char format[], ...);
void print_pap_status(const unsigned char *status);
void int_cmdline_set(int argc, char *argv[]);
void int_addrcache_save(const char printer[], const char interface[], const char address[], const char resolution[]);
char *int_addrcache_load(const char printer[], const char interface[], const char address[], int *age);
int int_connect_tcpip(int connect_timeout, int sndbuf_size, gu_boolean refused_engaged, int refused_retries, unsigned int *address_ptr);
void int_snmp_status(void *addr_ptr);
void int_printer_error_tcpip(int error_number);
void int_copy_job(int portfd, int idle_status_interval, void (*fatal_prn_err)(int err), void (*snmp_function)(void *address), void *address, int snmp_status_interval);
void int_exit(int exitvalue)
#ifdef __GNUC__
__attribute__ (( noreturn ))
#endif
;

/* end of file */

