mouse:~ppr/src/tests/README.txt
1 March 2001

This directory contains the PPR test suite.  The tests are contained in
subdirectories whose names begin with the string "test-".

The tools/ subdirectory contains

The misc_old/ directory contains input files that were used at some point 
in the past to diagnose problems but were never part of an automated test.

Use the run_tests program to run the tests.  Its parameters should be a
space-separated list of test subdirectories.  For example, to run the
pprdrv tests:

$ ./do_tests test-pprdrv

To run all of the tests:

$ ./do_tests test-*

