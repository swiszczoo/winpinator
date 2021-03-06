@echo off

REM
REM Command line arguments:
REM 1. Directory of files required to exist in the executable dir
REM 2. Directory where the executable is located
REM 3. Directory of source code
REM

REM -------- compile translations --------
echo Compiling translations...
cd "%3/../po"
setlocal enabledelayedexpansion
for /f %%f in ('dir /b *.po') do (
	set output=%%f
	set output=!output:.po=.mo!
	msgfmt %%f -o !output!
)

echo Copying translations to appropriate directories...
cd "%3/../po"
if not exist "../res/to_copy/locales/NUL" mkdir "../res/to_copy/locales"
for /f %%f in ('dir /b *.mo') do (
	set output=%%f
	set output=!output:.mo=!
	if not exist "../res/to_copy/locales/!output!/NUL" mkdir "../res/to_copy/locales/!!output!!"
	echo Processing !output!...
	copy /Y %%f "../res/to_copy/locales/!output!/winpinator.mo"
)

REM -------- copy files --------
echo Copying files from %1 to %2
xcopy "%1" "%2" /f /s /e /y /I