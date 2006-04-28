#
# mouse:~ppr/src/nonppr_tcl/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 28 April 2006.
#

include ../Makefile.conf

# In Makefile.conf are defined the following values that are needed to build.
# There are additional things needed for install.
#CC=gcc
#CFLAGS=-O2
#LD=gcc
#LDFLAGS=
#BINDIR=
#OBJ=o
#LIBCMD=ar -cr
#RANLIB=ranlib
#DOTEXE=

# Directory from which applications will reference the library of Tcl scripts.
TCL_LIBRARY = $(LIBDIR)

#
# Mathematical functions like sin and atan2 are enabled for expressions
# by default.  To disable them, reverse the comment characters on the
# following pairs of lines:
#
# If you don't have them enabled, then you can't run the regression tests. 
#
#MATH_FLAGS =
#MATH_LIBS = -lm
MATH_FLAGS = -DTCL_NO_MATH
MATH_LIBS =

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

GENERIC_OBJS=regexp.o \
	tclAsync.o \
	tclBasic.o \
	tclCkalloc.o \
	tclCmdAH.o \
	tclCmdIL.o \
	tclCmdMZ.o \
	tclExpr.o \
	tclGet.o \
	tclHash.o \
	tclLink.o \
	tclParse.o \
	tclProc.o \
	tclUtil.o \
	tclVar.o 

UNIX_OBJS=panic.o \
	tclEnv.o \
	tclGlob.o \
	tclMain.o \
	tclUnixAZ.o \
	tclUnixStr.o \
	tclUnixUtil.o

OBJS = $(GENERIC_OBJS) $(UNIX_OBJS)

BIN_PROGS=ppr-tclsh$(DOTEXE)

#=== Build ==================================================================

all: libtcl.a $(BIN_PROGS)

.c.o:
	$(CC) $(CFLAGS) $(MORE_CFLAGS) -c $*.c

libtcl.a: $(OBJS)
	$(RMF) $@
	$(LIBCMD) $@ $^
	$(RANLIB) $@

ppr-tclsh$(DOTEXE): ppr-tclsh.o libtcl.a ../libppr.a ../libgu.a
	$(LD) $(LDFLAGS) -o $@ $^ $(MATH_LIBS)

#=== Test ===================================================================

test: tcltest$(DOTEXE)
	( cd tests && ../tcltest all )

tcltest$(DOTEXE): tclTest.o libtcl.a
	$(LD) $(LDFLAGS) -o tcltest$(DOTEXE) tclTest.o libtcl.a ../libgu.a $(MATH_LIBS)

#=== Install ================================================================

install: $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(BINDIR) $(BIN_PROGS)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.o $(BACKUPS) libtcl.a $(BIN_PROGS) tcltest$(DOTEXE)

# end of file
