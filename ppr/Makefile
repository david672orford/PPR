#
# mouse:~ppr/src/ppr/Makefile
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
# Last modified 3 August 2003.
#

include ../Makefile.conf

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
	ppr_features.$(OBJ) \
	../libppr.$(LIBEXT) ../libgu.$(LIBEXT) ../libpprdb.$(LIBEXT)
ppr$(DOTEXE): $(OBJS1)
	$(LD) $(LDFLAGS) -o $@ $(OBJS1) $(DBLIBS) $(INTLLIBS) $(ZLIBLIBS)
	$(CHMOD) 4755 $@

ppr_infile.$(OBJ): ppr_infile.c
	$(CC) $(CFLAGS) $(UNCOMPRESS_OPTS) -c ppr_infile.c

#=== Install ================================================================

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6755 $(HOMEDIR)/bin ppr$(DOTEXE)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR)  755 $(HOMEDIR)/lib tbcp2bin$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(PROGS)

# end of file
