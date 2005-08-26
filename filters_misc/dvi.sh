#! /bin/sh
#
# mouse:~ppr/src/filters_misc/dvi.sh
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
# Last modified 2 April 2005.
#

#
# This filter invokes DVIPS.  If DVIPS works, this script
# sends the output to the printer.
#
# /usr/ppr/install/setup_filters passes this program thru a sed
# script before installing it.	The sed script fills in the
# path of DVIPS.
#
# This script contains two grep commands to remove the string "Got a new
# papersize" from the output.  If you are using dvipsk 5.58f these are not
# needed and you may remove them.  If however, you are using 5.58a, and
# possibly some later versions, they are necessary.	 Version 5.58a will
# emmit this string on stdout when a page size is explicitly selected.
# Naturally, this string gets mixed in with the intended PostScript output
# and makes the output unprintable.
#

# Standard directories
LIBDIR="?"
TEMPDIR="?"
PRINTERS_CACHEDIR="?"
FILTDIR="?"

# Path of dvips.  Setup_filters modifies this line.
DVIPS="?"

# Assign names to the parameters
OPTIONS="$1"
PRINTER="$2"
TITLE="$3"

# Expand the path to include the place where MakeTeXPK
# is likely to be:
PATH=$PATH:`dirname $DVIPS`
export PATH

# We will use this temporary file for divps's error output
TEMPFILE=`$LIBDIR/mkstemp $TEMPDIR/ppr-dvi-XXXXXX`

# Directory in which to create configuration files
DVIPS_CONFDIR="$PRINTERS_CACHEDIR/$PRINTER"

# Add this to the path to be searched
export TEXCONFIG="$DVIPS_CONFDIR:"

# Variables into which to read parameters
DVIPS_CONFIG=""
RESOLUTION=""
NOISY=""
FREEVM=""
MFMODE=""

# Look for parameters we should pay attention to
for pair in $OPTIONS
	do
	case "$pair" in
	dvipsconfig=* )
		DVIPS_CONFIG=`echo $pair | cut -d'=' -f2`
		;;
	noisy=[yYtT1]* )
		NOISY="yes"
		;;
	resolution=* )
		RESOLUTION=`echo $pair | cut -d'=' -f2`
		;;
	freevm=* )
		FREEVM=`echo $pair | cut -d'=' -f2`
		;;
	mfmode=* )
		MFMODE=`echo $pair | cut -d'=' -f2`
		;;
	esac
	done

# If a dvipsconfig= did not appear, we must provide a
# configuration file, even if we have to create one.
if [ -z "$DVIPS_CONFIG" ]
	then
	if [ -n "$MFMODE" -a -n "$RESOLUTION" -a -n "$FREEVM" ]
	then
	DVIPS_CONFIG="$MFMODE-$RESOLUTION-$FREEVM"
	if [ -n "$NOISY" ]; then echo "Generating \"$DVIPS_CONFDIR/config.$DVIPS_CONFIG\"" >&2; fi
	if [ ! -f "$DVIPS_CONFDIR/config.$DVIPS_CONFIG" ]
		then
		./make_dvips_conf $PRINTER $MFMODE $RESOLUTION $FREEVM
		fi
	fi
	fi

# If $DVIPS_CONFIG is defined, turn it into a switch
if [ -n "$DVIPS_CONFIG" ]
	then
	DVIPS_CONFIG="-P $DVIPS_CONFIG"
	fi

# Run dvips, catching the error output in a file
if [ -z "$NOISY" ]		# if not noisy
	then
	$DVIPS $DVIPS_CONFIG -f -R 2>$TEMPFILE	 | grep -v '^Got a new papersize$'
	else				# if noisy
	echo "Command: $DVIPS $DVIPS_CONFIG -f -R" >&2
	$DVIPS $DVIPS_CONFIG -f -R		 | grep -v '^Got a new papersize$'
	fi
cd $LIBDIR

# If there were errors and we are not in noisy mode, try to print the
# DVIPS error messages instead of the DVIPS output.
if [ $? -ne 0 -a -z "$NOISY" ]
	then
	$FILTDIR/filter_lp <$TEMPFILE
	fi

# Remove the error output file.
rm -f $TEMPFILE

exit 0

