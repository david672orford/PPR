#
# mouse:~ppr/src/nonppr_tcl/Makefile
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 10 December 2004.
#

include ../Makefile.conf

# In Makefile.conf are defined the following values that are needed to build.
# There are additional things needed for install.
#CC=gcc
#CFLAGS=-O2
#HOMEDIR=
#OBJ=o
#LIBCMD=ar -cr
#RANLIB=ranlib
#DOTEXE=

# Directory from which applications will reference the library of Tcl scripts.
TCL_LIBRARY = $(HOMEDIR)/lib

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

GENERIC_OBJS=regexp.$(OBJ) \
	tclAsync.$(OBJ) \
	tclBasic.$(OBJ) \
	tclCkalloc.$(OBJ) \
	tclCmdAH.$(OBJ) \
	tclCmdIL.$(OBJ) \
	tclCmdMZ.$(OBJ) \
	tclExpr.$(OBJ) \
	tclGet.$(OBJ) \
	tclHash.$(OBJ) \
	tclLink.$(OBJ) \
	tclParse.$(OBJ) \
	tclProc.$(OBJ) \
	tclUtil.$(OBJ) \
	tclVar.$(OBJ) \
	gu.$(OBJ)

UNIX_OBJS=panic.$(OBJ) \
	tclEnv.$(OBJ) \
	tclGlob.$(OBJ) \
	tclMain.$(OBJ) \
	tclUnixAZ.$(OBJ) \
	tclUnixStr.$(OBJ) \
	tclUnixUtil.$(OBJ)

OBJS = $(GENERIC_OBJS) $(UNIX_OBJS)

BIN_PROGS=ppr-tclsh$(DOTEXE)

#=== Build ==================================================================

all: libtcl.$(LIBEXT) $(BIN_PROGS)

.c.$(OBJ):
	$(CC) $(CFLAGS) $(MORE_CFLAGS) -c $*.c

libtcl.$(LIBEXT): $(OBJS)
	$(RMF) $@
	$(LIBCMD) $@ $^
	$(RANLIB) $@

ppr-tclsh$(DOTEXE): tclAppInit.$(OBJ) libtcl.$(LIBEXT)
	$(CC) $(CC_SWITCHES) -o $@ $^ $(MATH_LIBS)

#=== Test ===================================================================

test: tcltest$(DOTEXE)
	( cd tests && ../tcltest all )

tcltest$(DOTEXE): tclTest.$(OBJ) libtcl.$(LIBEXT)
	$(CC) $(CC_SWITCHES) tclTest.$(OBJ) libtcl.$(LIBEXT) $(MATH_LIBS) -o tcltest$(DOTEXE)

#=== Install ================================================================

install: $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(BIN_PROGS)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) libtcl.$(LIBEXT) $(BIN_PROGS) tcltest$(DOTEXE)

# end of file
