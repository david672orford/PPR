#! /bin/sh
#
# mouse:~ppr/src/po/extract_to_pot.sh
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 29 June 2000.
#

echo "Extracting master string lists..."

echo "PPR.pot"
xgettext --default-domain=PPR --keyword=_ --keyword=N_ --sort-output \
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
	../uprint/*.c
mv PPR.po PPR.pot

echo "PPRDRV.pot"
xgettext --default-domain=PPRDRV --keyword=_ --keyword=N_ --sort-output \
	../pprdrv/pprdrv*.c
mv PPRDRV.po PPRDRV.pot

echo "PPRD.pot"
xgettext --default-domain=PPRD --keyword=_ --keyword=N_ --sort-output \
	../pprd/pprd*.c
mv PPRD.po PPRD.pot

echo "PAPSRV.pot"
xgettext --default-domain=PAPSRV --keyword=_ --keyword=N_ --sort-output \
	../papsrv/papsrv*.c
mv PAPSRV.po PAPSRV.pot

echo "PPRWWW.pot"
./perl_to_pseudo_c.perl ../www/*.perl ../www/*.pl \
	| xgettext --default-domain=PPRWWW --keyword=_ --keyword=N_ --keyword=H_ --sort-output --language=c -
mv PPRWWW.po PPRWWW.pot

echo "Done."

exit 0

