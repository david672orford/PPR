mouse:~ppr/src/tests/README.txt
17 November 2002

This directory contains the PPR test suite.  The tests are contained in
subdirectories whose names begin with the string "test-".  Each test 
consists of at least three files.  If the test is named 201-enervated, 
these files would be:

    201-enervated.run
	A program which performs the test.  Most of these are shell
	scripts.
    201-enervated.ok
	The correct output for the test program.
    201-enervated.out
	The output which the test program actually producted the
	last time it was run.

If a test requires input files, it should give then names starting with the 
test name and a hyphen, like this:

    201-enervated-tiresome.ps

Of course, it is neater to enclose the test input within the test program
script, perhaps as a "herefile".  (A herefile is a feature of the Born
shell, Perl, and likely other scripting languages.)

Within each directory, the tests are run in numberical order (or rather 
they will be if you begin the name of each with a three digit number as
I have done).

The tools/ subdirectory contains small utilities employed by the tests:

    test_interface_1

    test_interface_2

    clear_output

    cat_output

    dump_group

    dump_printer

The misc_old/ directory contains input files that were used at some point 
in the past to diagnose problems but were never part of an automated test.

Use the run_tests program to run the tests.  Its parameters should be a
space-separated list of test subdirectories.  For example, to run the
pprdrv tests:

$ ./do_tests test-pprdrv

To run all of the tests:

$ ./do_tests test-*

Need I say that I would like it very much if people were to contribute
tests?

