mouse:~ppr/src/pprdrv/README.txt
13 November 1998

The program pprdrv is launched by pprd when it is time to send a job to
a printer.  It opens the various files into which the job has been stored
by ppr.  It also launches an interface program to communicate with the printer.
The interface program and pprdrv communicate over two pipes.  Pprdrv also
opens the printer's PPD file.  It then reassembles the job, inserting PPD file
code and downloaded fonts and stuff like that.  The data so constructed is
written into the pipe to the interface program.  Data received from the
interface is scanned for printer status messages and PostScript error messages.


