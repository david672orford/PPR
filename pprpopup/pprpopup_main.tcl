#! /usr/bin/wish
#
# pprpopup_main.tcl
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
#
# Last revised 11 December 2001.
#

set register_url "${ppr_root_url}cgi-bin/popup_register.cgi"
set help_url "${ppr_root_url}docs/"

set about_text "PPR Popup 0.5
11 December 2001
Copyright 1995--2001, Trinity College Computing Center
Written by David Chappell"

source ./urlfetch.itk
source ./browser.itk

# This is the port that this server should listen on:
set server_socket 15009

# Set options in order to make the Macintosh version look more like the others.
option add *foreground black
option add *background #a4b6dd
option add *activeForeground black
option add *activeBackground #a4b6dd
option add *textBackground white

# Window serial number.  This is used to generate unique Tk window
# command names.
set wserial 0

# On MS-Windows and MacOS, there is a Tk console.  For those platforms we
# define console_visibility to hid or unhide it.  For other platforms
# it is a no-op.
if {$tcl_platform(platform) == "windows" || $tcl_platform(platform) == "macintosh"} {
   proc console_visibility {yes} {
	if {$yes} { console show } else { console hide }
	}
    } else {
    proc console_visibility {yes} {}
    }

# The MacOS Finder can hide applications.  On MacOS we define this function
# to unhide this application.  On other plaforms it is a no-op.
if {$tcl_platform(platform) == "macintosh"} {
    package require Tclapplescript
    proc activate {} { AppleScript execute "tell application \"PPR Popup\" to activate" }
    } else {
    proc activate {} {}
    }

#
# Put up a dialog box for bad errors.
#
proc alert {message} {
    iwidgets::messagedialog .alert \
    	-modality application \
    	-title "PPR Popup Malfunctioned" \
    	-text $message
    .alert buttonconfigure OK -text "Close"
    .alert hide "Cancel"
    .alert activate
    exit 1
    }

#
# Call attention to a window that the user is neglecting.
#
proc window_reopen {win} {
    wm deiconify $win
    raise $win
    focus $win
    }

#
# Command to ask the user for his name.
#
proc command_USER {file message} \
    {
    global result
    global wserial

    set result ""

    # Make a top level window that we can pop up.
    set w [toplevel .user_[incr wserial]]
    wm title $w "User Identification"

    # This frame will be placed inside the toplevel window
    # but it will be packed with padding.
    frame $w.padded
    pack $w.padded -padx 20 -pady 20

    message $w.padded.message1 \
	-text $message \
	-fg red \
	-width 300
    pack $w.padded.message1 \
	-side top \
	-anchor w \
	-padx 5 \
	-pady 5

    label $w.padded.message2 -text "Please enter your name:"
    entry $w.padded.entry -width 40
    bind $w.padded.entry <Return> [list $w.ok invoke]
    pack $w.padded.message2 $w.padded.entry \
	-side top \
	-anchor w \
	-padx 5 -pady 5

    button $w.ok -text "OK" -command [list command_USER_ok $w]
    button $w.cancel -text "Cancel Job" -command "
	set result {-ERR cancel job}
	destroy $w"
    pack $w.ok $w.cancel -side right

    # Set the focus on the entry field and wait
    # for the window to be destroyed.
    focus $w.padded.entry
    tkwait window $w

    # The user may have closed the window using
    # a window manager command.
    if {$result == ""} {
	set result "-ERR window destroyed"
	}

    puts $file $result
    }

#
# Procedure which is called when the user presses
# <Return> or OK after entering his name.
#
proc command_USER_ok {w} {
    global result
    set r [$w.padded.entry get]
    if { $r != "" } {
	set result "+OK $r"
	destroy $w
	}
    }

#
# Display a message from PPR.
#
proc command_MESSAGE {file for} {
    global wserial

    set w [toplevel .message_[incr wserial]]
    wm title $w "Message for $for"
    frame $w.message
    pack $w.message -side top -fill both -expand true

    text $w.message.text \
 	-width 75 -height 8 \
 	-bg white \
	-yscrollcommand "$w.message.sb set"
    scrollbar $w.message.sb \
	-orient vert \
	-command "$w.message.text yview"
    pack $w.message.text \
	-side left \
	-fill both \
	-expand true
    pack $w.message.sb \
	-side right \
	-fill y

    button $w.dismiss \
	-text "Dismiss" \
	-command [list destroy $w]
    pack $w.dismiss \
	-side right

    bind $w <Return> [list $w.dismiss invoke]

    # Keep it iconified until it is full
    wm iconify $w

    # Arange for a callback whenever data is available.
    fileevent $file readable [list command_MESSAGE_datarecv $file $w]
    }

#
# This is called as a fileevent every time a line
# of the message is received.
#
proc command_MESSAGE_datarecv {file w} {

    # If there is a line to be had,
    if {[gets $file line] >= 0} {

	# If this is the last line,
	if {$line == "."} {
	    # Acknowledge the completed command.
	    puts $file "+OK"
	    flush $file

	    # Return the socket to the command dispatcher.
	    fileevent $file readable [list server_reader $file]

	    #wm deiconify $w
	    #focus $w

	    return
	    }

	# Not the last line, add this text.
	$w.message.text insert end "$line\n"
	}

    }

#
# Create a web browser window and download a page into it.
#
proc command_QUESTION {file url width height} {
    global wserial
    global open_windows

    if [info exists open_windows($jobname)] {
    	puts "  Already exists"
	window_reopen $open_windows($jobname)
    	return
    	}

    set w .html_[incr wserial]
    set open_windows($jobname) $w
    .questions.list insert end $jobname

    toplevel $w
    urlfetch $w.urlfetch
    Browser $w.browser \
	-width $width -height $height \
	-wrap word -linkcommand "$w.browser import" -padx 10 \
	-hscrollmode dynamic \
	-vscrollmode dynamic \
	-getcommand [itcl::code $w.urlfetch get] \
	-postcommand [itcl::code $w.urlfetch post]
    pack $w.browser -side top -anchor w -fill both -expand 1
    $w.browser import $url

    wm protocol $w WM_DELETE_WINDOW [list command_QUESTION_close $jobname]

    raise $w
    focus $w

    puts $file "+OK"
    }

proc command_QUESTION_close {jobname} {
    global open_windows
    puts "Window manager request to close window $open_windows($jobname) for $jobname."

    destroy $open_windows($jobname)
    unset open_windows($jobname)
    .questions.list delete [lsearch -exact [.questions.list get 0 end] $jobname]
    }

#
# These commands change the list of jobs.
#
proc command_JOB_STATUS {file jobname args} {
    set pos [lsearch -exact [.jobs.list get 0 end] $jobname]
    if {$pos == -1} {
	.jobs.list insert end $jobname
	}
    puts $file "+OK"
    }

proc command_JOB_REMOVE {file jobname} {
    if {[catch { .jobs.list delete [lsearch -exact [.jobs.list get 0 end] $jobname] }]} {
    	puts $file "+OK"
    	} else {
    	puts $file "-ERR no such job"
    	}
    }

#
# This is the function which is called when a connexion
# is made to this server.
#
proc server_function {file cli_addr cli_port} \
  {
  puts "$file: Connection from $cli_addr:$cli_port"
  fconfigure $file -blocking false
  fileevent $file readable [list server_reader $file]
  activate
  }

#
# This is called whenever there is anything to read on the socket on
# one of the connected sockets.
#
proc server_reader {file} {

    # Get the next line from the socket.
    if {[set line [gets $file]] == "" && [eof $file]} {
	set line "EOF"
	}

    puts "$file: $line"

    # Act on the command received
    switch -glob -- $line {
	"USER *" {
	    if {[regexp {^USER (.+)$} $line junk prompt]} {
		command_USER $file $prompt
		} else {
		puts $file "-ERR prompt missing
		}
	    }
	"MESSAGE *" {
	    if {[regexp {^MESSAGE (.+)$} $line junk for]} {
		command_MESSAGE $file $for
		} else {
		puts $file "-ERR recipient missing"
		}
            }
	"QUESTION *" {
	    if {[regexp {^QUESTION ([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$} $line junk jobname url width height]} {
		command_QUESTION $file $jobname $url $width $height
		} else {
		puts $file "-ERR missing parameters"
		}
    	    }
	"JOB *" {
	    if {[regexp {^JOB STATUS ([^ ]+) (.*)$} $line junk jobname args]} {
		command_JOB_STATUS $file $jobname $args
	    	} else {
		if {[regexp {^JOB REMOVE ([^ ]+)$} $line junk jobname]} {
		    command_JOB_REMOVE $file $jobname
		    } else {
		    puts $file "-ERR invalid subcommand"
		    }
	    	}
	    }
	EOF {
	    catch { close $file }
	    return
	    }
	QUIT {
            puts $file "+OK"
	    catch { close $file }
	    return
            }
	* {
            puts $file "-ERR Unknown command"
            }
	}

  # Push out the reply.
  flush $file
  }

#========================================================================
#
#========================================================================
proc do_register {} {
    package require http 2.2
    global register_url
    puts "Registering with server at $register_url..."
    set data "zone=haha"
    ::http::geturl $register_url -query $data -command [namespace code register_callback]
    }

proc register_callback {token} {
    puts "Registration finished:"

    # Get the result codes for the POST.
    upvar #0 $token state
    regexp {^HTTP/[0-9]+\.[0-9]+ ([0-9]+)} $state(http) junk ncode
    puts "    State: $state(status)"
    puts "    Ncode: $ncode"
    puts "    Size: $state(totalsize)"

    # Test the status for errors.
    if {[string compare $state(status) "ok"] != 0} {
	alert "POST failed while registering with PPR server:\n$state(status)\n$state(error)"
	} else {
	if {$ncode != 200} {
	    alert "POST failed while registering with the PPR server:\n$state(http)"
	    }
	}

    # Register again in 10 minutes.
    after 600000 [list do_register]
    }

#========================================================================
# main and its support routines
#========================================================================

set menu_view_console_visibility 0

proc main {} {
    console_visibility 0

    # Set up a Quit handler for the main window.  This will get called if
    # the user uses the window manager to close the main window.
    wm protocol . WM_DELETE_WINDOW { menu_file_quit }

    # Create the menubar and attach it to the default toplevel window.
    . configure -menu .menubar
    menu .menubar -border 1 -relief groove

    .menubar add cascade -label "File" -menu [menu .menubar.file -tearoff 0 -border 1]
    .menubar.file add command -label Quit -command { menu_file_quit }

    .menubar add cascade -label "View" -menu [menu .menubar.view -tearoff 0 -border 1]
    .menubar.view add check -variable menu_view_console_visibility -label "Tk Console" -command { console_visibility $menu_view_console_visibility }

    .menubar add cascade -label "Help" -menu [menu .menubar.help -tearoff 0 -border 1]
    .menubar.help add command -label "Help Contents" -command { menu_help_contents }
    .menubar.help add command -label "About PPR Popup" -command { menu_help_about }

    # Create the scrolling listbox with the outstanding jobs.
    frame .jobs -border 3
    label .jobs.label -text "Outstanding Jobs:"
    iwidgets::scrolledlistbox .jobs.list \
	-borderwidth 2 -relief groove \
	-hscrollmode none \
	-vscrollmode static \
	-scrollmargin 0 \
	-visibleitems 20x2
    pack .jobs.label -side top -anchor w
    pack .jobs.list -fill both -expand 1
    pack .jobs -side top -fill both -expand 1

    # Create the scrolling listbox with the outstanding questions.
    frame .questions -border 3
    label .questions.label -text "Question Windows:"
    iwidgets::scrolledlistbox .questions.list \
	-borderwidth 2 -relief groove \
	-hscrollmode none \
	-vscrollmode static \
	-scrollmargin 0 \
	-visibleitems 20x2 \
	-dblclickcommand {
		global open_windows
		set url [.questions.list getcurselection]
		puts "\$url = $url"
		if {[info exists open_windows($url)]} {
		    window_reopen $open_windows($url)
		    }
		}
    pack .questions.label -side top -anchor w
    pack .questions.list -fill both -expand 1
    pack .questions -side top -fill both -expand 1

    # We need this for Macintosh as otherwise the drag handle at the
    # bottom right will overlap scrollbar.
    frame .bottom_pad -height 15
    pack .bottom_pad -side bottom -fill x

    # Set up the server to listen on a TCP socket
    global server_socket
    if [catch {socket -server server_function $server_socket} result] {
	alert "Can't bind to port: $result"
	exit 1
	}

    # Register with the server.
    do_register

    # Show the main window.
    wm geometry . 600x200
    wm title . "PPR Popup"
    wm deiconify .
    }

proc menu_file_quit {} {
    puts "Quit!"
    iwidgets::messagedialog .quit_confirm \
            -modality application \
            -title "Confirmation" \
            -text "If you close this program, you will be unable to print to the public printers."
    .quit_confirm buttonconfigure OK -text "Close"
    .quit_confirm buttonconfigure Cancel -text "Don't Close"
    if {[.quit_confirm activate]} {
        exit 0
        } else {
        destroy .quit_confirm
        }
    }

proc menu_help_contents {} {
    global wserial
    global help_url
    set w .help_[incr wserial]
    toplevel $w
    urlfetch $w.urlfetch
    Browser $w.browser \
	-width 6i -height 6i \
	-wrap word \
	-linkcommand "$w.browser import" \
	-padx 10 \
	-hscrollmode dynamic \
	-vscrollmode dynamic \
	-getcommand [itcl::code $w.urlfetch get] \
	-postcommand [itcl::code $w.urlfetch post]
    pack $w.browser -side top -anchor w -fill both -expand 1
    $w.browser import $help_url
    }

proc menu_help_about {} {
    global about_text
    iwidgets::messagedialog .about \
    	-modality application \
    	-title "About PPR Popup" \
    	-text $about_text
    .about buttonconfigure OK -text "OK"
    .about hide "Cancel"
    .about activate
    destroy .about
    }

main

# end of file
