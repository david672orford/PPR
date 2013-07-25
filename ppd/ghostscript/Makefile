#
# mouse:~ppr/src/ppd/ghostscript/Makefile
# Copyright 1995--2012, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified: 31 October 2012
#

include ../../Makefile.conf

#=== Inventory ==============================================================

PPDS=hpiiigs.ppd hpdj5gs.ppd dm24gs.ppd hpdj550c.ppd epsc62gs.ppd bromfc.ppd

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

bromfc.ppd: ghostscript_master Makefile
	$(CPP) -DPRINTER_BROMFC ghostscript_master >bromfc.ppd

#=== Install ================================================================

install: all
	../$(INSTALLDATA) $(PPDDIR) $(PPDS)

#=== Housekeeping ===========================================================

clean:
	$(RMF) $(BACKUPS) *.ppd

# end of file

