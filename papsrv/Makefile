#
# mouse:~src/ppr/papsrv/Makefile
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 11 February 2001.
#

include ../makeprogs/global.mk

#=== Inventory ==============================================================

BIN_PROGS=papsrv$(DOTEXE) papsrv-kill

#=== Build ==================================================================

# If we aren't building with AppleTalk support, then ALLATALK will be 
# undefined and this will do nothing.  If we are building with AppleTalk
# support then it will be set to "allatalk".
all: $(ALLATALK)

# List if things to build if we have AppleTalk.
allatalk: $(BIN_PROGS)

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
install: $(INSTALLATALK)

# List if things to install if we have AppleTalk.
installatalk: $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 4755 $(HOMEDIR)/bin papsrv
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR)  755 $(HOMEDIR)/bin papsrv-kill

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(BIN_PROGS)

veryclean: clean
	$(RMF) papsrv_ppd_parse.c

# end of file

