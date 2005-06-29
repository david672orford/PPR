#
# mouse:~ppr/src/po/Makefile
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 22 June 2005.
#

include ../Makefile.conf
include ../makeprogs/paths.mk

#=== Inventory ==============================================================

# Run "make depend" if you change these.
LANGUAGES=fr_FR ru_RU de_DE
DIVISIONS=PPR PPRD PPRWWW PAPD

#=== Build ==================================================================

# There are not depencies listed here.  They are all listed
# in .depend.
all:

# Extract the lists of translatable strings.  The rules are a bit 
# fudged.  If we try to extract one list, we get them all.
PPRD.pot: PPR.pot
PAPD.pot: PPR.pot
PPRWWW.pot: PPR.pot
PPR.pot:
	xgettext \
		--default-domain=PPR \
		--output=PPR.tmp \
		--keyword=_ --keyword=N_ \
		--sort-output \
		--no-wrap \
		../filter*/*.c \
		../fontutils/*.c \
		../interfaces/*.c \
		../lib*/*.c \
		../lprsrv/*.c \
		../ppad/ppad*.c \
		../ppop/ppop*.c \
		../ppr/ppr*.c \
		../ppuser/*.c \
		../samba/*.c \
		../unixuser/*.c \
		../libuprint/*.c \
		../ppr-sysv/*.c \
		../ppr-bsd/*.c \
		../pprdrv/pprdrv*.c
	xgettext \
		--default-domain=PPRD \
		--output=PPRD.tmp \
		--keyword=_ --keyword=N_ \
		--sort-output \
		--no-wrap \
		../pprd/pprd*.c
	xgettext --default-domain=PAPD \
		--output=PAPD.tmp \
		--keyword=_ --keyword=N_ \
		--sort-output \
		--no-wrap \
		../papd/papd*.c
	xgettext \
		--debug \
		--default-domain=PPRWWW \
		--output=PPRWWW.tmp \
		--keyword=_ --keyword=N_ --keyword=H_ --keyword='\H_' --keyword=H_NB_ \
		--sort-output \
		--no-wrap \
		--language=Perl \
		../www/*.perl ../www/*.pl
	@for division in $(DIVISIONS); \
		do \
		perl -p -i -e 's/charset=CHARSET/charset=iso-8859-1/g' $$division.tmp; \
		if diff $$division.tmp $$division.pot >/dev/null; \
			then \
			echo "No changes to $$division."; \
			rm $$division.tmp; \
			else \
			echo "Changes found in $$division."; \
			mv $$division.tmp $$division.pot; \
			fi \
		done

#=== Install ================================================================

install: $(INTERNATIONAL_INSTALL)

install-international: all
	for lang in $(LANGUAGES); \
		do \
		./install_mo.sh $$lang $(LOCALEDIR) || exit 1; \
		done

#=== Housekeeping ===========================================================

include .depend

depend:
	@for language in $(LANGUAGES); \
		do \
		for division in $(DIVISIONS); \
			do \
			echo "$$language-$$division.po: $$division.pot"; \
			echo '	msgmerge --update --no-wrap --sort-by-file --backup=numbered $$@ $$^ && touch $$@'; \
			echo "all: $$language-$$division.po"; \
			done \
		done >.depend

clean:
	$(RMF) $(BACKUPS) *.mo

veryclean: clean
	$(RMF) *.pot

# end of file
