#
# urlfetch.itk
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
# Last modified 5 November 2001.
#

package require Tcl 8.3
package require Tk 8.3
package require http 2.2
package require Itcl 3.1

#-----------------------------------------------------------
# This object manages the HTTP connections and may someday
# include a cache.
#-----------------------------------------------------------
itcl::class urlfetch {
  protected variable token_commands

  public method get {url command} {
    puts "url::get $url $command"
    set token [::http::geturl $url -command [list $this callback]]
    set token_commands($token) $command
    }

  public method post {url data command} {
    puts "url::post $url $data $command"
    set token [::http::geturl $url -query $data -command [list $this callback]]
    set token_commands($token) $command
    }

  public method callback {token} {
    if [ catch {
	upvar #0 $token state

	set code -1
	set message "unknown"
	switch -- $state(status) {
	    ok {
	      regexp {^HTTP/[0-9]+\.[0-9]+ ([0-9]+) (.*)$} $state(http) match code message
	      }
	    eof {
	      set message "eof"
	      }
	    error {
	      set $state(error)
	      }
	    }

	set content_type "application/octet-stream"
	array set meta $state(meta)
	if [info exists meta(Content-Type)] {
	  set content_type $meta(Content-Type)
	  }

	$token_commands($token) $code $message $content_type $state(body)

	::http::cleanup $token
	} result] { puts $result }
    }
  }

# end of file
#
# browser.itk
# Copyright (c) 1996 DSC Technologies Corporation
# Copyright 1995--2001, Trinity College Computing Center.
# Written by David Chappell.
# Last modified 7 December 2001.
#

package require Itcl
package require Itk
package require Iwidgets

#===========================================================
# WWW Browser Object DOM (very limited)
#===========================================================

#-----------------------------------------------------------
# This object represents the contents of a <form> tag.
#-----------------------------------------------------------
itcl::class Browser::form {
  protected variable browser
  public variable action ""
  public variable method "GET"
  public variable name ""
  protected variable elements {}

  # This constructor simply records the browser object which
  # contains this form.
  constructor {browser_obj} {
    set browser $browser_obj
    }

  public method get_browser {} {
    return $browser
    }

  # This is called whenever an element is added to the form.  The element
  # objects are kept in a list so that they can all be found at submit time.
  public method add_element {newelement} {
    $newelement configure -containing_form $this
    lappend elements $newelement
    }

  # This is called to reset the form to its initial state.
  public method reset {} {
    puts "Browser::form::reset"
    foreach element $elements {
      $element reset
      }
    }

  # This is the function which is called to submit the form.
  # It must gather the values, do the GET or POST, and
  # fill the browser window with the returned document.
  public method submit {} {
    puts "Browser::form::submit"

    # Get the value from each element and collapse them all into
    # a flat list of name and value pairs.
    set list {}
    foreach element $elements {
      #puts "  $element"
      set ret [$element get_value]
      #puts "    $ret"
      set list [concat $list $ret]
      }

    # Encode the form values.
    set encoded [eval ::http::formatQuery $list]
    #puts $encoded

    # Make sure this form has an action URL.
    if {$action == ""} {
      puts "!!! No Action for Form \"$name\" !!!"
      return
      }

    # Take the required action according to the request method.
    switch -exact -- [string toupper $method] {
      GET {
    	$browser import $action $encoded
    	}
      POST {
    	$browser import $action "" $encoded
    	}
      default {
	puts "Invalid form method \"$method\""
        }
      }

    }

  }

#-----------------------------------------------------------
# This is the basic class for representing <input>,
# <select>, <textarea>, <button>, etc.
#-----------------------------------------------------------
itcl::class Browser::form_element {
  public variable containing_form ""
  public variable name ""
  public variable value_initial ""
  public variable value ""
  protected variable widget ""

  # Restore the field to its original value.
  public method reset {} {
    set value $value_initial
    }

  # Get the name and value as a list suitable for submitting.
  public method get_value {} {
    if {[string compare $name ""] != 0} {
      return [list $name $value]
      }
    return {}
    }

  # This is used to determine the background color.
  protected method get_bgcolor {} {
    return [[$containing_form get_browser] get_bgcolor]
    }

  # Create the window and return its name so that it
  # can be added to the text widget.
  public method window {newname}
  }

# If the default value is changed, change the current value.
itcl::configbody Browser::form_element::value_initial {
  set value $value_initial
  }

#-----------------------------------------------------------
# <input type="button">
#-----------------------------------------------------------
itcl::class Browser::form_element_input_button {
  inherit form_element

  public method window {newname} {
    button $newname
    if [info exists value] {
      $newname configure -text $value
      }
    set widget $newname
    return $newname
    }

  }

#-----------------------------------------------------------
# <input type="checkbox">
#-----------------------------------------------------------
itcl::class Browser::form_element_input_checkbox {
  inherit form_element

  public variable checked 0

  public method reset {} {
    if {$checked} {
      set value $value_initial
      } else {
      set value ""
      }
    }

  public method window {newname} {
    checkbutton $newname \
    	-offvalue "" \
    	-onvalue $value_initial \
    	-variable [itcl::scope value] \
    	-background [get_bgcolor] \
    	-activebackground [get_bgcolor] \
    	-highlightthickness 0
    $this reset
    set widget $newname
    return $newname
    }

  # We must override this function since checkboxes contribute
  # no value when they are unchecked.
  public method get_value {} {
    if {[string compare $name ""] != 0} {
      if {[string compare $value ""] != 0} {
        return [list $name $value]
        }
      }
    return {}
    }

  }

#-----------------------------------------------------------
# <input type="hidden">
#-----------------------------------------------------------
itcl::class Browser::form_element_input_hidden {
  inherit form_element
  }

#-----------------------------------------------------------
# <input type="image">
# Imagemaps are not implemented yet.
#-----------------------------------------------------------
itcl::class Browser::form_element_input_image {
  inherit form_element

  public variable src ""

  public method window {newname} {
    button $newname -text $src
    set widget $newname
    return $newname
    }

  }

#-----------------------------------------------------------
# <input type="radio">
# There must be a better way than a global variable.
#-----------------------------------------------------------
itcl::class Browser::form_element_input_radio {
  inherit form_element

  public method window {newname} {
    global radio_groups
    radiobutton $newname \
    	-value $value_initial \
    	-variable radio_groups($containing_form.$name) \
    	-background [get_bgcolor] \
    	-activebackground [get_bgcolor] \
    	-highlightthickness 0
    set widget $newname
    return $newname
    }

  public method get_value {} {
    global radio_groups
    if {[string compare $name ""] != 0} {
      if {[string compare $value $radio_groups($containing_form.$name)] == 0} {
	return [list $name $value]
        }
      }
    return {}
    }

  }

#-----------------------------------------------------------
# <input type="reset">
#-----------------------------------------------------------
itcl::class Browser::form_element_input_reset {
  inherit form_element

  public method window {newname} {

    button $newname -command [itcl::code $containing_form reset]
    if [info exists value] {
      $newname configure -text $value
      }
    set widget $newname
    return $newname
    }

  # A reset button doesn't submit a value.
  public method get_value {} {
    return {}
    }

  }

#-----------------------------------------------------------
# <input type="submit">
# The button will not inherit the background color.
#-----------------------------------------------------------
itcl::class Browser::form_element_input_submit {
  inherit form_element

  protected variable pressed false

  public method press {} {
    set pressed true
    $containing_form submit
    }

  public method window {newname} {
    button $newname -command [itcl::code $this press]
    if [info exists value] {
      $newname configure -text $value
      }
    set widget $newname
    return $newname
    }

  public method get_value {} {
    if {$pressed && [string compare $name ""] != 0} {
      return [list $name $value]
      }
    return {}
    }

  }

#-----------------------------------------------------------
# <input type="text">
#-----------------------------------------------------------
itcl::class Browser::form_element_input_text {
  inherit form_element

  public variable size 15
  public variable maxlength 256

  public method window {newname} {
    entry $newname \
    	-textvariable [itcl::scope value] \
    	-background white \
    	-width $size
    set widget $newname
    return $newname
    }

  }

#-----------------------------------------------------------
# <input type="password">
#-----------------------------------------------------------
itcl::class Browser::form_element_input_password {
  inherit form_element_input_text

  public method window {newname} {
    entry $newname \
	-textvariable [itcl::scope value] \
	-background white \
	-width $size \
	-show *
    set widget $newname
    return $newname
    }

  }

#-----------------------------------------------------------
# <select>
#-----------------------------------------------------------
itcl::class Browser::form_element_select {
  inherit form_element

  public variable multiple 0
  public variable size 1
  private variable items {}

  # This is called when the <option> tag is seen.
  public method option {label value selected} {
    #puts "<option label=\"$label\" value=\"$value\">"
    lappend items [list $label $value $selected]
    }

  # This is called with the contents of the <option> tag.
  public method appendlabel {newtext} {
    #puts "    $newtext"
    set last_index [llength $items]
    if {$last_index > 0} {
	incr last_index -1
	set last [lindex $items $last_index]
	set last_label [lindex $last 0]
	set last_value [lindex $last 1]
	set last_selected [lindex $last 2]
	set last_label "$last_label$newtext"
	set items [lreplace $items $last_index 1 [list $last_label $last_value $last_selected]]
	}
    }

  # Reset it to the original state.
  public method reset {} {
    if {$size > 1} {
    	$widget selection clear 0 end
	set x 0
	foreach item $items {
	    if {[lindex $item 2]} {
	    	$widget selection set $x
	    	}
	    incr x
	    }
   	} else {
    	$widget select 0
	set x 0
	foreach item $items {
	    if {[lindex $item 2]} {
	    	$widget select $x
	    	}
	    incr x
	    }
    	}
    }

  # Create the Tk window and return its handle.
  public method window {newname} {

    # Use a listbox or a drop-down menu.
    if {$size > 1} {
	iwidgets::Scrolledlistbox $newname \
		-hscrollmode dynamic -vscrollmode dynamic \
		-textbackground white \
		-borderwidth 2 -relief groove
	if {$multiple} {
	    $newname configure -selectmode multiple
	    }
    } else {
	iwidgets::Optionmenu $newname -background [get_bgcolor]
    }

    # Insert the items into the listbox while remembering the
    # length of the longest one.
    set maxwidth 4
    foreach item $items {
	set item_text [lindex $item 0]
	$newname insert end $item_text
	set item_text_len [string length $item_text]
	if {$item_text_len > $maxwidth} {
	    set maxwidth $item_text_len
	    }
	}

    # If the listbox is a listbox and not a drop down menu, set
    # its width and height.
    if {$size > 1} {
	set listboxwidth [expr $maxwidth + 2]
	$newname configure -visibleitems "${listboxwidth}x${size}"
	}

    set widget $newname
    $this reset

    return $newname
    }

  # This is called at submit time.  It returns a list of
  # name, value lists.
  public method get_value {} {

    if {[string compare $name ""] == 0} {
    	return {}
    	}

    if {$size > 1} {
	set selectedlist [$widget getcurselection]
        } else {
	set selectedlist [list [$widget get]]
        }
    #puts "  $name: $selectedlist"

    set values_list {}
    foreach item $selectedlist {
	lappend values_list $name
	lappend values_list $item
    	}

    return $values_list
    }

  }

#-----------------------------------------------------------
# <textarea>
#-----------------------------------------------------------
itcl::class Browser::form_element_textarea {
  inherit form_element

  protected variable cols 80
  protected variable rows 6

  public method appendtext {text} {
    append value_initial $text
    }

  public method reset {} {
    $widget delete 1.0 end
    $widget insert end $value_initial
    }

  public method window {newname} {
    iwidgets::Scrolledtext $newname \
	-textbackground white \
	-hscrollmode dynamic \
	-vscrollmode dynamic \
	-visibleitems "${cols}x${rows}" \
	-borderwidth 2 -relief groove

    set widget $newname
    $this reset
    return $newname
    }

  public method get_value {} {
    if {[string compare $name ""] == 0} {
    	return {}
    	}
    return [list $name [$widget get 1.0 end]]
    }

  }

#===========================================================
# The actual browser class
#===========================================================
itcl::class Browser {
  inherit iwidgets::Scrolledhtml

  constructor {args} {
    eval itk_initialize $args
    }

  public method clear {}
  public method import {args}
  public method import_callback {code message content_type data}
  public method render {html {wd .}}
  public method write {html}
  public method img_callback {label code message content_type data}
  public method get_bgcolor {} { return $_bgcolor }

  protected method _entity_!doctype {args} {
    #puts "Browser::_entity_!doctype"
    }
  protected method _entity_html {args} {
    #puts "Browser::_entity_html"
    }
  protected method _entity_/html {args} {
    #puts "Browser::_entity_/html"
    }
  protected method _entity_head {args} {
    #puts "Browser::_entity_head"
    }
  protected method _entity_/head {args} {
    #puts "Browser::_entity_/head"
    }
  protected method _entity_/p {} {
    #puts "Browser::_entity_/p"
    }
  protected method _entity_img {args}
  protected method _entity_/th {}
  protected method _entity_form {args}
  protected method _entity_/form {}
  protected method _entity_input {args}
  protected method _entity_button {args}
  protected method _entity_select {args}
  protected method _entity_option {args}
  protected method _entity_/select {}
  protected method _entity_textarea {args}
  protected method _entity_/textarea {args}

  public variable getcommand
  public variable postcommand

  protected variable _forms
  protected variable _current_form
  protected variable _anchorname
  protected variable _current_select
  protected variable _current_textarea

  protected method _append_text {text}
  protected method _parse_fields {array_var string}
  protected method to_anchor {}
  protected method parse_url {url}
}

#-----------------------------------------------------------
# Break a URL up into its components.
#-----------------------------------------------------------
itcl::body Browser::parse_url {url} {
  puts "Browser::parse_url $url"

  # Break the current base directory into method, host, and path.
  if ![regexp {^([a-zA-Z]+)://([^/]*)(.*)$} $_cwd dummy cwd_method cwd_host cwd_dir] {
    set cwd_method "file"
    set cwd_host ""
    set cwd_dir ""
    }
  puts "_cwd: $_cwd, cwd_method: $cwd_method, cwd_host: $cwd_host, cwd_dir: $cwd_dir"

  # If the URL doesn't have a method and host (possibly empty),
  # then take the method and host from the base.
  if ![regexp {^([a-zA-Z]+)://([^/]*)(.*)$} $url dummy method host remainder] {
    set method $cwd_method
    set host $cwd_host
    set remainder $url
    }
  puts "method: $method, host: $host, remainder: $remainder"

  # Add the directory to the front if we have to.
  if [regexp {^~} $remainder] {
    set remainder "/$remainder"
    } else {
    if {$remainder != "" && ![regexp {^/} $remainder]} {
      set remainder "$cwd_dir/$remainder"
      }
    }
  puts "remainder: $remainder"

  # Seperate remainder2#anchorname (each is optional).
  if ![regexp {^(.*)#(.*)$} $remainder dummy remainder2 anchorname] {
    set remainder2 $remainder
    set anchorname ""
    }
  puts "remainder2: $remainder2, _anchorname: $anchorname"

  # Separate filename and query.
  if ![regexp {^(.*)(\?.*)$} $remainder2 dummy filename query] {
    set filename $remainder2
    set query ""
    }
  puts "filename: $filename, query: $query"

  return [list $method $host $filename $query $anchorname]
  }

#-----------------------------------------------------------
# Clear and reset the HTML widget.
#-----------------------------------------------------------
itcl::body Browser::clear {} {
  iwidgets::Scrolledhtml::clear
  set _forms {}
  set _current_form {}
  set _current_select {}
  set _current_textarea {}
}

#----------------------------------------------------------------------------
# METHOD import filename?#anchorname?
#
# Read HTML text from a URL.
#
# If '#anchorname' is appended to the URL, the page is displayed starting
# at the anchor named 'anchorname' If an anchor is specified without a
# filename, the current page is assumed.
#-----------------------------------------------------------------------------
itcl::body Browser::import {args} {
  puts "Browser::import $args"

  set len [llength $args]
  set url [lindex $args 0]
  if {$len > 1} { set extra_query [lindex $args 1] }
  if {$len > 2} { set post_data [lindex $args 2] }

  set result [parse_url $url]
  set method [lindex $result 0]
  set host [lindex $result 1]
  set filename [lindex $result 2]
  set query [lindex $result 3]
  set _anchorname [lindex $result 4]

  # If it is just an anchor jump,
  if [regexp {^#} $url] {
    to_anchor

    # If it it more than an anchor jump,
    } else {

    # Clear the text widget.
    clear

    # Change base.  We can't use "file dirname" because it behaves
    # differently on a Macintosh.
    #set dirname [file dirname $filename]
    set dirname ""
    regexp {^(/.+)/[^/]*$} $filename junk dirname
    set _cwd "$method://$host$dirname"

    # Append extra query data if necessary.
    if {[info exists extra_query] && [string compare $extra_query ""] != 0} {
      if {$query != ""} {
      	set query "$query&$extra_query"
      	} else {
      	set query "?$extra_query"
      	}
      }

    if [info exists post_data] {
      $postcommand "$method://$host$filename$query" $post_data [itcl::code $this import_callback]
      } else {
      $getcommand "$method://$host$filename$query" [itcl::code $this import_callback]
      }
    }

  }

itcl::body Browser::import_callback {code message content_type data} {
  puts "Browser::import_callback $code $message ..."

  # Write the data into the widget.
  write $data

  # Move to anchor
  to_anchor
  }

itcl::body Browser::to_anchor {} {
  # if an anchor was requested, move that anchor into view
  if {[info exists _anchorname] && $_anchorname!=""} \
    {
    if [info exists _anchor($_anchorname)] \
      {
      $itk_component(text) see end
      $itk_component(text) see $_anchor($_anchorname)
      } else {
      $itk_component(text) see 0.0
      }
    }
  }

#------------------------------------------------------------
# METHOD: render text ?wd?
#
# Clear the text widget, then render the HTML formatted text
# in the provided string.  The optional wd argument sets the
# base directory for any links or images.  By default it is
# the current directory.
#------------------------------------------------------------
itcl::body Browser::render {html {wd .}} {
    clear
    set _cwd $wd
    write $html
}

#-----------------------------------------------------------
# This is like JavaScript document.write().
#-----------------------------------------------------------
itcl::body Browser::write {html} {

    # make text writable
    $itk_component(text) config -state normal

    # Create a text widget tag for the characters we may be about to insert?
    _set_tag

    set continuerendering 1
    while {$continuerendering} {

	# While there is still HTML to render,
	while {[set len [string length $html]]} {

	    # Look for text up to the next <> element and append it
	    # to the text widget.
	    if [regexp -indices "^\[^<\]+" $html match] {
		set text [string range $html 0 [lindex $match 1]]
		_append_text "$text"
		set html [string range $html [expr [lindex $match 1]+1] end]
	    }

	    # Now we're either at a <>, or at the EOT.
	    if [regexp -indices "^<((\[^>\"\]+|(\"\[^\"\]*\"))*)>" $html match entity] {
		regsub -all "\n" [string range $html [lindex $entity 0] [lindex $entity 1]] "" entity
		set cmd [string tolower [lindex $entity 0]]

		if [catch {
			eval _entity_$cmd [lrange $entity 1 end]
			} result] {
		    puts "==============================================="
		    puts "_entity_$cmd [lrange $entity 1 end]: $result"
		    global errorInfo
		    puts $errorInfo
		    puts "=============="
		    }

		set html [string range $html [expr [lindex $match 1]+1] end]
	    }

	    # Call the progress feedback function, if there is one.
	    if {$itk_option(-feedback) != {} } {
	      eval $itk_option(-feedback) $len
	    }

	    if $_verbatim break
	}

	# We reach here if html is empty, or _verbatim is 1.
	# If html is empty, stop now.
	if !$len break

  	# _verbatim must be 1
	# append text until next tag is reached
	if [regexp -indices "<.*>" $html match] {
	    set text [string range $html 0 [expr [lindex $match 0]-1]]
	    set html [string range $html [expr [lindex $match 0]] end]
	} else {
	    set text $html
	    set html ""
	}
	_append_text "$text"
    }

    # Make the text widget read-only again.
    $itk_component(text) config -state disabled
}

# ------------------------------------------------------------------
# PRIVATE METHOD: _append_text text
#
# append text in the format described by the state variables
# ------------------------------------------------------------------
itcl::body Browser::_append_text {text} {

    if {!$_intable && $itk_option(-update)} {update}

    # If not in <pre></pre>, then convert each intances of whitespace
    # to a single space.
    if !$_pre {
	set text [string trim $text "\n\r"]
	regsub -all "\[ \n\r\t\]+" $text " " text
	if ![string length $text] return
	}

    # Convert character entities.  Is this efficient?
    if {[string first "&" $text] != -1} {
       regsub -nocase -all "&amp;" $text {\&} text
       regsub -nocase -all "&lt;" $text "<" text
       regsub -nocase -all "&gt;" $text ">" text
       regsub -nocase -all "&quot;" $text "\"" text
       regsub -nocase -all "&nbsp;" $text " " text
    }

    # If we are between <title></title> tags, then just stash the data.
    if {$_intitle} {
	append _title $text
	return
	}

    # If we are within a <select></select>, then add the text to the
    # current item.
    if {$_current_select != ""} {
	$_current_select appendlabel $text
	return
	}

    # If we are within a <textarea></textarea>, then add the text to the
    # current item.
    if {$_current_textarea != ""} {
	$_current_textarea appendtext $text
	return
	}

    # Within <pre></pre> tags, we just insert the text into
    # the Tk text widget.
    if {$_pre} {
	$_hottext insert end $text $_tag
	return
    }

    # If we get here, it is typical HTML text.
    set p [$_hottext get "end - 2c"]
    set n [string index $text 0]
    if {$n == " " && $p == " "} {
	set text [string range $text 1 end]
	}
    $_hottext insert end $text $_tag
}

# ------------------------------------------------------------------
# PRIVATE METHOD: _parse_fields array_var string
#
# parse fields from a href or image tag. At the moment, doesn't support
# spaces in field values. (e.g. alt="not avaliable")
# ------------------------------------------------------------------
itcl::body Browser::_parse_fields {array_var string} {
  upvar $array_var array
  if {$string != "{}" } {
    regsub -all "( *)=( *)" $string = string
    regsub -all {\\\"} $string \" string
    while {$string != ""} {
      if ![regexp "^ *(\[^ \n\r=\]+)=\"(\[^\"\n\r\t\]*)(.*)" $string \
                      dummy field value newstring] {
        if ![regexp "^ *(\[^ \n\r=\]+)=(\[^\n\r\t \]*)(.*)" $string \
                      dummy field value newstring] {
          if ![regexp "^ *(\[^ \n\r\]+)(.*)" $string dummy field newstring] {
            error "malformed command field; field = \"$string\""
            continue
          }
          set value ""
        }
      }
      set array([string tolower $field]) $value
      set string "$newstring"
    }
  }
}

# ------------------------------------------------------------------
# PRIVATE METHOD: _entity_img
#
# display an image. takes argument of the form img=<filename>
# ------------------------------------------------------------------
itcl::body Browser::_entity_img {{args {}}} {
  #puts "Browser::_entity_img: $args"

  _parse_fields ar $args

  # This frame will hold the button that holds the image.
  set imgframe $_hottext.img[incr _counter]

  # if this is an anchor
  if $_anchorcount {

    # create a frame with a border and bindings.
    frame $imgframe -borderwidth 2 -background $_link
    bind $imgframe <Enter> [list $imgframe configure -background $_alink]
    bind $imgframe <Leave> [list $imgframe configure -background $_link]

    # If not,
    } else {

    # create plain frame.
    frame $imgframe -borderwidth 0 -background $_color
    }

  # If height= and width= exist, make the frame the proper size.
  if {[info exists ar(width)] && [info exists ar(height)] } {
    $imgframe configure -width $ar(width) -height $ar(height)
    pack propagate $imgframe false
    }

  # Find a string to display until the image arrives.
  if [info exists ar(alt)] {
    set alt $ar(alt)
    } else {
    if [info exists ar(src)] {
      regexp {[^/]*$} $ar(src) alt
      } else {
      set alt "image"
      }
    }

  # Make a label and put the alt text in it.  We will
  # add the image as soon as we have it.
  set win $imgframe.label
  label $win -text "\[$alt\]" -borderwidth 0 -background $_bgcolor -foreground $_color
  pack $win -fill both -expand true

  #
  # Determine alignment to use.
  #
  set align bottom
  if [info exists ar(align)] {
    switch $ar(align) {
      middle {
        set align center
        }
      right {
        set align center
        }
      default {
        set align [string tolower $ar(align)]
        }
      }
    }

  #
  # Create window in text to display image.  Align it as
  # determined above.
  #
  $_hottext window create end -window $imgframe -align $align

  #
  # Set a tag for the image window.  If it is enclosed
  # in <a>...</a>, then bind mouse button one to it.
  #
  $_hottext tag add $_tag $imgframe
  if $_anchorcount {
    set href [_peek href]

    # These next two lines seem to be unused!!!
    set href_tag href[incr _counter]
    set tags [list $_tag $href_tag]

    if { $itk_option(-linkcommand) != {} } {
      bind $win <1> [itcl::code $itk_option(-linkcommand) $href]
      }
    }

  #
  # Start the image loading.
  #
  if [info exists ar(src)] {
    set result [parse_url $ar(src)]
    set method [lindex $result 0]
    set host [lindex $result 1]
    set filename [lindex $result 2]
    set query [lindex $result 3]

    $getcommand "$method://$host$filename$query" [itcl::code $this img_callback $win]
    }

  }

itcl::body Browser::img_callback {label code message content_type data} {
  #puts "Browser::img_callback: $label $code $message $content_type ..."

  if {[catch {image create photo -data $data} img]} {
    puts "Image create failed: $img"
    } else {
    $label configure -image $img
    lappend _images $img
    }
  }

#-----------------------------------------------------------
# Override defective </th>
#-----------------------------------------------------------
itcl::body Browser::_entity_/th {} {
  _entity_/b
  _entity_/td
}

#-----------------------------------------------------------
# <form>
#-----------------------------------------------------------
itcl::body Browser::_entity_form {args} {
  #puts "Browser::_entity_form $args"
  _parse_fields ar $args

  set _current_form [uplevel #0 Browser::form $_hottext.#auto $this]
  lappend _forms $_current_form

  if [info exists ar(name)] {
    $_current_form configure -name $ar(name)
    }
  if [info exists ar(action)] {
    $_current_form configure -action $ar(action)
    }
  if [info exists ar(method)] {
    $_current_form configure -method $ar(method)
    }

  _entity_p
}

#-----------------------------------------------------------
# </form>
#-----------------------------------------------------------
itcl::body Browser::_entity_/form {} {
  #puts "Browser::_entity_/form"
  set _current_form {}
  _entity_/p
}

#-----------------------------------------------------------
# <input>
#-----------------------------------------------------------
itcl::body Browser::_entity_input {args} {
  #puts "Browser::_entity_input"
  _parse_fields ar $args

  if {[string compare $_current_form ""] == 0} {
    puts "Form element outside form"
    }

  set type ""
  if [info exists ar(type)] {
    set type [string tolower $ar(type)]
    }

  switch -exact -- $type {
    button {
      set obj [form_element_input_button #auto]
    }
    checkbox {
      set obj [form_element_input_checkbox #auto]
    }
    hidden {
      set obj [form_element_input_hidden #auto]
    }
    image {
      set obj [form_element_input_image #auto]
    }
    password {
      set obj [form_element_input_password #auto]
    }
    radio {
      set obj [form_element_input_radio #auto]
    }
    reset {
      set obj [form_element_input_reset #auto]
    }
    submit {
      set obj [form_element_input_submit #auto]
    }
    text {
      set obj [form_element_input_text #auto]
    }
    default {
      puts "Unknown <input> type \"$type\", substituting text."
      set obj [form_element_input_text #auto]
    }
  }

  # Add this form element to the form so that it can
  # be found at submit time.
  $_current_form add_element [itcl::code $obj]

  # Pass options to the form element.
  if [info exists ar(name)] {
    $obj configure -name $ar(name)
    }
  if [info exists ar(value)] {
    $obj configure -value_initial $ar(value)
    }
  if [info exists ar(size)] {
    catch { $obj configure -size $ar(size) }
    }
  if [info exists ar(maxlength)] {
    catch { $obj configure -maxlength $ar(maxlength) }
    }
  if [info exists ar(checked)] {
    catch { $obj configure -checked 1 }
    }
  if [info exists ar(src)] {
    catch { $obj configure -src $ar(src) }
    }

  # Create the form elements visible manifestation (window) and
  # add it to the text widget.  Of course, hidden fields don't
  # have windows.
  if {[string compare $type "hidden"] != 0} {
    $_hottext window create end -window [$obj window $_hottext.input[incr _counter]] -align bottom
    }
}

#-----------------------------------------------------------
# <button>
#-----------------------------------------------------------
itcl::body Browser::_entity_button {args} {
  puts "Browser::_entity_button"
}

#-----------------------------------------------------------
# <select>
#-----------------------------------------------------------
itcl::body Browser::_entity_select {args} {
  #puts "Browser::_entity_select"
  _parse_fields ar $args

  set _current_select [form_element_select #auto]

  if [info exists ar(name)] {
    $_current_select configure -name $ar(name)
    }

  if [info exists ar(multiple)] {
    $_current_select configure -multiple 1
    }

  if [info exists ar(size)] {
    $_current_select configure -size $ar(size)
    }

}

#-----------------------------------------------------------
# </select>
# We have to use baseline alignment because bottom
# alignment doesn't work.
#-----------------------------------------------------------
itcl::body Browser::_entity_/select {} {
  #puts "Browser::_entity_/select"
  $_current_form add_element [itcl::code $_current_select]
  $_hottext window create end -window [$_current_select window $_hottext.select[incr _counter]] -align baseline
  set _current_select {}
}

#-----------------------------------------------------------
# <option>
#-----------------------------------------------------------
itcl::body Browser::_entity_option {args} {
  #puts "Browser::_entity_option"
  _parse_fields ar $args

  set label ""
  if [info exists ar(label)] {
    set label $ar(label)
    }

  set value ""
  if [info exists ar(value)] {
    set value $ar(value)
    }

  set selected 0
  if [info exists ar(selected)] {
    set selected 1
    }

  $_current_select option $label $value $selected
}

#-----------------------------------------------------------
# <textarea>
#-----------------------------------------------------------
itcl::body Browser::_entity_textarea {args} {
  #puts "Browser::_entity_textarea"
  _parse_fields ar $args

  set _current_textarea [form_element_textarea #auto]

  if [info exists ar(name)] {
    $_current_textarea configure -name $ar(name)
    }
  if [info exists ar(rows)] {
    $_current_textarea configure -rows $ar(rows)
    }
  if [info exists ar(cols)] {
    $_current_textarea configure -cols $ar(cols)
    }
}

#-----------------------------------------------------------
# </textarea>
#-----------------------------------------------------------
itcl::body Browser::_entity_/textarea {args} {
  #puts "Browser::_entity_/textarea"
  $_current_form add_element [itcl::code $_current_textarea]
  $_hottext insert end "\n"
  $_hottext window create end -window [$_current_textarea window $_hottext.textarea[incr _counter]]
  $_hottext insert end "\n"
  set _current_textarea {}
}

# end of file
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

#source ./urlfetch.itk
#source ./browser.itk

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
	    puts $file "+OK"
	    flush $file
	    fileevent $file readable [list server_reader $file]
	    wm deiconify $w
	    focus $w
	    return
	    }

	# Not the last line, add this text.
	$w.message.text insert end "$line\n"
	}

    }

#
# Create a web browser window and download a page into it.
#
proc command_HTML {file url width height} {
    global wserial
    global open_windows

    if [info exists open_windows($url)] {
    	puts "  Already exists"
    	wm deiconify $open_windows($url)
    	raise $open_windows($url)
    	focus $open_windows($url)
    	return
    	}

    set w .html_[incr wserial]
    set open_windows($url) $w
    .questions.list insert end $url

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

    wm protocol $w WM_DELETE_WINDOW [list command_HTML_close $url]

    raise $w
    focus $w

    puts $file "+OK"
    }

proc command_HTML_close {url} {
    global open_windows
    puts "Window manager request to close window $open_windows($url) for $url."

    destroy $open_windows($url)
    unset open_windows($url)
    .questions.list delete [lsearch -exact [.questions.list get 0 end] $url]
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
	"HTML *" {
	    if {[regexp {^HTML ([^ ]+) ([^ ]+) ([^ ]+)$} $line junk url width height]} {
		command_HTML $file $url $width $height
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
		    wm deiconify $open_windows($url)
		    raise $open_windows($url)
		    focus $open_windows($url)
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
    #wm withdraw .
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
