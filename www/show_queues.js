//
// mouse:~ppr/src/www/show_queues.js
// Copyright 1995--2002, Trinity College Computing Center.
// Written by David Chappell.
// Last revised 8 March 2002.
//

//
// Detect Netscape Navigator 4.x and Microsoft Internet Explorer 4.x both of
// which use their own non-standard Document Object Models.  If neither
// is detected, we will assume that the browser conforms to the w3 standards.
//
var nav4_dom = 0;
var ie5_dom = 0;
var browser_version = parseFloat(navigator.appVersion);
if(browser_version >= 4.0 && browser_version < 5.0)
	{
	if(navigator.appName.indexOf("Microsoft") >= 0)
		{
		ie5_dom = 1;
		}
	else
		{
		nav4_dom = 1;
		}
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

		if(nav4_dom)
			{
			document.forms[0].x.value = window.pageXOffset;
			document.forms[0].y.value = window.pageYOffset;
			}
		else if(ie5_dom)
			{
			document.forms[0].x.value = window.document.body.scrollLeft;
			document.forms[0].y.value = window.document.body.scrollTop;
			}
		else		// Mozilla
			{
			document.forms[0].x.value = window.scrollX;
			document.forms[0].y.value = window.scrollY;
			}

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
function menu_show(w)
	{
	if(nav4_dom)
		{
		w.visibility = 'show';
		}
	else	// W3C DOM and IE 4.x
		{
		w.style.visibility = 'visible';
		}
	page_lock();
	}
function menu_hide(w)
	{
	if(nav4_dom)
		{
		w.visibility = 'hide';
		}
	else	// W3C DOM and IE 4.x
		{
		w.style.visibility = 'hidden';
		}
	page_unlock();
	}

// Open a wizard in a window of the right size.
function wizard(wizard_url)
	{
	window.open(wizard_url, '_blank', 'width=775,height=500,resizable');
	return false;
	}

// Open a queue listing window.
function show_jobs(queue)
	{
	window.open('show_jobs.cgi?name=' + queue, '_blank', 'width=775,height=500,resizable,scrollbars');
	// Don't call menu_hide() here!
	}

//
// These functions are called from the popup menu.  They open a new window
// and then hide the menu.
//
function prn_control(w, printer_name)
	{
	window.open('prn_control.cgi?name=' + printer_name, '_blank', 'width=750,height=350,resizable');
	menu_hide(w);
	return false;
	}
function prn_properties(w, printer_name)
	{
	window.open('prn_properties.cgi?name=' + printer_name, '_blank', 'width=750,height=580,resizable');
	menu_hide(w);
	return false;
	}
function prn_testpage(w, printer_name)
	{
	window.open('prn_testpage.cgi?name=' + printer_name, '_blank', 'width=750,height=580,resizable');
	menu_hide(w);
	return false;
	}
function grp_properties(w, name)
	{
	window.open('grp_properties.cgi?name=' + name, '_blank', 'width=675,height=580,resizable');
	menu_hide(w);
	return false;
	}
function cliconf(w, name)
	{
	window.open('cliconf.cgi?name=' + name, '_blank', 'width=675,height=580,resizable');
	menu_hide(w);
	return false;
	}
function show_printlog(w, type, name)
	{
	window.open('show_printlog.cgi?' + type + '=' + name, '_blank', 'width=800,height=600,resizable,scrollbars');
	menu_hide(w);
	return false;
	}
function delete_queue(w, type, name)
	{
	window.open('delete_queue.cgi?type=' + type + '&name=' + name, '_blank', 'width=600,height=150');
	menu_hide(w);
	return false;
	}

//
// These are the functions for the popup menus.
//
function printer(event, name)
	{
	var body = '<table class="menu"><tr><td>\n'
		+ '<a href="" name="L1" id="L1"><nobr>View Queue</nobr></a><br>\n'
		+ '<a href="" name="L2" id="L2"><nobr>Printer Control</nobr></a><br>\n'
		+ '<a href="" name="L3" id="L3"><nobr>Printer Properties</nobr></a><br>\n'
		+ '<a href="" name="L4" id="L4"><nobr>Test Page</nobr></a><br>\n'
		+ '<a href="" name="L5" id="L5"><nobr>Client Configuration</nobr></a><br>\n'
		+ '<a href="" name="L6" id="L6"><nobr>Printlog</nobr></a><br>\n'
		+ '<a href="" name="L7" id="L7"><nobr>Delete Printer</nobr></a><br>\n'
		+ '</td></tr></table>\n';
	var w;
	var lnks;
	if(nav4_dom)
		{
		w = window.document.popup;
		var d = w.document;
		d.write(body);
		d.close();

		w.moveTo((event.x - 40), (event.y - 15));

		lnks = [d.links[0],
			d.links[1],
			d.links[2],
			d.links[3],
			d.links[4],
			d.links[5],
			d.links[6]
			];
		}
	else if(ie5_dom)
		{
		w = window.document.all.popup;
		w.innerHTML = body;

		// Figure out what document the event came from and where it is scrolled
		// to and add the offsets within the window and fudge a little.
		w.style.pixelLeft = (event.srcElement.document.body.scrollLeft + event.x - 40);
		w.style.pixelTop = (event.srcElement.document.body.scrollTop + event.y - 15);

		lnks = [w.all.L1,
			w.all.L2,
			w.all.L3,
			w.all.L4,
			w.all.L5,
			w.all.L6,
			w.all.L7
			];

		// IE 5.0 makes everthing so complicated!
		lnks[0].onmouseout = function () { event.cancelBubble = true };
		lnks[1].onmouseout = function () { event.cancelBubble = true };
		lnks[2].onmouseout = function () { event.cancelBubble = true };
		lnks[3].onmouseout = function () { event.cancelBubble = true };
		lnks[4].onmouseout = function () { event.cancelBubble = true };
		lnks[5].onmouseout = function () { event.cancelBubble = true };
		lnks[6].onmouseout = function () { event.cancelBubble = true };
		}
	else	// W3C DOM, Mozilla
		{
		w = document.getElementById("popup");

		w.style.left = window.scrollX + event.clientX - 40 + "px";	// scrollX, scrollY may be Mozilla only
		w.style.top = window.scrollY + event.clientY - 15 + "px";

		var rng = document.createRange();
		rng.selectNodeContents(w);
		rng.deleteContents();
		var htmlfrag = rng.createContextualFragment(body);
		w.appendChild(htmlfrag);

		lnks = [document.getElementById("L1"),
			document.getElementById("L2"),
			document.getElementById("L3"),
			document.getElementById("L4"),
			document.getElementById("L5"),
			document.getElementById("L6"),
			document.getElementById("L7")
			];
		}
	lnks[0].onclick = function () { show_jobs(name); menu_hide(w); return false; };
	lnks[1].onclick = function () { return prn_control(w, name); };
	lnks[2].onclick = function () { return prn_properties(w, name); };
	lnks[3].onclick = function () { return prn_testpage(w, name); };
	lnks[4].onclick = function () { return cliconf(w, name); };
	lnks[5].onclick = function () { return show_printlog(w, 'printer', name); };
	lnks[6].onclick = function () { return delete_queue(w, 'printer', name); };
	menu_show(w);
	if(ie5_dom || nav4_dom) { w.onmouseout = function () { menu_hide(w) }; }
	return false;
	}
function group(event, name)
	{
	var body = '<table class="menu"><tr><td>\n'
		+ '<a href="" name="L1" id="L1"><nobr>View Queue</nobr></a><br>\n'
		+ '<a href="" name="L2" id="L2"><nobr>Group Properties</nobr></a><br>\n'
		+ '<a href="" name="L3" id="L3"><nobr>Client Configuration</nobr></a><br>\n'
		+ '<a href="" name="L4" id="L4"><nobr>Printlog</nobr></a><br>\n'
		+ '<a href="" name="L5" id="L5"><nobr>Delete Group</nobr></a><br>\n'
		+ '</td></tr></table>\n';
	var w;
	var lnks;
	if(nav4_dom)
		{
		w = window.document.popup;
		var d = w.document;
		d.write(body);
		d.close();
		w.moveTo((event.x - 40), (event.y - 15));
		lnks = [ d.links[0], d.links[1], d.links[2], d.links[3], d.links[4] ];
		}
	else if(ie5_dom)
		{
		w = window.document.all.popup;
		w.innerHTML = body;
		w.style.pixelLeft = (event.srcElement.document.body.scrollLeft + event.x - 40);
		w.style.pixelTop = (event.srcElement.document.body.scrollTop + event.y - 15);
		lnks = [ w.all.L1, w.all.L2, w.all.L3, w.all.L4, w.all.L5 ];
		lnks[0].onmouseout = function () { event.cancelBubble = true };
		lnks[1].onmouseout = function () { event.cancelBubble = true };
		lnks[2].onmouseout = function () { event.cancelBubble = true };
		lnks[3].onmouseout = function () { event.cancelBubble = true };
		lnks[4].onmouseout = function () { event.cancelBubble = true };
		}
	else	// W3C DOM, Mozilla
		{
		w = document.getElementById("popup");
		w.style.left = window.scrollX + event.clientX - 40 + "px";
		w.style.top = window.scrollY + event.clientY - 15 + "px";
		var rng = document.createRange();
		rng.selectNodeContents(w);
		rng.deleteContents();
		var htmlfrag = rng.createContextualFragment(body);
		w.appendChild(htmlfrag);
		lnks = [document.getElementById("L1"),
			document.getElementById("L2"),
			document.getElementById("L3"),
			document.getElementById("L4"),
			document.getElementById("L5")
			];
		}
	lnks[0].onclick = function () { show_jobs(name); menu_hide(w); return false; };
	lnks[1].onclick = function () { return grp_properties(w, name); };
	lnks[2].onclick = function () { return cliconf(w, name); };
	lnks[3].onclick = function () { return show_printlog(w, 'queue', name); };
	lnks[4].onclick = function () { return delete_queue(w, 'group', name); };
	menu_show(w);
	if(ie5_dom || nav4_dom) { w.onmouseout = function () { menu_hide(w) }; }
	return false;
	}

// end of file
