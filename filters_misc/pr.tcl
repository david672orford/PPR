#! /bin/sh
#
# mouse:~ppr/src/misc_filters/pr.tcl
# Copyright 1995--2002, Trinity College Computing Center.
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
# Last modified 11 November 2002.
#

#
# PR filter for the PPR spooling system.
# Combine pr and filter_lp to make a PostScript pr.
#
# $1 is the space-separated list of name=value options
# $2 is the name of the printer or group
# $3 is the title
#
# and so on...
#

# These are filled in when the script is installed.
set HOMEDIR "/usr/lib/ppr"
set TEMPDIR "/tmp"
set PR "/usr/bin/pr"

# Process the options
regsub "title=\"" [lindex $argv 0] "\"title=" options
set title [lindex $argv 2]
set width ""
set length ""
foreach option $options {
	regexp {^([^=]+)=(.*)$} $option junk name value
	puts stderr "name=$name, value=$value"
	switch -exact -- $name {
		width {
			set width "-w $value"
			}
		length {
			set length "-l $value"
			}
		title {
			set title $value
			}
		default {
			}
		}
	}

set tempfile ""

set result [catch {

    # Create a temporary file.
    set tempfile [exec $HOMEDIR/lib/mkstemp $TEMPDIR/ppr-pr-XXXXXX]

    # Now, run pr. 
    set command [list $PR -f -h $title $width $length >$tempfile 2>@stderr]
    if {$width != ""} {
	lappend command $width
	}
    if {$length != ""} {
	lappend command $length
	}
    puts $command
    exec $command

    # Now, run filter_lp on the temporary file.
    exec filters/filter_lp "$1" <$tempfile >@stdout 2>@stderr

    } error ]


# We can remove the temporary file now.
if {$tempfile != ""} {
    exec rm -f $tempfile
    }

# Make non-zero exit if something failed.
if {$result != 0} {
    puts "pr filter failed: $error"
    exit 2
    }

# We are done, we were sucessfull.
exit 0
