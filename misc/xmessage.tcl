#! /usr/bin/wish
#
# mouse:~ppr/src/misc/xmessage.tcl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software and documentation are provided "as is" without
# express or implied warranty.
#
# Last modified 31 July 2001.
#

#
# This is a reimplementation of xmessage in Tcl/Tk.  It is provided
# in case your operating system supplier ommited xmessage as
# "non-essential".
#

option add "*Label*borderWidth" 0
option add "*highlightThickness" 0

set file ""
set buttons "okay:0"
set message ""
set default ""
set print 0
set center 0
set nearmouse 0
set timeout 0
set title "xmessage"
set font "-*-courier-medium-r-*-*-*-120-*-*-*-*-*-*"

set argv_len [llength $argv]

for {set x 0} {$x < $argv_len} {incr x 1} \
	{
	switch -exact -- [lindex $argv $x] \
		{
		-file {incr x
			set file [lindex $argv $x]
			}
		-buttons {incr x
			set buttons [lindex $argv $x]
			}
		-default {incr x
			set default [lindex $argv $x]
			}
		-print {set print 1
			}
		-center {set center 1
			}
		-nearmouse {set nearmouse 1
			}
		-timeout {incr x
			set timeout [expr [lindex $argv $x] * 1000]
			}
		-title {incr x
			set title [lindex $argv $x]
			}
		-fg {incr x
			set value [lindex $argv $x]
			option add "*Foreground" $value
			}
		-bg {incr x
			set value [lindex $argv $x]
			option add "*Background" $value
			. configure -background $value
			}
		-font {incr x
			set font [lindex $argv $x]
			}
		default {set new [lindex $argv $x]
			set message "$message$new "
			}
		}
	}

# Set the font for new stuff
option add "*font" $font

# A frame to hold everything
frame .main
pack .main -padx 5 -pady 5

# A frame to hold the lines
frame .main.lines -relief sunken -borderwidth 2
pack .main.lines -side top -anchor w

# Add the lines in their box
set linenum 0

foreach line [split $message \n] \
	{
	label .main.lines.msg_$linenum -text $line -anchor w -borderwidth 0
	pack .main.lines.msg_$linenum -side top -anchor w
	incr linenum
	}

if [string compare $file ""] \
	{
	if { $file == "-" } { set stream stdin} \
	else { set stream [open $file r] }

	while {[gets $stream line] >= 0} \
		{
		label .main.lines.msg_$linenum -text $line -anchor w
		pack .main.lines.msg_$linenum -side top -anchor w
		incr linenum
		}

	if {! [string compare $file "-"]} { close $stream }
	}

# Button press procedure
proc press { name code } \
	{
	global print
	if { $print != 0 } { puts $name }
	exit $code
	}

# A divider frame
frame .main.divider -height 5
pack .main.divider -side top

# Add the buttons
frame .main.buttons
pack .main.buttons -side top -anchor w
set buttonnum 0
foreach button [split $buttons ,] \
	{
	set fields [split "$button:0" :]
	set name [lindex $fields 0]
	set code [lindex $fields 1]
	if { ! [string compare $name $default] } \
		{
		frame .main.buttons.default -borderwidth 1 -bg black
		button .main.buttons.default.button_$buttonnum -text $name -command [list press $name $code]
		pack .main.buttons.default -side left -padx 5
		pack .main.buttons.default.button_$buttonnum -padx 2 -pady 2
		bind . <Return> [list .main.buttons.default.button_$buttonnum invoke]
		} \
	else \
		{
		button .main.buttons.button_$buttonnum -text $name -command [list press $name $code]
		pack .main.buttons.button_$buttonnum -side left -padx 5
		}
	incr buttonnum
	}

# Set the timeout if we have been asked to.
if { $timeout != 0 } \
	{ after $timeout {exit 0} }

# Set the title
wm title . $title

# end of file

