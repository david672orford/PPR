#
# mouse:~ppr/src/misc/Makefile
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
# Last modified 3 April 2001.
#

include ../makeprogs/global.mk

#=== Inventory ==============================================================

PROGS=ppr-sync \
	customs.ppr \
	sgi_glp_hack \
	remove_ppr ppd2macosdrv custom_hook_docutech xmessage

#=== Build ==================================================================

all: $(PROGS)

xmessage: xmessage.tcl
	cp xmessage.tcl xmessage
	$(CHMOD) 755 xmessage

#=== Install ================================================================

install: $(PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin ppr-sync ppd2macosdrv
	if [ `uname -s` = "IRIX" ];\
		then\
		$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin sgi_glp_hack;\
		fi
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib custom_hook_docutech xmessage
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/fixup customs.ppr remove_ppr

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(PROGS)

# end of file
