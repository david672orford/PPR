~ppr/src/nonppr_misc/pcre/README.txt
28 March 2003

This directory contains a stript-down version of PCRE 3.9.  The original
(complete with documentation and a test suite) may be obtained from:

    http://www.pcre.org

It was stript down by doing most of the things recomended in NON-UNIX-USE
and then:

* Removing any unused files

* Removing the documentation

* Removing the tests

* Edited the #include directives to find internals.h, config.h, and
  chartables.c in this directory.


