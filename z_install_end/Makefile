#
# mouse:~ppr/src/templates/Makefile
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
# Last modified 2 August 2003.
#

include ../Makefile.conf

#=== Inventory ==============================================================

ALL=ppr.conf.sample getservbyname$(DOTEXE) puts$(DOTEXE) ppr

#=== Build ==================================================================

all: $(ALL)

ppr.conf.sample: make_ppr_conf_sample.sh
	./make_ppr_conf_sample.sh

#=== Install ================================================================

install: $(ALL)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(CONFDIR) ppr.conf.sample
	./install_sample_config.sh
	./install_atalk_links.tcl
	[ -z "$(RPM_BUILD_ROOT)" ] && ./install_crontab.sh || true
	./install_bin_links.sh
	./install_inetd_conf.sh
	./install_init_script.sh
	#./install_alternatives.tcl
	[ -z "$(RPM_BUILD_ROOT)" ] && ./fixup_obsolete.sh || true
	[ -z "$(RPM_BUILD_ROOT)" ] && touch $(CONFDIR)/media.db \
		&& chown $(USER_PPR) $(CONFDIR)/media.db \
		&& chgrp $(GROUP_PPR) $(CONFDIR)/media.db \
		&& $(HOMEDIR)/bin/ppad media import $(CONFDIR)/media.sample >/dev/null \
		|| true
	[ -z "$(RPM_BUILD_ROOT)" ] && $(HOMEDIR)/bin/ppr-index >/dev/null || true

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.$(OBJ) $(BACKUPS) $(ALL)

# end of file
