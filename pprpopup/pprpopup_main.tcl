#! /usr/bin/wish
#
# mouse:~ppr/src/pprpopup/pprpopup_main.tcl
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

set about_text "PPR Popup 1.51
10 March 2003
Copyright 1995--2003, Trinity College Computing Center
Written by David Chappell"

# This is the URL that is loaded when the user selects Help/Contents.
set help_contents_file "docs/pprpopup/"

# This is the port that this server should listen on.  It would be nice
# to get rid of this and have the port assigned dynamically, but Tcl
# doesn't seem to support creating a server port without binding
# it to a specific address.
set server_port 15009

# This is the seed for the token which the server must present for access.
if {[catch {set ppr_magic_cookie_seed [random_load]} errormsg]} {
    puts "Failed to load magic cookie seed."
    set ppr_magic_cookie_seed ""
    }

# This is how frequently we re-register (in seconds).
set registration_interval 600

# Set options in order to make the Macintosh version look more
# like the others.
set background_color #a4b6dd
set button_color #8892a8
option add *foreground black
option add *background $background_color
option add *activeForeground black
option add *activeBackground $background_color
option add *textBackground white

# Window serial number.  This is used to generate unique Tk window
# command names.
set wserial 0

#
# Put up a dialog box for bad errors.  We don't use iwidgets::messagedialog
# because it won't appear on WinNT if the main window isn't visible.
#
proc alert {message} {
    global wserial

    set w .alert_[incr wserial]

    set frown_image [image create photo -data {
R0lGODlhUgBSAKEAAAAAAPDw8P//AAAAVSH5BAEAAAEALAAAAABSAFIAAAL+
jI+pywfQopy0ooezzrb7qQniSJaC9qXdZrbtpsZL5tb2ickyffcup/NgfMRa
LkgZFpe/BzLyYEqNkGdCOc2Wjlas9iviBr1gsDgWLatJzh1gDR+1U+l4fL6q
29fn5Htvh+cHCCgI9Ue4ZziD2MNiFkK0eNVos6EndVlJJYFJpZml6dl06CNK
Znq6+VLFmKq6+qmayjBqAmvbBEurkLuFu4Qbe9vqMPy7WyQsWWxwjDyrDCxJ
KZ3MfO1Y7Et8mhnNfPENOuXN1MqtGxl6WV6Vrg5/8/jt/JxI2HaPn+/M///C
H8CBYQQSBAhB3sEyPBbya+gwEcSIhVBRvGPxIp/OhBLN9QuwL5iwdXwMQhop
qiRIhiinfXkXktUyHDPd2WOXEls7myBjysk5bqdIB+eEniRp6kJMchvpaVOq
MyNLpJzEaaOKEeutXs+c4vNKjKslsB0n/nIl0+xDrZNyaf1KdlLPTxTByp2r
ceyou3jz/iDVyaffMM1qCc7LF+rgpxakLqYp5LDDxGgfn1WhcCHlUpZxIMn8
b7MfyYFEV3Ac2nRj0HD6WOlJGufrBmpLq9aB+uTs1az1Ft4NonZU4G5yswFC
/HPL28kjw2huoAAAOw==
}]

    toplevel $w
    wm title $w "PPR Popup has Malfunctioned"

    frame $w.title
    pack $w.title -side top -padx 10 -pady 10
    #label $w.title.icon -bitmap error
    label $w.title.icon -image $frown_image
    label $w.title.text -text "PPR Popup has Malfunctioned"
    pack $w.title.icon $w.title.text -side left

    label $w.message -justify left -text $message
    pack $w.message -side top -fill both -expand true

    button $w.dismiss -text "Dismiss" -command [list destroy $w]
    pack $w.dismiss -side right -padx 20 -pady 5
    tkwait window $w

    image delete $frown_image
    }

#
# The MacOS Finder can hide applications.  On MacOS we define this function
# to unhide this application.  On other plaforms it is a no-op.
#
# Also, the Macintosh needs "Command" instead of "Alt" in menu accelerators.  For some mysterious reason,
# the "Open Apple" is called "Command".
#
if {$tcl_platform(platform) == "macintosh"} {
    package require Tclapplescript
    proc activate {} { AppleScript execute "tell application \"PPR Popup\" to activate" }
    set menu_accel_modifier "Command"
    } else {
    proc activate {} {}
    set menu_accel_modifier "Alt"
    }

#
# Different operating systems need different id getting functions.
# The ID is a piece of information that will be present in their print
# jobs  that can be used to correlate specific jobs with specific
# computers running PPR Popup.
#
switch -exact -- $tcl_platform(platform) {
    macintosh {
	proc get_client_id {} {
		package require Tclapplescript

		set id [AppleScript execute {
			tell application "Network Setup Scripting"
				try
					open database
					set con_set to current configuration set
					set conAt to item 1 of AppleTalk configurations of con_set
					set network to network ID of conAt
					set node to node ID of conAt
					close database
					return (network as string) & "." & (node as string)
				on error errmsg
					tell application "Finder"
						display dialog "Error: " & errmsg
					end tell
				end try
			end tell
			}]

		# Remove the quote marks from the result and return it.
		regsub {^"([^"]+)"$} $id {\1} id
		return $id
		}
	}
    windows {
	package require registry 1.0
	proc get_client_id {} {
		if [catch { set computername [registry get "HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\control\\ComputerName" "ComputerName"] } ] {
		    if [catch { set computername [registry get "HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\control\\ComputerName\\ComputerName" "ComputerName"] } ] {
			alert "Can't find ComputerName in registry!"
			exit 1
			}
		    }
		return $computername
		}
	}
    unix {
	proc get_client_id {} {
		global env
		if { [info exists env(PPR_RESPONDER_ADDRESS)] } {
		    return $env(PPR_RESPONDER_ADDRESS)
		    }
		if { [ info exists env(LOGNAME)] } {
		    return "$env(LOGNAME)@localhost"
		    } 
		if { [ info exists env(USER)] } {
		    return "$env(USER)@localhost"
		    } 
		alert "Neither the environment variable PPR_RESPONDER_ADDRESS nor USER nor LOGNAME is defined."
		exit 1
		}
	}
    }

#
# Make a window that we have kept withdrawn while we are
# preparing it appear or call attention to a window that
# the user has been neglecting.
#
#
proc window_reopen {win} {

    # This call to update works around a problem in the X-Windows
    # version which causes windows to appear at a default size
    # for a few seconds.
    update

    # This overrides a semi-bug in MS-Windows NT 5.0.  If we don't do this
    # then the window will remain behind the application that has focus.
    # What we do is temporarily make it an unmanaged window.  Unmanaged
    wm overrideredirect $win 1
    wm overrideredirect $win 0

    # We may have iconified the window while we were filling it with data.
    # Make sure it is out in the open.
    wm deiconify $win

    # Raise the window to the top and try to give it focus.  Hopefully this
    # will work within the context of the whole desktop and not just
    # this program.
    raise $win
    focus $win
    }

#
# Command to ask the user for his name.
#
proc command_USER {file} \
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

    label $w.padded.message -text "Please enter your name:"
    entry $w.padded.entry -width 40 -background white
    bind $w.padded.entry <Return> [list $w.ok invoke]
    pack $w.padded.message $w.padded.entry \
	-side top \
	-anchor w \
	-padx 5 -pady 5

    frame $w.pad -width 15
    button $w.ok -text "OK" -command [list command_USER_ok $w]
    button $w.cancel -text "Cancel Job" -command "
	set result {-ERR cancel job}
	destroy $w"
    pack $w.pad -side right
    pack $w.ok $w.cancel -side right -padx 5 -pady 5

    # Set the focus on the entry field and wait
    # for the window to be destroyed.
    window_reopen $w
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
#    wm withdraw $w

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
	-side right \
	-padx 20 -pady 5

    bind $w <Return> [list $w.dismiss invoke]

    # Arrange for a callback whenever data is available.
    fileevent $file readable [list command_MESSAGE_datarecv $file $w]
    }

#
# This is called as a fileevent every time a line
# of the message is received.
#
proc command_MESSAGE_datarecv {file w} {

    # If there is a line to be had,
    if {[gets $file line] >= 0} {

	# A single period indicates end-of-message.
	if {$line == "."} {
	    # Acknowledge the completed command.
	    puts $file "+OK"
	    flush $file

	    # Return the socket to the command dispatcher.
	    fileevent $file readable [list server_reader $file]

	    # Deiconify the finished window and bring it the the front.
	    window_reopen $w

	    return
	    }

	# If not EOM, add this line as text.
	$w.message.text insert end "$line\n"
	}

    }

#
# Create a web browser window and download a page into it.
# We have to list the close function first due to forward
# reference problems.
#
proc command_QUESTION_close {jobname} {
    global open_windows
    #puts "Window manager request to close window $open_windows($jobname) for $jobname."

    destroy $open_windows($jobname)
    unset open_windows($jobname)
    .questions.list delete [lsearch -exact [.questions.list get 0 end] $jobname]
    }

proc command_QUESTION {file jobname url width height} {
    global wserial
    global open_windows

    if {![server_authcheck $file]} {
	return
	}

    if [info exists open_windows($jobname)] {
    	#puts "  Already exists"
	window_reopen $open_windows($jobname)
	puts $file "+OK already exists"
    	return
    	}

    set w .html_[incr wserial]
    toplevel $w
#    wm withdraw $w

    set open_windows($jobname) $w
    .questions.list insert end $jobname
    wm protocol $w WM_DELETE_WINDOW [list command_QUESTION_close $jobname]

    Browser $w.browser \
	-width $width -height $height \
	-wrap word -linkcommand "$w.browser import" -padx 10 \
	-hscrollmode dynamic \
	-vscrollmode dynamic \
	-getcommand [itcl::code shared_urlfetch get] \
	-postcommand [itcl::code shared_urlfetch post] \
	-closecommand [itcl::code command_QUESTION_close $jobname] \
	-titlecommand [itcl::code wm title $w]
    pack $w.browser -side top -anchor w -fill both -expand 1
    $w.browser import $url

    window_reopen $w

    puts $file "+OK"
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
  #puts "$file: Connection from $cli_addr:$cli_port"
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
	catch { close $file }
	return
	}

    #puts "$file: $line"

    # Act on the command received
    switch -glob -- $line {
	"COOKIE *" {
	    if {[regexp {^COOKIE (.+)} $line junk try_cookie]} {
		command_COOKIE $file $try_cookie
		} else {
		puts $file "-ERR wrong number of parameters"
		}
	    }
	"USER" {
	    command_USER $file
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
		puts $file "-ERR wrong number of parameters"
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
	QUIT {
            puts $file "+OK"
	    server_close $file
	    return
            }
	* {
            puts $file "-ERR Unknown command: $line"
            }
	}

    # Push out the reply.
    if [catch { flush $file }] {
	puts "$file: Unexepected disconnect by print server!"
	server_close $file 
	}
    }

#========================================================================
# Magic cookie verification
#========================================================================
proc command_COOKIE {file try_cookie} {
    global magic_cookie
    global authenticated_clients

    if {![info exists magic_cookie]} {
	puts "$file: -ERR no magic cookie yet"
	puts $file "-ERR no magic cookie yet"
	return
	}

    if {[string compare $try_cookie $magic_cookie] != 0} {
	puts "$file: -ERR incorrect cookie"
	puts $file "-ERR incorrect cookie"
	return
	}

    set authenticated_clients($file) 1
    #puts "$file: +OK"
    puts $file "+OK"
    }

proc server_close {file} {
    global authenticated_clients
    catch { close $file }
    catch { unset authenticated_clients($file) }
    }

proc server_authcheck {file} {
    global authenticated_clients
    if {[info exists authenticated_clients($file)]} {
	return 1
	} else {
	puts "-ERR must issue sucessful COOKIE command first"
	puts $file "-ERR must issue sucessful COOKIE command first"
	return 0
	}
    }

# This is called by a handler bound to mouse motion.
proc entropy {entropy} {
    global ppr_magic_cookie_seed
    #puts "\$entropy = \"$entropy\""

    # Add this mouse motion (or whatever) to the seed for the next magic cookie.
    append ppr_magic_cookie_seed $entropy

    # If we have as much entry as we think we need, top intercepting mouse
    # motion events, it is a waste of time.
    #puts "Seed size: [string length $ppr_magic_cookie_seed]"
    if {[string length $ppr_magic_cookie_seed] >= 256} {
	bind all <Motion> {}
	bind all <KeyPress> {}
	}
    }

#========================================================================
# This is what we use to register with the server.
#========================================================================

proc register_with_server {register_url} {
    package require http 2.3
    global server_socket
    global magic_cookie
    global client_id

    #puts "Registering with server at <$register_url>..."

    set sockname [fconfigure $server_socket -sockname]
    set ip [lindex $sockname 0]
    set port [lindex $sockname 2]
    set pprpopup_address "$ip:$port"

    #puts "client_id=$client_id, pprpopup_address=$pprpopup_address, magic_cookie=$magic_cookie"
    set data [eval ::http::formatQuery [list client $client_id pprpopup_address $pprpopup_address magic_cookie $magic_cookie]]

    if [catch {::http::geturl $register_url -query $data -command [namespace code register_callback]} errormsg] {
	puts "Can't contact server $register_url, $errormsg, retry in 30 seconds."

	# Re-schedual for 30 seconds (30,000 milliseconds) in the future.
	after 30000 [list register_with_server $register_url]
	}
    }

proc register_callback {token} {
    global registration_status

    # Get the result codes for the POST.
    upvar #0 $token state
    #puts "Registration with <$state(url)> finished:"
    regexp {^HTTP/[0-9]+\.[0-9]+ ([0-9]+)} $state(http) junk ncode
    #puts "    State: $state(status)"
    #puts "    Ncode: $ncode"
    #puts "    Size: $state(totalsize)"

    # Test the status for errors.
    if {[string compare $state(status) "ok"] == 0 && $ncode == 200} {
	# Sucess
	set registration_status($state(url)) 0

	} else {

	# Set error counter.
	incr registration_status($state(url))

	# Describe the error.
	if {[string compare $state(status) "ok"] != 0} {
	    # Transaction didn't complete.
	    alert "PPR Popup was unable to register with the print server for the reason indicate below.\n\nPOST to <$state(url)> failed:\n$state(status)\n\n$state(error)"
	    } else {
	    # Transaction completed but result was unsatisfactory.
	    alert "PPR Popup was unable to register with the print server for the reason indicated below.\n\nPOST to <$state(url)> failed:\n$state(http)\n\n$state(body)"
	    }

	# Re-schedual for 30 seconds (30,000 milliseconds) in the future.
	after 30000 [list register_with_server $state(url)]
	}
    }

proc do_register {} {
    global client_id
    global ppr_server_list
    global magic_cookie
    global ppr_magic_cookie_seed
    global registration_interval
    global registration_status

    #puts "do_register"

    # We only get the client ID once and cache the result.
    # This reduces AppleScript timeout problems on Macintoshes.
    if {![info exists client_id]} {
	if [catch { set client_id [get_client_id] } result] {
	    global errorInfo
	    alert "Error getting client ID: $result\n\n$errorInfo"
	    after 60000 do_register
	    return
	    }
	}

    # If there is insufficient entropy to generate a new magic cookie,
    # then time some code and use the noise.  I assume we are measuring
    # schedualer noise, but I have no idea how random it is.
    if {[string length $ppr_magic_cookie_seed] < 64} {
	#puts -nonewline "Looking for more entropy."
	for {set x 0} {[string length $ppr_magic_cookie_seed] < 64} {incr x} {
	    #puts -nonewline "."
	    set time_output [time { catch { open "_not_a_real_file_name_$x" } }]
	    regexp {^([0-9]+)} $time_output count
	    append ppr_magic_cookie_seed [expr $count % 10]
	    }
	#puts ""
	}

    # Reduce the seed to an MD5 hash, hash it with the current time to make
    # the new magic cookie, and keep the first hash as the start of the next
    # magic cookie seed.
    set ppr_magic_cookie_seed [md5pure::md5 $ppr_magic_cookie_seed]
    set magic_cookie [md5pure::md5 "[clock clicks] $ppr_magic_cookie_seed"]

    # We safe this proto-seed to the config file so that when we are restarted
    # we don't have to start from scratch.
    if {[catch {random_save $ppr_magic_cookie_seed} errormsg]} {
	puts "Failed to save magic cookie seed."
	}

    # Start collecting entropy for next time.  The least significating
    # (decimal) digit of the mouse's x and y coordinates will be added
    # to the seed.
    bind all <Motion> { entropy [expr %x %% 10][expr %y %% 10] }    
    bind all <KeyPress> { entropy [expr [clock clicks] %% 100] }

    # Start the registration process for each server.
    foreach url $ppr_server_list {
        set url "${url}cgi-bin/pprpopup_register.cgi"
	#puts "$url"

	# Make sure we have a status entry for this URL.
	if {![info exists registration_status($url)]} {
	    set registration_status($url) 0
	    }

	# If there isn't already an (errored) registration in progress,
	# start a new one.
	if {$registration_status($url) == 0} {
	    incr registration_status($url)
	    register_with_server $url 
	    } else {
	    puts "Registration already in progress, tries = $registration_status($url)."
	    }
	}

    # Register again in a few minutes.
    after [expr $registration_interval * 1000] do_register
    }

#========================================================================
# main and its support routines
#========================================================================

proc main {} {
    global ppr_root_url
    global ppr_server_list

    # This is supposed to be set by pprpopup_loader.tcl.
    if {![info exists ppr_root_url]} {
	alert "You can't execute this script directly.  It must be started by pprpopup_launch.tcl."
	exit 1
	}

    # And this provides a default for the list of servers we register with.
    if {![info exists ppr_server_list]} {
	set ppr_server_list [list $ppr_root_url]
	}

    # Set the monitor DPI to 90.
    global system_dpi
    set system_dpi [winfo pixels . 1i]
    tk scaling 1.25

    # Create a URL fetching object which will be used for registration and for
    # fetching questions.
    urlfetch shared_urlfetch

    # Set up a Quit handler for the main window.  This will get called if
    # the user uses the window manager to close the main window.
    wm protocol . WM_DELETE_WINDOW { menu_file_quit }

    # Create a set of bindings which we can attach to widgets.  This has no effect on Macintosh.
    # I don't know why.
    bind GlobalBindTags <Alt-q> { menu_file_quit }
    bind GlobalBindTags <Alt-slash> { menu_help_contents }
    bind GlobalBindTags <Alt-question> { menu_help_contents }
    bindtags . GlobalBindTags

    # Create the menubar and attach it to the default toplevel window.
    . configure -menu .menubar
    menu .menubar -border 1 -relief groove

    # Here are some MacOS hacks.
    global tcl_platform
    if {$tcl_platform(platform) == "macintosh"} {

	# Macintosh needs another toplevel to keep the menu in place.
	# We will place it off the screen.
        toplevel .dummy -menu .menubar
        wm geometry .dummy 25x25+2000+2000
	bindtags .dummy GlobalBindTags

	# We also add a menu called "apple" to override the default "About Tcl & Tk".
	.menubar add cascade -menu [menu .menubar.apple -tearoff 0 -border 1]
	.menubar.apple add command -label "About PPR Popup" -command { menu_help_about }
        }

    # This global string has the name of the Alt/Ctrl/Shift/Meta/Cmd key used in menu accelerators.
    global menu_accel_modifier

    .menubar add cascade -label "File" -menu [menu .menubar.file -tearoff 0 -border 1]
    .menubar.file add command -label "Quit" -accelerator "${menu_accel_modifier}-Q" -command { menu_file_quit }

    .menubar add cascade -label "View" -menu [menu .menubar.view -tearoff 0 -border 1]
    .menubar.view add check -variable menu_view_main_visibility -label "Show Main Window" -command { menu_view_main $menu_view_main_visibility }
    .menubar.view add check -variable menu_view_tk_console_visibility -label "Show Tk Console" -command { menu_view_tk_console $menu_view_tk_console_visibility }

    .menubar add cascade -label "Tools" -menu [menu .menubar.tools -tearoff 0 -border 1]

    .menubar add cascade -label "Help" -menu [menu .menubar.help -tearoff 0 -border 1]
    .menubar.help add command -label "Help Contents" -accelerator "${menu_accel_modifier}-?" -command { menu_help_contents }
    .menubar.help add command -label "About PPR Popup" -command { menu_help_about }
    .menubar.help add command -label "About System" -command { menu_help_about_system }

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
    label .questions.label -text "Outstanding Questions:"
    iwidgets::scrolledlistbox .questions.list \
	-borderwidth 2 -relief groove \
	-hscrollmode none \
	-vscrollmode static \
	-scrollmargin 0 \
	-visibleitems 20x2 \
	-dblclickcommand {
		global open_windows
		set url [.questions.list getcurselection]
		#puts "\$url = $url"
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
    global server_port
    if [catch {set server_socket [socket -server server_function $server_port]} result] {
	alert "Can't bind to port: $result"
	exit 1
	}

    # Register with the servers for the first time and schedual
    # future registrations.
    do_register

    # Set the main window size, title, and visibility.
    wm geometry . 600x200
    wm title . "PPR Popup"
    menu_view_main 0

    #puts "Saving configuration..."
    if {[catch {config_save "set ppr_root_url \"$ppr_root_url\"\nset ppr_server_list {$ppr_server_list}\n"} errormsg]} {
	puts "Failed to save configuration:\n$errormsg"
	#alert "Failed to save configuration:\n$errormsg"
	}
    }

proc menu_file_quit {} {
    global button_color
    global wserial

    # Create a unique window name.
    set w .quit_confirm_[incr wserial]

    # This is a GIF file converted to base64 with uuencode -m.
    set stop_image [image create photo -data {
R0lGODlhiwB5AKEAAPDw8P8AAP///wAAVSH5BAEAAAAALAAAAACLAHkAAAL+
hI+pq+EPo5y02huZ3rz7hIXiSAbfiaZgybasCseaS9eWjOOQwPf+DwwKh8Ri
Joc8RXi2JokJSUo5O4HzGoI+ptxGFQu2aB3dMuAbTkvGJvMUrY4/2G4pXC4f
15F3fJy+F7Nk5ecHGJjSV6h2iPgxuFjY6Eg1RxiJN0nJoIjJeEm2udHpmaYp
akBaGnYqCrkqCdqGeqAKC9bq+HprKEubaskbqUdrK4zri2p8jJVrtsx85dy1
G917hAht7TRtp73d1M0XDD6c7PZdHn7+TK6+KA6T/m4TnzhPX2Ov5J4PD0oN
nz8a+zoIHEiQnY4dCD0VnNGvYSxsMg5KdPFwRcT+i9eiVNzIseMWeRZDtsh4
BqTJTAo9LFlZ6mFJmCdbjlJJk6VHlzNzsojX0+dPmxodsBE6cSdEo7LEDJlQ
xAgFIhii+hDh7GUWq4OsBoHKtYLXHlspemG61WuVsT/WsJ3KtilYs7Vwrh1r
KS7Zu3jdxi2rFJhdpm357tV7Ne9fv4ud0oVWGGxisZPhflUcGbPlypQpVqO8
V3LozXINlw5wWTPp030QaMvMmDVqzl1p12YD+zbWZFrTjk74+7ZswrhtqwYc
6ltq4EcN+x6Tm++I5minSy0R/fgFINJXW3dsPexzp8FBQze+fLuElNXDiydP
nfhw7Yi/e/z8nOp24/L+3YOqj9xI7LX3xHuisZbdgaZdVxU7+A0lBHwI8hfb
gk/t9lhQsyW4YXwdeojZf2oVSNeAIGJHIYfcNfgbhyQGVpddK5o3YXnOSSiL
i9Otx4kqM5Im4X4JpqdjgLMs8KB8NdrYH44tpvfhOjwuRaBwxbmoI2JyFblf
iUXNRmNfNOZnYJMJeXlWlRZeGCSLYuqmD1FIDjaim/MpGeGYZ8J402Afcmkm
mRQG+tOUj2iIFFxoUglmop8Yyo+fjg61aCVqThpnpZZeimlNmm6KWqdS8nmP
pKKKBakgiAqFUpqNnvoiqSSZCmuVAXFaq1ufloprrhG1Q2unrYJ6IqxN1ZGh
ZK7D8tTrqUcFkqyzcnLRm6+vhpLNqgMti0K0jnLbrbb0HOuKuOWAq4K52zxb
jLrRoDtrszDBG++1306ri7u30PuRvCGR+4tr+saEb7lzJMpuwF8WixC/4wT7
jsMP++uPxBPby5HFSQycSaoKu8rwuQV/PCDG2wJMMqMhW6MxtRw3M3LKgv2p
Zc1a7ipztSHJHK5QPEea08890yQ00CvtUQAAOw==
}]

    # For some reason, the -justify option doesn't work on MacOS.
    iwidgets::messagedialog $w \
            -modality application \
            -title "Confirmation" \
	    -image $stop_image \
            -text \
"If you close this program, this computer will be unable to print to 
the public printers.  As a courtesy to the next user, you should
leave it running."
    $w buttonconfigure OK -text "Close" \
		-background $button_color \
		-activebackground $button_color \
		-defaultring false \
		-defaultringpad 0 \
		-highlightthickness 0
    $w buttonconfigure Cancel -text "Don't Close" \
		-background $button_color \
		-activebackground $button_color \
		-defaultring false \
		-defaultringpad 0 \
		-highlightthickness 0
    $w default Cancel
    if {[$w activate]} {
        exit 0
        } else {
        destroy $w
	image delete $stop_image
        }
    }

set menu_view_tk_console_visibility 0
proc menu_view_tk_console {yes} {
    if {$yes} {
	if [catch { console show }] {
	    alert "No Tk Console support on this platform."
	    }
	} else {
	catch { console hide }
	}
    }

set menu_view_main_visibility 0
proc menu_view_main {yes} {
    if {$yes} {
	wm deiconify .
	} else {
	# Withdraw on MacOS and iconify on others.  If we try to iconify it
	# on MacOS, it is reduced to a title bar (window shade up).
	global tcl_platform
	if {$tcl_platform(platform) == "macintosh"} {
	    wm withdraw .
	    } else {
	    # Iconify it.  Since the user must deiconify it to interact with the
	    # menu, we will set visibility back to true.
	    wm iconify .
	    global menu_view_main_visibility
	    set menu_view_main_visibility 1
	    }
	}
    }

proc menu_help_contents {} {
    global wserial
    global ppr_root_url
    global help_contents_file
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
	-postcommand [itcl::code $w.urlfetch post] \
	-titlecommand [itcl::code wm title $w]
    pack $w.browser -side top -anchor w -fill both -expand 1
    $w.browser import $ppr_root_url$help_contents_file
    }

proc menu_help_about {} {
    global about_text
    global button_color
    iwidgets::messagedialog .about \
    	-modality application \
    	-title "About PPR Popup" \
    	-text $about_text
    .about buttonconfigure OK -text "OK" \
		-background $button_color \
		-activebackground $button_color \
		-defaultring false \
		-defaultringpad 0 \
		-highlightthickness 0
    .about hide "Cancel"
    .about default OK
    .about activate
    destroy .about
    }

proc menu_help_about_system {} {
    global system_dpi
    global tcl_platform
    global button_color
    global env

    set info ""

    set tclversion [info tclversion]
    set patchlevel [info patchlevel]
    append info "Tcl Version: $tclversion patch level $patchlevel\n"

    foreach name [array names tcl_platform] {
	set value $tcl_platform($name)
	append info "Platform $name: $value\n"
	}

    set screenheight [winfo screenheight .]
    set screenwidth [winfo screenwidth .]
    append info "Screen size: ${screenwidth}x${screenheight}\n"

    set dpi [winfo pixels . 1i]
    append info "Screen resolution (OS): $system_dpi DPI\n"
    append info "Screen resolution (Tk): $dpi DPI\n"

    # Create a dialog box.
    iwidgets::dialog .si \
    	-modality application \
    	-title "About System"
    .si buttonconfigure OK -text "Dismiss" \
		-background $button_color \
		-activebackground $button_color \
		-defaultring false \
		-defaultringpad 0 \
		-highlightthickness 0
    .si hide "Cancel"
    .si hide "Apply"
    .si hide "Help"
    .si default OK

    # Get a reference to the childsite so we can start packing stuff in.
    set childsite [.si childsite]

    # First we add a label containing the text we got above.
    label $childsite.label -justify left -text $info
    pack $childsite.label -side left -anchor nw

    # Add a spacer.
    frame $childsite.pad1 -width 15
    pack $childsite.pad1 -side left

    # Now add a frame with DPI information.
    frame $childsite.dpi
    pack $childsite.dpi -side left -anchor nw
    label $childsite.dpi.title -text "Resolution Test"
    pack $childsite.dpi.title -side top
    frame $childsite.dpi.square1 -width $system_dpi -height $system_dpi -background white
    pack $childsite.dpi.square1 -side top -anchor w
    label $childsite.dpi.label1 -text "1 inch at $system_dpi DPI"
    pack $childsite.dpi.label1 -side top -anchor w
    frame $childsite.dpi.pad -height 5
    pack $childsite.dpi.pad -side top
    frame $childsite.dpi.square2 -width $dpi -height $dpi -background white
    pack $childsite.dpi.square2 -side top -anchor w
    label $childsite.dpi.label2 -text "1 inch at $dpi DPI"
    pack $childsite.dpi.label2 -side top -anchor w

    # Add a spacer.
    frame $childsite.pad2 -width 15
    pack $childsite.pad2 -side left

    frame $childsite.env
    pack $childsite.env -side left
    label $childsite.env.label -text "Environment Variables"
    pack $childsite.env.label -side top
    text $childsite.env.text -width 40 -height 20 -wrap none -background white
    scrollbar $childsite.env.vsb -command "$childsite.env.text yview"
    scrollbar $childsite.env.hsb -orient horizontal -command "$childsite.env.text xview"
    $childsite.env.text configure -xscrollcommand "$childsite.env.hsb set" -yscrollcommand "$childsite.env.vsb set"
    pack $childsite.env.vsb -side right -fill y
    pack $childsite.env.hsb -side bottom -fill x
    pack $childsite.env.text -side left -fill both -expand 1
    foreach key [lsort [array names env]] {
	$childsite.env.text insert end "$key=$env($key)\n"
	}

    .si activate
    destroy .si
    }

main

# end of file
