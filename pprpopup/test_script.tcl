#! /usr/bin/expect

#spawn telnet chappell-3.pc.trincoll.edu 15009
spawn telnet chappell-4.pc.trincoll.edu 15009
expect "Escape"

#send "MESSAGE David Chappell\n"
#send "Hello!\n"
#send ".\n"
#expect "+OK"

send "HTML http://mouse.trincoll.edu/~chappell/pprpopup/testdoc.html 6i 6i\n"
expect "+OK"

send "QUIT\n"
expect "+OK"
