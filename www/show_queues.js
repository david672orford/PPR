//
// mouse:~ppr/src/www/show_queues.js
// Copyright 1995--2003, Trinity College Computing Center.
// Written by David Chappell.
// Last revised 19 December 2003.
//

// Width in pixels of invisible border round the table.
var invisible_border_width = 50;

// Amount to adjust the popup menu up and to the left so that the mouse
// pointer is well inside it.
var pointer_in = 10;

// This is a list of the various CGI scripts and the window options to use with them.
var windows = new Array();
windows['show_queues.cgi'] =		'width=800,height=600,scrollbars,resizable';
windows['prn_addwiz.cgi'] =			'width=775,height=500,resizable';
windows['grp_addwiz.cgi'] =			'width=775,height=500,resizable';
windows['alias_addwiz.cgi'] =		'width=775,height=500,resizable';
windows['show_jobs.cgi'] =			'width=775,height=500,resizable,scrollbars';
windows['prn_control.cgi'] =		'width=750,height=350,resizable';
windows['grp_control.cgi'] =		'width=750,height=400,resizable';
windows['prn_media.cgi'] =			'width=700,height=300,scrollbars,resizable';
windows['prn_properties.cgi'] =		'width=675,height=580,resizable';
windows['grp_properties.cgi'] =		'width=675,height=580,resizable';
windows['alias_properties.cgi'] =	'width=675,height=580,resizable';
windows['prn_testpage.cgi'] =		'width=775,height=525,resizable';
windows['cliconf.cgi'] =			'width=700,height=525,resizable';
windows['show_printlog.cgi'] =		'width=800,height=600,resizable,scrollbars';
windows['delete_queue.cgi'] =		'width=600,height=150';
windows['about.cgi'] =				'width=550,height=275,resizable';	
windows['login_cookie.html'] =		'width=350,height=250,resizable';
windows['df_html.cgi'] =			'width=600,height=500,resizable,scrollbars';

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
// The gentle_reload() function is called as a periodic event to save the 
// current scrolling position of the window in two form variables and then 
// submit the form.  It is called as the action of a Javascript timeout.  If
// the variable page_locked is non-zero, then the reload is reschedualed
// for 5 seconds in the future.
//
var page_locked = 0;
function gentle_reload()
	{
	if(page_locked == 0)
		{
		reload();
		}
	else
		{
		window.setTimeout("gentle_reload()", 5000);
		}
	}

function reload()
	{
	document.forms[0].x.value = scrolled_x();
	document.forms[0].y.value = scrolled_y();
	document.forms[0].seq.value++;				// serial number 
	document.forms[0].submit();
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
	page_locked = 1;
	return false;
	}

// This is called when the user clicks on one of the items on the menu bar.
// container: the <a> object
// name: the id attribute of the <table> containing the menu
function popup2(container, name)
	{
	// Close any other menus
	var menus = document.getElementsByName('menubar');
	for(var i=0; i < menus.length; i++)
		{
		menus.item(i).style.visibility = 'hidden';
		}

	// !!!	
	menus = document.getElementById('m_file');
	menus.style.visibility = 'hidden';
	menus = document.getElementById('m_view');
	menus.style.visibility = 'hidden';
	
	// Find the popup menu.
	var w = document.getElementById(name);

	// Move the popup menu under the menu bar item that activated it.
	w.style.left = container.offsetLeft - 50 + 'px';
	w.style.top = container.offsetParent.offsetHeight + 'px';	// buttom of menubar <div>

	w.style.visibility = 'visible';
	page_locked = 1;
	return false;
	}

// This is called for all mouseover events inside the DIV which holds
// a popup menu or any of its child elements.  If the mouse pointer entered
// the exposed part of the DIV element (which is transparent), then the
// style of the DIV is altered so that it (and its children) are hidden.
//
// This is difficult because event.target.className doesn't exist in IE 5.x
// for Mac and event.target doesn't exist in IE 6.x Win32!
function offmenu(event)
	{
	var target = event.target ? event.target : event.srcElement;
	//alert(target.className);
	if(target.className == 'popup')
		{
		target.style.visibility = 'hidden';
		page_locked = 0;
		}
	}

//
// These functions are called from the onclick handlers of the links in the 
// popup menu.  They open a new window and then hide the menu.
//
function wopen(event, url)
	{
	// Extract the base script name from the URL.
	var script = url;
	var query_offset = script.indexOf('?');
	if(query_offset >= 0)
		script = script.substr(0, query_offset);
	script = script.substr(script.lastIndexOf('/') + 1);

	// Look the base script name up in the list to find the window options
	// to use.
	var options = windows[script];
	if(options == null)
		options = 'menubar,resizable,scrollbars,toolbar';

	// We have all the information we need.  Do it now.
	window.open(url, '_blank', options);

	// Try to pop the menu down.  This isn't absolutely necessary,
	// so we put it in a try block so that we can return false
	// even if it fails.
	try {
		if(event)
			{
			var target = event.target ? event.target : event.srcElement;
			var node = target.parentNode;		// search upward for the popup frame
			while(node.className != 'popup')
				{
				node = node.parentNode;
				}
			node.style.visibility = 'hidden';
			page_locked = 0;
			}
		}
	catch(message)
		{
		//alert("Can't pop menu down: " + message);
		}
	
	// Returning false indicates that we suceeded and that
	// non-Javascript default actions (such as following
	// a link) shouldn't be taken.
	return false;
	}

// end of file
