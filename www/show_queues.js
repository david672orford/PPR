//
// mouse:~ppr/src/www/show_queues.js
// Copyright 1995--2003, Trinity College Computing Center.
// Written by David Chappell.
// Last revised 10 December 2003.
//

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
function page_lock()
	{
	page_locked = 1;
	}
function page_unlock()
	{
	page_locked = 0;
	}
function gentle_reload()
	{
	if(page_locked == 0)
		{
		(document.forms[0] != null) || alert("Assertion failed: form missing");
		(document.forms[0].x.value != null) || alert("Assertion failed: x value missing");
		(document.forms[0].y.value != null) || alert("Assertion failed: y value missing");
		(document.forms[0].seq.value != null) || alert("Assertion failed: seq value missing");

		document.forms[0].x.value = window.scrollX;
		document.forms[0].y.value = window.scrollY;

		document.forms[0].seq.value++;
		document.forms[0].submit();
		}
	else
		{
		window.setTimeout("gentle_reload()", 5000);
		}
	}

//
// These 2 functions reveal and hide the popup menu.
//
function popup(event, name)
	{
	var w;
	w = document.getElementById(name);
	w.style.left = window.scrollX + event.clientX - 40 + "px";      // scrollX, scrollY may be Mozilla only
	w.style.top = window.scrollY + event.clientY - 60 + "px";
	w.style.visibility = 'visible';
	page_lock();
	return false;
	}
function menu_hide(event)
	{
	if(event.currentTarget == event.target)
		{
		event.currentTarget.style.visibility = 'hidden';
		page_unlock();
		}
	}

// Open a wizard in a window of the right size.
function wizard(wizard_url)
	{
	window.open(wizard_url, '_blank', 'width=775,height=500,resizable');
	return false;
	}

// Open a queue listing window.
function show_jobs_window(queue)
	{
	window.open('show_jobs.cgi?name=' + queue, '_blank', 'width=775,height=500,resizable,scrollbars');
	}

//
// These functions are called from the popup menu.  They open a new window
// and then hide the menu.
//
function show_jobs(event, queue)
	{
	show_jobs_window(queue);
	return false;
	}
function prn_control(event, printer_name)
	{
	window.open('prn_control.cgi?name=' + printer_name, '_blank', 'width=750,height=350,resizable');
	return false;
	}
function grp_control(event, printer_name)
	{
	window.open('grp_control.cgi?name=' + printer_name, '_blank', 'width=750,height=400,resizable');
	return false;
	}
function prn_properties(event, printer_name)
	{
	window.open('prn_properties.cgi?name=' + printer_name, '_blank', 'width=675,height=580,resizable');
	return false;
	}
function grp_properties(event, name)
	{
	window.open('grp_properties.cgi?name=' + name, '_blank', 'width=675,height=580,resizable');
	return false;
	}
function alias_properties(event, name)
	{
	window.open('alias_properties.cgi?name=' + name, '_blank', 'width=675,height=580,resizable');
	return false;
	}
function prn_testpage(event, printer_name)
	{
	window.open('prn_testpage.cgi?name=' + printer_name, '_blank', 'width=775,height=525,resizable');
	return false;
	}
function cliconf(event, name)
	{
	window.open('cliconf.cgi?name=' + name, '_blank', 'width=700,height=525,resizable');
	return false;
	}
function show_printlog(event, type, name)
	{
	window.open('show_printlog.cgi?' + type + '=' + name, '_blank', 'width=800,height=600,resizable,scrollbars');
	return false;
	}
function delete_queue(event, type, name)
	{
	window.open('delete_queue.cgi?type=' + type + '&name=' + name, '_blank', 'width=600,height=150');
	return false;
	}

// end of file
