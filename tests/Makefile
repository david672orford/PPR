#
# mouse:~ppr/src/tests/Makefile
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
# Last modified 8 May 2001.
#

include ../Makefile.conf

#=== Inventory ==============================================================

GROUPS=test-ppr test-pprdrv test-consistency test-filters test-interface test-ppad

#=== Build ==================================================================

all:
	./do_tests $(GROUPS)

#=== Housekeeping ===========================================================

depend:

clean:
	$(RMF) $(BACKUPS)
	(cd tools; $(RMF) $(BACKUPS))
	(cd misc_old; $(RMF) $(BACKUPS) *.out)
	for d in $(GROUPS); do echo $$d; (cd $$d; $(RMF) $(BACKUPS) *.out); done

# end of file
