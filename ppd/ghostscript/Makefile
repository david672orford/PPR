#
# mouse:~ppr/src/ppd/ghostscript/Makefile
# Copyright 1995--2002, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 7 August 2002.
#

include ../../Makefile.conf

#========================================================================
# Choose the list of fonts which should be included in Ghostscript
# PPD files.  Some common settings are given.  More details can be found
# in the comments at the start of the file "ghostscript_master".
#========================================================================

# Say Ghostscript has 14 fonts from Adobe Acrobat reader.
#FONTS=-DFONTS_TO13 -DFONTS_DINGBATS

# Say Ghostscript has 35 fonts typically found in PostScript printers
# and the URW fonts (for which the normal 35 are actually aliases).
FONTS=-DFONTS_TO35 -DFONTS_GHOSTSCRIPT

#========================================================================

all: hpiiigs.ppd hpdj5gs.ppd dm24gs.ppd hpdj550c.ppd

hpiiigs.ppd: ghostscript_master Makefile
	$(CPP) -DPRINTER_HPIII $(FONTS) ghostscript_master >hpiiigs.ppd

hpdj5gs.ppd: ghostscript_master Makefile
	$(CPP) -DPRINTER_HPDJ500 $(FONTS) ghostscript_master >hpdj5gs.ppd

dm24gs.ppd: ghostscript_master Makefile
	$(CPP) -DPRINTER_DM24 $(FONTS) ghostscript_master >dm24gs.ppd

hpdj550c.ppd: ghostscript_master Makefile
	$(CPP) -DPRINTER_HPDJ550C $(FONTS) ghostscript_master >hpdj550c.ppd

install: all
	../$(INSTALLCP) hpiiigs.ppd $(SHAREDIR)/PPDFiles/'HP LaserJet III Ghostscript'
	../$(INSTALLCP) hpdj5gs.ppd $(SHAREDIR)/PPDFiles/'HP DeskJet 500 Ghostscript'
	../$(INSTALLCP) dm24gs.ppd $(SHAREDIR)/PPDFiles/'NEC Pinwriter 6 Ghostscript'
	../$(INSTALLCP) hpdj550c.ppd $(SHAREDIR)/PPDFiles/'HP DeskJet 550C Ghostscript'

clean:
	$(RMF) $(BACKUPS) *.ppd

# end of file

