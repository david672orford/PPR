#! /bin/sh
#
# mouse:~ppr/src/po/extract_to_pot.sh
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
# Last modified 26 January 2005.
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
	../ppr-sysv/*.c \
	../ppr-bsd/*.c \
	../pprdrv/pprdrv*.c
#./sort_by_file.perl PPR.po PPR.pot
mv PPR.po PPR.pot

echo "PPRD.pot"
xgettext --default-domain=PPRD --keyword=_ --keyword=N_ --sort-output \
	../pprd/pprd*.c
./sort_by_file.perl <PPRD.po >PPRD.pot
rm PPRD.po

echo "PAPD.pot"
xgettext --default-domain=PAPD --keyword=_ --keyword=N_ --sort-output \
	../papd/papd*.c
./sort_by_file.perl <PAPD.po >PAPD.pot
rm PAPD.po

echo "PPRWWW.pot"
./perl_to_pseudo_c.perl ../www/*.perl ../www/*.pl \
	| xgettext --default-domain=PPRWWW --keyword=_ --keyword=N_ --keyword=H_ --keyword=H_NB_ --sort-output --language=c - \
	2>&1 | grep -v "unterminated "
./sort_by_file.perl <PPRWWW.po >PPRWWW.pot
rm PPRWWW.po

echo "Done."

exit 0

