#! /bin/sh
#
# mouse:~ppr/src/misc_filters/texinfo.sh
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
# Last modified 17 January 2005.
#

# These will be filled in when this filter is installed.
FILTDIR=""
TEMPDIR=""
TEXI2DVI=""

# Give names to the arguments.
OPTIONS="$1"
PRINTER="$2"
TITLE="$3"
INVOKEDIR="$4"

# Look for parameters we should pay attention to
NOISY=0
for pair in $OPTIONS
	do
	case "$pair" in
	noisy=[yYtT1]* )
		NOISY=1
		;;
	esac
	done

# If INVOKEDIR is defined, add it to TEXINPUTS.	 The trailing
# colon means to search the system directories after.
if [ -n "$INVOKEDIR" ]
	then
	TEXINPUTS="$INVOKEDIR:"
	export TEXINPUTS
	fi

# It seems that the shell may cause the euid to revert to the real uid
# in that case, the user who invoked ppr must be able to read and
# write in the temporary directory.	 This does not provide the
# tightest security, but it works.
umask 0

# Make a directory to work in.
TEXITEMPDIR="$TEMPDIR/ppr-texinfo-$$"
mkdir $TEXITEMPDIR
cd $TEXITEMPDIR

# Copy stdin to a file we can name.
if [ $NOISY -ne 0 ]; then echo "Copying file to a temporary directory" >&2; fi
cat - >tempfile.texi

# Convert to a DVI file by using the GNU project program.
if [ $NOISY -ne 0 ]; then echo "Running $TEXI2DVI" >&2; fi
if [ $NOISY -ne 0 ]
  then
  $TEXI2DVI tempfile.texi >&2
  else
  $TEXI2DVI tempfile.texi >/dev/null
  fi
exval=$?
if [ $NOISY -ne 0 ]; then echo "$TEXI2DVI returned $exval" >&2; fi

# Remove the origional file to save space.
rm -f tempfile.texi

# Convert DVI file to PostScript
exval2=0
if [ $exval = 0 ]
  then
  if [ ! -r tempfile.dvi ]
	then
	if [ $NOISY -ne 0 ]; then echo "$TEXI2DVI did not generate any output" >&2; fi
	else
	if [ $NOISY -ne 0 ]; then echo "Running DVI filter" >&2; fi
	$FILTDIR/filter_dvi "$OPTIONS" "$PRINTER" "$TITLE" "$INVOKEDIR" <tempfile.dvi
	exval2=$?
	if [ $NOISY -ne 0 ]; then echo "filter_dvi returned $exval2" >&2; fi
	fi
  fi

# Remove the temporary files and directory
rm -f *
cd ..
rmdir $TEXITEMPDIR

# If either stage failed, we failed.
if [ $exval != 0 -o $exval2 != 0 ]
	then
	if [ $NOISY -ne 0 ]; then echo "Failed" >&2; fi
	exit 1
	fi

if [ $NOISY -ne 0 ]; then echo "Done" >&2; fi
exit 0

# end of file
