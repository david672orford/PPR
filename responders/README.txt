mouse:~ppr/responders/README.txt
3 April 2001

This directory contains little programs which PPR invokes when it wants
to tell the user what happened to his job.

The program called time_elapsed is used by some of they to tell the user how
long ago the job was submitted.  The first parameter is the submission time
in seconds since the Unix Epoc.  The second is an interval seconds.  If the
age of the job is less than the interval then the program produces no
output.

