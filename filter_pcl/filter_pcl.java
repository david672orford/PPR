/*
** mouse:~ppr/src/filter_pcl/filter_pcl.java
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
** Last modified 6 March 2000.
*/

import java.io.*;

public class filter_pcl
    {
    public static void main(String [] argv)
    	{
	try {
	    //FileOutputStream file_out = new FileOutputStream("out.ps");
	    PCL pcl_out = new PCL(System.out);
	    pcl_out.parse_stream(System.in);
	    pcl_out.close_doc();
	    }
	catch(IOException e)
	    {
	    System.out.print("Error: " + e);
	    System.exit(1);
	    }
	catch(AssertionFailed e)
	    {
	    System.out.print("Assertion failed: " + e);
	    System.exit(10);
	    }
    	}
    }

// end of filter_pcl.java

