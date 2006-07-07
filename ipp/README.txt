mouse:~ppr/src/ipp/README.txt
1 June 2006

This directory contains PPR's IPP server.

PPR will soon implement IPP 1.0 and 1.1 as well as the CUPS extensions.  It can
already work to some extent with many IPP clients intended for use with CUPS.
These include the CUPS command-line programs lp, lpr, lpq, lpstat, etc., as
well as KDE's printing subsystem, Openoffice.org, and Mozilla.

=== Notes on CUPS cancel ===

* The argument "-" deletes the current job.  It does this by sending an 
IPP-Cancel-Job request with a printer-uri of ipp://localhost/printers/ and a
job ID of 0.  If the -U switch is used to specify a username other than that of
the user executing cancel, then the request is posted to ipp://localhost/admin/
(which makes the request invalid) otherwise it is posted to
ipp://localhost/jobs/ (which also makes the request invalid).  Which is the
"current job" is unclear.  If the -a switch was used, then an IPP-Purge-Jobs
request is sent instead of IPP-Cancel-Job and a boolean operation attribute
called "purge-jobs" with a value of "true" is added.

* Any argument which matches the name of a destination deletes the current job
on that destination.  It does this by sending an IPP-Cancel-Job request with 
a URI of ipp://localhost/printers/<destination>.  Note the seeming assumption
that the destination is a printer.  Also note that the URI is constructed
blindly rather than using the URI returned by CUPS-Get-Printers.  If the -a option
is used, then the operation is switch to IPP-Purge-Jobs as described above.
The request is posted to incorrect URIs as described above.

* An argument in the form "<destination>-<number><possible non-digits>" deletes
the job with the ID number <number>.  The <destination> is ignored.  An
IPP-Cancel-Job request is sent with a job-uri of ipp://localhost/jobs/<number>.
No attempt is made to determine the actual job-uri.

* An argument begining with one or more digits is converted to a number which 
is interpreted as the ID number of a job to be deleted.  Exactly the same
request is generated as for the previous variant.


