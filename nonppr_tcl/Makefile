#
# mouse:~ppr/src/nonppr_tcl/Makefile
#
# Last modified 18 January 2002.
#

include ../makeprogs/global.mk

# In global.mk are defined the following values that are needed to build.
# There are additional things needed for install.
#CC=gcc
#CFLAGS=-O2
#HOMEDIR=

# Directory from which applications will reference the library of Tcl scripts.
TCL_LIBRARY = $(HOMEDIR)/lib

#
# Mathematical functions like sin and atan2 are enabled for expressions
# by default.  To disable them, reverse the comment characters on the
# following pairs of lines:
#
# If you don't have them enabled, then you can't run the regression tests. 
#
MATH_FLAGS =
MATH_LIBS = -lieee -lm
#MATH_FLAGS = -DTCL_NO_MATH
#MATH_LIBS =

#
# To enable memory debugging reverse the comment characters on the following
# lines.  Warning:  if you enable memory debugging, you must do it
# *everywhere*, including all the code that calls Tcl, and you must use
# ckalloc and ckfree everywhere instead of malloc and free.
#
MEM_DEBUG_FLAGS =
#MEM_DEBUG_FLAGS = -DTCL_MEM_DEBUG

# Compiler flags in addition to those used to compile all programs.
MORE_CFLAGS = -I. $(MATH_FLAGS) $(MEM_DEBUG_FLAGS) -DTCL_LIBRARY=\"$(TCL_LIBRARY)\"

#=== Inventory ==============================================================

GENERIC_OBJS =	regexp.o tclAsync.o tclBasic.o tclCkalloc.o \
	tclCmdAH.o tclCmdIL.o tclCmdMZ.o tclExpr.o tclGet.o \
	tclHash.o tclLink.o tclParse.o tclProc.o \
	tclUtil.o tclVar.o gu.o

UNIX_OBJS = panic.o tclEnv.o tclGlob.o tclMain.o tclUnixAZ.o tclUnixStr.o tclUnixUtil.o

OBJS = $(GENERIC_OBJS) $(UNIX_OBJS)

BIN_PROGS=ppr-tclsh$(DOTEXE)

#=== Build ==================================================================

all: libtcl.a $(BIN_PROGS)

.c.$(OBJ):
	$(CC) $(CFLAGS) $(MORE_CFLAGS) -c $*.c

libtcl.a: $(OBJS)
	$(RMF) libtcl.a
	ar cr libtcl.a $(OBJS)

ppr-tclsh: tclAppInit.o libtcl.a
	$(CC) $(CC_SWITCHES) tclAppInit.o libtcl.a $(MATH_LIBS) -o ppr-tclsh

#=== Test == ================================================================

test: tcltest
	( cd tests && ../tcltest all )

tcltest: tclTest.o libtcl.a
	$(CC) $(CC_SWITCHES) tclTest.o libtcl.a $(MATH_LIBS) -o tcltest

#=== Install ================================================================

install: $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(BIN_PROGS)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) libtcl.a $(BIN_PROGS) tcltest

# end of file
