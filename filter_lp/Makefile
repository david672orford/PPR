#
# mouse:~ppr/src/filter_lp/Makefile
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
# Last modified 14 May 2003.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS=filter_lp$(DOTEXE)

#=== Build ==================================================================

all: $(PROGS)

filter_lp$(DOTEXE): filter_lp.$(OBJ) ../libgu.$(LIBEXT) ../libppr.$(LIBEXT)
	$(LD) $(LDFLAGS) -o filter_lp$(DOTEXT) filter_lp.$(OBJ) ../libppr.$(LIBEXT) ../libgu.$(LIBEXT) $(INTLLIBS)

#=== Install ================================================================

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/filters filter_lp$(DOTEXE)
	$(INSTALLLN) filter_lp$(DOTEXE) $(HOMEDIR)/filters/filter_lp_autolf$(DOTEXE)
	$(INSTALLLN) filter_lp$(DOTEXE) $(HOMEDIR)/filters/filter_fortran$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file
