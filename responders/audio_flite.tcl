#! @PPR_TCLSH@
#
# mouse:~ppr/src/responders/audio_flite.tcl
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
# This responder attempts to send the message with the write
# program.	If that fails, it invokes the mail responder.
#

foreach option $argv {
	regexp {^([^=]+)=(.*)$} $option junk name value
	switch -exact -- $name {
		for {
			set for $value
			}
		responder_address {
			set responder_address $value
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

# Send the message with write.
set command [ppr_popen_w [list flite]]
puts $command [ppr_wordwrap $long_message 78]
set result [catch { close $command } error]

if {$result != 0} {
	puts "responder audio_flite: flite failed, exit code is $result"
	}

exit 0
