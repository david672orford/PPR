#
# mouse:~ppr/src/procsets/Makefile
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
# Last modified 21 June 2001.
#

#
# There is a problem here, namely, make can not handle
# spaces in file names.
#

include ../makeprogs/global.mk

PROGS=

SQUEEZEME=TrinColl-PPR-dmm-nup-3-6 \
	TrinColl-PPR-ReEncode-1.1-0 \
	TrinColl-PPR-NewMetrics-1-0

all: $(PROGS)

install: $(PROGS) $(SQUEEZEME)
	for i in $(SQUEEZEME);\
	    do\
	    $(INSTALLCP) --squeeze $$i $(SHAREDIR)/cache/procset/$$i;\
	    done

depend:

clean:
	$(RMF) $(BACKUPS) $(PROGS)

# end of file

