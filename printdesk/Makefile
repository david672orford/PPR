#
# mouse:~ppr/src/printdesk/Makefile
# Copyright 1995--2003, Trinity College Computing Center.
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
# Last modified 30 June 2003.
#

include ../Makefile.conf

#=== Inventory ==============================================================

PROGS=ppr-panel

ATALK_PROGS=nbp_lookup$(DOTEXE) ppr-chooser

PRINTDESK_SUB_SCRIPTS=ATchooser.pm BatchDialog.pm \
	GENfolder.pm GENjobbuttons.pm GENprnbuttons.pm \
	PPRlistqueue.pm PPRprintdialog.pm \
	PPRprnstatus.pm PPRstateupdate.pm \
	group.xbm printer.xbm

#=== Build ==================================================================

all: $(PROGS) $(ALL_ATALK)

all-atalk: $(ATALK_PROGS)

nbp_lookup$(DOTEXE): nbp_lookup_$(ATALKTYPE).$(OBJ) ../libgu.$(LIBEXT)
	$(LD) $(LDFLAGS) -o nbp_lookup nbp_lookup_$(ATALKTYPE).$(OBJ) ../libgu.$(LIBEXT) $(ATALKLIBS)

#=== Install ================================================================

install: $(PROGS) PrintDesk.pm $(PRINTDESK_SUB_SCRIPTS) $(INSTALL_ATALK)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin $(PROGS)
	$(INSTALLDATA) $(HOMEDIR)/lib PrintDesk.pm
	$(MAKE_NEW_DIR) $(HOMEDIR)/lib/PrintDesk
	$(INSTALLDATA) $(HOMEDIR)/lib/PrintDesk $(PRINTDESK_SUB_SCRIPTS)

install-atalk: $(ATALK_PROGS)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/bin ppr-chooser
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(HOMEDIR)/lib nbp_lookup$(DOTEXE)

#=== Housekeeping ===========================================================

include .depend

clean:
	$(RMF) $(BACKUPS) *.$(OBJ) $(PROGS) $(ATALK_PROGS)

depend:
	$(PPR_MAKE_DEPEND) ../include

# end of file
