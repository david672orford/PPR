#! ppr-tclsh
#
# mouse:~ppr/src/browsers/parallel.tcl
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
# Last modified 5 March 2003.
#

set HOMEDIR "?"

# Name the command line parameters.
set domain [lindex $argv 0]
set node [lindex $argv 1]

# If we are run without a domain specified, then we are to print a list of 
# the domains which the user may choose to browse.
if { $domain == "" } {
    exec $HOMEDIR/lib/getzones >@stdout 2>@stderr
    exit 0
    }

set f [open "| $HOMEDIR/lib/nbp_lookup \"=:LaserWriter@$domain\"" "r"]
while {[gets $f line] >= 0} {
    #puts $line
    if [regexp {^[^ ]+ [^ ]+ (([^:]+).+)$} $line junk address name] {
	puts "\[$name\]"
	puts "interface=atalk,\"$address\""
	puts ""
	}
    }
close $f

exit 0
