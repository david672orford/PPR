#! /bin/sh
#
# mouse:~ppr/src/misc_filters/gif.sh
# Copyright 1995--2005, Trinity College Computing Center.
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
# Last modified 17 January 2005.
#

#
# This program invokes appropriate filters to convert GIF
# files to PostScript.
#
# "ppr-index filters" passes this program thru a sed
# script before installing it.
#

# Paths of filters
GIFTOPNM="?"
PPMTOPGM="?"
PNMTOPS="?"

# Assign names to the parameters
OPTIONS="$1"
PRINTER="$2"
TITLE="$3"

# Set COLOUR option to convert image to grayscale.
# Set the RESOLUTION option to use the default.
COLOUR=""
RESOLUTION=""

# Look for parameters we should pay attention to
for pair in $OPTIONS
	do
	case "$pair" in
	colour=[yYtT1]* )
		COLOUR="YES"
		;;
	colour=[nNfF0]* )
		COLOUR=""
		;;
	color=[yYtT1]* )
		COLOUR="YES"
		;;
	color=[nNfF0]* )
		COLOUR=""
		;;
	resolution=* )
		RESOLUTION="-dpi `echo $pair | cut -d'=' -f2`"
		;;
	esac
	done

# Run the filters
if [ -n "$COLOUR" ]
	then
	$GIFTOPNM | $PNMTOPS $RESOLUTION | grep -v '^%%Title:'
	else
	$GIFTOPNM | $PPMTOPGM | $PNMTOPS $RESOLUTION | grep -v '^%%Title:'
	fi

# Pass on the exit value of the filter pipeline.
exit $?

# end of file
