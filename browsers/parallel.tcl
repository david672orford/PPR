#! /usr/bin/tclsh

set domain [lindex $argv 0]

if { $domain == "" } {
    puts "Parallel Ports"
    exit 0
    }

if { $domain != "Parallel Ports" } {
    puts "Unknown domain \"$domain\"."
    exit 1
    }

# Linux 2.2.X
if [file isdirectory /proc/parport] {
    foreach port [glob /proc/parport/*] {
	puts "\[$port\]"
	puts "comment=\"Parallel port $number\""
	set f [open "/proc/parport/$port/autoprobe" "r"]
	set line [read $f]
	puts $line
	close $f
	puts ""
	}
    exit 0
    }

# Linux 2.4.X
if [file isdirectory /proc/sys/dev/parport] {

    exit 0
    }

# Other
foreach port [glob /dev/lp*] {
    puts "\[$port\]"

    puts ""
    }
exit 0

