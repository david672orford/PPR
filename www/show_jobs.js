//
// mouse:~ppr/src/www/show_jobs.js
// Copyright 1995--2001, Trinity College Computing Center.
// Written by David Chappell.
// Last modified 20 November 2001.
//

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
		window.open('job_modify.cgi?jobname=' + element.value, '_blank', 'width=650,height=550,resizable');
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
