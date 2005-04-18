#
# mouse:~ppr/src/ppd/generic/Makefile
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

include ../../Makefile.conf

#=== Inventory ==============================================================

PPDS=pprgener.ppd pprgene2.ppd pprgene3.ppd

#=== Build ==================================================================

all: $(PPDS)

pprgener.ppd: generic_master
	$(CPP) -DGENERIC_LEVEL1 generic_master >pprgener.ppd

pprgene2.ppd: generic_master
	$(CPP) -DGENERIC_LEVEL2 generic_master >pprgene2.ppd

pprgene3.ppd: generic_master
	$(CPP) -DGENERIC_LEVEL2_TTF generic_master >pprgene3.ppd

#=== Install ================================================================

install: all
	../$(INSTALLDATA) $(PPDDIR) $(PPDS)

#=== Housekeeping ===========================================================

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PPDS)

# end of file

