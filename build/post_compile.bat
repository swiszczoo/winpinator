REM
REM Command line arguments:
REM 1. Directory of files required to exist in the executable dir
REM 2. Directory where the executable is located
REM

echo Copying files from %1 to %2
xcopy "%1" "%2" /f /s /e /y /I
