#
# mouse:~ppr/src/ppd/Makefile
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
# Last modified 18 April 2005.
#

include ../Makefile.conf

#=== Inventory ==============================================================

BIN_PROGS=
LIB_PROGS=indexppds$(DOTEXE)
PROGS=$(BIN_PROGS) $(LIB_PROGS)

#=== Build ==================================================================

all: $(PROGS)
	( cd ghostscript; $(MAKE) )
	( cd generic; $(MAKE) )

indexppds$(DOTEXE): indexppds.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT)
	$(LD) $(LDFLAGS) -o $@ $^ $(INTLLIBS) $(ZLIBLIBS)

#=== Install ================================================================

install: $(PROGS)
	#$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(BINDIR) $(BIN_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(LIBDIR) $(LIB_PROGS)
	( cd ghostscript; $(MAKE) install )
	$(INSTALLDATA) $(PPDDIR) other/*
	( cd generic; $(MAKE) install )
	$(INSTALLDATA) $(MISCDIR) pagesizes.conf

#=== Housekeeping ===========================================================

depend:

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(PROGS)
	( cd ghostscript; $(MAKE) clean )
	( cd generic; $(MAKE) clean )
	( cd other; $(RMF) $(BACKUPS) )

# end of file
