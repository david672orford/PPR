#
# mouse:~ppr/src/po/Makefile
# Copyright 1995--2007, Trinity College Computing Center.
# Written by David Chappell.
#
# This file is part of PPR.  You can redistribute it and modify it under the
# terms of the revised BSD licence (without the advertising clause) as
# described in the accompanying file LICENSE.txt.
#
# Last modified 1 June 2007.
#

include ../Makefile.conf
include ../makeprogs/paths.mk

#=== Inventory ==============================================================

# Run "make depend" if you change these.
LANGUAGES=fr_FR ru_RU de_DE
DIVISIONS=PPR PPRD PPRWWW PAPD

#=== Build ==================================================================

# There are no depencies listed here.  They are all listed
# in .depend.

# Disabled because it cause the translations to change a lot and create lots of merge
# conflicts when there is no translator around.
all: #$(INTERNATIONAL_ALL)

all-international:

PPR.pot: pot
PPRD.pot: pot
PPRWWW.pot: pot
PPRPAPD.pot: pot

# Extract the lists of translatable strings.
pot:
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
		../ppad/ppad*.c ../ppad/dispatch_table.c \
		../ppop/ppop*.c \
		../ppr/ppr*.c \
		../ppuser/*.c \
		../samba/*.c \
		../unixuser/*.c \
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
		--keyword=_ --keyword=N_ --keyword=H_ --keyword='\H_' --keyword=H_NB_ --keyword='\H_NB_' \
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
			mv -f $$division.tmp $$division.pot; \
			fi \
		done

#=== Install ================================================================

install: $(INTERNATIONAL_INSTALL)

install-international: all-international
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
			echo "all-international: $$language-$$division.po"; \
			echo; \
			done \
		done >.depend

clean:
	$(RMF) $(BACKUPS) *.mo

veryclean: clean
	$(RMF) *.pot

# end of file
