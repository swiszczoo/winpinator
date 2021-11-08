REM xgettext extraction script

echo Run xgettext...
cd "../po"
powershell "Get-ChildItem -Recurse *.?pp | Resolve-Path -Relative" > "../po/filelist.txt"
xgettext -C -k_ -kwxTRANSLATE -kwxPLURAL:1,2 -f "../po/filelist.txt" --add-comments=TRANSLATORS --no-wrap -o "../po/winpinator.pot"
del "../po/filelist.txt"