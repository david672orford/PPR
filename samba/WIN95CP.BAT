@echo off
rem
rem mouse:~ppr/src/samba/win95cp.bat
rem Copyright 1995--1999, Trinity College Computing Center
rem Written by David Chappell.
rem
rem Permission to use, copy, modify, and distribute this software and its
rem documentation for any purpose and without fee is hereby granted, provided
rem that the above copyright notice appear in all copies and that both that
rem copyright notice and this permission notice appear in supporting
rem documentation.  This software and documentation are provided "as is"
rem without express or implied warranty.
rem
rem Last modified 16 June 1999.
rem

echo This batch file will copy the MS-Windows 95 printer driver
echo files from the computer you run it on to the directory you
echo specify.
echo.

if not "%1" == "" goto skip1
echo Usage: win95cp <destdir>
goto end
:skip1

rem Change this if your windows directory
rem is elsewhere.
c:
cd \windows\system

echo Copying PSCRIPT 4.0 ...
copy pscript.drv %1
copy pscript.hlp %1
copy fonts.mfm %1
copy iconlib.dll %1
copy psmon.dll %1
copy pscript.ini %1
echo.

echo Copying ADOBEPS 4.x ...
copy adbeps41.drv %1
copy adobeps4.drv %1
copy adobeps4.hlp %1
copy adfonts.mfm %1
echo.

echo Done.
:end
