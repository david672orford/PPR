~ppr/src/nonppr_misc/c2lib/README.txt
13 January 2005

This directory contains a stript-down version of c2lib 1.4.2.  The
original (complete with documentation and a test suite) may be obtained
from:

    http://www.annexia.org/freeware/c2lib/index.msp

It has been stript down by removing the build system, documentation, test
suite, matvec.*, added config.h (based on one generated using the deleted 
build system), and editing the files to find config.h in this directory.

Also, some identifiers have been renamed, as shown below.

Old		New
--------------------------------------------
pmalloc()	c2_pmalloc()		
pcalloc()	c2_pcalloc()
prealloc()	c2_prealloc()



