#
# mouse:~src/ppr/papsrv/Makefile
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 19 February 2003.
#

include ../Makefile.conf

#=== Inventory ==============================================================

BIN_PROGS=papsrv$(DOTEXE)

#=== Build ==================================================================

# If we aren't building with AppleTalk support, then ALLATALK will be 
# undefined and this will do nothing.  If we are building with AppleTalk
# support then it will be set to "allatalk".
all: $(ALL_ATALK)

# List if things to build if we have AppleTalk.
all-atalk: $(BIN_PROGS)

# This is the server.
OBJS1=papsrv.$(OBJ) \
	papsrv_$(ATALKTYPE).$(OBJ) \
	papsrv_query.$(OBJ) \
	papsrv_conf.$(OBJ) \
	papsrv_ppd_parse.$(OBJ) \
	papsrv_authorize.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT) \
	../libpprdb.$(LIBEXT)
papsrv$(DOTEXE): $(OBJS1)
	$(LD) $(LDFLAGS) -o $@ $(OBJS1) $(LIB_LEX) $(ATALKLIBS) $(DBLIBS)
	$(CHMOD) 6755 papsrv$(DOTEXE)

# This one module might need extra flags to find the AppleTalk library 
# include files.
papsrv_$(ATALKTYPE).$(OBJ): papsrv_$(ATALKTYPE).c
	$(CC) $(ATALKFLAGS) $(CFLAGS) -c papsrv_$(ATALKTYPE).c

# How to build the C source for the PPD file lexer.
papsrv_ppd_parse.c: papsrv_ppd_parse.l
	$(LEX) -v papsrv_ppd_parse.l
	mv lex.yy.c papsrv_ppd_parse.c

#=== Install ================================================================

# The install target.  This also does nothing if we
# don't have AppleTalk.
install: $(INSTALL_ATALK)

# List if things to install if we have AppleTalk.
install-atalk: $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 4755 $(HOMEDIR)/bin papsrv$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(BIN_PROGS)

veryclean: clean
	$(RMF) papsrv_ppd_parse.c

# end of file
