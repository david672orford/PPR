.ppr
.For: Robert Greene
.AuthCode: x
.end
@echo off
echo Booting with StarLAN network
prompt $p$g
SET ATTLANROOT=c:\lanman.dos
path=c:\;%attlanroot%\netprog
attstart
net logon br6
bulink n: u cberry.serve mjagger.serve 
if not exist n:\lab.bat goto no_net
n:\lab br
:no_net
cls
echo The network did not load properly.  Please reboot the machine.
echo If you see this message again, please report the problem to a consultant.
echo ÿ
prompt Network did not load. Please reboot machine. $p$g
