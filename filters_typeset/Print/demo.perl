/*
** demo.cc
*/

#include <iostream.h>
#include <string>
#include "typesetter.h"

int main()
	{
	try
	{
	Typesetter test("ascii.set", 5000, 500);
	test.LowLevelSelectFont(test.LoadFont("Times-Roman"));
	test.LoadFont("Times-Bold");
	test.LoadFont("Times-Italic");
	test.LoadFont("Times-BoldItalic");

	test.AddString("Now is the time for all good men");
	test.AddPenalty(-10000);
	test.AddString(" to come to the aid of the party.  ");
	test.AddString("The quick brown fox jumped over the lazy yellow dogs.  ");
	test.AddString("It is a truth universally acknowledged");
	test.AddPenalty(10000);
	test.AddString(" that a young man ");
	test.AddString("in possesion of a good fortune must be in want of a wife.  ");
	test.EndParagraph();

	test.AddString("This is another paragraph ");

	// Print four paragraphs, each demonstrating a
	// different font in increasing sizes
	int x, y;
	for(x=1; x < 4; x++)
		{
		test.LowLevelSelectFont(x);
		for(y=0; y < 150; y++)
			{
			//test.SelectSize( PT(10 + (y/10)) );

			test.AddString("dis");
			test.DisHyphen();
			test.AddString("cre");
			test.DisHyphen();
			test.AddString("tion");
			test.DisHyphen();
			test.AddString("ary ");
			}
		test.EndParagraph();
		}

	// Insert an zero length but infinitely
	// stretchable glue to fill up the page
	test.VSkip(zeropt, fil, zeropt);

	// Add a negative vertical penalty
	// which forces a page break

	test.AddPenalty(-10000);

	// Dump the horizontal and vertical lists
	//test.debug();

	// Shut down the typesetter
	test.End();

	}
	catch(const char *str)
		{
		cerr << str << '\n';
		return 1;
		}
	catch(string *str)
		{
	    cerr << *str << '\n';
	    delete str;
	    return 1;
		}

	return 0;
	}

// end of file
