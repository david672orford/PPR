#! ppr-tclsh
#
# mouse:~ppr/src/browsers/parallel.tcl
# Copyright 1995--2004, Trinity College Computing Center.
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
# Last modified 16 April 2004.
#

set HOMEDIR "?"

# Name the command line parameters.
set domain [lindex $argv 0]
set node [lindex $argv 1]

# We need Netatalk's or CAP's getzones.  Find it.
foreach dir {/usr/local/atalk/bin /usr/local/bin /usr/bin} {
	if {![file executable $dir/getzones]} {
		set getzones $dir/getzones
		break
		}
	}

# Assume that if there is no getzones command, there is no AppleTalk.
if {![info exists getzones]} {
	puts ";AppleTalk support not installed (can't find getzones)."
	exit 0
	}

# If we are run without a domain specified, then we are to print a list of 
# the domains which the user may choose to browse.
if { $domain == "" } {
    set f [open "| $HOMEDIR/lib/getzones" "r"]
	set zones {}
	while {[gets $f line] >= 0} {
		lappend zones $line
		}
	puts -nonewline [join [lsort $zones] "\n"]
    exit 0
    }

#
# Build a list of names with a type of LaserWriter.  At the same time take
# note of other names which could server as names for groups of LaserWriter
# names.
#
array set printers {}
array set workstations {}
array set snmp_agents {}
array set afp_servers {}
array set manufacturers {}
array set models {}

proc add_printer {address name_name name} {
	global printers
	if {![info exists printers($address)]} {
		set printers($address) {}
		}
	lappend printers($address) [list $name_name $name]
	}

set f [open "| $HOMEDIR/lib/nbp_lookup \"=:=@$domain\"" "r"]
while {[gets $f line] >= 0} {
    #puts $line
    if [regexp {^([0-9]+:[0-9]+):[0-9]+ [0-9]+ (([^:]+):([^@]+)@.+)$} $line junk address name name_name name_type] {
		switch -exact -- $name_type {
			"LaserWriter" {
				add_printer $address $name_name $name
				}
			"DeskWriter" {
				add_printer $address $name_name $name
				}
			"Workstation" {
				set workstations($address) $name_name
				}
			"SNMP Agent" {
				set snmp_agents($address) $name_name
				}
			"AFPServer" {
				set afp_servers($address) $name_name
				}
			"HP Zoner Responder" {
				set manufacturer($address) "HP"
				}
			default {
				# Some printers have additional names which suggest
				# the printer model
				switch -regexp -- $name_type {
					{^LaserJet } {
						# "LaserJet 4 Plus"
						set model($address) $name_type
						}
					{^DESK} {
						# "DESKJET 890C"
						set model($address) $name_type
						}
					{^HP } {
						# "HP LaserJet"
						regsub {^HP } $name_type {} model($address)
						}
					}
				}
			}
		}
    }
close $f

# We use this function to figure out what we will call this network node.  We
# use a function because we couldn't get the Tcl syntax right for doing
# it inline.
proc nodename {address} {
	global afp_servers workstations snmp_agents printers
	if [info exists afp_servers($address)] {
		return $afp_servers($address)
		}
	if [info exists workstations($address)] {
		return $workstations($address)
		}
	if [info exists snmp_agents($address)] {
		return $snmp_agents($address)
		}
	return [lindex [lindex $printers($address) 0] 0]
	}

#
# Loop through the groups of LaserWriter-type names printing
# a section for each.
#
foreach address [array names printers] {

	# Skip nodes which don't have printers.
	if {![info exists printers($address)]} {
		continue
		}

	puts "\[[nodename $address]\]"

	if [info exists manufacturer($address)] {
		puts "manufacturer=$manufacturer($address)"
		}
	if [info exists model($address)] {
		puts "model=$model($address)"
		}

	# List the printers on this node.
	foreach socket $printers($address) {
		set name [lindex $socket 0]
		set address [lindex $socket 1]
		puts "interface=atalk,\"$address\""
		}

	puts ""
	}

exit 0
