#
# mouse:~ppr/src/ppr/Makefile
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

#=== Inventory ==============================================================

PROGS=ppr$(DOTEXE) tbcp2bin$(DOTEXE)

#=== Build ==================================================================

all: $(PROGS)

ppr$(DOTEXE): \
		ppr_main.o ppr_infile.o ppr_outfile.o \
		ppr_media.o ppr_rcache.o ppr_req.o \
		ppr_res.o ppr_old2new.o ppr_dscdoc.o \
		ppr_respond.o ppr_split.o ppr_conffile.o \
		ppr_nest.o ppr_things.o \
		ppr_simplify.o ppr_editps.o \
		ppr_features.o \
		../libppr.a ../libgu.a ../libpprdb.a
	$(LD) $(LDFLAGS) -o $@ $^ $(DBLIBS) $(INTLLIBS) $(ZLIBLIBS)
	$(CHMOD) 4755 $@

#=== Install ================================================================

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 6755 $(BINDIR) ppr$(DOTEXE)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR)  755 $(LIBDIR) tbcp2bin$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) $(BACKUPS) *.o $(PROGS)

# end of file
