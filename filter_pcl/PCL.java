/*
** mouse:~ppr/src/filter_pcl/PCL.java
** Copyright 1995--2000, Trinity College Computing Center.
** Written by David Chappell.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appears in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software and documentation are provided "as is"
** without express or implied warranty.
**
** Last modified 11 May 2000.
*/

import java.io.*;

class PCL extends PSout
    {
    // Create a new PCL parser and attached an open file to
    // receive the PostScript.
    public PCL(OutputStream outfile) throws AssertionFailed
    	{
	super(outfile);
	moveto(left_margin, top_margin);
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

    // Some of the current state.
    private int top_margin = 4175;	// In theory, 4275 for 4 lines @ 8 LPI
    private int left_margin = 2375;
    private int line_spacing = 1200;	// 6 LPI
    private int hmi = 720;		// 10 CPI

    // This function performs a control character function.
    public void control(int c)
        {
	debug("control "); debug(c); debug(" (");

	switch(c)
	    {
	    case 8:
	    	debug("backspace");
		rxmoveto(0 - get_space_width());
	    	break;
	    case 10:
	    	debug("line feed");
		rymoveto(line_spacing);
	    	break;
	    case 12:
	    	debug("form feed");
		close_page();
		moveto(left_margin, top_margin);
	    	break;
	    case 13:
		debug("carriage return");
		xmoveto(left_margin);
		break;
	    case 32:
	    	debug("space");
	    	rxmoveto(hmi);
	    	break;
	    }

	debug(")\n");
        }

    // This function performs a simple PCL ESC.
    public void simple_esc(int c)
        {
	debug("ESC "); debug((char)c); debug(" (");

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
	debug("ESC "); debug((char)parameterized_char); debug(" "); debug((char)group_char); debug(" ");
		debug(value_thousandths / 1000.0); debug(" "); debug((char)parameter_char);
		debug(" (");

	int command = ((parameterized_char << 16) | (group_char << 8) | parameter_char);
	switch(command)
	    {
	    case 0x252d58:
		debug("UEL");
	    	break;
	    case 0x266c58:
	    	debug("# copies");
	    	break;
	    case 0x266c53:
	    	debug("simplex/duplex");
	    	break;
	    case 0x266c55:
	    	debug("long edge offset");
	    	break;
	    case 0x266c5a:
	    	debug("short edge offset");
		break;
	    case 0x266147:
	    	debug("page size selection");
	    	break;
	    case 0x266c54:
	    	debug("job separation");
	    	break;
	    case 0x266cc47:
	    	debug("output bin");
	    	break;
	    case 0x267544:
	    	debug("units of measure");
	    	break;
	    case 0x266c48:
	    	debug("paper source");
	    	break;
	    case 0x266c41:
	    	debug("page size");
	    	break;
            case 0x266c4f:
            	debug("orientation");
		if(value_thousandths >= 0 && value_thousandths < 4000 && (value_thousandths % 1000) == 0)
		    set_orientation(value_thousandths / 1000);
		else
		    debug(", invalid");
            	break;
            case 0x266150:
            	debug("print direction");
            	break;
            case 0x266c45:
            	debug("top margin, ");
		top_margin = (line_spacing * value_thousandths / 1000);
		debug(top_margin); debug("/7200");
		ymoveto(top_margin);
            	break;
            case 0x266c46:
            	debug("text length");
            	break;
            case 0x26614c:
            	debug("left margin, ");
		left_margin = (hmi * value_thousandths / 1000);
		debug(left_margin); debug("/7200");
		xmoveto(left_margin);
            	break;
            case 0x26614d:
            	debug("right margin");
            	break;
            case 0x266c4c:
            	debug("perforation skip");
            	break;
            case 0x266b48:
            	debug("horizontal motion index");
            	break;
            case 0x266c43:
            	debug("vertical motion index");
		line_spacing = (value_thousandths * 3 / 20);	// rounding problems?
            	break;
            case 0x266c44:
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
	    case 0x266b47:
	    	debug("line termination");
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
	    	break;
	    case 0x297348:
	    	debug("secondary pitch");
	    	break;
	    case 0x266b53:
	    	debug("set pitch mode");
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
		    set_font(0);
		else
		    set_font(1);
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
	    case 0x267058:		// !!!
	    	debug("transparent print data");
	    	break;


	    case 0x267343:
	    	debug("end of line wrap");
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
		printable(c);
	    	}
            }
    	}

    } // class PCL

// end of PCL.java

