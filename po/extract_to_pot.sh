#! /bin/sh
#
# mouse:~ppr/src/po/extract_to_pot.sh
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 30 October 2001.
#

echo "Extracting master string lists..."

echo "PPR.pot"
xgettext --default-domain=PPR --keyword=_ --keyword=N_ --sort-output  \
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
./sort_by_file.perl <PPRDRV.po >PPRDRV.pot
rm PPRDRV.po

echo "PPRD.pot"
xgettext --default-domain=PPRD --keyword=_ --keyword=N_ --sort-output \
	../pprd/pprd*.c
./sort_by_file.perl <PPRD.po >PPRD.pot
rm PPRD.po

echo "PAPSRV.pot"
xgettext --default-domain=PAPSRV --keyword=_ --keyword=N_ --sort-output \
	../papsrv/papsrv*.c
./sort_by_file.perl <PAPSRV.po >PAPSRV.pot
rm PAPSRV.po

echo "PPRWWW.pot"
./perl_to_pseudo_c.perl ../www/*.perl ../www/*.pl \
	| xgettext --default-domain=PPRWWW --keyword=_ --keyword=N_ --keyword=H_ --keyword=H_NB_ --sort-output --language=c -
./sort_by_file.perl <PPRWWW.po >PPRWWW.pot
rm PPRWWW.po

echo "Done."

exit 0

