#! /usr/bin/expect

#spawn telnet chappell-3.pc.trincoll.edu 15009
spawn telnet chappell-4.pc.trincoll.edu 15009
#spawn telnet localhost 15009
expect "Escape"

#send "MESSAGE David Chappell\n"
#send "Hello!\n"
#send ".\n"
#expect "+OK"

send "QUESTION mouse:dummy-1000.0(mouse) http://mouse.trincoll.edu:15010/html/test_forms.html 6i 6i\n"
expect "+OK"

send "QUIT\n"
expect "+OK"
