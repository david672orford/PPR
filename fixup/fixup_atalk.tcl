#! ppr-tclsh
#
# mouse:~ppr/src/fixup/fixup_atalk.tcl
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
# Last modified 10 May 2002.
#

set HOMEDIR "?"

#======================================================================
# Find getzones and create a links to it where PPR programs will
# look for it.
#======================================================================

puts "Searching for getzones..."
exec rm -f "$HOMEDIR/lib/getzones"
foreach file {/usr/local/atalk/bin/getzones /usr/bin/getzones} {
    puts "    Trying \"$file\"..."
    if [ file executable $file ] {
	puts "\tFound, linking to $HOMEDIR/lib/getzones."
	exec ln -s $file "$HOMEDIR/lib/getzones"
	break
	}	
    }
if [ file executable "$HOMEDIR/lib/getzones" ] {
    puts "Done."
    } else {
    puts "No getzones found."
    }
puts ""

exit 0
