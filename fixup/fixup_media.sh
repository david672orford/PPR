#! /bin/sh
#
# mouse:~ppr/src/fixup/fixup_media.sh
# Copyright 1995--2000, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is"
# without express or implied warranty.
#
# Last modified 13 September 2000.
#

#
# This script initializes the PPR media database.
#
# European media sizes are underrepresented here.  If people will contribute
# definitions, I will be happy to add them.
#

HOMEDIR="?"
CONFDIR="?"
USER_PPR=?

echo "Creating base media database entries..."

if [ -f "$CONFDIR/media" ]
    then
    mv "$CONFDIR/media" "$CONFDIR/media.db"
    fi

touch "$CONFDIR/media.db"
chown $USER_PPR "$CONFDIR/media.db"

PATH="$HOMEDIR/bin:$PATH"

# White paper in various sizes and weights:
ppad media put "letter" "8.5 in" "11.0 in" 75.0 "white" "" 7
ppad media put "a4" "210 mm" "297 mm" 75.0 "white" "" 7
ppad media put "master" "8.5 in" "11.0 in" 300.0 "white" "" 2
ppad media put "legal" "8.5 in" "14 in" 75.0 "white" "" 5

# Junk paper for banner pages:
# Stupid colour field value may be important.  We don't want
# it every selected as a match for some real media requirement.
ppad media put "banner" "612.0 psu" "792.0 psu" 0 "VARIES" "" 10

# other stuff
ppad media put "3hole" "612.0 psu" "792.0 psu" 0.0 "white" "3Hole" 6
ppad media put "labels" "612.0 psu" "792.0 psu" 0.0 "white" "Labels" 1

# envelopes
ppad media put "comm10" "297.0 psu" "684.0 psu" 0.0 "white" "Envelope" 1
ppad media put "monarch" "540.0 psu" "279.0 psu" 0.0 "white" "Envelope" 1

# normal paper of different colours
ppad media put "yellow" "612.0 psu" "792.0 psu" 75.0 "yellow" "" 5
ppad media put "red" "612.0 psu" "792.0 psu" 75.0 "yellow" "" 5
ppad media put "blue" "612.0 psu" "792.0 psu" 75.0 "yellow" "" 5
ppad media put "green" "612.0 psu" "792.0 psu" 75.0 "yellow" "" 5
ppad media put "pink" "612.0 psu" "792.0 psu" 75.0 "yellow" "" 5

# various kinds of letterhead
ppad media put "letterhead" "612.0 psu" "792.0 psu" 0.0 "white" "DeptLetterHead" 3
ppad media put "custletterhead" "612.0 psu" "792.0 psu" 0.0 "white" "CustLetterHead" 3
ppad media put "userletterhead" "612.0 psu" "792.0 psu" 0.0 "white" "UserLetterHead" 3
ppad media put "halfletterhead" "396.0 psu" "612.0 psu" 0.0 "white" "CustLetterHead" 3
ppad media put "executivelethead" "522.0 psu" "756.0 psu" 0.0 "white" "DeptLetterHead" 3

echo "Done."
echo

# end of file
