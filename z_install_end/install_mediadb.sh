#! /bin/sh
#
# mouse:~ppr/src/ppad/insert_initial_media.sh
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
# Last modified 4 March 2003.
#

#
# This script initializes the PPR media database.
#
# European media sizes are underrepresented here.  If people will contribute
# definitions, I will be happy to add them.
#

. ../makeprogs/paths.sh

touch "$RPM_BUILD_ROOT$CONFDIR/media.db"

PATH="$HOMEDIR/bin:$PATH"
export PATH

# White paper in various sizes and weights:
ppad media put "letter" "8.5 in" "11.0 in" 75.0 "white" "" 7
ppad media put "a4" "210 mm" "297 mm" 75.0 "white" "" 7
ppad media put "master" "8.5 in" "11.0 in" 300.0 "white" "" 2
ppad media put "legal" "8.5 in" "14 in" 75.0 "white" "" 5

# Junk paper for banner pages
# The colour field value "VARIES" is there for a reason.  We don't want
# it every selected as a match for some real media requirement.
ppad media put "banner" "612.0 psu" "792.0 psu" 0 "VARIES" "" 10

# other stuff
ppad media put "3hole" "612.0 psu" "792.0 psu" 0.0 "white" "3Hole" 6
ppad media put "labels" "612.0 psu" "792.0 psu" 0.0 "white" "Labels" 1

# envelopes
ppad media put "comm10" "297.0 psu" "684.0 psu" 0.0 "white" "Envelope" 1
ppad media put "monarch" "540.0 psu" "279.0 psu" 0.0 "white" "Envelope" 1

# normal paper of various colours
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

# end of file
