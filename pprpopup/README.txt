mouse:~ppr/src/pprpopup/README.txt
3 August 2003

This directory contains PPR Popup which is a GUI program which is intended
to be run on by users who are submitting print jobs through PPR.  It is
written in Tcl/Tk/Itcl/Itk/Iwidgets.  It has been used sucessfully on
Solaris, Linux, MS-Windows, and MacOS classic.

The chief role of this program is to provide a way for the PPR server to
send messages or questions to the user.  The messages can be presented in 
a plain text window or an HTML window.  The questions must be presented 
as an HTML form.  For text windows, the text of the message is sent to PPR
Popup.  For HTML windows, a URL is sent and the HTML widget fetches the
actual message or question from the PPR server.  Generally, a query string
is used so that the PPR server can produce the right message or question.

PPR Popup listens on a TCP socket for connections from the PPR server.  When
it connects, the PPR server must send a command containing a magic cookie
which PPR Popup previously uploaded to the PPR server.  PPR Popup will
upload a new magic cookie every few minutes.  It will accept either of the
two previous cookies as valid authentication.

For the next version other languages and HTML widgets are being considered. 
These include:

* A Mozilla application (good CSS and JavaScript support, ported to
everthing, no support for listening sockets, is huge)

* KHTML (good CSS support, ported to MacOS X, port to Win32 in progress)

* WxWindows (doesn't support forms, runs everywhere)

* Browsex (http://www.browsex.com) (works on X-Windows, can be compiled for
Win32 with trouble, is bloated with unneeded features)

* Dillo (small, might run on Win32 if ported to Gtk2's thread model)

* GtkHTML (don't know yet)


