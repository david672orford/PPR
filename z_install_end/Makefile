#
# mouse:~ppr/src/z_install_end/Makefile
# Copyright 1995--2006, Trinity College Computing Center.
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
# Last modified 11 April 2006.
#

include ../Makefile.conf

#=== Inventory ==============================================================

ALL=ppr.conf.sample puts$(DOTEXE) cron.d init.d

#=== Build ==================================================================

all: $(ALL)

ppr.conf.sample: make_ppr_conf_sample.sh
	./make_ppr_conf_sample.sh

#=== Install ================================================================

install: $(ALL)
	$(INSTALLPROGS) $(USER_PPR) $(GROUP_PPR) 755 $(CONFDIR) ppr.conf.sample
	./install_sample_config.sh
	./install_bin_links.sh
	[ -z "$(DH_OPTIONS)" ] && ./install_init_script.sh || true
	[ -z "$(RPM_BUILD_ROOT)" ] && $(MAKE) install-real || true

# These are operations which are defered if we are building a package
# rather than actually installing the program.
install-real:
	#
	# Create PPR's crontab.
	./install_crontab.sh
	#
	# Initialize the media database.
	$(PPAD_PATH) media import $(CONFDIR)/media.sample >/dev/null
	#
	# Create indexes of fonts, PPD files, and filters.
	$(BINDIR)/ppr-index

#=== Housekeeping ===========================================================

include .depend

depend:
	$(PPR_MAKE_DEPEND) ../include

clean:
	$(RMF) *.o $(BACKUPS) $(ALL)

# end of file
