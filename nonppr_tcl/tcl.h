/*
 * tcl.h --
 *
 *	This header file describes the externally-visible facilities
 *	of the Tcl interpreter.
 *
 * Copyright (c) 1987-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 * Copyright (c) 2002 Trinity College Computing Center.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Last modified 18 January 2002.
 */

#ifndef _TCL
#define _TCL

#include <stdio.h>

#define TCL_VERSION "7.4"
#define TCL_MAJOR_VERSION 7
#define TCL_MINOR_VERSION 4

/*
 * Definitions that allow this header file to be used either with or
 * without ANSI C features like function prototypes.
 */

#undef _ANSI_ARGS_
#define _ANSI_ARGS_(x)	x

/* What is this? */
#ifndef _CLIENTDATA
    typedef void *ClientData;
#define _CLIENTDATA
#endif

/*
 * Data structures defined opaquely in this module.  The definitions
 * below just provide dummy types.  A few fields are made visible in
 * Tcl_Interp structures, namely those for returning string values.
 * Note:  any change to the Tcl_Interp definition below must be mirrored
 * in the "real" definition in tclInt.h.
 */

typedef struct Tcl_Interp{
    char *result;		/* Points to result string returned by last
				 * command. */
    void (*freeProc) _ANSI_ARGS_((char *blockPtr));
				/* Zero means result is statically allocated.
				 * If non-zero, gives address of procedure
				 * to invoke to free the result.  Must be
				 * freed by Tcl_Eval before executing next
				 * command. */
    int errorLine;		/* When TCL_ERROR is returned, this gives
				 * the line number within the command where
				 * the error occurred (1 means first line). */
} Tcl_Interp;

typedef int *Tcl_Trace;
typedef int *Tcl_Command;
typedef struct Tcl_AsyncHandler_ *Tcl_AsyncHandler;
typedef struct Tcl_RegExp_ *Tcl_RegExp;

/*
 * When a TCL command returns, the string pointer interp->result points to
 * a string containing return information from the command.  In addition,
 * the command procedure returns an integer value, which is one of the
 * following:
 *
 * TCL_OK		Command completed normally;  interp->result contains
 *			the command's result.
 * TCL_ERROR		The command couldn't be completed successfully;
 *			interp->result describes what went wrong.
 * TCL_RETURN		The command requests that the current procedure
 *			return;  interp->result contains the procedure's
 *			return value.
 * TCL_BREAK		The command requests that the innermost loop
 *			be exited;  interp->result is meaningless.
 * TCL_CONTINUE		Go on to the next iteration of the current loop;
 *			interp->result is meaningless.
 */

#define TCL_OK		0
#define TCL_ERROR	1
#define TCL_RETURN	2
#define TCL_BREAK	3
#define TCL_CONTINUE	4

#define TCL_RESULT_SIZE 200

/*
 * Argument descriptors for math function callbacks in expressions:
 */

typedef enum {TCL_INT, TCL_DOUBLE, TCL_EITHER} Tcl_ValueType;
typedef struct Tcl_Value {
    Tcl_ValueType type;		/* Indicates intValue or doubleValue is
				 * valid, or both. */
    long intValue;		/* Integer value. */
    double doubleValue;		/* Double-precision floating value. */
} Tcl_Value;

/*
 * Procedure types defined by Tcl:
 */

typedef int (Tcl_AppInitProc)(Tcl_Interp *interp);
typedef int (Tcl_AsyncProc)(ClientData clientData, Tcl_Interp *interp, int code);
typedef void (Tcl_CmdDeleteProc) _ANSI_ARGS_((ClientData clientData));
typedef int (Tcl_CmdProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int argc, char *argv[]));
typedef void (Tcl_CmdTraceProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int level, char *command, Tcl_CmdProc *proc,
	ClientData cmdClientData, int argc, char *argv[]));
typedef void (Tcl_FreeProc) _ANSI_ARGS_((char *blockPtr));
typedef void (Tcl_InterpDeleteProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp));
typedef int (Tcl_MathProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, Tcl_Value *args, Tcl_Value *resultPtr));
typedef char *(Tcl_VarTraceProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, char *part1, char *part2, int flags));

/*
 * The structure returned by Tcl_GetCmdInfo and passed into
 * Tcl_SetCmdInfo:
 */

typedef struct Tcl_CmdInfo {
    Tcl_CmdProc *proc;			/* Procedure that implements command. */
    ClientData clientData;		/* ClientData passed to proc. */
    Tcl_CmdDeleteProc *deleteProc;	/* Procedure to call when command
					 * is deleted. */
    ClientData deleteData;		/* Value to pass to deleteProc (usually
					 * the same as clientData). */
} Tcl_CmdInfo;

/*
 * The structure defined below is used to hold dynamic strings.  The only
 * field that clients should use is the string field, and they should
 * never modify it.
 */

#define TCL_DSTRING_STATIC_SIZE 200
typedef struct Tcl_DString {
    char *string;		/* Points to beginning of string:  either
				 * staticSpace below or a malloc'ed array. */
    int length;			/* Number of non-NULL characters in the
				 * string. */
    int spaceAvl;		/* Total number of bytes available for the
				 * string and its terminating NULL char. */
    char staticSpace[TCL_DSTRING_STATIC_SIZE];
				/* Space to use in common case where string
				 * is small. */
} Tcl_DString;

#define Tcl_DStringLength(dsPtr) ((dsPtr)->length)
#define Tcl_DStringValue(dsPtr) ((dsPtr)->string)
#define Tcl_DStringTrunc Tcl_DStringSetLength

/*
 * Definitions for the maximum number of digits of precision that may
 * be specified in the "tcl_precision" variable, and the number of
 * characters of buffer space required by Tcl_PrintDouble.
 */

#define TCL_MAX_PREC 17
#define TCL_DOUBLE_SPACE (TCL_MAX_PREC+10)

/*
 * Flag that may be passed to Tcl_ConvertElement to force it not to
 * output braces (careful!  if you change this flag be sure to change
 * the definitions at the front of tclUtil.c).
 */

#define TCL_DONT_USE_BRACES	1

/*
 * Flag values passed to Tcl_RecordAndEval.
 * WARNING: these bit choices must not conflict with the bit choices
 * for evalFlag bits in tclInt.h!!
 */

#define TCL_NO_EVAL		0x10000
#define TCL_EVAL_GLOBAL		0x20000

/*
 * Special freeProc values that may be passed to Tcl_SetResult (see
 * the man page for details):
 */

#define TCL_VOLATILE	((Tcl_FreeProc *) 1)
#define TCL_STATIC	((Tcl_FreeProc *) 0)
#define TCL_DYNAMIC	((Tcl_FreeProc *) 3)

/*
 * Flag values passed to variable-related procedures.
 */

#define TCL_GLOBAL_ONLY		1
#define TCL_APPEND_VALUE	2
#define TCL_LIST_ELEMENT	4
#define TCL_TRACE_READS		0x10
#define TCL_TRACE_WRITES	0x20
#define TCL_TRACE_UNSETS	0x40
#define TCL_TRACE_DESTROYED	0x80
#define TCL_INTERP_DESTROYED	0x100
#define TCL_LEAVE_ERR_MSG	0x200

/*
 * Types for linked variables:
 */

#define TCL_LINK_INT		1
#define TCL_LINK_DOUBLE		2
#define TCL_LINK_BOOLEAN	3
#define TCL_LINK_STRING		4
#define TCL_LINK_READ_ONLY	0x80

/*
 * Permission flags for files:
 */

#define TCL_FILE_READABLE	1
#define TCL_FILE_WRITABLE	2

/*
 * The following declarations either map ckalloc and ckfree to
 * malloc and free, or they map them to procedures with all sorts
 * of debugging hooks defined in tclCkalloc.c.
 */

#ifdef TCL_MEM_DEBUG

extern char *		Tcl_DbCkalloc _ANSI_ARGS_((unsigned int size,
			    char *file, int line));
extern int		Tcl_DbCkfree _ANSI_ARGS_((char *ptr,
			    char *file, int line));
extern char *		Tcl_DbCkrealloc _ANSI_ARGS_((char *ptr,
			    unsigned int size, char *file, int line));
extern int		Tcl_DumpActiveMemory _ANSI_ARGS_((char *fileName));
extern void		Tcl_ValidateAllMemory _ANSI_ARGS_((char *file, int line));
#define ckalloc(x) Tcl_DbCkalloc(x, __FILE__, __LINE__)
#define ckfree(x)  Tcl_DbCkfree(x, __FILE__, __LINE__)
#define ckrealloc(x,y) Tcl_DbCkrealloc((x), (y),__FILE__, __LINE__)

#else

#define ckalloc(x) malloc(x)
#define ckfree(x)  free(x)
#define ckrealloc(x,y) realloc(x,y)
#define Tcl_DumpActiveMemory(x)
#define Tcl_ValidateAllMemory(x,y)

#endif /* TCL_MEM_DEBUG */

/*
 * Macro to free up result of interpreter.
 */

#define Tcl_FreeResult(interp)					\
    if ((interp)->freeProc != 0) {				\
	if ((interp)->freeProc == (Tcl_FreeProc *) free) {	\
	    ckfree((interp)->result);				\
	} else {						\
	    (*(interp)->freeProc)((interp)->result);		\
	}							\
	(interp)->freeProc = 0;					\
    }

/*
 * Forward declaration of Tcl_HashTable.  Needed by some C++ compilers
 * to prevent errors when the forward reference to Tcl_HashTable is
 * encountered in the Tcl_HashEntry structure.
 */

#ifdef __cplusplus
struct Tcl_HashTable;
#endif

/*
 * Structure definition for an entry in a hash table.  No-one outside
 * Tcl should access any of these fields directly;  use the macros
 * defined below.
 */

typedef struct Tcl_HashEntry {
    struct Tcl_HashEntry *nextPtr;	/* Pointer to next entry in this
					 * hash bucket, or NULL for end of
					 * chain. */
    struct Tcl_HashTable *tablePtr;	/* Pointer to table containing entry. */
    struct Tcl_HashEntry **bucketPtr;	/* Pointer to bucket that points to
					 * first entry in this entry's chain:
					 * used for deleting the entry. */
    ClientData clientData;		/* Application stores something here
					 * with Tcl_SetHashValue. */
    union {				/* Key has one of these forms: */
	char *oneWordValue;		/* One-word value for key. */
	int words[1];			/* Multiple integer words for key.
					 * The actual size will be as large
					 * as necessary for this table's
					 * keys. */
	char string[4];			/* String for key.  The actual size
					 * will be as large as needed to hold
					 * the key. */
    } key;				/* MUST BE LAST FIELD IN RECORD!! */
} Tcl_HashEntry;

/*
 * Structure definition for a hash table.  Must be in tcl.h so clients
 * can allocate space for these structures, but clients should never
 * access any fields in this structure.
 */

#define TCL_SMALL_HASH_TABLE 4
typedef struct Tcl_HashTable {
    Tcl_HashEntry **buckets;		/* Pointer to bucket array.  Each
					 * element points to first entry in
					 * bucket's hash chain, or NULL. */
    Tcl_HashEntry *staticBuckets[TCL_SMALL_HASH_TABLE];
					/* Bucket array used for small tables
					 * (to avoid mallocs and frees). */
    int numBuckets;			/* Total number of buckets allocated
					 * at **bucketPtr. */
    int numEntries;			/* Total number of entries present
					 * in table. */
    int rebuildSize;			/* Enlarge table when numEntries gets
					 * to be this large. */
    int downShift;			/* Shift count used in hashing
					 * function.  Designed to use high-
					 * order bits of randomized keys. */
    int mask;				/* Mask value used in hashing
					 * function. */
    int keyType;			/* Type of keys used in this table. 
					 * It's either TCL_STRING_KEYS,
					 * TCL_ONE_WORD_KEYS, or an integer
					 * giving the number of ints in a
					 */
    Tcl_HashEntry *(*findProc) _ANSI_ARGS_((struct Tcl_HashTable *tablePtr,
	    char *key));
    Tcl_HashEntry *(*createProc) _ANSI_ARGS_((struct Tcl_HashTable *tablePtr,
	    char *key, int *newPtr));
} Tcl_HashTable;

/*
 * Structure definition for information used to keep track of searches
 * through hash tables:
 */

typedef struct Tcl_HashSearch {
    Tcl_HashTable *tablePtr;		/* Table being searched. */
    int nextIndex;			/* Index of next bucket to be
					 * enumerated after present one. */
    Tcl_HashEntry *nextEntryPtr;	/* Next entry to be enumerated in the
					 * the current bucket. */
} Tcl_HashSearch;

/*
 * Acceptable key types for hash tables:
 */

#define TCL_STRING_KEYS		0
#define TCL_ONE_WORD_KEYS	1

/*
 * Macros for clients to use to access fields of hash entries:
 */

#define Tcl_GetHashValue(h) ((h)->clientData)
#define Tcl_SetHashValue(h, value) ((h)->clientData = (ClientData) (value))
#define Tcl_GetHashKey(tablePtr, h) \
    ((char *) (((tablePtr)->keyType == TCL_ONE_WORD_KEYS) ? (h)->key.oneWordValue \
						: (h)->key.string))

/*
 * Macros to use for clients to use to invoke find and create procedures
 * for hash tables:
 */

#define Tcl_FindHashEntry(tablePtr, key) \
	(*((tablePtr)->findProc))(tablePtr, key)
#define Tcl_CreateHashEntry(tablePtr, key, newPtr) \
	(*((tablePtr)->createProc))(tablePtr, key, newPtr)

/*
 * Exported Tcl variables:
 */

extern int		tcl_AsyncReady;
extern void		(*tcl_FileCloseProc)(FILE *f);
extern char *		tcl_RcFileName;

/*
 * Exported Tcl procedures:
 */

extern void		Tcl_AddErrorInfo(Tcl_Interp *interp,
			    char *message);
extern void		Tcl_AllowExceptions(Tcl_Interp *interp);
extern void		Tcl_AppendElement(Tcl_Interp *interp,
			    char *string);
extern void		Tcl_AppendResult(Tcl_Interp *interp, ...);
extern int		Tcl_AppInit(Tcl_Interp *interp);
extern void		Tcl_AsyncMark(Tcl_AsyncHandler async);
extern Tcl_AsyncHandler	Tcl_AsyncCreate(Tcl_AsyncProc *proc,
			    ClientData clientData);
extern void		Tcl_AsyncDelete(Tcl_AsyncHandler async);
extern int		Tcl_AsyncInvoke(Tcl_Interp *interp,
			    int code);
extern char		Tcl_Backslash(char *src,
			    int *readPtr);
extern void		Tcl_CallWhenDeleted(Tcl_Interp *interp,
			    Tcl_InterpDeleteProc *proc,
			    ClientData clientData);
extern int		Tcl_CommandComplete(char *cmd);
extern char *		Tcl_Concat(int argc, char **argv);
extern int		Tcl_ConvertElement(char *src,
			    char *dst, int flags);
extern Tcl_Command	Tcl_CreateCommand(Tcl_Interp *interp,
			    char *cmdName, Tcl_CmdProc *proc,
			    ClientData clientData,
			    Tcl_CmdDeleteProc *deleteProc);
extern Tcl_Interp *	Tcl_CreateInterp(void);
extern void		Tcl_CreateMathFunc(Tcl_Interp *interp,
			    char *name, int numArgs, Tcl_ValueType *argTypes,
			    Tcl_MathProc *proc, ClientData clientData);
extern int		Tcl_CreatePipeline(Tcl_Interp *interp,
			    int argc, char **argv, int **pidArrayPtr,
			    int *inPipePtr, int *outPipePtr,
			    int *errFilePtr);
extern Tcl_Trace	Tcl_CreateTrace(Tcl_Interp *interp,
			    int level, Tcl_CmdTraceProc *proc,
			    ClientData clientData);
extern int		Tcl_DeleteCommand(Tcl_Interp *interp,
			    char *cmdName);
extern void		Tcl_DeleteHashEntry(
			    Tcl_HashEntry *entryPtr);
extern void		Tcl_DeleteHashTable(
			    Tcl_HashTable *tablePtr);
extern void		Tcl_DeleteInterp(Tcl_Interp *interp);
extern void		Tcl_DeleteTrace(Tcl_Interp *interp,
			    Tcl_Trace trace);
extern void		Tcl_DetachPids(int numPids, int *pidPtr);
extern void		Tcl_DontCallWhenDeleted(
			    Tcl_Interp *interp, Tcl_InterpDeleteProc *proc,
			    ClientData clientData);
extern char *		Tcl_DStringAppend(Tcl_DString *dsPtr,
			    char *string, int length);
extern char *		Tcl_DStringAppendElement(
			    Tcl_DString *dsPtr, char *string);
extern void		Tcl_DStringEndSublist(Tcl_DString *dsPtr);
extern void		Tcl_DStringFree(Tcl_DString *dsPtr);
extern void		Tcl_DStringGetResult(Tcl_Interp *interp,
			    Tcl_DString *dsPtr);
extern void		Tcl_DStringInit(Tcl_DString *dsPtr);
extern void		Tcl_DStringResult(Tcl_Interp *interp,
			    Tcl_DString *dsPtr);
extern void		Tcl_DStringSetLength(Tcl_DString *dsPtr,
			    int length);
extern void		Tcl_DStringStartSublist(
			    Tcl_DString *dsPtr);
extern void		Tcl_EnterFile(Tcl_Interp *interp,
			    FILE *file, int permissions);
extern char *		Tcl_ErrnoId(void);
extern int		Tcl_Eval(Tcl_Interp *interp, char *cmd);
extern int		Tcl_EvalFile(Tcl_Interp *interp,
			    char *fileName);
extern int		Tcl_ExprBoolean(Tcl_Interp *interp,
			    char *string, int *ptr);
extern int		Tcl_ExprDouble(Tcl_Interp *interp,
			    char *string, double *ptr);
extern int		Tcl_ExprLong(Tcl_Interp *interp,
			    char *string, long *ptr);
extern int		Tcl_ExprString(Tcl_Interp *interp,
			    char *string);
extern int		Tcl_FilePermissions(FILE *file);
extern Tcl_HashEntry *	Tcl_FirstHashEntry(
			    Tcl_HashTable *tablePtr,
			    Tcl_HashSearch *searchPtr);
extern int		Tcl_GetBoolean(Tcl_Interp *interp,
			    char *string, int *boolPtr);
extern int		Tcl_GetCommandInfo(Tcl_Interp *interp,
			    char *cmdName, Tcl_CmdInfo *infoPtr);
extern char *		Tcl_GetCommandName(Tcl_Interp *interp,
			    Tcl_Command command);
extern int		Tcl_GetDouble(Tcl_Interp *interp,
			    char *string, double *doublePtr);
extern int		Tcl_GetInt(Tcl_Interp *interp,
			    char *string, int *intPtr);
extern int		Tcl_GetOpenFile(Tcl_Interp *interp,
			    char *string, int write, int checkUsage,
			    FILE **filePtr);
extern char *		Tcl_GetVar(Tcl_Interp *interp,
			    char *varName, int flags);
extern char *		Tcl_GetVar2(Tcl_Interp *interp,
			    char *part1, char *part2, int flags);
extern int		Tcl_GlobalEval(Tcl_Interp *interp,
			    char *command);
extern char *		Tcl_HashStats(Tcl_HashTable *tablePtr);
extern int		Tcl_Init(Tcl_Interp *interp);
extern void		Tcl_InitHashTable(Tcl_HashTable *tablePtr,
			    int keyType);
extern void		Tcl_InitMemory(Tcl_Interp *interp);
extern int		Tcl_LinkVar(Tcl_Interp *interp,
			    char *varName, char *addr, int type);
extern void		Tcl_Main(int argc, char **argv,
			    Tcl_AppInitProc *appInitProc);
extern char *		Tcl_Merge(int argc, char **argv);
extern Tcl_HashEntry *	Tcl_NextHashEntry(
			    Tcl_HashSearch *searchPtr);
extern char *		Tcl_ParseVar(Tcl_Interp *interp,
			    char *string, char **termPtr);
extern char *		Tcl_PosixError(Tcl_Interp *interp);
extern void		Tcl_PrintDouble(Tcl_Interp *interp,
			    double value, char *dst);
extern void		Tcl_ReapDetachedProcs(void);
extern int		Tcl_RecordAndEval(Tcl_Interp *interp,
			    char *cmd, int flags);
extern Tcl_RegExp	Tcl_RegExpCompile(Tcl_Interp *interp,
			    char *string);
extern int		Tcl_RegExpExec(Tcl_Interp *interp,
			    Tcl_RegExp regexp, char *string, char *start);
extern int		Tcl_RegExpMatch(Tcl_Interp *interp,
			    char *string, char *pattern);
extern void		Tcl_RegExpRange(Tcl_RegExp regexp,
			    int index, char **startPtr, char **endPtr);
extern void		Tcl_ResetResult(Tcl_Interp *interp);
#define Tcl_Return Tcl_SetResult
extern int		Tcl_ScanElement(char *string,
			    int *flagPtr);
extern int		Tcl_SetCommandInfo(Tcl_Interp *interp,
			    char *cmdName, Tcl_CmdInfo *infoPtr);
extern void		Tcl_SetErrorCode(Tcl_Interp *interp, ...);
extern int		Tcl_SetRecursionLimit(Tcl_Interp *interp,
			    int depth);
extern void		Tcl_SetResult(Tcl_Interp *interp,
			    char *string, Tcl_FreeProc *freeProc);
extern char *		Tcl_SetVar(Tcl_Interp *interp,
			    char *varName, char *newValue, int flags);
extern char *		Tcl_SetVar2(Tcl_Interp *interp,
			    char *part1, char *part2, char *newValue,
			    int flags);
extern char *		Tcl_SignalId(int sig);
extern char *		Tcl_SignalMsg(int sig);
extern int		Tcl_SplitList(Tcl_Interp *interp,
			    char *list, int *argcPtr, char ***argvPtr);
extern int		Tcl_StringMatch(char *string,
			    char *pattern);
extern char *		Tcl_TildeSubst(Tcl_Interp *interp,
			    char *name, Tcl_DString *bufferPtr);
extern int		Tcl_TraceVar(Tcl_Interp *interp,
			    char *varName, int flags, Tcl_VarTraceProc *proc,
			    ClientData clientData);
extern int		Tcl_TraceVar2(Tcl_Interp *interp,
			    char *part1, char *part2, int flags,
			    Tcl_VarTraceProc *proc, ClientData clientData);
extern void		Tcl_UnlinkVar(Tcl_Interp *interp,
			    char *varName);
extern int		Tcl_UnsetVar(Tcl_Interp *interp,
			    char *varName, int flags);
extern int		Tcl_UnsetVar2(Tcl_Interp *interp,
			    char *part1, char *part2, int flags);
extern void		Tcl_UntraceVar(Tcl_Interp *interp,
			    char *varName, int flags, Tcl_VarTraceProc *proc,
			    ClientData clientData);
extern void		Tcl_UntraceVar2(Tcl_Interp *interp,
			    char *part1, char *part2, int flags,
			    Tcl_VarTraceProc *proc, ClientData clientData);
extern int		Tcl_UpVar(Tcl_Interp *interp,
			    char *frameName, char *varName,
			    char *localName, int flags);
extern int		Tcl_UpVar2(Tcl_Interp *interp,
			    char *frameName, char *part1, char *part2,
			    char *localName, int flags);
extern int		Tcl_VarEval(Tcl_Interp *interp, ...);
extern ClientData	Tcl_VarTraceInfo(Tcl_Interp *interp,
			    char *varName, int flags,
			    Tcl_VarTraceProc *procPtr,
			    ClientData prevClientData);
extern ClientData	Tcl_VarTraceInfo2(Tcl_Interp *interp,
			    char *part1, char *part2, int flags,
			    Tcl_VarTraceProc *procPtr,
			    ClientData prevClientData);

#endif /* _TCL */
