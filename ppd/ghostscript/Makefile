#
# mouse:~ppr/src/ppd/ghostscript/Makefile
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
# Last modified 15 December 2004.
#

include ../../Makefile.conf

#=== Inventory ==============================================================

PPDS=hpiiigs.ppd hpdj5gs.ppd dm24gs.ppd hpdj550c.ppd epsc62gs.ppd

#=== Build ==================================================================

all: $(PPDS)

hpiiigs.ppd: ghostscript_master Makefile
	$(CPP) -DPRINTER_HPIII ghostscript_master >hpiiigs.ppd

hpdj5gs.ppd: ghostscript_master Makefile
	$(CPP) -DPRINTER_HPDJ500 ghostscript_master >hpdj5gs.ppd

dm24gs.ppd: ghostscript_master Makefile
	$(CPP) -DPRINTER_DM24 ghostscript_master >dm24gs.ppd

hpdj550c.ppd: ghostscript_master Makefile
	$(CPP) -DPRINTER_HPDJ550C ghostscript_master >hpdj550c.ppd

epsc62gs.ppd: ghostscript_master Makefile
	$(CPP) -DPRINTER_EPSC62 ghostscript_master >epsc62gs.ppd

#=== Install ================================================================

install: all
	../$(INSTALLCP) hpiiigs.ppd $(SHAREDIR)/PPDFiles/'HP LaserJet III Ghostscript'
	../$(INSTALLCP) hpdj5gs.ppd $(SHAREDIR)/PPDFiles/'HP DeskJet 500 Ghostscript'
	../$(INSTALLCP) dm24gs.ppd $(SHAREDIR)/PPDFiles/'NEC Pinwriter 6 Ghostscript'
	../$(INSTALLCP) hpdj550c.ppd $(SHAREDIR)/PPDFiles/'HP DeskJet 550C Ghostscript'
	../$(INSTALLCP) epsc62gs.ppd $(SHAREDIR)/PPDFiles/'Epson Stylus C62 Ghostscript'

#=== Housekeeping ===========================================================

clean:
	$(RMF) $(BACKUPS) *.ppd

# end of file

