@echo off

REM
REM Command line arguments:
REM 1. Directory of files required to exist in the executable dir
REM 2. Directory where the executable is located
REM 3. Directory of source code
REM

echo Copying files from %1 to %2
xcopy "%1" "%2" /f /s /e /y /I

echo Run xgettext...
cd "%3"
dir /b /s /r *.cpp *.hpp > "../po/filelist.txt"
xgettext -C -k_ -kwxTRANSLATE -kwxPLURAL:1,2 -f "%3/../po/filelist.txt" -j --add-comments=TRANSLATORS --no-wrap -o "%3/../po/winpinator.pot"
del "../po/filelist.txt"