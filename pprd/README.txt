mouse:~ppr/src/pprd/README.txt
14 December 2006

This directory contains the source code for pprd.  Pprd is the spooler daemon.
It keeps track of the queue, launches instances of pprdrv to print jobs, and
notifies users when their jobs are finished, canceled or arrested.

A word about file and function naming conventions:  First, the old code
doesn't follow any.  Second, as new code is written and old code is revised,
as much as possible, the functions in each module all begin with the part
of the module name between the "pprd_" and the ".c" as the followed by
an underscore.  For instance, pprd_pprdrv.c contains pprdrv_start() and
pprdrv_child_hook().


