#
# mouse:~ppr/src/encodings/Makefile
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

include ../Makefile.conf

all:

ENCODINGS=PPR-ASCII \
	ISOLatin1Encoding \
	PPR-ISOLatin2 \
	PPR-ISOLatin3 \
	PPR-ISOLatin4 \
	PPR-ISOLatin5 \
	PPR-KOI8-R \
	PPR-CP437 \
	PPR-CP1250 \
	PPR-Macintosh

install: charsets.conf $(ENCODINGS)
	$(INSTALLDATA) $(SHAREDIR)/lib charsets.conf
	for enc in $(ENCODINGS);\
		do\
		$(INSTALLCP) --squeeze $$enc $(SHAREDIR)/cache/encoding/$$enc;\
		done

depend:

clean:
	$(RMF) $(BACKUPS)

# end of file
