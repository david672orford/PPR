/*
** ~ppr/src/include/cap_proto.h
** Copyright abandoned.
** Written by David Chappell.
**
** Last revised 8 September 2000.
*/

/*
** Function prototypes for the CAP libraries.
*/

#ifdef __cplusplus
extern "C" {
#endif

/* I am not sure all the return types are correct. */
int abInit(int debug_flag);
int nbpInit(void);
int PAPInit(void);
int PAPOpen(int *cno, const char *lwname, int quantum, PAPStatusRec *status, OSErr *compstate);
int PAPRead(int cno, char *buff, int *datasize, int *eof, OSErr *compstate);
int PAPWrite(int cno, char *buff, int datasize, int eof, OSErr *compstate);
int abSleep(int duration, int waitflag);
int PAPClose(int cno);
int SLInit(int *cno, char *printername, int quantum, PAPStatusRec *status);
int GetNextJob(int srefnum, int *refnum, OSErr *compstate);
int SLClose(int cno);
int PAPShutdown(int cno);
int PAPRemName(int cno, char *name);
int PAPUnload(void);
int PAPGetNetworkInfo(int cno, AddrBlock *addr);
int PAPStatus(const char *address, PAPStatusRec *status, AddrBlock *addr);

/* We can define this if abpap.h from CAP60 has already been included. */
#ifdef NUMPAP
PAPSOCKET *cnotopapskt(int cno);
#endif

#ifdef __cplusplus
} ;
#endif

/* end of file */
