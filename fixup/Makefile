#
# mouse:~ppr/src/tools4fixup/Makefile
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
# Last modified 22 November 2000.
#

include ../makeprogs/global.mk

FIXUP_PROGS=fixup \
	fixup_accounts \
	fixup_conf \
	fixup_cron \
	fixup_filters \
	fixup_init \
	fixup_links \
	fixup_lmx \
	fixup_obsolete \
	fixup_perms \
	fixup_samples \
	fixup_inetd \
	fixup_media

all: $(FIXUP_PROGS)

install: $(FIXUP_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/fixup $(FIXUP_PROGS)

depend:

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(FIXUP_PROGS)

# end of file
