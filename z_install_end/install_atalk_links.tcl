#! ../nonppr_tcl/ppr-tclsh
#
# mouse:~ppr/src/fixup/fixup_atalk.tcl
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
# Last modified 21 February 2003.
#

#
# Find getzones and create a links to it where PPR programs will
# look for it.
#

source ../makeprogs/paths.tcl

if [info exists $env(RPM_BUILD_ROOT)] {
    set RPM_BUILD_ROOT $env(RPM_BUILD_ROOT)
    } else {
    set RPM_BUILD_ROOT ""
    }

exec rm -f "$HOMEDIR/lib/getzones"

puts "Searching for getzones..."

foreach file {/usr/local/atalk/bin/getzones /usr/bin/getzones} {
    puts "    Trying \"$file\"..."
    if [ file executable $file ] {
	puts "\tFound, linking to $HOMEDIR/lib/getzones."
	exec ln -s $file "$RPM_BUILD_ROOT$HOMEDIR/lib/getzones"
	break
	}	
    }

if {! [ file executable "$HOMEDIR/lib/getzones" ]} {
    puts "  No getzones found."
    }

exit 0
