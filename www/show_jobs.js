//
// mouse:~ppr/src/www/show_jobs.js
// Copyright 1995--2001, Trinity College Computing Center.
// Written by David Chappell.
// Last modified 1 March 2001.
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
		else
			{
			}

		document.forms[0].seq.value++;

		// Netscape hack.  If we didn't do this then submit() would
		// take us to the "Settings" menu!
		document.forms[0].elements[0].value = "Refresh";

		document.forms[0].submit();
		}
	else
		{
		window.setTimeout("gentle_reload()", 5000);
		}
	}

// Open a Modify window.
function do_modify()
    {
    var x;
    var count = 0;

    for(x=0; x < document.forms[0].elements.length; x++)
	{
	var element = document.forms[0].elements[x];
	if(element.type == "checkbox" && element.checked)
	    {
	    if(element.value != "")
		{
		window.open('job_modify.cgi?jobname=' + element.value, '_blank', 'width=650,height=550,scrollbars,resizable');
		count++;
		}
	    element.checked = false;
	    }
	}

    if(count == 0)
    	{
    	alert("No jobs selected.");
    	}

    return false;
    }

// Open a job log window.
function do_log()
    {
    var x;
    var count = 0;

    for(x=0; x < document.forms[0].elements.length; x++)
	{
	var element = document.forms[0].elements[x];
	if(element.type == "checkbox" && element.checked)
	    {
	    if(element.value != "")
		{
		window.open('job_log.cgi?jobname=' + element.value, '_blank', 'width=700,height=300,scrollbars,resizable');
		count++;
		}
	    element.checked = false;
	    }
	}

    if(count == 0)
    	{
    	alert("No jobs selected.");
    	}

    return false;
    }

// end of file
