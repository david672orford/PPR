#
# mouse:~ppr/src/ppr/Makefile
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
# Last modified 4 September 2001.
#

include ../makeprogs/global.mk

#=== Inventory ==============================================================

PROGS=ppr$(DOTEXE) tbcp2bin$(DOTEXE)

#=== Build ==================================================================

all: $(PROGS)

OBJS1=ppr_main.$(OBJ) ppr_infile.$(OBJ) ppr_outfile.$(OBJ) \
	ppr_media.$(OBJ) ppr_rcache.$(OBJ) ppr_req.$(OBJ) \
	ppr_res.$(OBJ) ppr_old2new.$(OBJ) ppr_dscdoc.$(OBJ) \
	ppr_respond.$(OBJ) ppr_split.$(OBJ) ppr_conffile.$(OBJ) \
	ppr_mactt.$(OBJ) ppr_nest.$(OBJ) ppr_things.$(OBJ) \
	ppr_simplify.$(OBJ) ppr_editps.$(OBJ) \
	../libppr.$(LIBEXT) ../libgu.$(LIBEXT) ../libpprdb.$(LIBEXT)
ppr$(DOTEXE): $(OBJS1)
	$(LD) $(LDFLAGS) -o $@ $(OBJS1) $(DBLIBS) $(INTLLIBS)
	$(CHMOD) 4755 $@

ppr_infile.$(OBJ): ppr_infile.c
	$(CC) $(CFLAGS) $(UNCOMPRESS_OPTS) -c ppr_infile.c

#=== Install ================================================================

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6711 $(HOMEDIR)/bin ppr$(DOTEXE)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR)  711 $(HOMEDIR)/lib tbcp2bin$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(PROGS)

# end of file
