@echo off
set "MSYS2_PATH=C:\msys64"

echo Starting Qt and UCRT64 dependency deployment...

call "%MSYS2_PATH%\msys2_shell.cmd" -ucrt64 -here -defterm -no-start -shell bash -c "cd build/gui/GUI_App && windeployqt GUI_App.exe && for file in $(find . -type f \( -name '*.exe' -o -name '*.dll' \)); do ldd \"$file\" | grep '/ucrt64/bin/' | awk '{print $3}' | xargs -I {} cp -u {} .; done"

echo Deployment complete!
pause