#
# mouse:~ppr/src/po/Makefile
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
# Last modified 8 January 2001.
#

include ../makeprogs/global.mk
include ../makeprogs/paths.mk

LANGUAGES=fr ru

all:

install: $(INTERNATIONAL_INSTALL)

install-international:
	for lang in $(LANGUAGES); do ./install_mo.sh $$lang $(LOCALEDIR); done

depend:

clean:
	$(RMF) $(BACKUPS) *.mo

# end of file



