mouse:~ppr/responders/README.txt
9 March 2003

This directory primarily contains little programs which PPR invokes when it 
wants to tell the user what happened to his job.

The program called time_elapsed is used by some of they to tell the user how
long ago the job was submitted.  The first parameter is the submission time
in seconds since the Unix Epoc.  The second is an interval seconds.  If the
age of the job is less than the interval then the program produces no
output.

The program ppr-respond is a wrapper around the responder programs.  It is
called by ppr and pprd.  It gathers the necessary information, formats it
properly, and then invokes the responder program.  This is also where the 
meta responder "followme" is implemented.

