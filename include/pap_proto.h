/*
** ~ppr/src/include/pap_proto.h
** Copyright abandoned.
** Written by David Chappell.
**
** Last modified 8 September 2000.
*/

/*
** This file contains function definitions for the AT&T/Apple AppleTalk 
** Library Interface.
**
** For some reason AT&T/NCR does not supply an include file with prototypes for
** these funtions.  This one has been built from the descriptions in the ALI
** reference manual.
*/

#ifdef __cplusplus
extern "C" {
#endif

int nbp_parse_entity(at_entity_t *entity, const char *str);
int nbp_lookup(at_entity_t *entity, at_nbptuple_t *buf, int max, at_retry_t *retry, u_char *more);
int nbp_register(at_entity_t *entity, int fd, at_retry_t *retry);
int nbp_remove(at_entity_t *entity, int fd);
int nbp_confirm(at_entity_t *entity, at_inet_t *dest, at_retry_t *retry);

/* These differ from manual which makes the improbable claim that status is a char pointer. */
int pap_open(at_nbptuple_t *tuple, u_short *quantum, unsigned char *status, short retry);
int paps_status(int fd, unsigned char *status);
int pap_status(at_nbptuple_t *tuple, unsigned char *status);

int pap_close(int fd);
int pap_look(int fd);
int pap_look(int fd);
int pap_read(int fd, char *data, int len, u_char *eof_flag);
int pap_sync(int fd);
int pap_write(int fd, const char *data, int len, u_char eof_flag, u_char mode);
int paps_get_next_job(int fd, u_short *quantum, at_inet_t *src);
int paps_open(u_short quantum);

/*
** Unless David Chappell's ALI compatibility library for Netatalk (NATALI)
** is being instead of the real ALI, define the macro pap_abrupt_close(). 
** There is a function of this name provided by NATALI.  In the AT&T ALI
** implementation an equivelent operation may be performed by simply calling
** close().
*/
#ifndef _NATALI_PAP
#define pap_abrupt_close close
#endif

#ifdef __cplusplus
} ;
#endif

/* end of file */
