mouse:~ppr/src/pprd/README.txt
16 June 2000

This directory contains the source code for pprd.  Pprd is the local print
daemon.  It keeps track of the queue, launches instances of pprdrv to print
jobs, and notifies users when their jobs are finished, canceled or arrested.

Parts of pprd are being rewritten to allow the sending of jobs to remote
systems.  If a job is for a remote system, pushto_ppr will be invoked instead
of pprdrv.  It will transmit the queue file and the other job files to the
remote system.  The queue file will be kept on the local system to prevent
generating duplicate queue id numbers but the other job files will be deleted
from the local system once they have been sent to the remote one.

A word about file and function nameing conventions:  First, the old code
doesn't follow any.  Second, as new code is written and old code is revised,
as much as possible, the functions in each module all begin with the part
of the module name between the "pprd_" and the ".c" as the followed by
a slash.  For instance, pprd_pprdrv.c contains pprdrv_start() and
pprdrv_child_hook().


