/*
** mouse:~ppr/src/filter_pcl/PCLparse.java
** Copyright 1995--2002, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 14 May 2002.
*/

import java.io.*;

class PCLparse
    {
    // PostScript output object.
    private PSout out;

    // Font structures for the primary and secondary fonts.
    private PSfont primary;
    private PSfont secondary;

    // Some of the current state.
    private int top_margin = 4175;	// In theory, 4275 for 4 lines @ 8 LPI
    private int left_margin = 2375;
    private int line_spacing = 1200;	// 6 LPI
    private int hmi = 720;		// 10 CPI

    // Create a new PCL parser and attached an open file to
    // receive the PostScript.
    public PCLparse(PSout ps_out_object) throws AssertionFailed
    	{
	out = ps_out_object;
	out.moveto(left_margin, top_margin);
	out.set_orientation(0);
	out.set_pagesize((int)(8.5 * 7200), (int)(11 * 7200), "Letter");

	primary = new PSfont();
	secondary = new PSfont();
	out.set_font(primary);
    	}

    // Call this when there are no more files you want to parse.
    public void finish()
	{
	out.finish();
	}

    // Print debugging output.
    private void debug(String s)
        {
        System.err.print(s);
        }
    private void debug(char[] s)
        {
        System.err.print(s);
        }
    private void debug(int i)
        {
        System.err.print(i);
        }
    private void debug(double d)
        {
        System.err.print(d);
        }
    private void debug(char c)
	{
	System.err.print(c);
	}

    private void debug_control(int c, String name)
	{
	debug("control ");
	debug(c);
	debug(" (");
	debug(name);
	debug(")\n");
	}

    // This function performs a control character function.
    public void control(int c)
        {
	switch(c)
	    {
	    case 8:
	    	//debug_control(c, "backspace");
		out.rxmoveto(0 - out.get_space_width());
	    	break;
	    case 9:
		debug_control(c, "tab");
		break;
	    case 10:
	    	//debug_control(c, "line feed");
		out.rymoveto(line_spacing);
		if(true)
		     out.xmoveto(left_margin);
	    	break;
	    case 12:
	    	debug_control(c, "form feed");
		out.close_page();
		out.moveto(left_margin, top_margin);
	    	break;
	    case 13:
		//debug_control(c, "carriage return");
		out.xmoveto(left_margin);
		break;
	    case 32:
	    	//debug_control(c, "space");
	    	out.rxmoveto(hmi);
	    	break;
	    default:
		debug_control(c, "?");
		break;
	    }
        }

    // This function performs a simple PCL ESC.
    public void simple_esc(int c)
        {
	debug("ESC ");
		debug((char)c);
		debug(" (");

	switch(c)
	    {
	    case 'E':
	    	debug("reset printer");
	    	break;
	    case '9':
	    	debug("clear horizontal margins");
	    	break;
	    case '=':
	    	debug("half line feed");
	    	break;
	    case 'Y':
		debug("display functions on");
		break;
	    case 'Z':
	    	debug("display functions off");
	    	break;

	    }

	debug(")\n");
        }

    // This function dispatches a PCL parameterized ESC.  The wierd
    // terminology is HP's.
    public void parameterized_esc(int parameterized_char, int group_char, int value_thousandths, int parameter_char) throws AssertionFailed
        {
	debug("ESC ");
		debug((char)parameterized_char);
		debug(" ");
		debug((char)group_char);
		debug(" ");
		debug(value_thousandths / 1000.0);
		debug(" ");
		debug((char)parameter_char);

	debug(" (");

	int command = ((parameterized_char << 16) | (group_char << 8) | parameter_char);
	switch(command)
	    {
	    case 0x252d58:
		debug("UEL");
	    	break;
	    case 0x266b47:
	    	debug("line termination mode");
	    	break;
	    case 0x266c58:			// ESC & l X
	    	debug("# copies");
	    	break;
	    case 0x266c53:			// ESC & l
	    	debug("simplex/duplex");
	    	break;
	    case 0x266c55:			// ESC & l
	    	debug("long edge offset");
	    	break;
	    case 0x266c5a:			// ESC & l
	    	debug("short edge offset");
		break;
	    case 0x266147:			// ESC & a G
	    	debug("page size selection");
	    	break;
	    case 0x266c54:			// ESC & l
	    	debug("job separation");
	    	break;
	    case 0x266cc47:			// ESC & l
	    	debug("output bin");
	    	break;
	    case 0x267544:
	    	debug("units of measure");
	    	break;
	    case 0x266c48:			// ESC & l
	    	debug("paper source");
	    	break;
	    case 0x266c41:			// ESC & l A
	    	debug("page size");
	    	break;
            case 0x266c4f:			// ESC & l
            	debug("orientation");
		if(value_thousandths >= 0 && value_thousandths < 4000 && (value_thousandths % 1000) == 0)
		    out.set_orientation(value_thousandths / 1000);
		else
		    debug(", invalid");
            	break;
            case 0x266150:
            	debug("print direction");
            	break;
            case 0x266c45:			// ESC & l
            	debug("top margin, ");
		top_margin = (line_spacing * value_thousandths / 1000);
		debug(top_margin); debug("/7200");
		out.ymoveto(top_margin);
            	break;
            case 0x266c46:			// ESC & l
            	debug("text length");
            	break;
            case 0x26614c:
            	debug("left margin, ");
		left_margin = (hmi * value_thousandths / 1000);
		debug(left_margin); debug("/7200");
		out.xmoveto(left_margin);
            	break;
            case 0x26614d:
            	debug("right margin");
            	break;
            case 0x266b48:
            	debug("horizontal motion index");
            	break;
            case 0x266c4c:			// ESC & l
            	debug("perforation skip");
            	break;
            case 0x266c43:			// ESC & l
            	debug("vertical motion index");
		line_spacing = (value_thousandths * 3 / 20);	// rounding problems?
            	break;
            case 0x266c44:			// ESC & l
            	debug("line spacing");
		line_spacing = (7200000 / value_thousandths);	// "invalid" values work!!!
            	break;
	    case 0x266152:
	    	debug("vertical position in rows");
	    	break;
	    case 0x2a7059:
	    	debug("vertical position in units");
	    	break;
	    case 0x266156:
	    	debug("vertical position in decipoints");
		break;
	    case 0x266143:
	    	debug("horizontal position in columns");
	    	break;
	    case 0x2a7058:
	    	debug("horizontal position in units");
	    	break;
	    case 0x266148:
	    	debug("horizontal position in decipoints");
	    	break;
            case 0x266653:
            	debug("push/pop position");
            	break;
	    case 0x280044:
	    case 0x280045:
	    case 0x280046:
	    case 0x280047:
	    case 0x280049:
	    case 0x28004a:
	    case 0x28004c:
	    case 0x28004d:
	    case 0x28004e:
	    case 0x280053:
	    case 0x280054:
	    case 0x280055:
		debug("primary symbol set");
		break;
	    case 0x290044:
	    case 0x290045:
	    case 0x290046:
	    case 0x290047:
	    case 0x290049:
	    case 0x29004a:
	    case 0x29004c:
	    case 0x29004d:
	    case 0x29004e:
	    case 0x290053:
	    case 0x290054:
	    case 0x290055:
		debug("secondary symbol set");
		break;
	    case 0x287350:
	    	debug("primary spacing");
	    	break;
	    case 0x297350:
	    	debug("secondary spacing");
	    	break;
	    case 0x287348:
	    	debug("primary pitch");
		hmi = (7200000 / value_thousandths);
		primary.set_size((int)(7200000 / 0.6 / value_thousandths + 0.5));
	    	break;
	    case 0x297348:
	    	debug("secondary pitch");
	    	break;
	    case 0x266b53:
	    	debug("set pitch mode: ");
		switch(value_thousandths / 1000)
		    {
		    case 0:
			debug("10.0 cpi");
			primary.set_size(1200);		// 12 point
			break;
		    case 2:
			debug("compressed (16.5-16.7 cpi)");
			primary.set_size(720);		// 7.2 point
			break;
		    case 4:
			debug("elite (12.0 cpi)");
			primary.set_size(1000);		// 10 point
			break;
		    }
		hmi = out.get_space_width();
	    	break;
            case 0x287356:
            	debug("primary height");
            	break;
            case 0x297356:
            	debug("secondary height");
            	break;
	    case 0x287353:
	    	debug("primary style");
	    	break;
	    case 0x297353:
	    	debug("secondary style");
	    	break;
	    case 0x287342:
	    	debug("primary stroke weight");
		if(value_thousandths == 0)
		    primary.set_bold(false);
		else
		    primary.set_bold(true);
	    	break;
	    case 0x297342:
	    	debug("secondary stroke weight");
	    	break;
	    case 0x287354:
	    	debug("primary typeface family");
	    	break;
	    case 0x297354:
	    	debug("secondary typeface family");
	    	break;
	    case 0x283364:
	    	debug("primary font");
	    	break;
	    case 0x293340:
	    	debug("secondary font");
	    	break;
	    case 0x266444:
	    	debug("underline");
	    	break;
	    case 0x266440:
	    	debug("disable underline");
	    	break;
	    case 0x267058:			// !!!
	    	debug("transparent print data");
	    	break;
	    case 0x267343:
	    	debug("end of line wrap");
	    	break;
	    case 0x2a7242:			// ESC * r B
		debug("end raster graphics");
		break;
	    case 0x2a7452:			// ESC * t R
		debug("set resolution");
		break;
	    case 0x2a7241:			// ESC * r A
		debug("start raster graphics");
		break;
	    case 0x2a624d:			// ESC * b M
		debug("set compression method");
		break;
	    case 0x2a6257:			// ESC * b W
		debug("raster row");
		break;
	    case 0x2a7246:			// ESC * r F
		debug("raster graphics orientation");
		break;

	    }

	debug(")\n");
        }

    // Read a byte from the input stream, throwing an exception if
    // EOF is encountered.
    private int must_getc(InputStream infile) throws IOException, EOFException
        {
	int c = infile.read();
	if(c == -1)
	    throw new EOFException("EOF in ESCape");
	return c;
        }

    // Read PCL from a stream, parse it, and call the appropriate
    // member functions.
    public void parse_stream(InputStream infile) throws IOException, AssertionFailed
    	{
	int c;
        while((c = infile.read()) != -1)
            {
	    // If ESC,
	    if(c == 27)
	    	{
		// Get next character,
		c = must_getc(infile);

		// If it is in the range for parameterized escapes,
		if(c >= 33 && c <= 47)
		    {
		    int parameterized_character = c;
		    int group_character = 0;
		    int iteration = 0;

		    // Read the next character.
		    c = must_getc(infile);

		    // If it is within the group character range, accept it as
		    // the group character and read another.  Some
		    // parameterized escape sequences don't have a group
		    // character!
		    if(c >= 96 && c <= 126)
			group_character = c;

		    // Do all of the possibly combined commands.
		    do  {
                        int value = 0;
                        int value_sign = 1;
                        int parameter_character;

			// If c was taken as the group character or this is
			// a 2nd or subsequent command in a sequence of
			// combined commands,
			if(group_character != 0 || iteration++ > 0)
			    c = must_getc(infile);

                        // If their is a sign, act on it and get another character.
                        if(c == '+' || c == '-')
                            {
                            if(c == '-')
                                value_sign = -1;
		    	    c = must_getc(infile);
                            }

                        // Process the whole part of the number.
                        while(c >= '0' && c <= '9')
                            {
                            value *= 10;
                            value += (c - '0');
		    	    c = must_getc(infile);
                            }

                        // If explicit fractional part,
                        int fraction_count = 0;
                        if(c == '.')
                            {
			    c = must_getc(infile);

                            // Process up to 2 digits.
                            while(fraction_count < 3 && c >= '0' && c <= '9')
                                {
                                value *= 10;
                                value += (c - '0');
                                fraction_count++;
		    		c = must_getc(infile);
                                }
                            // Throw away the rest of the fractional part.
                            while(c >= '0' && c <= '9')
                                {
		    		c = must_getc(infile);
                                }
                            }
			// Heed the place value of missing fractional digits.
			while(fraction_count < 3)
			    {
			    value *= 10;
			    fraction_count++;
			    }

			if(c >= 64 && c <= 94)
			    parameter_character = c;
			else if(c >= 96 && c <= 126)
			    parameter_character = c - 32;
			else
			    {
		    	    debug("Parameterized ESC misterminated.\n");
			    break;
			    }

                        parameterized_esc(parameterized_character, group_character, (value*value_sign), parameter_character);

			} while(c >= 96 && c <= 126);	// while parameter_character is upper case
		    }

		// If non-parameterized escape,
		else
		    {
		    simple_esc(c);
		    }
	    	}
	    // If control character other than escape,
	    else if(c <= 32)
	    	{
		control(c);
	    	}
	    // If it gets this far, it must be printable.
	    else
	    	{
		out.printable(c);
	    	}
            }
    	}

    } // class PCLparse

// end of PCLparse.java

