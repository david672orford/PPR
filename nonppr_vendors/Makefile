#
# mouse:~ppr/src/vendors/Makefile
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 22 November 2000.
#

include ../Makefile.conf

PPD_DESTDIR=$(SHAREDIR)/PPDFiles
PROCSET_DESTDIR=$(SHAREDIR)/cache/procset
FONT_DESTDIR=$(SHAREDIR)/fonts

all:

install:
	$(INSTALLDATA) $(PPD_DESTDIR) \
			ppd_adobe/*[!~] \
			ppd_apple/*[!~] \
			ppd_hp/*[!~] \
			ppd_canon/*[!~] \
			ppd_ricoh/*[!~]
	$(INSTALLDATA) $(PROCSET_DESTDIR) \
			procset_adobe/*[!~] \
			procset_apple/*[!~]
	$(INSTALLDATA) $(FONT_DESTDIR) \
			font_adobe/*.afm \
			font_ibm/*.afm font_ibm/*.pf[ab] \
			font_paragraph/*.afm font_paragraph/*.pf[ab]

depend:

clean:
	$(RMF) $(BACKUPS)

# end of file

