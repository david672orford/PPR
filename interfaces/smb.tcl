#! @PPR_TCLSH@
#
# mouse:~ppr/src/interfaces/smb.tcl
# Copyright 1995--2005, Trinity College Computing Center.
# Written by Klaus Reimann and David Chappell.
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
# Last modified 21 January 2005.
#

#########################################################################
#
# This interface program uses Samba's smbclient to print to SMB
# (Microsoft LAN Manager, Microsoft Windows) print queues.
#
# OPTIONS:
#	smbuser=
#	smbpassword=
#
# EXAMPLE
#	smbprint pprprinter '\\server\lp1' "smbuser=pprxx smbpassword=xx"
#
#########################################################################

cd @LIBDIR@

# source the file which defines the exit codes
source "./interface.tcl"

if {[string compare [lindex $argv 0] "--probe"] == 0} {
	puts stderr "The interface program \"`basename $0`\" does not support probing."
	exit $EXIT_PRNERR_NORETRY_BAD_SETTINGS
	}

# give the parameters names
set printer [lindex $argv 0]
set address [lindex $argv 1]
set options [lindex $argv 2]

# Look in ppr.conf to find out where smbclient is.
set SMBCLIENT [ppr_conf_query samba smbclient 0 /usr/local/samba/bin/smbclient]

if {![file exists $SMBCLIENT]} {
	ppr_alert $printer TRUE "$SMBCLIENT does not exist.  Install it or adjust @CONFDIR@/ppr.conf."
	exit $EXIT_PRNERR_NORETRY
	}

# make sure the address parameter is not empty
switch -regexp -- $address {
	{^\\\\[^\\]+\\[^\\]+$} {
		#puts "Address $address is ok"
		}	
	{^$} {
		ppr_alert $printer TRUE "The printer address is empty."
		exit $EXIT_PRNERR_NORETRY
		}	
	default {
		ppr_alert $printer TRUE  "Syntax error in printer address.  Address syntax is \\\\server\\printer where server"
		ppr_alert $printer FALSE "is the NetBIOS name of the server and printer is the SMB share name of the queue."
		exit $EXIT_PRNERR_NORETRY
		}	
	}

# parse options
set smbuser "ppr"
set smbpassword ""
foreach opt $options {
	regexp {^([^=]+)=(.*)$} $opt junk name value
	switch -exact -- $name {
		smbuser {
			set smbuser $value
			}
		smbpassword {
			set smbpassword $value
			}
		default {
			ppr_alert "$printer" TRUE "Unrecognized interface option: $opt"
			exit $EXIT_PRNERR_NORETRY
			}
		}
	}

# Launch smbclient and capture its output.  It would be nice to add -z 600000 in 
# order to increase the timeout, but it seems that not all versions have such an 
# option.
regsub -all {\\} $address {\\\\} escaped_address
set f [open "| $SMBCLIENT $escaped_address $smbpassword -U $smbuser -N -P -c \"print -\" <@stdin 2>&1" "r"]

# Read the output generated by Smbclient and take note of serious errors.
set err_msg ""
set first "TRUE"
while {[gets $f line] >= 0} {
	switch -glob -- $line {
		"added interface ip=*" { 
			# 1st startup message
			}
		"Got a positive name query response*" { 
			# 2nd startup message
			}
		"Domain=*" { 
			# 3rd startup message
			}
		"(* kb/s) (* kb/s)" {
			# Speed of transmission (failure)
			}
		"putting file - as * (* kb/s) (average * kb/s)" {
			# Sucess
			}
		"ERROR:*" { 
			# don't remember what this is, but don't want it mistaken for a RIP error
			}
		"Connection to * failed" {
			set err_msg "Can't connect to server for $address"
			}
		"tree connect failed: ERRSRV - ERRinvnetname *" {
			set err_msg "Share name part of $address does not exist."
			}
		"*Error writing file:*" {
			regexp {Error writing file: (.+)} $line junk error
			set err_msg "Smbclient reports error \"$error\" when writing to \"$address\"."
			}
		"NT_STATUS_ACCESS_DENIED opening remote file *" {
			set err_msg "Access to \"$address\" is denied to user \"$smbuser\"."
			}
		default { 
			# Anything else goes into the alert log just in case.  We set
			# first to "FALSE" so that the date/time line won't appear 
			# twice if another message is sent to the printer's alert log.
			ppr_alert $printer $first "smbclient: $line"
			set first "FALSE"
			}
		}	
	}

if {[string length $err_msg] != 0} {
		ppr_alert $printer $first "$err_msg"
		exit $EXIT_PRNERR
	} else {
		exit $EXIT_PRINTED
	}

# vim: set tabstop=4 nowrap:
