//
// mouse:~ppr/src/www/show_queues.js
// Copyright 1995--2003, Trinity College Computing Center.
// Written by David Chappell.
// Last revised 11 December 2003.
//

// Width in pixels of invisible border round the table.
var invisible_border_width = 50;

// Amount to adjust the popup menu up and to the left so that the mouse
// pointer is well inside it.
var pointer_in = 10;

// These two functions determine the X and Y coordinates to which the window is
// scrolled.  Since the w3c hasn't defined this, we have to hunt for it.  Some have
// suggested simply adding together the two variables IE might set, but this
// doesn't work since Opera sets both.
function scrolled_x()
	{
	if(window.document.documentElement.scrollLeft)			// IE 6.0 w3c mode, Mozilla 1.5, Konqueror 3.1.3, Opera 7.23
		return window.document.documentElement.scrollLeft;
	if(window.scrollX)										// Netscape 4.x, Mozilla 1.5
		return window.scrollX;
	if(window.document.body.scrollLeft)						// IE 5.x, IE 6.0 compatibility mode, Opera 7.23
		return window.document.body.scrollLeft;
	return 0;
	}

function scrolled_y()
	{
	if(window.document.documentElement.scrollTop)
		return window.document.documentElement.scrollTop;
	if(window.scrollY)
		return window.scrollY;
	if(window.document.body.scrollTop)
		return window.document.body.scrollTop;
	return 0;
	}

//
// The gentle_reload() function is called to save the current scrolling
// position of the window in two form variables and then submit the form.
// It is called as the action of a Javascript timeout.
//
// The page_lock() and page_unlock() functions are used to inhibit this
// behavior while the user is making menu selections.  If gentle_reload()
// is called while the page is locked, the operation is reschedualed for
// 5 seconds in the future.
//
var page_locked = 0;

function gentle_reload()
	{
	if(page_locked == 0)
		{
		document.forms[0].x.value = scrolled_x();
		document.forms[0].y.value = scrolled_y();
		document.forms[0].seq.value++;				// serial number 
		document.forms[0].submit();
		}
	else
		{
		window.setTimeout("gentle_reload()", 5000);
		}
	}

function page_lock()
	{
	page_locked = 1;
	}

function page_unlock()
	{
	page_locked = 0;
	}

// This is called when the user clicks to bring up the context
// menu for a queue icon.
function popup(event, name)
	{
	var w;
	w = document.getElementById(name);
	w.style.left = scrolled_x() + event.clientX - invisible_border_width - pointer_in + "px";
	w.style.top = scrolled_y() + event.clientY - invisible_border_width - pointer_in + "px";
	w.style.visibility = 'visible';
	page_lock();
	return false;
	}

// This is called whenever the mouse pointer enters the DIV which holds
// a popup menu or any of its child elements.  If the mouse pointer entered
// the exposed part of the DIV element (which is transparent), then the
// style of the DIV is altered so that it (and its children) are hidden.
function offmenu(event)
	{
	if(event.currentTarget == event.target)
		{
		menu_hide(event.target);
		}
	}

// This is called when the user clicks on one of the links.  It searches
// up through the node hierarcy until it comes to the DIV (which it
// recognizes because its style class is "popup").  It calls menu_hide()
// on the DIV.
function click_link(event)
	{
	var node = event.target.parentNode;
	while(node.className != 'popup')
		{
		node = node.parentNode;
		}
	menu_hide(node);
	}

// This hides the indicated element and removes the page refresh lock.
function menu_hide(w)
	{
	w.style.visibility = 'hidden';
	page_unlock();
	}

//
// These functions are called from the onclick handlers of the links in the 
// popup menu.  They open a new window and then hide the menu.
//

// cgi_wizard.pl
function wizard(event, url)
	{
	window.open(url, '_blank', 'width=775,height=500,resizable');
	if(event)
		click_link(event);
	return false;
	}

function show_jobs(event, url)
	{
	window.open(url, '_blank', 'width=775,height=500,resizable,scrollbars');
	if(event)
		click_link(event);
	return false;
	}
	
function prn_control(event, printer_name)
	{
	window.open('prn_control.cgi?name=' + printer_name, '_blank', 'width=750,height=350,resizable');
	click_link(event);
	return false;
	}
	
function grp_control(event, printer_name)
	{
	window.open('grp_control.cgi?name=' + printer_name, '_blank', 'width=750,height=400,resizable');
	click_link(event);
	return false;
	}
	
// cgi_tabbed.pl window
function properties(event, url)
	{
	window.open(url, '_blank', 'width=675,height=580,resizable');
	click_link(event);
	return false;
	}
	
function show_printlog(event, type, name)
	{
	window.open('show_printlog.cgi?' + type + '=' + name, '_blank', 'width=800,height=600,resizable,scrollbars');
	click_link(event);
	return false;
	}
	
// confirmation dialog
function confirm(event, url)
	{
	window.open(url, '_blank', 'width=600,height=150');
	click_link(event);
	return false;
	}

// end of file
