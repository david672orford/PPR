#! /usr/lib/ppr/bin/ppr-tclsh
#
# Last modified 6 December 2002.
#

# Name the command line parameters.
set domain [lindex $argv 0]
set node [lindex $argv 1]

# If we are run without a domain specified, then we are to print a list of 
# the domains which the user may choose to browse.
if { $domain == "" } {
    puts "Parallel Ports"
    exit 0
    }

# There is only one domain we can browse.
if { $domain != "Parallel Ports" } {
    puts "Unknown domain \"$domain\"."
    exit 1
    }

#
# This function probes the system and defines the following:
#
# ports_list
#	This is a function which returns a list of ports.  The port names
#	returned by this function will be plugged into the templates.
# dev_template
#	This is an sprintf format used to make the /dev/ file name from
#	a port name as returned by ports_list.
# autoprobe_template
#	This is an sprintf format used to make the Linux /proc/ filename
#	for getting IEEE autoprobe information. 
#
proc probe_system {} {
    global autoprobe_template
    global dev_template

    # Linux 2.2.X
    if [file isdirectory /proc/parport] {
	set autoprobe_template "/proc/parport/%d/autoprobe"
	set dev_template "/dev/lp%d"
	proc ports_list {} {
	    set retval {}
	    foreach port [glob /proc/parport/*] {
		regexp {([0-9]+)$} $port junk number
		set retval [lappend $retval $number]
		}
	    return $retval
	    }
	return
	}

    # Linux 2.4.X
    if [file isdirectory /proc/sys/dev/parport] {
	set autoprobe_template "/proc/sys/dev/parport/parport%d/autoprobe"
	set dev_template "/dev/lp%d"
	proc ports_list {} {
	    set retval {}
	    foreach port [glob /proc/sys/dev/parport/parport*] {
		regexp {([0-9]+)$} $port junk number
		set retval [lappend $retval $number]
		}
	    return $retval
	    }
	return
	}

    # Other/Unknown
    set autoprobe_template ""
    set dev_template "/dev/lp%d"
    proc ports_list {} {
	set retval {}
	foreach port [glob /dev/lp*] {
	    regexp {(\d+)$} $port junk number
	    set retval [lappend $retval $number]
	    }
	return $retval
	}
    }

#
# This function opens and extracts information from a Linux /proc
# autoprobe file.
# 
proc autoprobe {filename} {
    set file [open $filename r]
    set manufacturer ""
    set model ""
    while {[gets $file line] >= 0} {
	#puts $line
	regexp {^([^:]+):([^;]+);$} $line junk name value
	switch -exact -- $name {
	    MANUFACTURER {
		set manufacturer $value
		}
	    MODEL {
		set model $value
		}
	    }
	}    
    close $file
    return "manufacturer=$manufacturer\nmodel=$model"
    }

# Detect the OS and OS version and define OS specific functions and
# templates.
probe_system

# Probe each of the ports.
foreach port [ports_list] {
    puts "\[$port\]"
    puts "comment=Parallel Port $port"
    set lp [format $dev_template $port]
    set ap [format $autoprobe_template $port]
    if {$ap != ""} {
	puts [autoprobe $ap]
	}
    puts "interface=simple,$lp"
    puts "interface=parallel,$lp"
    puts ""
    }

exit 0
