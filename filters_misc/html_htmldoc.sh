#! /bin/sh
#
# mouse:~ppr/src/filters_misc/html_htmldoc.sh
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
# Last modified 17 October 2003.
#

# This is changed to the path of HTMLDOC by setup_filters:
HTMLDOC="?"

# Assign names to the parameters
OPTIONS="$1"
PRINTER="$2"
TITLE="$3"

# Set colour option to convert image to grayscale
colour=""
charset=""
level="1"

# Look for parameters we should pay attention to
for pair in $OPTIONS
	do
	case "$pair" in
	colour=[yYtT1]* )
		colour="--color"
		;;
	colour=[nNfF0]* )
		colour="--gray"
		;;
	color=[yYtT1]* )
		colour="--color"
		;;
	color=[nNfF0]* )
		colour="--gray"
		;;
	charset=* )
		charset="--charset `cut -d= -f2`"
		;;
	level=[1-9] )
		level=`cut -d= -f2`
	esac
	done
# Run the filter:
$HTMLDOC --webpage $colour $charset --format ps$level -

# Pass on the exit code:
exit $?
