#
# mouse:~ppr/src/nonppr_tcl/Makefile
# Copyright 1995--2005, Trinity College Computing Center.
# Written by David Chappell.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Last modified 17 January 2005.
#

include ../Makefile.conf

# In Makefile.conf are defined the following values that are needed to build.
# There are additional things needed for install.
#CC=gcc
#CFLAGS=-O2
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
	$(CC) $(CC_SWITCHES) -o $@ $^ $(MATH_LIBS)

#=== Test ===================================================================

test: tcltest$(DOTEXE)
	( cd tests && ../tcltest all )

tcltest$(DOTEXE): tclTest.o libtcl.a
	$(CC) $(CC_SWITCHES) -o tcltest$(DOTEXE) tclTest.o libtcl.a ../libgu.a $(MATH_LIBS)

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
