==========================================================================
 mouse:~ppr/src/www/README.txt
 16 February 2001.
==========================================================================

This directory contains HTML pages, CGI scripts, and images which together
form an interface for managing and monitoring PPR from a web browser.

The web interface is now pretty reliable, though it is still far from
complete.  So far you can browse the printers, view the queue, manipulate
jobs, add and delete printers and groups, and stop and start printers.

==========================================================================
 Setting up the PPR WWW Interface with ppr-httpd
==========================================================================

PPR has a mini web server called  ppr-httpd which runs out of Inetd.  That way
it will put no additional load the system when it is not being used.

To enable ppr-httpd, add this line to /etc/services:

ppradmin	15010/tcp

and one like this to /etc/inetd.conf:

ppradmin stream tcp	nowait.400	pprwww	/usr/sbin/tcpd /usr/ppr/lib/ppr-httpd

The fixup program should have added these lines for you already, but it will
have left the line in inetd.conf commented out.  After uncommenting the line,
send Inetd the the HUP signal to tell it to reload inetd.conf.

Since the PPR WWW interface may contain security flaws, it is suggested that
you limited its use to specific networks.  That is why the inetd.conf line
will use TCP Wrappers if available.  Be sure that you set /etc/hosts.deny
and /etc/hosts.allow to limit access appropriately.

Features which do anything beyond merely viewing the status of printers and
jobs are password protected.  Passwords are checked using the new digest
authentication method.  Unfortunately, most web browsers don't support digest
authentication yet.  Netscape doesn't as of version 6.0.  Microsoft Internet
Explorer 5.0 and Amaya 3.1 do.

New in PPR version 1.41 is the ability to authenticate by means of a system
that uses cookies, JavaScript, and MD5 hashes.  Look for the "Cookie Login" 
button in the Printer Control Panel.

If you are running under Linux and you run the web browser on the same machine
as ppr-httpd and you connect through localhost (127.0.0.1), then you won't
need to use digest or any other kind of HTTP authentication because ppr-httpd
will be able to examine /proc/net/tcp to figure out which Linux user opened
the connexion.

In order to set passwords for digest authenication, make sure that the Apache
web server's htdigest program is in your PATH and then run
/usr/ppr/bin/ppr-passwd.  An entry will be created in /etc/ppr/htpasswd.

The usernames you use in /etc/ppr/htpasswd can be the same as Unix usernames,
but they don't have to be.  If they are the same, then the user will be able
to manipulate the cooresponding Unix user's print jobs.  List a user in
/etc/ppr/acl/ppop.allow if you want him to be able to manipulate other people's
jobs and start and stop printers, /etc/ppr/acl/ppad.allow if you want him to be
able to change the configuration of printers.  (Notice that the permission
granted by putting a username in one of these files is also granted to any Unix
user of the same name.)
