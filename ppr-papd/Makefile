#
# mouse:~src/ppr/ppr-papd/Makefile
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 4 January 2002.
#

include ../makeprogs/global.mk

#=== Inventory ==============================================================

BIN_PROGS=ppr-papd$(DOTEXE)

#=== Build ==================================================================

# If we aren't building with AppleTalk support, then ALLATALK will be 
# undefined and this will do nothing.  If we are building with AppleTalk
# support then it will be set to "allatalk".
all: $(ALLATALK)

# List if things to build if we have AppleTalk.
allatalk: $(BIN_PROGS)

# This is the server.
OBJS1=ppr-papd.$(OBJ) \
	ppr-papd_$(ATALKTYPE).$(OBJ) \
	ppr-papd_query.$(OBJ) \
	ppr-papd_conf.$(OBJ) \
	ppr-papd_ppd_parse.$(OBJ) \
	ppr-papd_authorize.$(OBJ) \
	../libppr.$(LIBEXT) \
	../libgu.$(LIBEXT) \
	../libpprdb.$(LIBEXT)
ppr-papd$(DOTEXE): $(OBJS1)
	$(LD) $(LDFLAGS) -o $@ $(OBJS1) $(LIB_LEX) $(ATALKLIBS) $(DBLIBS)
	$(CHMOD) 6755 ppr-papd$(DOTEXE)

# This one module might need extra flags to find the AppleTalk library 
# include files.
ppr-papd_$(ATALKTYPE).$(OBJ): ppr-papd_$(ATALKTYPE).c
	$(CC) $(ATALKFLAGS) $(CFLAGS) -c ppr-papd_$(ATALKTYPE).c

# How to build the C source for the PPD file lexer.
ppr-papd_ppd_parse.c: ppr-papd_ppd_parse.l
	$(LEX) -v ppr-papd_ppd_parse.l
	mv lex.yy.c ppr-papd_ppd_parse.c

#=== Install ================================================================

# The install target.  This also does nothing if we
# don't have AppleTalk.
install: $(INSTALLATALK)

# List if things to install if we have AppleTalk.
installatalk: $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 4755 $(HOMEDIR)/bin ppr-papd

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(BIN_PROGS)

veryclean: clean
	$(RMF) ppr-papd_ppd_parse.c

# end of file

