#! @PPR_TCLSH@
#
# mouse:~ppr/src/responders/xwin.sh
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
# Last modified 29 March 2005.
#

#
# This responder sends the response by means of the X-Windows program 
# "xmessage" or, if xmessage is not available, by means of wish and our
# xmessage clone script, or if that isn't available either, by means of 
# xterm, sh, and echo.
#

#==============================
# Parse the command line
#==============================
puts $argv
foreach option $argv {
	regexp {^([^=]+)=(.*)$} $option junk name value
	switch -exact -- $name {
		for {
			set for $value
			}
		responder_address {
			set responder_address $value
			}
		responder_options {
			set responder_options $value
			}
		subject {
			set subject $value
			}
		short_message {
			set short_message $value
			}
		long_message {
			set long_message $value
			}
		}
	}

#==============================
# Parse the responder options
#==============================
set option_timeout 10000
foreach option [split $responder_options " "] {
	regexp {^([^=]+)=(.*)$} $option junk name value
	switch -exact -- $name {
		timeout {
			if [regexp {^[0-9]+$} $value value] {
				set option_timeout $value
				}
			}
		}
	}

#===========================================================
# Figure out which program we can use so send the message.
#===========================================================

if [file executable @XWINBINDIR@/xmessage] {
	set command [list @XWINBINDIR@/xmessage -geometry +100+100 -default okay -timeout $option_timeout -file -]
} elseif [file executable /usr/local/bin/wish] {
	set command [list /usr/local/bin/wish @LIBDIR@/xmessage -geometry +100+100 -default okay -timeout $option_timeout -file -]
} elseif [file executable /usr/bin/wish] {
	set command [list /usr/bin/wish $LIBDIR/xmessage -geometry +100+100 -default okay -timeout $option_timeout -file -]
} elseif [file executable @XWINBINDIR@/rxvt] {
	set command [list @XWINBINDIR@/rxvt -geometry 80x5+100+100 -e /bin/sh -c 'cat; echo "Press ENTER to dismiss this message."; read x']
} elseif [file executable @XWINBINDIR@/xterm] {
	set command [list @XWINBINDIR@/xterm -geometry 80x5+100+100 -e /bin/sh -c 'cat; echo "Press ENTER to dismiss this message."; read x']
} else {
	puts "Can't find a program by which to respond!"
	exit 1
	}

#====================================
# Dispatch the message.
#====================================

set command [lappend command -display $responder_address -title "Message for $for: $subject" -bg skyblue -fg black]
set pipe [ppr_popen_w $command]
#puts $pipe [ppr_wordwrap $short_message 80]
#puts $pipe "==========================================================================="
puts $pipe [ppr_wordwrap $long_message 80]
catch { close $pipe } error

exit 0
