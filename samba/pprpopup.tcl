#!/usr/bin/wish
#
# pprpopup.tcl
# Copyright 1997, Trinity College Computing Center.
# Written by David Chappell. 
#
# This nifty program is a server which when an outside entity 
# requests it, asks the user for his name.  It also can receive
# messages.
#
# Last revised 11 February 1997.
#

# This is the port that this server should listen on:
set server_socket 15009

# Window serial number.
set wserial 0

# Procedure to display an alert box.
proc Alert {title lines} \
    {
    global wserial

    incr wserial
    set w [toplevel .alert$wserial -bg black]
    wm title $w $title

    frame $w.i
    pack $w.i -padx 2 -pady 2
    
    frame $w.i.i
    pack $w.i.i -padx 10 -pady 10

    set x 0
    foreach e $lines \
	{
	label $w.i.i.$x -text $e
	pack $w.i.i.$x -side top -anchor w
	incr x
	}

    frame $w.i.i.pad1 -height 5
    button $w.i.i.ok -text OK -command [list destroy $w]
    pack $w.i.i.pad1 $w.i.i.ok -side top

    focus $w
    grab $w
    tkwait window $w
    }

# A routine to add debugging output to the text
# window which has been provided for that purpose.
set debug_lines 0
proc debug {message} \
  {
  global debug_lines
  incr debug_lines
  if { $debug_lines > 100 } { .text delete 1.0 2.0}
  .text insert end "$message\n"
  update idletasks
  }

#
# Command to ask the user for his name.
#
proc command_USER {file message} \
  {
  global result
  global wserial

  debug "$file: USER $message"

  set result ""

  # Make a top level window that we can 
  # pop up.
  incr wserial
  set w [toplevel .user$wserial]
  wm title $w "User Identification"

  # This frame will be placed inside the toplevel window
  # but it will be packed with padding.
  frame $w.padded
  pack $w.padded -padx 20 -pady 20

  if {$message != ""} \
    {
    message $w.padded.message1 \
	-text $message -fg red -width 300
    pack $w.padded.message1 -side top -anchor w -padx 5 -pady 5 
    }

  label $w.padded.message2 -text "Please enter your name:"
  entry $w.padded.entry -width 40 
  bind $w.padded.entry <Return> [list $w.ok invoke]
  pack $w.padded.message2 $w.padded.entry \
	-side top -anchor w -padx 5 -pady 5

  button $w.ok -text "OK" -command [list command_USER_ok $w]
  button $w.cancel -text "Cancel Job" -command "
	set result {-ERR cancel job}
	destroy $w"
  pack $w.ok $w.cancel -side right

  # Set the focus on the entry field and wait
  # for the window to be destroyed.
wm iconify $w
wm deiconify $w
  focus $w.padded.entry
  tkwait window $w

  if {$result == ""} {set result "-ERR window destroyed"}

  debug "$file: Reply: $result"
  puts $file $result
  flush $file
  }

#
# Procedure which is called when the user presses 
# <Return> or OK after entering his name.
#
proc command_USER_ok {w} \
  {
  global result

  set r [$w.padded.entry get]

  if { $r != "" } \
    {
    set result "+OK $r"
    destroy $w
    }
  }

#
# Display a message from the Spooler
#
proc command_MESSAGE {file for} \
  {
  global wserial

  debug "$file: MESSAGE $for"

  incr wserial
  set w [toplevel .message$wserial]
  wm title $w "Message for $for"
  frame $w.message
  pack $w.message -side top -fill both -expand true

  text $w.message.text \
	-width 75 -height 8 -bg white \
	-yscrollcommand "$w.message.sb set"
  scrollbar $w.message.sb -orient vert -command "$w.message.text yview"
  pack $w.message.text -side left -fill both -expand true
  pack $w.message.sb -side right -fill y 

  button $w.dismiss -text "Dismiss" -command [list destroy $w]
  pack $w.dismiss -side right

  bind $w <Return> [list $w.dismiss invoke]

  # Keep it iconified until it is full
  wm iconify $w

  # Establish a procedure to read the message.
  fileevent $file readable [list command_MESSAGE_datarecv $file $w]
  }

#
# This is called as a fileevent every time a line
# of the message is received.
#
proc command_MESSAGE_datarecv {file w} \
  {
  if {[gets $file line] >= 0} \
    {
    if {$line == "."} \
	{
	puts $file "+OK"
	flush $file
	fileevent $file readable [list server_reader $file]
      wm deiconify $w
      focus $w
	return
	}
    $w.message.text insert end "$line\n"
    }
  }

#
# Play a sound on an SMB server
#
proc command_SMBPLAY {file name} \
  {
  debug "$file: SMBPLAY $name"
  exec "c:/Program Files/Netscape/Navigator/Program/naplayer.exe" $name
  debug "$file: player done"
  puts $file "+OK"
  flush $file
  }

#
# This is the function which is called when a connexion
# is made to this server.
#
proc server_function {file cli_addr cli_port} \
  {
  debug "$file: Connection from $cli_addr:$cli_port"
  fconfigure $file -blocking false
  fileevent $file readable [list server_reader $file]
  }

#
# This is called whenever there
# is anything to read on the socket.
#
proc server_reader {file} \
  {
  # Get the next line from the socket
  if {[set line [gets $file]] == ""} \
    {
    debug "$file: EOF or unterminated line, closing connexion\n"
    catch [close $file] result
    return
    }
    
  # Act on the command received
  switch -glob -- $line \
    {
    QUIT	{
		debug "$file: QUIT command\n"
		puts $file "+OK"
		flush $file
		close $file
		}
    USER*	{
		debug "$file: USER command"
		set message ""
		regexp {^USER (.*)} $line junk message
		command_USER $file $message
		}
    MESSAGE*	{
		debug "$file: MESSAGE command"
		set for "???"
		regexp {^MESSAGE (.*)} $line junk for
		command_MESSAGE $file $for
		}
    SMBPLAY*	{
		debug "$file: SMBPLAY command"
		regexp {^SMBPLAY (.*)} $line junk name
		command_SMBPLAY $file $name
		}
    UNHIDE	{
		debug "$file: UNHIDE command"
		debug "UNHIDE"
		wm deiconify .
		}
    *		{
		debug "$file: Unknown command: \"$line\""
		puts $file "-ERR Unknown command"
		flush $file
		}
    }
  }

# Make a debug output window
text .text -yscrollcommand ".sb set"
scrollbar .sb -orient vert -command ".text yview"
pack .text -side left -fill both -expand true
pack .sb -side right -fill y 

# Set up the server to listen on a TCP socket
if [catch {socket -server server_function $server_socket} result] \
  {
  Alert "Popup Startup Error" [list "The printer user name popup program" \
	"could not be started for the following reason:" $result]
  exit 1
  }

# Hide the debug window
#wm withdraw .
wm iconify .
wm title . "PPR Popup Debug Output"

# end of file

