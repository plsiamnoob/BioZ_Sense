@echo off
set "MSYS2_PATH=C:\msys64"

echo Starting Qt and UCRT64 dependency deployment...

call "%MSYS2_PATH%\msys2_shell.cmd" -ucrt64 -here -defterm -no-start -shell bash -c "./deploy.sh"

echo Deployment complete!
pause