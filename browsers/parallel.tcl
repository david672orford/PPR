#! /usr/bin/tclsh

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

proc probe_system {} {
    global autoprobe_template

    # Linux 2.2.X
    if [file isdirectory /proc/parport] {
	set autoprobe_template "/proc/parport/%d/autoprobe"
	proc ports_list {} {
	    return [glob /proc/parport/*]
	    }
	return
	}

    # Linux 2.4.X
    if [file isdirectory /proc/sys/dev/parport] {
	set autoprobe_template "/proc/sys/dev/parport/parport%d/autoprobe"
	proc ports_list {} {
	    return [glob /proc/sys/dev/parport/parport*]
	    }
	return
	}

    # Other/Unknown
    set autoprobe_template ""
    proc ports_list {} {
	set retval {}
	foreach port [glob /dev/lp*] {
	    regexp {(\d+)$} $port junk number
	    set retval [lappend $retval $number]
	    }
	return $retval
	}
    }

probe_system

puts [ports_list]

exit 0

