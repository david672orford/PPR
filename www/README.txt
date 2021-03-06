==========================================================================
 mouse:~ppr/src/www/README.txt
 10 April 2006
==========================================================================

This directory contains HTML pages, CGI scripts, and images which together
form an interface for managing and monitoring PPR from a web browser.

Most of the programs in this directory require Perl 5 to run.  Rebuilding
the icons and other image files requires Perl5, Transfig, Ghostscript, and 
NetPBM.

==========================================================================
 Status of the PPR WWW Interface
==========================================================================

The web interface is now pretty reliable, though it is still far from
complete.  So far you can:

* browse the printers, groups and aliases

* view the queue and manipulate jobs

* add and delete printers, groups, and aliases

* change the properties printers, groups, and aliases

* monitor, stop, and start printers

==========================================================================
 Setting up the PPR WWW Interface
==========================================================================

The PPR web interface runs on the same port as PPR's IPP server, so
if you can't connect to it, make sure there aren't any other IPP
servers (such as CUPS) running.  Shut them down or move them to different
ports or IP addresses and restart pprd.

==========================================================================
 Connecting
==========================================================================

As as noted in the previous section, connexions to the web interface
are accepted on the IPP port which is.  If the browser is running on the 
same machine as PPR, enter this URL to connect to the PPR web interface:

	http://localhost:631

This will open a page of links.  The most important ones are the links to
the PPR documentation and to the PPR control panel.  The PPR control panel
is the jumping off point for managing queus and jobs.

==========================================================================
 Logging On
==========================================================================

Features which do anything beyond merely viewing the status of printers and
jobs are restricted to authorized users, just as they would be if one were
using the command-line tools.  For example, one must be listed in
/etc/ppr/acl/ppop.allow before one may manipulate other people's jobs or
start and stop printers and must be listed in /etc/ppr/acl/ppad.allow before
one is permitted to modify queues.

There are three ways that the identity of a user of the web interface can be
verified.  They are Linux localhost authentication, HTTP digest, and a
cookie based MD5 scheme.  The PPR web interface does not support HTTP Basic
authentication.

If you are running under Linux and you run the web browser on the same
machine as ppr-httpd and you connect through localhost (127.0.0.1), then
ppr-httpd will be able to examine /proc/net/tcp to figure out the user id
under which the browser that opened the connexion is running.  Thus no logon
will be necessary (or possible).

HTTP digest requires a browser that supports a relatively new authentication
scheme called HTTP Digest.  HTTP Digest improves upon the more widely
implekmented HTTP Basic authentication in that it does not send the password
over the network where it might been seen by other users.  Mozilla started
supporting this new scheme shortly before version 1.0.  Netscape 6.0 doesn't
support it but 7.0 probably does.  Microsoft Internet Explorer 5.0 does. 
Amaya 3.1 does too.

If your browser doesn't support HTTP digest but does support JavaScript and
cookies, then you can use a fallback password authentication system which
PPR provides.  Look for the "Cookie Login" button in the Printer Control
Panel.  It brings up a small window with a place to enter your username and
password.  If you enter them sucessfully you are logged in and will remain
logged in for as long as you keep the little window open.  This scheme does
not send the password over the network, but may not be as secure as HTTP
Digest authentication.

==========================================================================
 Setting Passwords and Granting Access
==========================================================================

In order to set passwords for digest authenication or cookie authentication,
run:

$ ppr-passwd --add <username>

You will be prompted for a password and an entry will be created for the
indicated user in /etc/ppr/htpasswd.  Note that only root, ppr, and users
listed in /etc/ppr/acl/passwd.allow will be able to add and remove users and
change other user's passwords.  However, once an authorized user has done
this, users can change their own passwords, like this:

$ ppr-passwd

Run "ppdoc ppr-passwd" to view the ppr-passwd manpage and lean about the
other options.

This command is new, so its behavior when run by ordinary users may change a
little.  For instance, they might be permitted to create and remove
/etc/ppr/htpasswd entries for themselves.  Comments on how this command
ought to behave are welcome on the PPR mailing list or they may be sent to 
<ppr-bugs@mail.trincoll.edu>.

You can add users to /etc/ppr/passwd even if there aren't corresponding
users in /etc/passwd, but be careful because if you add the username to
access control lists in /etc/ppr/acl and later add an account with the same
username to /etc/passwd, the new shell account will have those rights too,
which could be bad if you carelessly created them for different people.

