#
# mouse:~ppr/src/editps/Makefile
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
# Last modified 4 September 2002.
#

include ../Makefile.conf

EDITORS=Windows_NT_4.0 Windows_95_4.0 \
	Adobe_NT_5.0 ADOBEPS4.DRV_Version_4.24 \
	MATLAB \
	dvipsk_5.58f \
	LaserWriter_8_8.4 \
	Mozilla_1.0

all:

install: editps.conf $(EDITORS)
	$(INSTALLDATA) $(HOMEDIR)/editps editps.conf
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/editps $(EDITORS)

depend:

clean:
	$(RMF) $(BACKUPS)

# end of file
